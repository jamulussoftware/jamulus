/******************************************************************************\
 * Copyright (c) 2004-2020
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
                              CServerCoreInfo ( FromBase64ToString ( sName.trimmed().left ( MAX_LEN_SERVER_NAME ) ),
                                                lcCountry,
                                                FromBase64ToString ( sCity.trimmed().left ( MAX_LEN_SERVER_CITY ) ),
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
                                         const QString& sNCentServAddr,
                                         const QString& strServerListFileName,
                                         const QString& strServerInfo,
                                         const QString& strServerListFilter,
                                         const QString& strServerPublicIP,
                                         const int      iNumChannels,
                                         const bool     bNEnableIPv6,
                                         CProtocol*     pNConLProt ) :
    eCentralServerAddressType ( AT_CUSTOM ), // must be AT_CUSTOM for the "no GUI" case
    bEnableIPv6 ( bNEnableIPv6 ),
    eSvrRegStatus ( SRS_UNREGISTERED ),
    strMinServerVersion ( "" ), // disable version check with empty version
    pConnLessProtocol ( pNConLProt ),
    iSvrRegRetries ( 0 )
{
    // set the directory server address (also bIsCentralServer)
    SetCentralServerAddress ( sNCentServAddr );

    // set the server internal address, including internal port number
    QHostAddress qhaServerPublicIP;

    if ( strServerPublicIP == "" )
    {
        // No user-supplied override via --serverpublicip -> use auto-detection
        qhaServerPublicIP = NetworkUtil::GetLocalAddress().InetAddr;
    }
    else
    {
        // User-supplied --serverpublicip
        qhaServerPublicIP = QHostAddress ( strServerPublicIP );
    }
    qDebug() << "Using" << qhaServerPublicIP.toString() << "as external IP.";
    SlaveCurLocalHostAddress = CHostAddress ( qhaServerPublicIP, iNPortNum );

    if ( bEnableIPv6 )
    {
        // set the server internal address, including internal port number
        QHostAddress qhaServerPublicIP6;

        qhaServerPublicIP6 = NetworkUtil::GetLocalAddress6().InetAddr;
        qDebug() << "Using" << qhaServerPublicIP6.toString() << "as external IPv6.";
        SlaveCurLocalHostAddress6 = CHostAddress ( qhaServerPublicIP6, iNPortNum );
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

    // Init server list entry (server info for this server) with defaults. Per
    // definition the client substitutes the IP address of the directory server
    // itself for his server list. If we are a directory server, we assume that
    // we are a permanent server.
    CServerListEntry
        ThisServerListEntry ( CHostAddress(), SlaveCurLocalHostAddress, "", QLocale::system().country(), "", iNumChannels, bIsCentralServer );

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
        if ( ok && iCountry >= 0 && iCountry <= QLocale::LastCountry )
        {
            ThisServerListEntry.eCountry = static_cast<QLocale::Country> ( iCountry );
        }
    }

    // per definition, the very first entry is this server and this entry will
    // never be deleted
    ServerList.clear();

    // per definition, the first entry in the server list is the own server
    ServerList.append ( ThisServerListEntry );

    // Clear the persistent serverlist file name
    ServerListFileName.clear();

    if ( bIsCentralServer )
    {
        // Load any persistent server list (create it if it is not there)
        if ( !strServerListFileName.isEmpty() && ServerListFileName.isEmpty() )
        {
            CentralServerLoadServerList ( strServerListFileName );
        }

        // whitelist parsing
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
                    qInfo() << qUtf8Printable ( QString ( "Whitelist entry added: %1" ).arg ( CurWhiteListAddress.toString() ) );
                }
            }
        }
    }

    // for slave servers start the one shot timer for determining if it is a
    // permanent server
    if ( !bIsCentralServer )
    {
        // 1 minute = 60 * 1000 ms
        QTimer::singleShot ( SERVLIST_TIME_PERMSERV_MINUTES * 60000, this, SLOT ( OnTimerIsPermanent() ) );
    }

    // prepare the register server response timer (single shot timer)
    TimerCLRegisterServerResp.setSingleShot ( true );
    TimerCLRegisterServerResp.setInterval ( REGISTER_SERVER_TIME_OUT_MS );

    // Connections -------------------------------------------------------------
    QObject::connect ( &TimerPollList, &QTimer::timeout, this, &CServerListManager::OnTimerPollList );

    QObject::connect ( &TimerPingServerInList, &QTimer::timeout, this, &CServerListManager::OnTimerPingServerInList );

    QObject::connect ( &TimerPingCentralServer, &QTimer::timeout, this, &CServerListManager::OnTimerPingCentralServer );

    QObject::connect ( &TimerRegistering, &QTimer::timeout, this, &CServerListManager::OnTimerRegistering );

    QObject::connect ( &TimerCLRegisterServerResp, &QTimer::timeout, this, &CServerListManager::OnTimerCLRegisterServerResp );

    QObject::connect ( QCoreApplication::instance(), &QCoreApplication::aboutToQuit, this, &CServerListManager::OnAboutToQuit );
}

void CServerListManager::SetCentralServerAddress ( const QString sNCentServAddr )
{
    // if the address has not actually changed, do nothing
    if ( sNCentServAddr == strCentralServerAddress )
    {
        return;
    }

    // if we are registered to a custom directory server, unregister before updating the name
    if ( eCentralServerAddressType == AT_CUSTOM && GetSvrRegStatus() == SRS_REGISTERED )
    {
        SlaveServerUnregister();
    }

    QMutexLocker locker ( &Mutex );

    // now save the new name
    strCentralServerAddress = sNCentServAddr;

    SetCentralServerState();
}

void CServerListManager::SetCentralServerAddressType ( const ECSAddType eNCSAT )
{
    // if the type is changing, unregister before updating
    if ( eNCSAT != eCentralServerAddressType && GetSvrRegStatus() == SRS_REGISTERED )
    {
        SlaveServerUnregister();
    }

    QMutexLocker locker ( &Mutex );

    // now update the server type
    eCentralServerAddressType = eNCSAT;

    SetCentralServerState();
}

void CServerListManager::SetCentralServerState()
{
    // per definition: If we are in server mode and the directory server address
    // is the localhost address, and set to Custom, we are in directory server mode.
    bool bNCentralServer =
        ( ( !strCentralServerAddress.compare ( "localhost", Qt::CaseInsensitive ) || !strCentralServerAddress.compare ( "127.0.0.1" ) ) &&
          ( eCentralServerAddressType == AT_CUSTOM ) );
    if ( bIsCentralServer != bNCentralServer )
    {
        bIsCentralServer = bNCentralServer;
        qInfo() << ( bIsCentralServer ? "Now a directory server" : "No longer a directory server" );
    }
}

void CServerListManager::Update()
{
    QMutexLocker locker ( &Mutex );

    if ( bEnabled )
    {
        if ( bIsCentralServer )
        {
            // start timer for polling the server list if enabled
            // 1 minute = 60 * 1000 ms
            TimerPollList.start ( SERVLIST_POLL_TIME_MINUTES * 60000 );

            // start timer for sending ping messages to servers in the list
            TimerPingServerInList.start ( SERVLIST_UPDATE_PING_SERVERS_MS );
        }
        else
        {
            // initiate registration right away so that we do not have to wait
            // for the first time out of the timer until the slave server gets
            // registered at the directory server, note that we have to unlock
            // the mutex before calling the function since inside this function
            // the mutex is locked, too
            locker.unlock();
            {
                OnTimerRegistering();
            }
            locker.relock();

            // reset the retry counter to zero because update was called
            iSvrRegRetries = 0;

            // start timer for registration timeout
            TimerCLRegisterServerResp.start();

            // start timer for registering this server at the directory server
            // 1 minute = 60 * 1000 ms
            TimerRegistering.start ( SERVLIST_REGIST_INTERV_MINUTES * 60000 );

            // Start timer for ping the directory server in short intervals to
            // keep the port open at the NAT router.
            // If no NAT is used, we send the messages anyway since they do
            // not hurt (very low traffic). We also reuse the same update
            // time as used in the directory server for pinging the slave
            // servers.
            TimerPingCentralServer.start ( SERVLIST_UPDATE_PING_SERVERS_MS );
        }
    }
    else
    {
        // disable service -> stop timer
        if ( bIsCentralServer )
        {
            TimerPollList.stop();
            TimerPingServerInList.stop();
        }
        else
        {
            TimerCLRegisterServerResp.stop();
            TimerRegistering.stop();
            TimerPingCentralServer.stop();
        }
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
        // send empty message to keep NAT port open at slave server
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

void CServerListManager::CentralServerRegisterServer ( const CHostAddress&    InetAddr,
                                                       const CHostAddress&    LInetAddr,
                                                       const CServerCoreInfo& ServerInfo,
                                                       const QString          strVersion )
{
    if ( bIsCentralServer && bEnabled )
    {
        qInfo() << qUtf8Printable ( QString ( "Requested to register entry for %1 (%2): %3" )
                                        .arg ( InetAddr.toString() )
                                        .arg ( LInetAddr.toString() )
                                        .arg ( ServerInfo.strName ) );

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
                                                        iSelIdx == INVALID_INDEX ? ESvrRegResult::SRR_CENTRAL_SVR_FULL
                                                                                 : ESvrRegResult::SRR_REGISTERED );
    }
}

void CServerListManager::CentralServerUnregisterServer ( const CHostAddress& InetAddr )
{
    if ( bIsCentralServer && bEnabled )
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

void CServerListManager::CentralServerQueryServerList ( const CHostAddress& InetAddr )
{
    QMutexLocker locker ( &Mutex );

    if ( bIsCentralServer && bEnabled )
    {
        const int iCurServerListSize = ServerList.size();

        // allocate memory for the entire list
        CVector<CServerInfo> vecServerInfo ( iCurServerListSize );

        // copy the list (we have to copy it since the message requires
        // a vector but the list is actually stored in a QList object and
        // not in a vector object
        for ( int iIdx = 0; iIdx < iCurServerListSize; iIdx++ )
        {
            // copy list item
            vecServerInfo[iIdx] = ServerList[iIdx];

            if ( iIdx > 0 )
            {
                // check if the address of the client which is requesting the
                // list is the same address as one server in the list -> in this
                // case he has to connect to the local host address and port
                // to allow for NAT.
                if ( vecServerInfo[iIdx].HostAddr.InetAddr == InetAddr.InetAddr )
                {
                    vecServerInfo[iIdx].HostAddr = ServerList[iIdx].LHostAddr;
                }
                else if ( !NetworkUtil::IsPrivateNetworkIP ( InetAddr.InetAddr ) &&
                          NetworkUtil::IsPrivateNetworkIP ( vecServerInfo[iIdx].HostAddr.InetAddr ) &&
                          !NetworkUtil::IsPrivateNetworkIP ( ServerList[iIdx].LHostAddr.InetAddr ) )
                {
                    // We've got a request from a public client, the server
                    // list's entry's primary address is a private address,
                    // but it supplied an additional public address using
                    // --serverpublicip.
                    // In this case, use the latter.
                    // This is common when running a directory server with slave
                    // servers behind a NAT and dealing with external, public
                    // clients.
                    vecServerInfo[iIdx].HostAddr = ServerList[iIdx].LHostAddr;
                    // ?? Shouldn't this send the ping, as below ??
                }
                else
                {
                    // create "send empty message" for all registered servers
                    // (except of the very first list entry since this is this
                    // server (directory server) per definition) and also it is
                    // not required to send this message, if the server is on
                    // the same computer
                    pConnLessProtocol->CreateCLSendEmptyMesMes ( vecServerInfo[iIdx].HostAddr, InetAddr );
                }
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

void CServerListManager::CentralServerLoadServerList ( const QString strServerList )
{
    QFile file ( strServerList );

    if ( !file.open ( QIODevice::ReadWrite | QIODevice::Text ) )
    {
        qWarning() << qUtf8Printable (
            QString ( "Could not open '%1' for read/write.  Please check that Jamulus has permission (and that there is free space)." )
                .arg ( ServerListFileName ) );
        return;
    }

    // use entire file content for the persistent server list
    // and remember it for later writing
    qInfo() << "Setting persistent server list file:" << strServerList;
    ServerListFileName = strServerList;
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

        CServerListEntry serverListEntry =
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
}

void CServerListManager::CentralServerSaveServerList()
{
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
        out << ServerList[iIdx].toCSV() << "\n";
    }
}

/* Slave server functionality *************************************************/
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

    case ESvrRegResult::SRR_CENTRAL_SVR_FULL:
        SetSvrRegStatus ( ESvrRegStatus::SRS_CENTRAL_SVR_FULL );
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

