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
      strCentralServerAddress  ( "" ),
      strSelectedAddress       ( "" ),
      bServerListReceived      ( false ),
      bStateOK                 ( false ),
      bServerListItemWasChosen ( false )
{
    setupUi ( this );


    // Add help text to controls -----------------------------------------------
    // server address
    QString strServAddrH = tr ( "<b>Server Address:</b> The IP address or URL "
        "of the server running the llcon server software must be set here. "
        "A list of the most recent used server URLs is available for "
        "selection." );

    TextLabelServerAddr->setWhatsThis ( strServAddrH );
    LineEditServerAddr->setWhatsThis  ( strServAddrH );

    LineEditServerAddr->setAccessibleName        ( tr ( "Server address edit box" ) );
    LineEditServerAddr->setAccessibleDescription ( tr ( "Holds the current server "
        "URL. It also stores old URLs in the combo box list." ) );


    // init server address combo box (max MAX_NUM_SERVER_ADDR_ITEMS entries)
    LineEditServerAddr->setMaxCount     ( MAX_NUM_SERVER_ADDR_ITEMS );
    LineEditServerAddr->setInsertPolicy ( QComboBox::NoInsert );

    // set up list view for connected clients
    ListViewServers->setColumnWidth ( 0, 170 );
    ListViewServers->setColumnWidth ( 1, 65 );
    ListViewServers->setColumnWidth ( 2, 55 );
    ListViewServers->setColumnWidth ( 3, 130 );
    ListViewServers->clear();


    // Connections -------------------------------------------------------------
    // list view
    QObject::connect ( ListViewServers,
        SIGNAL ( itemSelectionChanged() ),
        this, SLOT ( OnServerListItemSelectionChanged() ) );

    QObject::connect ( ListViewServers,
        SIGNAL ( itemDoubleClicked ( QTreeWidgetItem*, int ) ),
        this, SLOT ( OnServerListItemDoubleClicked ( QTreeWidgetItem*, int ) ) );

    // combo boxes
    QObject::connect ( LineEditServerAddr,
        SIGNAL ( editTextChanged ( const QString& ) ),
        this, SLOT ( OnLineEditServerAddrEditTextChanged ( const QString& ) ) );

    // buttons
    QObject::connect ( CancelButton, SIGNAL ( clicked() ),
        this, SLOT ( close() ) );

    QObject::connect ( ConnectButton, SIGNAL ( clicked() ),
        this, SLOT ( OnConnectButtonClicked() ) );

    // timers
    QObject::connect ( &TimerPing, SIGNAL ( timeout() ),
        this, SLOT ( OnTimerPing() ) );

    QObject::connect ( &TimerReRequestServList, SIGNAL ( timeout() ),
        this, SLOT ( OnTimerReRequestServList() ) );
}

void CConnectDlg::Init ( const QString           strNewCentralServerAddr, 
                         const CVector<QString>& vstrIPAddresses )
{
    // take central server address string
    strCentralServerAddress = strNewCentralServerAddr;

    // load stored IP addresses in combo box
    LineEditServerAddr->clear();
    for ( int iLEIdx = 0; iLEIdx < MAX_NUM_SERVER_ADDR_ITEMS; iLEIdx++ )
    {
        if ( !vstrIPAddresses[iLEIdx].isEmpty() )
        {
            LineEditServerAddr->addItem ( vstrIPAddresses[iLEIdx] );
        }
    }
}

