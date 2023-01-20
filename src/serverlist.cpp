/******************************************************************************\
 * Copyright (c) 2004-2023
 *
 * Author(s):
 *  Volker Fischer
 *
 ******************************************************************************
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 *
\******************************************************************************/

#include "serverlist.h"
/* *\

--port           sets the port the server listens to locally
--serverbindip   sets the IP   the server listens to locally
--serverpublicip sets the public IP where server and directory are on the same LAN

Manual port forwarding is not supported: the internal port must be open to external requests.
Where a router modem does automatic port forwarding, directory pings MAY open the server to requests
from clients not on the same LAN as the directory.

*
 PROTMESSID_CLM_SERVER_LIST
 - SERVER internal to DIRECTORY (list entry external IP     same LAN)
   - CLIENT internal to DIRECTORY (CLM msg     same LAN): use "external" fields (local LAN address)
   - CLIENT external to DIRECTORY (CLM msg not same LAN): use "internal" fields (registered server public IP)
 - SERVER external to DIRECTORY (list entry external IP not same LAN)
   - CLIENT internal to SERVER (CLM     same IP as list entry external IP): use "internal" fields (local LAN address)
   - CLIENT external to SERVER (CLM not same IP as list entry external IP): use "external" fields (public IP address from protocol)
---
*
 PROTMESSID_CLM_REGISTER_SERVER
 - As SERVER create CLM message using "internal" fields (IP layer is the "external" fields):
   - DIRECTORY not on my LAN: self-determined  IP address and --port value
   - DIRECTORY     on my LAN: --serverpublicip IP address and --port value
 - As DIRECTORY store received CLM message as is (IP layer address is used to access the server list)
*

\* */

/* Implementation *************************************************************/

// --- CServerListEntry ---
CServerListEntry CServerListEntry::parse ( QString strHAddr,
                                           QString strLHAddr,
                                           QString sName,
                                           QString sCity,
                                           QString strCountry,
                                           QString strNumClients,
                                           bool    isPermanent,
                                           bool    bEnableIPv6 )
{
    CHostAddress haServerHostAddr;
    NetworkUtil::ParseNetworkAddress ( strHAddr, haServerHostAddr, bEnableIPv6 );
    if ( CHostAddress() == haServerHostAddr )
    {
        // do not proceed without server host address!
        return CServerListEntry();
    }

    CHostAddress haServerLocalAddr;
    NetworkUtil::ParseNetworkAddress ( strLHAddr, haServerLocalAddr, bEnableIPv6 );
    if ( haServerLocalAddr.iPort == 0 )
    {
        haServerLocalAddr.iPort = haServerHostAddr.iPort;
    }

    // Capture parsing success of integers
    bool ok;

    QLocale::Country lcCountry = QLocale::AnyCountry;
    int              iCountry  = strCountry.trimmed().toInt ( &ok );
    if ( ok && iCountry >= 0 && iCountry <= QLocale::LastCountry )
    {
        lcCountry = static_cast<QLocale::Country> ( iCountry );
    }

    int iNumClients = strNumClients.trimmed().toInt ( &ok );
    if ( !ok )
    {
        iNumClients = 10;
    }

    return CServerListEntry ( haServerHostAddr,
                              haServerLocalAddr,
                              CServerCoreInfo ( FromBase64ToString ( sName.trimmed() ).left ( MAX_LEN_SERVER_NAME ),
                                                lcCountry,
                                                FromBase64ToString ( sCity.trimmed() ).left ( MAX_LEN_SERVER_CITY ),
                                                iNumClients,
                                                isPermanent ) );
}

QString CServerListEntry::toCSV()
{
    QStringList sl;

    sl.append ( this->HostAddr.toString() );
    sl.append ( this->LHostAddr.toString() );
    sl.append ( ToBase64 ( this->strName ) );
    sl.append ( ToBase64 ( this->strCity ) );
    sl.append ( QString::number ( this->eCountry ) );
    sl.append ( QString::number ( this->iMaxNumClients ) );
    sl.append ( QString::number ( this->bPermanentOnline ) );

    return sl.join ( ";" );
}

