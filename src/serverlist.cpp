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
CServerListManager::CServerListManager ( const bool NbEbld,
                                         const bool NbIsCentralServer,
                                         CProtocol* pNConLProt )
    : bIsCentralServer ( NbIsCentralServer ),
      pConnLessProtocol ( pNConLProt )
{
    // per definition, the very first entry is this server and this entry will
    // never be deleted
    ServerList.clear();

// per definition the client substitudes the IP address of the central server
// itself for his server list
ServerList.append ( CServerListEntry (
    CHostAddress(),
    "Master Server", // TEST
    "",
    QLocale::Germany, // TEST
    "Munich", // TEST
    0, // will be updated later
    USED_NUM_CHANNELS,
    true ) ); // TEST



    // Connections -------------------------------------------------------------
    QObject::connect ( &TimerPollList, SIGNAL ( timeout() ),
        this, SLOT ( OnTimerPollList() ) );


    // call set enable function after the connection of the timer since in this
    // function the timer gets started
    SetEnabled ( NbEbld );
}

void CServerListManager::SetEnabled ( const bool bState )
{
    QMutexLocker locker ( &Mutex );

    bEnabled = bState;

    if ( bEnabled )
    {
        if ( bIsCentralServer )
        {
            // start timer for polling the server list if enabled
            // 1 minute = 60 * 1000 ms
            TimerPollList.start ( SERVLIST_POLL_TIME_MINUTES * 60000 );
        }
    }
    else
    {
        if ( bIsCentralServer )
        {
            // disable service -> stop timer
            TimerPollList.stop();
        }
    }
}


/* Central server functionality ***********************************************/
void CServerListManager::OnTimerPollList()
{
    QMutexLocker locker ( &Mutex );

    // check all list entries except of the very first one (which is the central
    // server entry) if they are still valid
    for ( int iIdx = 1; iIdx < ServerList.size(); iIdx++ )
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

            // Check if server is already registered. Use IP number and port
            // number to fully identify a server. The very first list entry must
            // not be checked since this is per definition the central server
            // (i.e., this server)
            int iSelIdx = ciInvalidIdx; // initialize with an illegal value
            for ( int iIdx = 1; iIdx < iCurServerListSize; iIdx++ )
            {
                if ( ServerList[iIdx].HostAddr == InetAddr )
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
                ServerList[iSelIdx].strName          = ServerInfo.strName;
                ServerList[iSelIdx].strTopic         = ServerInfo.strTopic;
                ServerList[iSelIdx].eCountry         = ServerInfo.eCountry;
                ServerList[iSelIdx].strCity          = ServerInfo.strCity;
                ServerList[iSelIdx].iNumClients      = ServerInfo.iNumClients;
                ServerList[iSelIdx].iMaxNumClients   = ServerInfo.iMaxNumClients;
                ServerList[iSelIdx].bPermanentOnline = ServerInfo.bPermanentOnline;
                ServerList[iSelIdx].UpdateRegistration();
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
        for ( int iIdx = 1; iIdx < iCurServerListSize; iIdx++ )
        {
            vecServerInfo[iIdx] = ServerList[iIdx];
        }

        // send the server list to the client
        pConnLessProtocol->CreateCLServerListMes ( InetAddr, vecServerInfo );


// TODO create "send empty message" for all registered servers

    }
}


/* Slave server functionality *************************************************/

// TODO