void CConnectDlg::showEvent ( QShowEvent* )
{
    // reset flags (on opening the connect dialg, we always want to request a
    // new updated server list per definition)
    bServerListReceived      = false;
    bStateOK                 = false;
    bServerListItemWasChosen = false;

    // clear current address
    strSelectedAddress = "";

    // clear server list view
    ListViewServers->clear();

    // get the IP address of the central server (using the ParseNetworAddress
    // function) when the connect dialog is opened, this seems to be the correct
    // time to do it
    if ( LlconNetwUtil().ParseNetworkAddress ( strCentralServerAddress,
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
    // get the IP address to be used according to the following definitions:
    // - if the list has focus and a line is selected, use this line
    // - if the list has no focus, use the current combo box text
    QList<QTreeWidgetItem*> CurSelListItemList =
        ListViewServers->selectedItems();

    if ( CurSelListItemList.count() > 0 )
    {
        // get host address from selected list view item as a string
        strSelectedAddress =
            CurSelListItemList[0]->data ( 0, Qt::UserRole ).toString();

        // set flag that a server list item was chosen to connect
        bServerListItemWasChosen = true;
    }
    else
    {
        strSelectedAddress = LineEditServerAddr->currentText();
    }

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
    // set flag and disable timer for resend server list request
    bServerListReceived = true;
    TimerReRequestServList.stop();

    // first clear list
    ListViewServers->clear();

    // add list item for each server in the server list
    const int iServerInfoLen = vecServerInfo.Size();

    for ( int iIdx = 0; iIdx < iServerInfoLen; iIdx++ )
    {
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

        // create new list view item
        QTreeWidgetItem* pNewListViewItem =
            new QTreeWidgetItem ( ListViewServers );

        // make the entry invisible (will be set to visible on successful ping
        // result)
        pNewListViewItem->setHidden ( true );

        // server name (if empty, show host address instead)
        if ( !vecServerInfo[iIdx].strName.isEmpty() )
        {
            pNewListViewItem->setText ( 0, vecServerInfo[iIdx].strName );
        }
        else
        {
            // IP address and port (use IP number without last byte)
            // Definition: If the port number is the default port number, we do
            // not show it.
            if ( vecServerInfo[iIdx].HostAddr.iPort == LLCON_DEFAULT_PORT_NUMBER )
            {
                // only show IP number, no port number
                pNewListViewItem->setText ( 0, CurHostAddress.
                    toString ( CHostAddress::SM_IP_NO_LAST_BYTE ) );
            }
            else
            {
                // show IP number and port
                pNewListViewItem->setText ( 0, CurHostAddress.
                    toString ( CHostAddress::SM_IP_NO_LAST_BYTE_PORT ) );
            }
        }

        // the ping time shall be shown in bold font
        QFont CurPingTimeFont = pNewListViewItem->font( 3 );
        CurPingTimeFont.setBold ( true );
        pNewListViewItem->setFont ( 1, CurPingTimeFont );

        // server location (city and country)
        QString strLocation = vecServerInfo[iIdx].strCity;
        if ( !strLocation.isEmpty() )
        {
            strLocation += ", ";
        }
        if ( vecServerInfo[iIdx].eCountry != QLocale::AnyCountry )
        {
            strLocation +=
                QLocale::countryToString ( vecServerInfo[iIdx].eCountry );
        }

        pNewListViewItem->setText ( 3, strLocation );

        // store host address
        pNewListViewItem->setData ( 0, Qt::UserRole,
            CurHostAddress.toString() );
    }

    // immediately issue the ping measurements and start the ping timer since
    // the server list is filled now
    OnTimerPing();
    TimerPing.start ( PING_UPDATE_TIME_SERVER_LIST_MS );
}

void CConnectDlg::OnServerListItemSelectionChanged()
{
    // get current selected item (we are only interested in the first selcted
    // item)
    QList<QTreeWidgetItem*> CurSelListItemList =
        ListViewServers->selectedItems();

    // if an item is clicked/selected, copy the server name to the combo box
    if ( CurSelListItemList.count() > 0 )
    {
        // make sure no signals are send when we change the text
        LineEditServerAddr->blockSignals ( true );
        {
            LineEditServerAddr->setEditText (
                CurSelListItemList[0]->text ( 0 ) );
        }
        LineEditServerAddr->blockSignals ( false );
    }
}

void CConnectDlg::OnServerListItemDoubleClicked ( QTreeWidgetItem* Item,
                                                  int )
{
    // if a server list item was double clicked, it is the same as if the
    // connect button was clicked
    if ( Item != 0 )
    {
        OnConnectButtonClicked();
    }
}

void CConnectDlg::OnLineEditServerAddrEditTextChanged ( const QString& )
{
    // in the server address combo box, a text was changed, remove selection
    // in the server list (if any)
    ListViewServers->clearSelection();
}

void CConnectDlg::OnConnectButtonClicked()
{
    // set state OK flag
    bStateOK = true;

    // close dialog
    close();
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

void CConnectDlg::SetPingTimeAndNumClientsResult ( CHostAddress& InetAddr,
                                                   const int     iPingTime,
                                                   const int     iPingTimeLEDColor,
                                                   const int     iNumClients )
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
                    topLevelItem ( iIdx )->setTextColor ( 1, Qt::darkGreen );
                break;

            case MUL_COL_LED_YELLOW:
                ListViewServers->
                    topLevelItem ( iIdx )->setTextColor ( 1, Qt::darkYellow );
                break;

            case MUL_COL_LED_RED:
                ListViewServers->
                    topLevelItem ( iIdx )->setTextColor ( 1, Qt::red );
                break;
            }

            // update ping text
            ListViewServers->topLevelItem ( iIdx )->
                setText ( 1, QString().setNum ( iPingTime ) + " ms" );

            // update number of clients text
            ListViewServers->topLevelItem ( iIdx )->
                setText ( 2, QString().setNum ( iNumClients ) );

            // a ping time was received, set item to visible
            ListViewServers->topLevelItem ( iIdx )->setHidden ( false );
        }
    }
}