// --- CServerListManager ---
CServerListManager::CServerListManager ( const quint16  iNPortNum,
                                         const QString& sNDirectoryAddress,
                                         const QString& strServerListFileName,
                                         const QString& strServerInfo,
                                         const QString& strServerListFilter,
                                         const QString& strServerPublicIP,
                                         const int      iNumChannels,
                                         const bool     bNEnableIPv6,
                                         CProtocol*     pNConLProt ) :
    DirectoryType ( AT_NONE ),
    bEnableIPv6 ( bNEnableIPv6 ),
    ServerListFileName ( strServerListFileName ),
    strDirectoryAddress ( "" ),
    bIsDirectoryServer ( false ),
    eSvrRegStatus ( SRS_NOT_REGISTERED ),
    strMinServerVersion ( "" ), // disable version check with empty version
    pConnLessProtocol ( pNConLProt ),
    iSvrRegRetries ( 0 )
{

    CHostAddress haServerAddr ( NetworkUtil::GetLocalAddress().InetAddr, iNPortNum );

    // set the server internal address, including internal port number
    QHostAddress qhaServerPublicIP;

    if ( strServerPublicIP == "" )
    {
        // No user-supplied override via --serverpublicip -> use auto-detection
        qhaServerPublicIP = haServerAddr.InetAddr;
    }
    else
    {
        // User-supplied --serverpublicip
        qhaServerPublicIP = QHostAddress ( strServerPublicIP );
    }
    qDebug() << "Using" << qhaServerPublicIP.toString() << "as external IP.";
    ServerPublicIP = CHostAddress ( qhaServerPublicIP, iNPortNum );

    if ( bEnableIPv6 )
    {
        // set the server internal address, including internal port number
        QHostAddress qhaServerPublicIP6;

        qhaServerPublicIP6 = NetworkUtil::GetLocalAddress6().InetAddr;
        qDebug() << "Using" << qhaServerPublicIP6.toString() << "as external IPv6.";
        ServerPublicIP6 = CHostAddress ( qhaServerPublicIP6, iNPortNum );
    }

    // prepare the server info information
    QStringList slServInfoSeparateParams;
    int         iServInfoNumSplitItems = 0;

    if ( !strServerInfo.isEmpty() )
    {
        // split the different parameter strings
        slServInfoSeparateParams = strServerInfo.split ( ";" );

        // get the number of items in the split list
        iServInfoNumSplitItems = slServInfoSeparateParams.count();
    }

    /*
     * Init server list entry (server info for this server) with defaults.
     *
     * The client will use the built in or custom address when retrieving the server list from a directory.
     * The values supplied here only apply when using the server as a server, not as a directory.
     *
     * If we are a directory, we assume that we are a permanent server.
     */
    CServerListEntry ThisServerListEntry ( haServerAddr, ServerPublicIP, "", QLocale::system().country(), "", iNumChannels, bIsDirectoryServer );

    // parse the server info string according to definition:
    // [this server name];[this server city];[this server country as QLocale ID] (; ... ignored)
    // per definition, we expect at least three parameters
    if ( iServInfoNumSplitItems >= 3 )
    {
        // [this server name]
        ThisServerListEntry.strName = slServInfoSeparateParams[0].left ( MAX_LEN_SERVER_NAME );

        // [this server city]
        ThisServerListEntry.strCity = slServInfoSeparateParams[1].left ( MAX_LEN_SERVER_CITY );

        // [this server country as QLocale ID]
        bool      ok;
        const int iCountry = slServInfoSeparateParams[2].toInt ( &ok );
        if ( ok )
        {
            if ( iCountry >= 0 && CLocale::IsCountryCodeSupported ( iCountry ) )
            {
                // Convert from externally-supplied format ("wire format", Qt5 codes) to
                // native format. On Qt5 builds, this is a noop, on Qt6 builds, a conversion
                // takes place.
                // We try to do such conversions at the outer-most interface which is capable of doing it.
                // Although the value comes from src/main -> src/server, this very place is
                // the first where we have access to the parsed country code:
                ThisServerListEntry.eCountry = CLocale::WireFormatCountryCodeToQtCountry ( iCountry );
            }
        }
        else
        {
            QLocale::Country qlCountry = CLocale::GetCountryCodeByTwoLetterCode ( slServInfoSeparateParams[2] );
            if ( qlCountry != QLocale::AnyCountry )
            {
                ThisServerListEntry.eCountry = qlCountry;
            }
        }
        qInfo() << qUtf8Printable ( QString ( "Using server info: name = \"%1\", city = \"%2\", country/region = \"%3\" (%4)" )
                                        .arg ( ThisServerListEntry.strName )
                                        .arg ( ThisServerListEntry.strCity )
                                        .arg ( slServInfoSeparateParams[2] )
                                        .arg ( QLocale::countryToString ( ThisServerListEntry.eCountry ) ) );
    }
    else
    {
        qWarning() << "Ignoring invalid serverinfo, please verify the parameter syntax.";
    }

    // per definition, the very first entry is this server and this entry will
    // never be deleted
    ServerList.clear();

    // per definition, the first entry in the server list is the own server
    ServerList.append ( ThisServerListEntry );

    // set the directory address - not the type, that gets done by app start up
    SetDirectoryAddress ( sNDirectoryAddress );

    // whitelist parsing - only used when bIsDirectoryServer
    if ( !strServerListFilter.isEmpty() )
    {
        // split the different parameter strings
        QStringList  slWhitelistAddresses = strServerListFilter.split ( ";" );
        QHostAddress CurWhiteListAddress;

        for ( int iIdx = 0; iIdx < slWhitelistAddresses.size(); iIdx++ )
        {
            // check for special case: [version]
            if ( ( slWhitelistAddresses.at ( iIdx ).length() > 2 ) && ( slWhitelistAddresses.at ( iIdx ).left ( 1 ) == "[" ) &&
                 ( slWhitelistAddresses.at ( iIdx ).right ( 1 ) == "]" ) )
            {
                strMinServerVersion = slWhitelistAddresses.at ( iIdx ).mid ( 1, slWhitelistAddresses.at ( iIdx ).length() - 2 );
            }
            else if ( CurWhiteListAddress.setAddress ( slWhitelistAddresses.at ( iIdx ) ) )
            {
                vWhiteList << CurWhiteListAddress;
            }
        }
    }

    // assume directoryType will get set to AT_CUSTOM
    if ( !strDirectoryAddress.compare ( "localhost", Qt::CaseInsensitive ) || !strDirectoryAddress.compare ( "127.0.0.1" ) )
    {
        if ( !strMinServerVersion.isEmpty() )
        {
            qInfo() << "Registering servers must be version" << strMinServerVersion << "or later.";
        }
        if ( !vWhiteList.isEmpty() )
        {
            qInfo() << "Directory registration white list active.  Only the following addresses can register:";
            foreach ( QHostAddress hostAddress, vWhiteList )
            {
                qInfo() << "  -" << hostAddress.toString();
            }
        }
    }

    // prepare the one shot timer for determining if this is a
    // permanent registered server
    TimerIsPermanent.setSingleShot ( true );
    TimerIsPermanent.setInterval ( SERVLIST_TIME_PERMSERV_MINUTES * 60000 );

    // prepare the register server response timer (single shot timer)
    TimerCLRegisterServerResp.setSingleShot ( true );
    TimerCLRegisterServerResp.setInterval ( REGISTER_SERVER_TIME_OUT_MS );

    // Connections -------------------------------------------------------------
    QObject::connect ( &TimerPollList, &QTimer::timeout, this, &CServerListManager::OnTimerPollList );

    QObject::connect ( &TimerPingServerInList, &QTimer::timeout, this, &CServerListManager::OnTimerPingServerInList );

    QObject::connect ( &TimerPingServers, &QTimer::timeout, this, &CServerListManager::OnTimerPingServers );

    QObject::connect ( &TimerRefreshRegistration, &QTimer::timeout, this, &CServerListManager::OnTimerRefreshRegistration );

    QObject::connect ( &TimerCLRegisterServerResp, &QTimer::timeout, this, &CServerListManager::OnTimerCLRegisterServerResp );

    QObject::connect ( &TimerIsPermanent, &QTimer::timeout, this, &CServerListManager::OnTimerIsPermanent );

    QObject::connect ( QCoreApplication::instance(), &QCoreApplication::aboutToQuit, this, &CServerListManager::OnAboutToQuit );
}

