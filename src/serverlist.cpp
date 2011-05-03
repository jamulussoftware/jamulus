/******************************************************************************\
 * Copyright (c) 2004-2011
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
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
\******************************************************************************/

#include "serverlist.h"


/* Implementation *************************************************************/
CServerListManager::CServerListManager ( const QString& sNCentServAddr,
                                         const QString& strServerInfo,
                                         CProtocol*     pNConLProt )
    : iNumPredefinedServers           ( 0 ),
      bUseDefaultCentralServerAddress ( false ),
      pConnLessProtocol               ( pNConLProt )
{
    // set the central server address
    SetCentralServerAddress ( sNCentServAddr );

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

    // per definition, the very first entry is this server and this entry will
    // never be deleted
    ServerList.clear();

    // init server list entry (server info for this server) with defaults, per
    // definition the client substitudes the IP address of the central server
    // itself for his server list
    CServerListEntry ThisServerListEntry (
        CHostAddress(),
        "",
        "",
        QLocale::system().country(),
        "",
        USED_NUM_CHANNELS,
        true );

    // parse the server info string according to definition:
    // [this server name];[this server city]; ...
    //    [this server country as QLocale ID]; ...
    // per definition, we expect at least three parameters
    if ( iServInfoNumSplitItems >= 3 )
    {
        // [this server name]
        ThisServerListEntry.strName = slServInfoSeparateParams[0];

        // [this server city]
        ThisServerListEntry.strCity = slServInfoSeparateParams[1];

        // [this server country as QLocale ID]
        const int iCountry = slServInfoSeparateParams[2].toInt();
        if ( ( iCountry >= 0 ) && ( iCountry <= QLocale::LastCountry ) )
        {
            ThisServerListEntry.eCountry = static_cast<QLocale::Country> (
                iCountry );
        }
    }

    // per definition, the first entry in the server list it the own server
    ServerList.append ( ThisServerListEntry );

    // parse the predefined server infos (if any) according to definition:
    // [server1 address];[server1 name];[server1 city]; ...
    //    [server1 country as QLocale ID]; ...
    //    [server2 address];[server2 name];[server2 city]; ...
    //    [server2 country as QLocale ID]; ...
    //    ...
    int iCurUsedServInfoSplitItems = 3; // three items are used for this server

    // we always expect four items per new server, also check for maximum
    // allowed number of servers in the server list
    while ( ( iServInfoNumSplitItems - iCurUsedServInfoSplitItems >= 4 ) &&
            ( iNumPredefinedServers <= MAX_NUM_SERVERS_IN_SERVER_LIST ) )
    {
        // create a new server list entry
        CServerListEntry NewServerListEntry (
            CHostAddress(),
            "",
            "",
            QLocale::AnyCountry,
            "",
            USED_NUM_CHANNELS,
            true );

        // [server n address]
        LlconNetwUtil().ParseNetworkAddress (
            slServInfoSeparateParams[iCurUsedServInfoSplitItems],
            NewServerListEntry.HostAddr );

        // [server n name]
        NewServerListEntry.strName =
            slServInfoSeparateParams[iCurUsedServInfoSplitItems + 1];

        // [server n city]
        NewServerListEntry.strCity =
            slServInfoSeparateParams[iCurUsedServInfoSplitItems + 2];

        // [server n country as QLocale ID]
        const int iCountry =
            slServInfoSeparateParams[iCurUsedServInfoSplitItems + 3].toInt();

        if ( ( iCountry >= 0 ) && ( iCountry <= QLocale::LastCountry ) )
        {
            NewServerListEntry.eCountry = static_cast<QLocale::Country> (
                iCountry );
        }

        // add the new server to the server list
        ServerList.append ( NewServerListEntry );

        // we have used four items and have created one predefined server
        // (adjust counters)
        iCurUsedServInfoSplitItems += 4;
        iNumPredefinedServers++;
    }


    // Connections -------------------------------------------------------------
    QObject::connect ( &TimerPollList, SIGNAL ( timeout() ),
        this, SLOT ( OnTimerPollList() ) );

    QObject::connect ( &TimerRegistering, SIGNAL ( timeout() ),
        this, SLOT ( OnTimerRegistering() ) );
}

void CServerListManager::SetCentralServerAddress ( const QString sNCentServAddr )
{
    QMutexLocker locker ( &Mutex );

    strCentralServerAddress = sNCentServAddr;

    // per definition: If the central server address is empty, the server list
    // is disabled.
    // per definition: If we are in server mode and the central server address
    // is the localhost address, we are in central server mode. For the central
    // server, the server list is always enabled.
    if ( !strCentralServerAddress.isEmpty() )
    {
        bIsCentralServer =
            ( !strCentralServerAddress.toLower().compare ( "localhost" ) ||
              !strCentralServerAddress.compare ( "127.0.0.1" ) );

        bEnabled = true;
    }
    else
    {
        bIsCentralServer = false;
        bEnabled         = true;
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
        }
        else
        {
            // initiate registration right away so that we do not have to wait
            // for the first time out of the timer until the slave server gets
            // registered at the central server, note that we have to unlock
            // the mutex before calling the function since inside this function
            // the mutex is locked, too
            locker.unlock();
            {
                OnTimerRegistering();
            }
            locker.relock();

            // start timer for registering this server at the central server
            // 1 minute = 60 * 1000 ms
            TimerRegistering.start ( SERVLIST_REGIST_INTERV_MINUTES * 60000 );
        }
    }
    else
    {
        // disable service -> stop timer
        if ( bIsCentralServer )
        {
            TimerPollList.stop();
        }
        else
        {
            TimerRegistering.stop();
        }
    }
}


