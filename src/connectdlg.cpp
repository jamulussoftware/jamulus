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

#include "connectdlg.h"


/* Implementation *************************************************************/
CConnectDlg::CConnectDlg ( QWidget* parent, Qt::WindowFlags f )
    : QDialog ( parent, f ),
      bServerListReceived ( false )
{
    setupUi ( this );


    // Add help text to controls -----------------------------------------------

// TODO


    // set up list view for connected clients
    ListViewServers->setColumnWidth ( 0, 170 );
    ListViewServers->setColumnWidth ( 1, 130 );
    ListViewServers->setColumnWidth ( 2, 55 );
    ListViewServers->setColumnWidth ( 3, 80 );
    ListViewServers->clear();


    // Connections -------------------------------------------------------------
    // timers
    QObject::connect ( &TimerPing, SIGNAL ( timeout() ),
        this, SLOT ( OnTimerPing() ) );

    QObject::connect ( &TimerReRequestServList, SIGNAL ( timeout() ),
        this, SLOT ( OnTimerReRequestServList() ) );
}

void CConnectDlg::showEvent ( QShowEvent* )
{
    // reset flag (on opening the connect dialg, we always want to request a new
    // updated server list per definition)
    bServerListReceived = false;


// TEST
QString strNAddr = "llcon.dyndns.org:22122";


    // get the IP address of the central server (using the ParseNetworAddress
    // function) when the connect dialog is opened, this seems to be the correct
    // time to do it
    if ( LlconNetwUtil().ParseNetworkAddress ( strNAddr,
                                               CentralServerAddress ) )
    {
        // send the request for the server list
        emit ReqServerListQuery ( CentralServerAddress );

        // start timer, if this message did not get any respond to retransmit
        // the server list request message
        TimerReRequestServList.start ( SERV_LIST_REQ_UPDATE_TIME_MS );
    }
}
	
void CConnectDlg::hideEvent ( QHideEvent* )
{
    // if window is closed, stop timers
    TimerPing.stop();
    TimerReRequestServList.stop();
}

void CConnectDlg::OnTimerReRequestServList()
{
    // if the server list is not yet received, retransmit the request for the
    // server list
    if ( !bServerListReceived )
    {
        emit ReqServerListQuery ( CentralServerAddress );
    }
}

void CConnectDlg::SetServerList ( const CHostAddress&         InetAddr,
                                  const CVector<CServerInfo>& vecServerInfo )
{
    // set flag
    bServerListReceived = true;

    // first clear list
    ListViewServers->clear();

    // add list item for each server in the server list
    const int iServerInfoLen = vecServerInfo.Size();

    for ( int iIdx = 0; iIdx < iServerInfoLen; iIdx++ )
    {
        QTreeWidgetItem* pNewListViewItem =
            new QTreeWidgetItem ( ListViewServers );

        // server name
        pNewListViewItem->setText ( 0, vecServerInfo[iIdx].strName );

        // server country
        pNewListViewItem->setText ( 1,
            QLocale::countryToString ( vecServerInfo[iIdx].eCountry ) );

        // number of clients
        pNewListViewItem->setText ( 2,
            QString().setNum ( vecServerInfo[iIdx].iNumClients ) );

        // store host address, note that for the very first entry which is
        // the central server, we have to use the receive host address
        // instead
        if ( iIdx > 0 )
        {
            pNewListViewItem->setData ( 0, Qt::UserRole,
                vecServerInfo[iIdx].HostAddr.toString() );
        }
        else
        {
            // substitude the receive host address for central server
            pNewListViewItem->setData ( 0, Qt::UserRole, InetAddr.toString() );
        }


// TEST
pNewListViewItem->setText ( 2,
    pNewListViewItem->data ( 0, Qt::UserRole ).toString() );


    }

    // start the ping timer since the server list is filled now
    TimerPing.start ( PING_UPDATE_TIME_MS );
}

void CConnectDlg::OnTimerPing()
{
    // send ping messages to the servers in the list
    const int iServerListLen = ListViewServers->topLevelItemCount();

    for ( int iIdx = 0; iIdx < iServerListLen; iIdx++ )
    {
        CHostAddress CurServerAddress;

        // try to parse host address string which is stored as user data
        // in the server list item GUI control element
        if ( LlconNetwUtil().ParseNetworkAddress (
                ListViewServers->topLevelItem ( iIdx )->
                data ( 0, Qt::UserRole ).toString(),
                CurServerAddress ) )
        {
            // if address is valid, send ping
            emit CreateCLPingMes ( CurServerAddress );
        }
    }
}

void CConnectDlg::SetPingTimeResult ( CHostAddress& InetAddr,
                                      const int     iPingTime )
{
    // apply the received ping time to the correct server list entry
    const int iServerListLen = ListViewServers->topLevelItemCount();

    for ( int iIdx = 0; iIdx < iServerListLen; iIdx++ )
    {
        // compare the received address with the user data string of the
        // host address by a string compare
        if ( !ListViewServers->topLevelItem ( iIdx )->
                data ( 0, Qt::UserRole ).toString().
                compare ( InetAddr.toString() ) )
        {
            ListViewServers->topLevelItem ( iIdx )->
                setText ( 3, QString().setNum ( iPingTime ) );
        }
    }
}