// set server infos -> per definition the server info of this server is
// stored in the first entry of the list, we assume here that the first
// entry is correctly created in the constructor of the class
// and, if registered, refresh the server list entry
void CServerListManager::SetServerName ( const QString& strNewName )
{
    if ( ServerList[0].strName != strNewName )
    {
        ServerList[0].strName = strNewName;
        SetRegistered ( eSvrRegStatus != SRS_NOT_REGISTERED );
    }
}

void CServerListManager::SetServerCity ( const QString& strNewCity )
{
    if ( ServerList[0].strCity != strNewCity )
    {
        ServerList[0].strCity = strNewCity;
        SetRegistered ( eSvrRegStatus != SRS_NOT_REGISTERED );
    }
}

void CServerListManager::SetServerCountry ( const QLocale::Country eNewCountry )
{
    if ( ServerList[0].eCountry != eNewCountry )
    {
        ServerList[0].eCountry = eNewCountry;
        SetRegistered ( eSvrRegStatus != SRS_NOT_REGISTERED );
    }
}

void CServerListManager::SetDirectoryAddress ( const QString sNDirectoryAddress )
{
    // if the address has not actually changed, do nothing
    if ( sNDirectoryAddress == strDirectoryAddress )
    {
        return;
    }

    if ( DirectoryType != AT_CUSTOM )
    {
        // just save the new name
        strDirectoryAddress = sNDirectoryAddress;
        return;
    }

    // sets the lock
    Unregister();

    QMutexLocker locker ( &Mutex );

    // now save the new name
    strDirectoryAddress = sNDirectoryAddress;

    SetIsDirectoryServer();

    locker.unlock();

    // sets the lock
    Register();
}