void CServerListManager::OnTimerPingCentralServer()
{
    QMutexLocker locker ( &Mutex );

    // first check if directory server address is valid
    if ( !( SlaveCurCentServerHostAddress == CHostAddress() ) )
    {
        // send empty message to directory server to keep NAT port open -> we do
        // not require any answer from the directory server
        pConnLessProtocol->CreateCLEmptyMes ( SlaveCurCentServerHostAddress );
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
                OnTimerRegistering();
            }
            locker.relock();

            // re-start timer for registration timeout
            TimerCLRegisterServerResp.start();
        }
    }
}

void CServerListManager::SlaveServerRegisterServer ( const bool bIsRegister )
{
    // we need the lock since the user might change the server properties at
    // any time
    QMutexLocker locker ( &Mutex );

    // get the correct directory server address
    const QString strCurCentrServAddr = NetworkUtil::GetCentralServerAddress ( eCentralServerAddressType, strCentralServerAddress );

    // For the slave server, the slave server properties are stored in the
    // very first item in the server list (which is actually no server list
    // but just one item long for the slave server).
    // Note that we always have to parse the server address again since if
    // it is an URL of a dynamic IP address, the IP address might have
    // changed in the meanwhile.

    // Allow IPv4 only for communicating with Directory Servers
    if ( NetworkUtil().ParseNetworkAddress ( strCurCentrServAddr, SlaveCurCentServerHostAddress, false ) )
    {
        if ( bIsRegister )
        {
            // register server
            SetSvrRegStatus ( SRS_REQUESTED );

            pConnLessProtocol->CreateCLRegisterServerExMes ( SlaveCurCentServerHostAddress, SlaveCurLocalHostAddress, ServerList[0] );
        }
        else
        {
            // unregister server
            SetSvrRegStatus ( SRS_UNREGISTERED );

            pConnLessProtocol->CreateCLUnregisterServerMes ( SlaveCurCentServerHostAddress );
        }
    }
    else
    {
        SetSvrRegStatus ( SRS_BAD_ADDRESS );
    }
}

void CServerListManager::SetSvrRegStatus ( ESvrRegStatus eNSvrRegStatus )
{
    // output regirstation result/update on the console
    qInfo() << qUtf8Printable ( QString ( "Server Registration Status update: %1" ).arg ( svrRegStatusToString ( eNSvrRegStatus ) ) );

    // store the state and inform the GUI about the new status
    eSvrRegStatus = eNSvrRegStatus;
    emit SvrRegStatusChanged();
}
