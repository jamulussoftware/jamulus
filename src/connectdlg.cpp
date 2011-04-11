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

// TEST
pListViewItem = new QTreeWidgetItem ( ListViewServers );


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



// TODO
// only activate ping timer if window is actually shown
//TimerPing.start ( PING_UPDATE_TIME_MS );

//    UpdateDisplay();
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

void CConnectDlg::SetServerList ( const CVector<CServerInfo>& vecServerInfo )
{
    // set flag
    bServerListReceived = true;


// TODO

}

void CConnectDlg::OnTimerPing()
{
    // send ping message to server

// TEST
//QHostAddress test ( "127.0.0.1" );
//emit CreateCLPingMes ( CHostAddress ( test, 22124 ) );

}

void CConnectDlg::SetPingTimeResult ( CHostAddress& InetAddr,
                                      const int iPingTime )
{

// TEST
pListViewItem->setText ( 0, QString().setNum ( iPingTime ) );

}