void CServerListManager::SetDirectoryType ( const EDirectoryType eNCSAT )
{
    // if the directory type is not changing, do nothing
    if ( eNCSAT == DirectoryType )
    {
        return;
    }

    // sets the lock
    Unregister();

    QMutexLocker locker ( &Mutex );

    // now update the server type
    DirectoryType = eNCSAT;

    SetIsDirectoryServer();

    locker.unlock();

    // sets the lock
    Register();
}

void CServerListManager::SetIsDirectoryServer()
{
    // this is called with the lock set

    // per definition: If we are registered and the directory
    // is the localhost address, we are in directory server mode.
    bool bNIsDirectoryServer = DirectoryType == AT_CUSTOM &&
                               ( !strDirectoryAddress.compare ( "localhost", Qt::CaseInsensitive ) || !strDirectoryAddress.compare ( "127.0.0.1" ) );

    if ( bIsDirectoryServer == bNIsDirectoryServer )
    {
        return;
    }

    bIsDirectoryServer = bNIsDirectoryServer;

    if ( bIsDirectoryServer )
    {
        qInfo() << "Now a directory";
        // Load any persistent server list (create it if it is not there)
        (void) Load();
    }
    else
    {
        qInfo() << "No longer a directory server";
    }
}

// When we unregister, set the status and stop timers
void CServerListManager::Unregister()
{
    // if not currently registered, nothing needs doing
    if ( DirectoryType == AT_NONE || ( DirectoryType == AT_CUSTOM && strDirectoryAddress.isEmpty() ) || eSvrRegStatus == SRS_NOT_REGISTERED )
    {
        return;
    }

    // Update server registration status - sets the lock
    SetRegistered ( false );

    // this is called without the lock set
    QMutexLocker locker ( &Mutex );

    // disable service -> stop timer
    if ( bIsDirectoryServer )
    {
        TimerPollList.stop();
        TimerPingServerInList.stop();
    }
    else
    {
        TimerCLRegisterServerResp.stop();
        TimerRefreshRegistration.stop();
        TimerPingServers.stop();
        TimerIsPermanent.stop();
    }
    ServerList[0].bPermanentOnline = false;
}

// When we register, set the status and start timers
void CServerListManager::Register()
{
    // if cannot currently register, or we have registered, nothing needs doing
    if ( DirectoryType == AT_NONE || ( DirectoryType == AT_CUSTOM && strDirectoryAddress.isEmpty() ) || eSvrRegStatus == SRS_REGISTERED )
    {
        // there are edge cases during registration...
        // maybe ! ( SRS_NOT_REGISTERED || SRS_BAD_ADDRESS )
        return;
    }

    // Update server registration status - sets the lock
    SetRegistered ( true );

    // this is called without the lock set
    QMutexLocker locker ( &Mutex );

    if ( bIsDirectoryServer )
    {
        // start timer for polling the server list if enabled
        // 1 minute = 60 * 1000 ms
        TimerPollList.start ( SERVLIST_POLL_TIME_MINUTES * 60000 );

        // start timer for sending ping messages to servers in the list
        TimerPingServerInList.start ( SERVLIST_UPDATE_PING_SERVERS_MS );

        // directory is permanent
        ServerList[0].bPermanentOnline = true;
    }
    else
    {
        // reset the retry counter to zero because update was called
        iSvrRegRetries = 0;

        // start timer for registration timeout - this gets restarted
        // in OnTimerCLRegisterServerResp on failure
        TimerCLRegisterServerResp.start();

        // start timer for registering this server at the directory server
        // 1 minute = 60 * 1000 ms
        TimerRefreshRegistration.start ( SERVLIST_REGIST_INTERV_MINUTES * 60000 );

        // Start timer for ping the directory server in short intervals to
        // keep the port open at the NAT router.
        // If no NAT is used, we send the messages anyway since they do
        // not hurt (very low traffic). We also reuse the same update
        // time as used in the directory server for pinging the registered
        // servers.
        TimerPingServers.start ( SERVLIST_UPDATE_PING_SERVERS_MS );

        // start the one shot timer for determining if this is a
        // permanent registered server
        TimerIsPermanent.start();
    }
}