/* Central server functionality ***********************************************/
void CServerListManager::OnTimerPollList()
{
    QMutexLocker locker ( &Mutex );

    // check all list entries except of the very first one (which is the central
    // server entry) and the predefined servers if they are still valid
    for ( int iIdx = 1 + iNumPredefinedServers; iIdx < ServerList.size(); iIdx++ )
    {
        // 1 minute = 60 * 1000 ms
        if ( ServerList[iIdx].RegisterTime.elapsed() >
                ( SERVLIST_TIME_OUT_MINUTES * 60000 ) )
        {
            // remove this list entry
            ServerList.removeAt ( iIdx );
        }
    }
}

void CServerListManager::RegisterServer ( const CHostAddress&    InetAddr,
                                          const CServerCoreInfo& ServerInfo )
{
    QMutexLocker locker ( &Mutex );

    if ( bIsCentralServer && bEnabled )
    {
        const int iCurServerListSize = ServerList.size();
        
        // check for maximum allowed number of servers in the server list
        if ( iCurServerListSize < MAX_NUM_SERVERS_IN_SERVER_LIST )
        {
            // define invalid index used as a flag
            const int ciInvalidIdx = -1;

            // Check if server is already registered. Use IP number to identify
            // a server. The very first list entry must not be checked since
            // this is per definition the central server (i.e., this server)
            int iSelIdx = ciInvalidIdx; // initialize with an illegal value
            for ( int iIdx = 1; iIdx < iCurServerListSize; iIdx++ )
            {
                if ( ServerList[iIdx].HostAddr.InetAddr == InetAddr.InetAddr )
                {
                    // store entry index
                    iSelIdx = iIdx;

                    // entry found, leave for-loop
                    continue;
                }
            }

            // if server is not yet registered, we have to create a new entry
            if ( iSelIdx == ciInvalidIdx )
            {
                // create a new server list entry and init with received data
                ServerList.append ( CServerListEntry ( InetAddr, ServerInfo ) );
            }
            else
            {
                // update all data and call update registration function
                ServerList[iSelIdx].HostAddr         = InetAddr; // because of port number
                ServerList[iSelIdx].strName          = ServerInfo.strName;
                ServerList[iSelIdx].strTopic         = ServerInfo.strTopic;
                ServerList[iSelIdx].eCountry         = ServerInfo.eCountry;
                ServerList[iSelIdx].strCity          = ServerInfo.strCity;
                ServerList[iSelIdx].iMaxNumClients   = ServerInfo.iMaxNumClients;
                ServerList[iSelIdx].bPermanentOnline = ServerInfo.bPermanentOnline;
                ServerList[iSelIdx].UpdateRegistration();
            }
        }
    }
}

void CServerListManager::UnregisterServer ( const CHostAddress& InetAddr )
{
    QMutexLocker locker ( &Mutex );

    if ( bIsCentralServer && bEnabled )
    {
        const int iCurServerListSize = ServerList.size();

        // Find the server to unregister in the list. The very first list entry
        // must not be checked since this is per definition the central server
        // (i.e., this server), also the predefined servers must not be checked
        for ( int iIdx = 1 + iNumPredefinedServers; iIdx < iCurServerListSize; iIdx++ )
        {
            if ( ServerList[iIdx].HostAddr == InetAddr )
            {
                // remove this list entry
                ServerList.removeAt ( iIdx );

                // entry found, leave for-loop
                continue;
            }
        }
    }
}

void CServerListManager::QueryServerList ( const CHostAddress& InetAddr )
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
                // case he has to connect to the local host address,
                // unfortunately, the port number is not known (because of NAT
                // port translating) so we assume that it is the default llcon
                // port number
                if ( vecServerInfo[iIdx].HostAddr.InetAddr == InetAddr.InetAddr )
                {
                    vecServerInfo[iIdx].HostAddr.InetAddr =
                        QHostAddress ( QHostAddress::LocalHost );

                    vecServerInfo[iIdx].HostAddr.iPort = LLCON_DEFAULT_PORT_NUMBER;
                }
                else
                {
                    // create "send empty message" for all registered servers
                    // (except of the very first list entry since this is this
                    // server (central server) per definition) and also it is
                    // not required to send this message, if the server is on
                    // the same computer
                    pConnLessProtocol->CreateCLSendEmptyMesMes ( 
                        vecServerInfo[iIdx].HostAddr,
                        InetAddr );
                }
            }
        }

        // send the server list to the client
        pConnLessProtocol->CreateCLServerListMes ( InetAddr, vecServerInfo );
    }
}


/* Slave server functionality *************************************************/
void CServerListManager::OnTimerRegistering()
{
    // we need the lock since the user might change the server properties at
    // any time
    QMutexLocker locker ( &Mutex );

    if ( !bIsCentralServer && bEnabled )
    {
        // get the correct central server address
        QString strCurCentrServAddr;
        if ( bUseDefaultCentralServerAddress )
        {
            strCurCentrServAddr = DEFAULT_SERVER_ADDRESS;
        }
        else
        {
            strCurCentrServAddr = strCentralServerAddress;
        }

        // For the slave server, the slave server properties are store in the
        // very first item in the server list (which is actually no server list
        // but just one item long for the slave server).
        // Note that we always have to parse the server address again since if
        // it is an URL of a dynamic IP address, the IP address might have
        // changed in the meanwhile.
        CHostAddress HostAddress;
        if ( LlconNetwUtil().ParseNetworkAddress ( strCurCentrServAddr,
                                                   HostAddress ) )
        {
            pConnLessProtocol->CreateCLRegisterServerMes ( HostAddress,
                                                           ServerList[0] );
        }
    }
}
