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
    ListViewServers->setColumnWidth ( 3, 65 );
    ListViewServers->setColumnWidth ( 4, 140 );
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

        // make the entry invisible (will be set to visible on successful ping
        // result)
        pNewListViewItem->setHidden ( true );

        // server name
        pNewListViewItem->setText ( 0, vecServerInfo[iIdx].strName );

        // server country
        pNewListViewItem->setText ( 1,
            QLocale::countryToString ( vecServerInfo[iIdx].eCountry ) );

        // number of clients
        pNewListViewItem->setText ( 2,
            QString().setNum ( vecServerInfo[iIdx].iNumClients ) );

        // the ping time shall be shown in bold font
        QFont CurPingTimeFont = pNewListViewItem->font( 3 );
        CurPingTimeFont.setBold ( true );
        pNewListViewItem->setFont ( 3, CurPingTimeFont );

        // get the host address, note that for the very first entry which is
        // the central server, we have to use the receive host address
        // instead
        CHostAddress CurHostAddress;
        if ( iIdx > 0 )
        {
            CurHostAddress = vecServerInfo[iIdx].HostAddr;
        }
        else
        {
            // substitude the receive host address for central server
            CurHostAddress = InetAddr;
        }

        // IP address and port (use IP number without last byte)
        // Definition: If the port number is the default port number, we do not
        // show it.
        if ( vecServerInfo[iIdx].HostAddr.iPort == LLCON_DEFAULT_PORT_NUMBER )
        {
            // only show IP number, no port number
            pNewListViewItem->setText ( 4, CurHostAddress.
                toString ( CHostAddress::SM_IP_NO_LAST_BYTE ) );
        }
        else
        {
            // show IP number and port
            pNewListViewItem->setText ( 4, CurHostAddress.
                toString ( CHostAddress::SM_IP_NO_LAST_BYTE_PORT ) );
        }

        // store host address
        pNewListViewItem->setData ( 0, Qt::UserRole,
            CurHostAddress.toString() );
    }

    // immediately issue the ping measurements and start the ping timer since
    // the server list is filled now
    OnTimerPing();
    TimerPing.start ( PING_UPDATE_TIME_SERVER_LIST_MS );
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
                                      const int     iPingTime,
                                      const int     iPingTimeLEDColor )
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
            // update the color of the ping time font
            switch ( iPingTimeLEDColor )
            {
            case MUL_COL_LED_GREEN:
                ListViewServers->
                    topLevelItem ( iIdx )->setTextColor ( 3, Qt::darkGreen );
                break;

            case MUL_COL_LED_YELLOW:
                ListViewServers->
                    topLevelItem ( iIdx )->setTextColor ( 3, Qt::darkYellow );
                break;

            case MUL_COL_LED_RED:
                ListViewServers->
                    topLevelItem ( iIdx )->setTextColor ( 3, Qt::red );
                break;
            }

            // update ping text
            ListViewServers->topLevelItem ( iIdx )->
                setText ( 3, QString().setNum ( iPingTime ) + " ms" );

            // a ping time was received, set item to visible
            ListViewServers->topLevelItem ( iIdx )->setHidden ( false );
        }
    }
}