/* Directory server list functionality ****************************************/
void CServerListManager::OnTimerPingServerInList()
{
    QMutexLocker locker ( &Mutex );

    const int iCurServerListSize = ServerList.size();

    // send ping to list entries except of the very first one (which is the directory
    // server entry)
    for ( int iIdx = 1; iIdx < iCurServerListSize; iIdx++ )
    {
        // send empty message to keep NAT port open at registered server
        pConnLessProtocol->CreateCLEmptyMes ( ServerList[iIdx].HostAddr );
    }
}

void CServerListManager::OnTimerPollList()
{
    CVector<CHostAddress> vecRemovedHostAddr;

    QMutexLocker locker ( &Mutex );

    // Check all list entries are still valid (omitting the directory server itself)
    for ( int iIdx = ServerList.size() - 1; iIdx > 0; iIdx-- )
    {
        // 1 minute = 60 * 1000 ms
        if ( ServerList[iIdx].RegisterTime.elapsed() > ( SERVLIST_TIME_OUT_MINUTES * 60000 ) )
        {
            // remove this list entry
            vecRemovedHostAddr.Add ( ServerList[iIdx].HostAddr );
            ServerList.removeAt ( iIdx );
        }
    }

    locker.unlock();

    foreach ( const CHostAddress HostAddr, vecRemovedHostAddr )
    {
        qInfo() << qUtf8Printable ( QString ( "Expired entry for %1" ).arg ( HostAddr.toString() ) );
    }
}

void CServerListManager::Append ( const CHostAddress&    InetAddr,
                                  const CHostAddress&    LInetAddr,
                                  const CServerCoreInfo& ServerInfo,
                                  const QString          strVersion )
{
    if ( bIsDirectoryServer )
    {
        // if the client IP address is a private one, it's on the same LAN as the directory
        bool serverIsExternal = !NetworkUtil::IsPrivateNetworkIP ( InetAddr.InetAddr );

        qInfo() << qUtf8Printable ( QString ( "Requested to register entry for %1 (%2): %3 (%4)" )
                                        .arg ( InetAddr.toString() )
                                        .arg ( LInetAddr.toString() )
                                        .arg ( ServerInfo.strName )
                                        .arg ( serverIsExternal ? "external" : "local" ) );

        // check for minimum server version
        if ( !strMinServerVersion.isEmpty() )
        {
#if ( QT_VERSION >= QT_VERSION_CHECK( 5, 6, 0 ) )
            if ( strVersion.isEmpty() ||
                 QVersionNumber::compare ( QVersionNumber::fromString ( strMinServerVersion ), QVersionNumber::fromString ( strVersion ) ) > 0 )
            {
                pConnLessProtocol->CreateCLRegisterServerResp ( InetAddr, SRR_VERSION_TOO_OLD );
                return; // leave function early, i.e., we do not register this server
            }
#endif
        }

        // check for whitelist (it is enabled if it is not empty per definition)
        if ( !vWhiteList.empty() )
        {
            // if the server is not listed, refuse registration and send registration response
            if ( !vWhiteList.contains ( InetAddr.InetAddr ) )
            {
                pConnLessProtocol->CreateCLRegisterServerResp ( InetAddr, SRR_NOT_FULFILL_REQIREMENTS );
                return; // leave function early, i.e., we do not register this server
            }
        }

        // access/modifications to the server list needs to be mutexed
        QMutexLocker locker ( &Mutex );

        const int iCurServerListSize = ServerList.size();

        // Check if server is already registered.
        // The very first list entry must not be checked since
        // this is per definition the directory server (i.e., this server)
        int iSelIdx = IndexOf ( InetAddr );

        // if server is not yet registered, we have to create a new entry
        if ( iSelIdx == INVALID_INDEX )
        {
            // check for maximum allowed number of servers in the server list
            if ( iCurServerListSize < MAX_NUM_SERVERS_IN_SERVER_LIST )
            {
                // create a new server list entry and init with received data
                ServerList.append ( CServerListEntry ( InetAddr, LInetAddr, ServerInfo ) );
                iSelIdx = iCurServerListSize;
            }
        }
        else
        {
            // update all data and call update registration function
            ServerList[iSelIdx].LHostAddr        = LInetAddr;
            ServerList[iSelIdx].strName          = ServerInfo.strName;
            ServerList[iSelIdx].eCountry         = ServerInfo.eCountry;
            ServerList[iSelIdx].strCity          = ServerInfo.strCity;
            ServerList[iSelIdx].iMaxNumClients   = ServerInfo.iMaxNumClients;
            ServerList[iSelIdx].bPermanentOnline = ServerInfo.bPermanentOnline;

            ServerList[iSelIdx].UpdateRegistration();
        }

        pConnLessProtocol->CreateCLRegisterServerResp ( InetAddr,
                                                        iSelIdx == INVALID_INDEX ? ESvrRegResult::SRR_SERVER_LIST_FULL
                                                                                 : ESvrRegResult::SRR_REGISTERED );
    }
}

void CServerListManager::Remove ( const CHostAddress& InetAddr )
{
    if ( bIsDirectoryServer )
    {
        qInfo() << qUtf8Printable ( QString ( "Requested to unregister entry for %1" ).arg ( InetAddr.toString() ) );

        QMutexLocker locker ( &Mutex );

        // Find the server to unregister in the list. The very first list entry
        // must not be removed since this is per definition the directory server
        // (i.e., this server).
        int iIdx = IndexOf ( InetAddr );
        if ( iIdx > 0 )
        {
            ServerList.removeAt ( iIdx );
        }
    }
}

/*
 PROTMESSID_CLM_SERVER_LIST
 - SERVER internal to DIRECTORY (list entry external IP same LAN)
   - CLIENT internal to DIRECTORY (CLM msg     same LAN): use "external" fields (local LAN address)
   - CLIENT external to DIRECTORY (CLM msg not same LAN): use "internal" fields (registered server public IP)
 - SERVER external to DIRECTORY (list entry external IP same LAN)
   - CLIENT internal to SERVER (CLM     same IP as list entry external IP): use "internal" fields (local LAN address)
   - CLIENT external to SERVER (CLM not same IP as list entry external IP): use "external" fields (public IP address from protocol)

 Finally, when retrieving the entry for the directory itself life is more complicated still.  It's easiest just to return "0"
 and allow the client connect dialogue instead to use the IP and Port from which the list was received.

 */
void CServerListManager::RetrieveAll ( const CHostAddress& InetAddr )
{
    QMutexLocker locker ( &Mutex );

    if ( bIsDirectoryServer )
    {
        // if the client IP address is a private one, it's on the same LAN as the directory
        bool clientIsInternal = NetworkUtil::IsPrivateNetworkIP ( InetAddr.InetAddr );

        CHostAddress clientPublicAddr = InetAddr;
        if ( clientIsInternal && CHostAddress().InetAddr != ServerList[0].LHostAddr.InetAddr &&
             !NetworkUtil::IsPrivateNetworkIP ( ServerList[0].LHostAddr.InetAddr ) )
        {
            // client and directory on same LAN, directory has public IP set, that should be suitable for the
            // client, too (i.e. same router with same public IP will be used for both), so use it for client public IP
            clientPublicAddr.InetAddr = ServerList[0].LHostAddr.InetAddr;
        }

        const ushort iCurServerListSize = static_cast<ushort> ( ServerList.size() );

        // allocate memory for the entire list
        CVector<CServerInfo> vecServerInfo ( iCurServerListSize );

        // copy list item for the directory and just let the protocol sort out the actual details
        vecServerInfo[0]          = ServerList[0];
        vecServerInfo[0].HostAddr = CHostAddress();

        // copy the list (we have to copy it since the message requires a vector but the list is actually stored in a QList object
        // and not in a vector object)
        for ( int iIdx = 1; iIdx < iCurServerListSize; iIdx++ )
        {
            // copy list item
            CServerInfo& siCurListEntry = vecServerInfo[iIdx] = ServerList[iIdx];

            bool serverIsInternal = NetworkUtil::IsPrivateNetworkIP ( siCurListEntry.HostAddr.InetAddr );

            bool wantHostAddr = clientIsInternal /* HostAddr is local IP if local server else external IP, so do not replace */ ||
                                ( !serverIsInternal &&
                                  InetAddr.InetAddr != siCurListEntry.HostAddr.InetAddr /* external server and client have different public IPs */ );

            if ( !wantHostAddr )
            {
                vecServerInfo[iIdx].HostAddr = siCurListEntry.LHostAddr;
            }

            // do not send a "ping" to a server local to the directory (no need)
            if ( !serverIsInternal )
            {
                // create "send empty message" for all other registered servers
                // this causes the server (vecServerInfo[iIdx].HostAddr)
                // to send a "reply" to the client (InetAddr or best guess public IP address if internal to directory)
                // - with the intent of opening the server firewall for the client
                pConnLessProtocol->CreateCLSendEmptyMesMes ( siCurListEntry.HostAddr, clientPublicAddr );
            }
        }

        // send the server list to the client, since we do not know that the client
        // has a UDP fragmentation issue, we send both lists, the reduced and the
        // normal list after each other
        pConnLessProtocol->CreateCLRedServerListMes ( InetAddr, vecServerInfo );
        pConnLessProtocol->CreateCLServerListMes ( InetAddr, vecServerInfo );
    }
}

int CServerListManager::IndexOf ( CHostAddress haSearchTerm )
{
    // Called with lock set.

    // Find the server in the list. The very first list entry
    // per definition is the directory server
    // (i.e., this server).
    for ( int iIdx = ServerList.size() - 1; iIdx > 0; iIdx-- )
    {
        if ( ServerList[iIdx].HostAddr == haSearchTerm )
        {
            return iIdx;
        }
    }
    return INVALID_INDEX;
}

bool CServerListManager::SetServerListFileName ( QString strFilename )
{
    QMutexLocker locker ( &Mutex );

    if ( ServerListFileName == strFilename )
    {
        return true;
    }

    if ( !ServerListFileName.isEmpty() )
    {
        // Save once to the old filename
        Save();
    }

    ServerListFileName = strFilename;
    return Load();
}

bool CServerListManager::Load()
{
    // this is called with the lock set

    if ( !bIsDirectoryServer || ServerListFileName.isEmpty() )
    {
        // this gets called again if either of the above change
        return true;
    }

    QFile file ( ServerListFileName );

    if ( !file.open ( QIODevice::ReadWrite | QIODevice::Text ) )
    {
        qWarning() << qUtf8Printable (
            QString ( "Could not open '%1' for read/write.  Please check that %2 has permission (and that there is free space)." )
                .arg ( ServerListFileName )
                .arg ( APP_NAME ) );
        ServerListFileName.clear();
        return false;
    }
    qInfo() << "Loading persistent server list file:" << ServerListFileName;

    // do not lose our entry
    CServerListEntry serverListEntry = ServerList[0];
    ServerList.clear();
    ServerList.append ( serverListEntry );

    // use entire file content for the persistent server list
    CHostAddress haServerHostAddr;

    QTextStream in ( &file );
    while ( !in.atEnd() )
    {
        QString     line   = in.readLine();
        QStringList slLine = line.split ( ";" );
        if ( slLine.count() != 7 )
        {
            qWarning() << qUtf8Printable ( QString ( "Could not parse '%1' successfully - bad line" ).arg ( line ) );
            continue;
        }

        NetworkUtil::ParseNetworkAddress ( slLine[0], haServerHostAddr, bEnableIPv6 );
        int iIdx = IndexOf ( haServerHostAddr );
        if ( iIdx != INVALID_INDEX )
        {
            qWarning() << qUtf8Printable ( QString ( "Skipping '%1' - duplicate host %2" ).arg ( line ).arg ( haServerHostAddr.toString() ) );
            continue;
        }

        serverListEntry =
            CServerListEntry::parse ( slLine[0], slLine[1], slLine[2], slLine[3], slLine[4], slLine[5], slLine[6].toInt() != 0, bEnableIPv6 );

        // We expect servers to have addresses...
        if ( ( CHostAddress() == serverListEntry.HostAddr ) )
        {
            qWarning() << qUtf8Printable ( QString ( "Could not parse '%1' successfully - invalid host" ).arg ( line ) );
            continue;
        }

        qInfo() << qUtf8Printable ( QString ( "Loading registration for %1 (%2): %3" )
                                        .arg ( serverListEntry.HostAddr.toString() )
                                        .arg ( serverListEntry.LHostAddr.toString() )
                                        .arg ( serverListEntry.strName ) );
        ServerList.append ( serverListEntry );
    }

    return true;
}

void CServerListManager::Save()
{
    // this is called with the lock set

    if ( ServerListFileName.isEmpty() )
    {
        return;
    }

    QFile file ( ServerListFileName );

    if ( !file.open ( QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text ) )
    {
        // Not a useable file
        qWarning() << QString ( tr ( "Could not write to '%1'" ) ).arg ( ServerListFileName );
        ServerListFileName.clear();

        return;
    }

    QTextStream out ( &file );
    // This loop *deliberately* omits the first element in the list
    // (that's this server, which is added automatically on start up, not read)
    for ( int iIdx = ServerList.size() - 1; iIdx > 0; iIdx-- )
    {
        qInfo() << qUtf8Printable ( QString ( "Saving registration for %1 (%2): %3" )
                                        .arg ( ServerList[iIdx].HostAddr.toString() )
                                        .arg ( ServerList[iIdx].LHostAddr.toString() )
                                        .arg ( ServerList[iIdx].strName ) );
        out << ServerList[iIdx].toCSV() << '\n';
    }
}

/* Registered server functionality *************************************************/
void CServerListManager::StoreRegistrationResult ( ESvrRegResult eResult )
{
    // we need the lock since the user might change the server properties at
    // any time so another response could arrive
    QMutexLocker locker ( &Mutex );

    // we got some response, so stop the retry timer
    TimerCLRegisterServerResp.stop();

    switch ( eResult )
    {
    case ESvrRegResult::SRR_REGISTERED:
        SetSvrRegStatus ( ESvrRegStatus::SRS_REGISTERED );
        break;

    case ESvrRegResult::SRR_SERVER_LIST_FULL:
        SetSvrRegStatus ( ESvrRegStatus::SRS_SERVER_LIST_FULL );
        break;

    case ESvrRegResult::SRR_VERSION_TOO_OLD:
        SetSvrRegStatus ( ESvrRegStatus::SRS_VERSION_TOO_OLD );
        break;

    case ESvrRegResult::SRR_NOT_FULFILL_REQIREMENTS:
        SetSvrRegStatus ( ESvrRegStatus::SRS_NOT_FULFILL_REQUIREMENTS );
        break;

    default:
        SetSvrRegStatus ( ESvrRegStatus::SRS_UNKNOWN_RESP );
        break;
    }
}

void CServerListManager::OnTimerPingServers()
{
    QMutexLocker locker ( &Mutex );

    // first check if directory address is valid
    if ( !( DirectoryAddress == CHostAddress() ) )
    {
        // send empty message to directory server to keep NAT port open -> we do
        // not require any answer from the directory server
        pConnLessProtocol->CreateCLEmptyMes ( DirectoryAddress );
    }
}

void CServerListManager::OnTimerCLRegisterServerResp()
{
    QMutexLocker locker ( &Mutex );

    if ( eSvrRegStatus == SRS_REQUESTED )
    {
        iSvrRegRetries++;

        if ( iSvrRegRetries >= REGISTER_SERVER_RETRY_LIMIT )
        {
            SetSvrRegStatus ( SRS_TIME_OUT );
        }
        else
        {
            locker.unlock();
            {
                OnTimerRefreshRegistration();
            }
            locker.relock();

            // re-start timer for registration timeout
            TimerCLRegisterServerResp.start();
        }
    }
}

void CServerListManager::OnAboutToQuit()
{
    {
        QMutexLocker locker ( &Mutex );
        Save();
    }

    // Sets the lock - also needs to come after Save()
    Unregister();
}

void CServerListManager::SetRegistered ( const bool bIsRegister )
{
    // we need the lock since the user might change the server properties at
    // any time
    QMutexLocker locker ( &Mutex );

    if ( !bIsRegister && eSvrRegStatus == SRS_NOT_REGISTERED )
    {
        // not wanting to set registered, not registered, nothing to do
        return;
    }

    if ( bIsDirectoryServer )
    {
        // this IS the directory, no network message to worry about
        SetSvrRegStatus ( bIsRegister ? SRS_REGISTERED : SRS_NOT_REGISTERED );
        return;
    }

    // get the correct directory address
    // Note that we always have to parse the server address again since if
    // it is an URL of a dynamic IP address, the IP address might have
    // changed in the meanwhile.
    // Allow IPv4 only for communicating with Directories
    const QString strNetworkAddress      = NetworkUtil::GetDirectoryAddress ( DirectoryType, strDirectoryAddress );
    const bool    bDirectoryAddressValid = NetworkUtil().ParseNetworkAddress ( strNetworkAddress, DirectoryAddress, false );

    if ( bIsRegister )
    {
        if ( bDirectoryAddressValid )
        {
            // register server
            SetSvrRegStatus ( SRS_REQUESTED );

            // For a registered server, the server properties are stored in the
            // very first item in the server list (which is actually no server list
            // but just one item long for the registered server).
            pConnLessProtocol->CreateCLRegisterServerExMes ( DirectoryAddress, ServerList[0].LHostAddr, ServerList[0] );
        }
        else
        {
            SetSvrRegStatus ( SRS_BAD_ADDRESS );
        }
    }
    else
    {
        // unregister server if it is registered - regardless of success of parse
        SetSvrRegStatus ( SRS_NOT_REGISTERED );

        if ( bDirectoryAddressValid )
        {
            pConnLessProtocol->CreateCLUnregisterServerMes ( DirectoryAddress );
        }
    }
}

void CServerListManager::SetSvrRegStatus ( ESvrRegStatus eNSvrRegStatus )
{
    // this is called with the lock set

    // if status isn't a change, do nothing
    if ( eNSvrRegStatus == eSvrRegStatus )
    {
        return;
    }

    // output regirstation result/update on the console
    qInfo() << qUtf8Printable ( QString ( "Server Registration Status update: %1" ).arg ( svrRegStatusToString ( eNSvrRegStatus ) ) );

    // store the state and inform the GUI about the new status
    eSvrRegStatus = eNSvrRegStatus;
    emit SvrRegStatusChanged();
}
