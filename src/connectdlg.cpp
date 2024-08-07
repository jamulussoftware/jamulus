/******************************************************************************\
 * Copyright (c) 2004-2024
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

#include "connectdlg.h"

/* Implementation *************************************************************/
CConnectDlg::CConnectDlg ( CClientSettings* pNSetP, const bool bNewShowCompleteRegList, const bool bNEnableIPv6, QWidget* parent ) :
    CBaseDlg ( parent, Qt::Dialog ),
    pSettings ( pNSetP ),
    strSelectedAddress ( "" ),
    strSelectedServerName ( "" ),
    bShowCompleteRegList ( bNewShowCompleteRegList ),
    bServerListReceived ( false ),
    bReducedServerListReceived ( false ),
    bServerListItemWasChosen ( false ),
    bListFilterWasActive ( false ),
    bShowAllMusicians ( true ),
    bEnableIPv6 ( bNEnableIPv6 )
{
    setupUi ( this );

    // Add help text to controls -----------------------------------------------
    // directory
    QString strDirectoryWT = "<b>" + tr ( "Directory" ) + ":</b> " +
                             tr ( "Shows the servers listed by the selected directory. "
                                  "You can add custom directories in Advanced Settings." );
    QString strDirectoryAN = tr ( "Directory combo box" );

    lblList->setWhatsThis ( strDirectoryWT );
    lblList->setAccessibleName ( strDirectoryAN );
    cbxDirectory->setWhatsThis ( strDirectoryWT );
    cbxDirectory->setAccessibleName ( strDirectoryAN );

    // filter
    QString strFilterWT = "<b>" + tr ( "Filter" ) + ":</b> " +
                          tr ( "Filters the server list by the given text. Note that the filter is case insensitive. "
                               "A single # character will filter for those servers with at least one person connected." );
    QString strFilterAN = tr ( "Filter edit box" );
    lblFilter->setWhatsThis ( strFilterWT );
    edtFilter->setWhatsThis ( strFilterWT );

    lblFilter->setAccessibleName ( strFilterAN );
    edtFilter->setAccessibleName ( strFilterAN );

    // show all mucisians
    chbExpandAll->setWhatsThis ( "<b>" + tr ( "Show All Musicians" ) + ":</b> " +
                                 tr ( "Uncheck to collapse the server list to show just the server details. "
                                      "Check to show everyone on the servers." ) );

    chbExpandAll->setAccessibleName ( tr ( "Show all musicians check box" ) );

    // server list view
    lvwServers->setWhatsThis ( "<b>" + tr ( "Server List" ) + ":</b> " +
                               tr ( "The Connection Setup window lists the available servers registered with "
                                    "the selected directory. Use the Directory dropdown to change the directory, "
                                    "find the server you want to join in the server list, click on it, and "
                                    "then click the Connect button to connect. Alternatively, double click on "
                                    "the server name to connect." ) +
                               "<br>" + tr ( "Permanent servers (those that have been listed for longer than 48 hours) are shown in bold." ) +
                               "<br>" + tr ( "You can add custom directories in Advanced Settings." ) );

    lvwServers->setAccessibleName ( tr ( "Server list view" ) );

    // server address
    QString strServAddrH = "<b>" + tr ( "Server Address" ) + ":</b> " +
                           tr ( "If you know the server address, you can connect to it "
                                "using the Server name/Address field. An optional port number can be added after the server "
                                "address using a colon as a separator, e.g. %1. "
                                "The field will also show a list of the most recently used server addresses." )
                               .arg ( QString ( "<tt>example.org:%1</tt>" ).arg ( DEFAULT_PORT_NUMBER ) );

    lblServerAddr->setWhatsThis ( strServAddrH );
    cbxServerAddr->setWhatsThis ( strServAddrH );

    cbxServerAddr->setAccessibleName ( tr ( "Server address edit box" ) );
    cbxServerAddr->setAccessibleDescription ( tr ( "Holds the current server address. It also stores old addresses in the combo box list." ) );

    tbtDeleteServerAddr->setAccessibleName ( tr ( "Delete server address button" ) );
    tbtDeleteServerAddr->setWhatsThis ( "<b>" + tr ( "Delete Server Address" ) + ":</b> " +
                                        tr ( "Click the button to clear the currently selected server address "
                                             "and delete it from the list of stored servers." ) );
    tbtDeleteServerAddr->setText ( u8"\u232B" );

    UpdateDirectoryComboBox();

    // init server address combo box (max MAX_NUM_SERVER_ADDR_ITEMS entries)
    cbxServerAddr->setMaxCount ( MAX_NUM_SERVER_ADDR_ITEMS );
    cbxServerAddr->setInsertPolicy ( QComboBox::NoInsert );

    // set up list view for connected clients (note that the last column size
    // must not be specified since this column takes all the remaining space)
#ifdef ANDROID
    // for Android we need larger numbers because of the default font size
    lvwServers->setColumnWidth ( 0, 200 );
    lvwServers->setColumnWidth ( 1, 130 );
    lvwServers->setColumnWidth ( 2, 100 );
#else
    lvwServers->setColumnWidth ( 0, 180 );
    lvwServers->setColumnWidth ( 1, 75 );
    lvwServers->setColumnWidth ( 2, 70 );
    lvwServers->setColumnWidth ( 3, 220 );
#endif
    lvwServers->clear();

    // make sure we do not get a too long horizontal scroll bar
    lvwServers->header()->setStretchLastSection ( false );

    // add invisible columns which are used for sorting the list and storing
    // the current/maximum number of clients
    // 0: server name
    // 1: ping time
    // 2: number of musicians (including additional strings like " (full)")
    // 3: location
    // 4: minimum ping time (invisible)
    // 5: maximum number of clients (invisible)
    lvwServers->setColumnCount ( 6 );
    lvwServers->hideColumn ( 4 );
    lvwServers->hideColumn ( 5 );

    // per default the root shall not be decorated (to save space)
    lvwServers->setRootIsDecorated ( false );

    // make sure the connect button has the focus
    butConnect->setFocus();

    // for "show all servers" mode make sort by click on header possible
    if ( bShowCompleteRegList )
    {
        lvwServers->setSortingEnabled ( true );
        lvwServers->sortItems ( 0, Qt::AscendingOrder );
    }

    // set a placeholder text to explain how to filter occupied servers (#397)
    edtFilter->setPlaceholderText ( tr ( "Filter text, or # for occupied servers" ) );

    // setup timers
    TimerInitialSort.setSingleShot ( true ); // only once after list request

#if defined( ANDROID ) || defined( Q_OS_IOS )
    // for the Android and iOS version maximize the window
    setWindowState ( Qt::WindowMaximized );
#endif

    // Connections -------------------------------------------------------------
    // list view
    QObject::connect ( lvwServers, &QTreeWidget::itemDoubleClicked, this, &CConnectDlg::OnServerListItemDoubleClicked );

    // to get default return key behaviour working
    QObject::connect ( lvwServers, &QTreeWidget::activated, this, &CConnectDlg::OnConnectClicked );

    // line edit
    QObject::connect ( edtFilter, &QLineEdit::textEdited, this, &CConnectDlg::OnFilterTextEdited );

    // combo boxes
    QObject::connect ( cbxServerAddr, &QComboBox::editTextChanged, this, &CConnectDlg::OnServerAddrEditTextChanged );

    QObject::connect ( cbxDirectory, static_cast<void ( QComboBox::* ) ( int )> ( &QComboBox::activated ), this, &CConnectDlg::OnDirectoryChanged );

    // check boxes
    QObject::connect ( chbExpandAll, &QCheckBox::stateChanged, this, &CConnectDlg::OnExpandAllStateChanged );

    // buttons
    QObject::connect ( butCancel, &QPushButton::clicked, this, &CConnectDlg::close );

    QObject::connect ( butConnect, &QPushButton::clicked, this, &CConnectDlg::OnConnectClicked );

    // tool buttons
    QObject::connect ( tbtDeleteServerAddr, &QToolButton::clicked, this, &CConnectDlg::OnDeleteServerAddrClicked );

    // timers
    QObject::connect ( &TimerPing, &QTimer::timeout, this, &CConnectDlg::OnTimerPing );

    QObject::connect ( &TimerReRequestServList, &QTimer::timeout, this, &CConnectDlg::OnTimerReRequestServList );
}

void CConnectDlg::showEvent ( QShowEvent* )
{
    // load stored IP addresses in combo box
    cbxServerAddr->clear();
    cbxServerAddr->clearEditText();

    for ( int iLEIdx = 0; iLEIdx < MAX_NUM_SERVER_ADDR_ITEMS; iLEIdx++ )
    {
        if ( !pSettings->vstrIPAddress[iLEIdx].isEmpty() )
        {
            cbxServerAddr->addItem ( pSettings->vstrIPAddress[iLEIdx] );
        }
    }

    // on opening the connect dialg, we always want to request a
    // new updated server list per definition
    RequestServerList();
}

void CConnectDlg::RequestServerList()
{
    // reset flags
    bServerListReceived        = false;
    bReducedServerListReceived = false;
    bServerListItemWasChosen   = false;
    bListFilterWasActive       = false;

    // clear current address and name
    strSelectedAddress    = "";
    strSelectedServerName = "";

    // clear server list view
    lvwServers->clear();

    // update list combo box (disable events to avoid a signal)
    cbxDirectory->blockSignals ( true );
    if ( pSettings->eDirectoryType == AT_CUSTOM )
    {
        // iCustomDirectoryIndex is non-zero only if eDirectoryType == AT_CUSTOM
        // find the combobox item that corresponds to vstrDirectoryAddress[iCustomDirectoryIndex]
        // (the current selected custom directory)
        cbxDirectory->setCurrentIndex ( cbxDirectory->findData ( QVariant ( pSettings->iCustomDirectoryIndex ) ) );
    }
    else
    {
        cbxDirectory->setCurrentIndex ( static_cast<int> ( pSettings->eDirectoryType ) );
    }
    cbxDirectory->blockSignals ( false );

    // Get the IP address of the directory server (using the ParseNetworAddress
    // function) when the connect dialog is opened, this seems to be the correct
    // time to do it. Note that in case of custom directories we
    // use iCustomDirectoryIndex as an index into the vector.

    // Allow IPv4 only for communicating with Directories
    if ( NetworkUtil().ParseNetworkAddress (
             NetworkUtil::GetDirectoryAddress ( pSettings->eDirectoryType, pSettings->vstrDirectoryAddress[pSettings->iCustomDirectoryIndex] ),
             haDirectoryAddress,
             false ) )
    {
        // send the request for the server list
        emit ReqServerListQuery ( haDirectoryAddress );

        // start timer, if this message did not get any respond to retransmit
        // the server list request message
        TimerReRequestServList.start ( SERV_LIST_REQ_UPDATE_TIME_MS );
        TimerInitialSort.start ( SERV_LIST_REQ_UPDATE_TIME_MS ); // reuse the time value
    }
}

void CConnectDlg::hideEvent ( QHideEvent* )
{
    // if window is closed, stop timers
    TimerPing.stop();
    TimerReRequestServList.stop();
}

void CConnectDlg::OnDirectoryChanged ( int iTypeIdx )
{
    // store the new directory type and request new list
    // if iTypeIdx == AT_CUSTOM, then iCustomDirectoryIndex is the index into the vector holding the user's custom directory servers
    // if iTypeIdx != AT_CUSTOM, then iCustomDirectoryIndex MUST be 0;
    if ( iTypeIdx >= AT_CUSTOM )
    {
        // the value for the index into the vector vstrDirectoryAddress is in the user data of the combobox item
        pSettings->iCustomDirectoryIndex = cbxDirectory->itemData ( iTypeIdx ).toInt();
        iTypeIdx                         = AT_CUSTOM;
    }
    else
    {
        pSettings->iCustomDirectoryIndex = 0;
    }
    pSettings->eDirectoryType = static_cast<EDirectoryType> ( iTypeIdx );
    RequestServerList();
}

void CConnectDlg::OnTimerReRequestServList()
{
    // if the server list is not yet received, retransmit the request for the
    // server list
    if ( !bServerListReceived )
    {
        // note that this is a connection less message which may get lost
        // and therefore it makes sense to re-transmit it
        emit ReqServerListQuery ( haDirectoryAddress );
    }
}

void CConnectDlg::SetServerList ( const CHostAddress& InetAddr, const CVector<CServerInfo>& vecServerInfo, const bool bIsReducedServerList )
{
    // If the normal list was received, we do not accept any further list
    // updates (to avoid the reduced list overwrites the normal list (#657)). Also,
    // we only accept a server list from the server address we have sent the
    // request for this to (note that we cannot use the port number since the
    // receive port and send port might be different at the directory server).
    if ( bServerListReceived || ( InetAddr.InetAddr != haDirectoryAddress.InetAddr ) )
    {
        return;
    }

    // special treatment if a reduced server list was received
    if ( bIsReducedServerList )
    {
        // make sure we only apply the reduced version list once
        if ( bReducedServerListReceived )
        {
            // do nothing
            return;
        }
        else
        {
            bReducedServerListReceived = true;
        }
    }
    else
    {
        // set flag and disable timer for resend server list request if full list
        // was received (i.e. not the reduced list)
        bServerListReceived = true;
        TimerReRequestServList.stop();
    }

    // first clear list
    lvwServers->clear();

    // add list item for each server in the server list
    const int iServerInfoLen = vecServerInfo.Size();

    for ( int iIdx = 0; iIdx < iServerInfoLen; iIdx++ )
    {
        // get the host address, note that for the very first entry which is
        // the directory server, we have to use the receive host address
        // instead
        CHostAddress CurHostAddress;

        if ( iIdx > 0 )
        {
            CurHostAddress = vecServerInfo[iIdx].HostAddr;
        }
        else
        {
            // substitute the receive host address for directory server
            CurHostAddress = InetAddr;
        }

        // create new list view item
        QTreeWidgetItem* pNewListViewItem = new QTreeWidgetItem ( lvwServers );

        // make the entry invisible (will be set to visible on successful ping
        // result) if the complete list of registered servers shall not be shown
        if ( !bShowCompleteRegList )
        {
            pNewListViewItem->setHidden ( true );
        }

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
            if ( vecServerInfo[iIdx].HostAddr.iPort == DEFAULT_PORT_NUMBER )
            {
                // only show IP number, no port number
                pNewListViewItem->setText ( 0, CurHostAddress.toString ( CHostAddress::SM_IP_NO_LAST_BYTE ) );
            }
            else
            {
                // show IP number and port
                pNewListViewItem->setText ( 0, CurHostAddress.toString ( CHostAddress::SM_IP_NO_LAST_BYTE_PORT ) );
            }
        }

        // in case of all servers shown, add the registration number at the beginning
        if ( bShowCompleteRegList )
        {
            pNewListViewItem->setText ( 0, QString ( "%1: " ).arg ( 1 + iIdx, 3 ) + pNewListViewItem->text ( 0 ) );
        }

        // show server name in bold font if it is a permanent server
        QFont CurServerNameFont = pNewListViewItem->font ( 0 );
        CurServerNameFont.setBold ( vecServerInfo[iIdx].bPermanentOnline );
        pNewListViewItem->setFont ( 0, CurServerNameFont );

        // the ping time shall be shown in bold font
        QFont CurPingTimeFont = pNewListViewItem->font ( 1 );
        CurPingTimeFont.setBold ( true );
        pNewListViewItem->setFont ( 1, CurPingTimeFont );

        // server location (city and country)
        QString strLocation = vecServerInfo[iIdx].strCity;

        if ( ( !strLocation.isEmpty() ) && ( vecServerInfo[iIdx].eCountry != QLocale::AnyCountry ) )
        {
            strLocation += ", ";
        }

        if ( vecServerInfo[iIdx].eCountry != QLocale::AnyCountry )
        {
            QString strCountryToString = QLocale::countryToString ( vecServerInfo[iIdx].eCountry );

            // Qt countryToString does not use spaces in between country name
            // parts but they use upper case letters which we can detect and
            // insert spaces as a post processing
#if QT_VERSION >= QT_VERSION_CHECK( 5, 0, 0 )
            if ( !strCountryToString.contains ( " " ) )
            {
                QRegularExpressionMatchIterator reMatchIt = QRegularExpression ( "[A-Z][^A-Z]*" ).globalMatch ( strCountryToString );
                QStringList                     slNames;
                while ( reMatchIt.hasNext() )
                {
                    slNames << reMatchIt.next().capturedTexts();
                }
                strCountryToString = slNames.join ( " " );
            }
#endif

            strLocation += strCountryToString;
        }

        pNewListViewItem->setText ( 3, strLocation );

        // init the minimum ping time with a large number (note that this number
        // must fit in an integer type)
        pNewListViewItem->setText ( 4, "99999999" );

        // store the maximum number of clients
        pNewListViewItem->setText ( 5, QString().setNum ( vecServerInfo[iIdx].iMaxNumClients ) );

        // store host address
        pNewListViewItem->setData ( 0, Qt::UserRole, CurHostAddress.toString() );

        // per default expand the list item (if not "show all servers")
        if ( bShowAllMusicians )
        {
            lvwServers->expandItem ( pNewListViewItem );
        }
    }

    // immediately issue the ping measurements and start the ping timer since
    // the server list is filled now
    OnTimerPing();
    TimerPing.start ( PING_UPDATE_TIME_SERVER_LIST_MS );
}

void CConnectDlg::SetConnClientsList ( const CHostAddress& InetAddr, const CVector<CChannelInfo>& vecChanInfo )
{
    // find the server with the correct address
    QTreeWidgetItem* pCurListViewItem = FindListViewItem ( InetAddr );

    if ( pCurListViewItem )
    {
        // first remove any existing children
        DeleteAllListViewItemChilds ( pCurListViewItem );

        // get number of connected clients
        const int iNumConnectedClients = vecChanInfo.Size();

        for ( int i = 0; i < iNumConnectedClients; i++ )
        {
            // create new list view item
            QTreeWidgetItem* pNewChildListViewItem = new QTreeWidgetItem ( pCurListViewItem );

            // child items shall use only one column
            pNewChildListViewItem->setFirstColumnSpanned ( true );

            // set the clients name
            QString sClientText = vecChanInfo[i].strName;

            // set the icon: country flag has priority over instrument
            bool bCountryFlagIsUsed = false;

            if ( vecChanInfo[i].eCountry != QLocale::AnyCountry )
            {
                // try to load the country flag icon
                QPixmap CountryFlagPixmap ( CLocale::GetCountryFlagIconsResourceReference ( vecChanInfo[i].eCountry ) );

                // first check if resource reference was valid
                if ( !CountryFlagPixmap.isNull() )
                {
                    // set correct picture
                    pNewChildListViewItem->setIcon ( 0, QIcon ( CountryFlagPixmap ) );

                    bCountryFlagIsUsed = true;
                }
            }

            if ( !bCountryFlagIsUsed )
            {
                // get the resource reference string for this instrument
                const QString strCurResourceRef = CInstPictures::GetResourceReference ( vecChanInfo[i].iInstrument );

                // first check if instrument picture is used or not and if it is valid
                if ( !( CInstPictures::IsNotUsedInstrument ( vecChanInfo[i].iInstrument ) || strCurResourceRef.isEmpty() ) )
                {
                    // set correct picture
                    pNewChildListViewItem->setIcon ( 0, QIcon ( QPixmap ( strCurResourceRef ) ) );
                }
            }

            // add the instrument information as text
            if ( !CInstPictures::IsNotUsedInstrument ( vecChanInfo[i].iInstrument ) )
            {
                sClientText.append ( " (" + CInstPictures::GetName ( vecChanInfo[i].iInstrument ) + ")" );
            }

            // apply the client text to the list view item
            pNewChildListViewItem->setText ( 0, sClientText );

            // add the new child to the corresponding server item
            pCurListViewItem->addChild ( pNewChildListViewItem );

            // at least one server has children now, show decoration to be able
            // to show the children
            lvwServers->setRootIsDecorated ( true );
        }

        // the clients list may have changed, update the filter selection
        UpdateListFilter();
    }
}

void CConnectDlg::OnServerListItemDoubleClicked ( QTreeWidgetItem* Item, int )
{
    // if a server list item was double clicked, it is the same as if the
    // connect button was clicked
    if ( Item != nullptr )
    {
        OnConnectClicked();
    }
}

void CConnectDlg::OnServerAddrEditTextChanged ( const QString& )
{
    // in the server address combo box, a text was changed, remove selection
    // in the server list (if any)
    lvwServers->clearSelection();
}

void CConnectDlg::OnCustomDirectoriesChanged()
{

    QString strPreviousSelection = cbxDirectory->currentText();
    UpdateDirectoryComboBox();
    // after updating the combobox, we must re-select the previous directory selection

    if ( pSettings->eDirectoryType == AT_CUSTOM )
    {
        // check if the currently select custom directory still exists in the now potentially re-ordered vector,
        // if so, then change to its new index.  (addresses Issue #1899)
        int iNewIndex = cbxDirectory->findText ( strPreviousSelection, Qt::MatchExactly );
        if ( iNewIndex == INVALID_INDEX )
        {
            // previously selected custom directory has been deleted.  change to default directory
            pSettings->eDirectoryType        = static_cast<EDirectoryType> ( AT_DEFAULT );
            pSettings->iCustomDirectoryIndex = 0;
            RequestServerList();
        }
        else
        {
            // find previously selected custom directory in the now potentially re-ordered vector
            pSettings->eDirectoryType        = static_cast<EDirectoryType> ( AT_CUSTOM );
            pSettings->iCustomDirectoryIndex = cbxDirectory->itemData ( iNewIndex ).toInt();
            cbxDirectory->blockSignals ( true );
            cbxDirectory->setCurrentIndex ( cbxDirectory->findData ( QVariant ( pSettings->iCustomDirectoryIndex ) ) );
            cbxDirectory->blockSignals ( false );
        }
    }
    else
    {
        // selected directory was not a custom directory
        cbxDirectory->blockSignals ( true );
        cbxDirectory->setCurrentIndex ( static_cast<int> ( pSettings->eDirectoryType ) );
        cbxDirectory->blockSignals ( false );
    }
}

void CConnectDlg::ShowAllMusicians ( const bool bState )
{
    bShowAllMusicians = bState;

    // update list
    if ( bState )
    {
        lvwServers->expandAll();
    }
    else
    {
        lvwServers->collapseAll();
    }

    // update check box if necessary
    if ( ( chbExpandAll->checkState() == Qt::Checked && !bShowAllMusicians ) || ( chbExpandAll->checkState() == Qt::Unchecked && bShowAllMusicians ) )
    {
        chbExpandAll->setCheckState ( bState ? Qt::Checked : Qt::Unchecked );
    }
}

void CConnectDlg::UpdateListFilter()
{
    const QString sFilterText = edtFilter->text();

    if ( !sFilterText.isEmpty() )
    {
        bListFilterWasActive     = true;
        const int iServerListLen = lvwServers->topLevelItemCount();

        for ( int iIdx = 0; iIdx < iServerListLen; iIdx++ )
        {
            QTreeWidgetItem* pCurListViewItem = lvwServers->topLevelItem ( iIdx );
            bool             bFilterFound     = false;

            // DEFINITION: if "#" is set at the beginning of the filter text, we show
            //             occupied servers (#397)
            if ( ( sFilterText.indexOf ( "#" ) == 0 ) && ( sFilterText.length() == 1 ) )
            {
                // special case: filter for occupied servers
                if ( pCurListViewItem->childCount() > 0 )
                {
                    bFilterFound = true;
                }
            }
            else
            {
                // search server name
                if ( pCurListViewItem->text ( 0 ).indexOf ( sFilterText, 0, Qt::CaseInsensitive ) >= 0 )
                {
                    bFilterFound = true;
                }

                // search location
                if ( pCurListViewItem->text ( 3 ).indexOf ( sFilterText, 0, Qt::CaseInsensitive ) >= 0 )
                {
                    bFilterFound = true;
                }

                // search children
                for ( int iCCnt = 0; iCCnt < pCurListViewItem->childCount(); iCCnt++ )
                {
                    if ( pCurListViewItem->child ( iCCnt )->text ( 0 ).indexOf ( sFilterText, 0, Qt::CaseInsensitive ) >= 0 )
                    {
                        bFilterFound = true;
                    }
                }
            }

            // only update Hide state if ping time was received
            if ( !pCurListViewItem->text ( 1 ).isEmpty() || bShowCompleteRegList )
            {
                // only update hide and expand status if the hide state has to be changed to
                // preserve if user clicked on expand icon manually
                if ( ( pCurListViewItem->isHidden() && bFilterFound ) || ( !pCurListViewItem->isHidden() && !bFilterFound ) )
                {
                    pCurListViewItem->setHidden ( !bFilterFound );
                    pCurListViewItem->setExpanded ( bShowAllMusicians );
                }
            }
        }
    }
    else
    {
        // if the filter was active but is now disabled, we have to update all list
        // view items for the "ping received" hide state
        if ( bListFilterWasActive )
        {
            const int iServerListLen = lvwServers->topLevelItemCount();

            for ( int iIdx = 0; iIdx < iServerListLen; iIdx++ )
            {
                QTreeWidgetItem* pCurListViewItem = lvwServers->topLevelItem ( iIdx );

                // if ping time is empty, hide item (if not already hidden)
                if ( pCurListViewItem->text ( 1 ).isEmpty() && !bShowCompleteRegList )
                {
                    pCurListViewItem->setHidden ( true );
                }
                else
                {
                    // in case it was hidden, show it and take care of expand
                    if ( pCurListViewItem->isHidden() )
                    {
                        pCurListViewItem->setHidden ( false );
                        pCurListViewItem->setExpanded ( bShowAllMusicians );
                    }
                }
            }

            bListFilterWasActive = false;
        }
    }
}

void CConnectDlg::OnConnectClicked()
{
    // get the IP address to be used according to the following definitions:
    // - if the list has focus and a line is selected, use this line
    // - if the list has no focus, use the current combo box text
    QList<QTreeWidgetItem*> CurSelListItemList = lvwServers->selectedItems();

    if ( CurSelListItemList.count() > 0 )
    {
        // get the parent list view item
        QTreeWidgetItem* pCurSelTopListItem = GetParentListViewItem ( CurSelListItemList[0] );

        // get host address from selected list view item as a string
        strSelectedAddress = pCurSelTopListItem->data ( 0, Qt::UserRole ).toString();

        // store selected server name
        strSelectedServerName = pCurSelTopListItem->text ( 0 );

        // set flag that a server list item was chosen to connect
        bServerListItemWasChosen = true;
    }
    else
    {
        strSelectedAddress = NetworkUtil::FixAddress ( cbxServerAddr->currentText() );
    }

    // tell the parent window that the connection shall be initiated
    done ( QDialog::Accepted );
}

void CConnectDlg::OnDeleteServerAddrClicked()
{
    if ( cbxServerAddr->currentText().isEmpty() )
    {
        return;
    }

    // move later items down one
    for ( int iLEIdx = 0; iLEIdx < MAX_NUM_SERVER_ADDR_ITEMS - 1; iLEIdx++ )
    {
        while ( pSettings->vstrIPAddress[iLEIdx].compare ( cbxServerAddr->currentText() ) == 0 )
        {
            for ( int jLEIdx = iLEIdx + 1; jLEIdx < MAX_NUM_SERVER_ADDR_ITEMS; jLEIdx++ )
            {
                pSettings->vstrIPAddress[jLEIdx - 1] = pSettings->vstrIPAddress[jLEIdx];
            }
        }
    }
    // empty last entry
    pSettings->vstrIPAddress[MAX_NUM_SERVER_ADDR_ITEMS - 1] = QString();

    // redisplay to pick up updated list
    showEvent ( nullptr );
}

void CConnectDlg::OnTimerPing()
{
    // send ping messages to the servers in the list
    const int iServerListLen = lvwServers->topLevelItemCount();

    for ( int iIdx = 0; iIdx < iServerListLen; iIdx++ )
    {
        CHostAddress haServerAddress;

        // try to parse host address string which is stored as user data
        // in the server list item GUI control element
        if ( NetworkUtil().ParseNetworkAddress ( lvwServers->topLevelItem ( iIdx )->data ( 0, Qt::UserRole ).toString(),
                                                 haServerAddress,
                                                 bEnableIPv6 ) )
        {
            // if address is valid, send ping message using a new thread
#if QT_VERSION >= QT_VERSION_CHECK( 6, 0, 0 )
            QFuture<void> f = QtConcurrent::run ( &CConnectDlg::EmitCLServerListPingMes, this, haServerAddress );
            Q_UNUSED ( f );
#else
            QtConcurrent::run ( this, &CConnectDlg::EmitCLServerListPingMes, haServerAddress );
#endif
        }
    }
}

void CConnectDlg::EmitCLServerListPingMes ( const CHostAddress& haServerAddress )
{
    // The ping time messages for all servers should not be sent all in a very
    // short time since it showed that this leads to errors in the ping time
    // measurement (#49). We therefore introduce a short delay for each server
    // (since we are doing this in a separate thread for each server, we do not
    // block the GUI).
    QThread::msleep ( 11 );

    emit CreateCLServerListPingMes ( haServerAddress );
}

void CConnectDlg::SetPingTimeAndNumClientsResult ( const CHostAddress& InetAddr, const int iPingTime, const int iNumClients )
{
    // apply the received ping time to the correct server list entry
    QTreeWidgetItem* pCurListViewItem = FindListViewItem ( InetAddr );

    if ( pCurListViewItem )
    {
        // check if this is the first time a ping time is set
        const bool bIsFirstPing = pCurListViewItem->text ( 1 ).isEmpty();
        bool       bDoSorting   = false;

        // update minimum ping time column (invisible, used for sorting) if
        // the new value is smaller than the old value
        int iMinPingTime = pCurListViewItem->text ( 4 ).toInt();

        if ( iMinPingTime > iPingTime )
        {
            // update the minimum ping time with the new lowest value
            iMinPingTime = iPingTime;

            // we pad to a total of 8 characters with zeros to make sure the
            // sorting is done correctly
            pCurListViewItem->setText ( 4, QString ( "%1" ).arg ( iPingTime, 8, 10, QLatin1Char ( '0' ) ) );

            // update the sorting (lowest number on top)
            bDoSorting = true;
        }

        // for debugging it is good to see the current ping time in the list
        // and not the minimum ping time -> overwrite the value for debugging
        if ( bShowCompleteRegList )
        {
            iMinPingTime = iPingTime;
        }

        // Only show minimum ping time in the list since this is the important
        // value. Temporary bad ping measurements are of no interest.
        // Color definition: <= 25 ms green, <= 50 ms yellow, otherwise red
        if ( iMinPingTime <= 25 )
        {
            pCurListViewItem->setForeground ( 1, Qt::darkGreen );
        }
        else
        {
            if ( iMinPingTime <= 50 )
            {
                pCurListViewItem->setForeground ( 1, Qt::darkYellow );
            }
            else
            {
                pCurListViewItem->setForeground ( 1, Qt::red );
            }
        }

        // update ping text, take special care if ping time exceeds a
        // certain value
        if ( iMinPingTime > 500 )
        {
            pCurListViewItem->setText ( 1, ">500 ms" );
        }
        else
        {
            // prepend spaces so that we can sort correctly (fieldWidth of
            // 4 is sufficient since the maximum width is ">500") (#201)
            pCurListViewItem->setText ( 1, QString ( "%1 ms" ).arg ( iMinPingTime, 4, 10, QLatin1Char ( ' ' ) ) );
        }

        // update number of clients text
        if ( pCurListViewItem->text ( 5 ).toInt() == 0 )
        {
            // special case: reduced server list
            pCurListViewItem->setText ( 2, QString().setNum ( iNumClients ) );
        }
        else if ( iNumClients >= pCurListViewItem->text ( 5 ).toInt() )
        {
            pCurListViewItem->setText ( 2, QString().setNum ( iNumClients ) + " (full)" );
        }
        else
        {
            pCurListViewItem->setText ( 2, QString().setNum ( iNumClients ) + "/" + pCurListViewItem->text ( 5 ) );
        }

        // check if the number of child list items matches the number of
        // connected clients, if not then request the client names
        if ( iNumClients != pCurListViewItem->childCount() )
        {
            emit CreateCLServerListReqConnClientsListMes ( InetAddr );
        }

        // this is the first time a ping time was received, set item to visible
        if ( bIsFirstPing )
        {
            pCurListViewItem->setHidden ( false );
        }

        // Update sorting. Note that the sorting must be the last action for the
        // current item since the topLevelItem(iIdx) is then no longer valid.
        // To avoid that the list is sorted shortly before a double click (which
        // could lead to connecting an incorrect server) the sorting is disabled
        // as long as the mouse is over the list (but it is not disabled for the
        // initial timer of about 2s, see TimerInitialSort) (#293).
        if ( bDoSorting && !bShowCompleteRegList &&
             ( TimerInitialSort.isActive() || !lvwServers->underMouse() ) ) // do not sort if "show all servers"
        {
            lvwServers->sortByColumn ( 4, Qt::AscendingOrder );
        }
    }

    // if no server item has children, do not show decoration
    bool      bAnyListItemHasChilds = false;
    const int iServerListLen        = lvwServers->topLevelItemCount();

    for ( int iIdx = 0; iIdx < iServerListLen; iIdx++ )
    {
        // check if the current list item has children
        if ( lvwServers->topLevelItem ( iIdx )->childCount() > 0 )
        {
            bAnyListItemHasChilds = true;
        }
    }

    if ( !bAnyListItemHasChilds )
    {
        lvwServers->setRootIsDecorated ( false );
    }

    // we may have changed the Hidden state for some items, if a filter was active, we now
    // have to update it to void lines appear which do not satisfy the filter criteria
    UpdateListFilter();
}

QTreeWidgetItem* CConnectDlg::FindListViewItem ( const CHostAddress& InetAddr )
{
    const int iServerListLen = lvwServers->topLevelItemCount();

    for ( int iIdx = 0; iIdx < iServerListLen; iIdx++ )
    {
        // compare the received address with the user data string of the
        // host address by a string compare
        if ( !lvwServers->topLevelItem ( iIdx )->data ( 0, Qt::UserRole ).toString().compare ( InetAddr.toString() ) )
        {
            return lvwServers->topLevelItem ( iIdx );
        }
    }

    return nullptr;
}

QTreeWidgetItem* CConnectDlg::GetParentListViewItem ( QTreeWidgetItem* pItem )
{
    // check if the current item is already the top item, i.e. the parent
    // query fails and returns null
    if ( pItem->parent() )
    {
        // we only have maximum one level, i.e. if we call the parent function
        // we are at the top item
        return pItem->parent();
    }
    else
    {
        // this item is already the top item
        return pItem;
    }
}

void CConnectDlg::DeleteAllListViewItemChilds ( QTreeWidgetItem* pItem )
{
    // loop over all children
    while ( pItem->childCount() > 0 )
    {
        // get the first child in the list
        QTreeWidgetItem* pCurChildItem = pItem->child ( 0 );

        // remove it from the item (note that the object is not deleted)
        pItem->removeChild ( pCurChildItem );

        // delete the object to avoid a memory leak
        delete pCurChildItem;
    }
}

void CConnectDlg::UpdateDirectoryComboBox()
{
    // directory type combo box
    cbxDirectory->clear();
    cbxDirectory->addItem ( DirectoryTypeToString ( AT_DEFAULT ) );
    cbxDirectory->addItem ( DirectoryTypeToString ( AT_ANY_GENRE2 ) );
    cbxDirectory->addItem ( DirectoryTypeToString ( AT_ANY_GENRE3 ) );
    cbxDirectory->addItem ( DirectoryTypeToString ( AT_GENRE_ROCK ) );
    cbxDirectory->addItem ( DirectoryTypeToString ( AT_GENRE_JAZZ ) );
    cbxDirectory->addItem ( DirectoryTypeToString ( AT_GENRE_CLASSICAL_FOLK ) );
    cbxDirectory->addItem ( DirectoryTypeToString ( AT_GENRE_CHORAL ) );

    // because custom directories are always added to the top of the vector, add the vector
    // contents to the combobox in reverse order
    for ( int i = MAX_NUM_SERVER_ADDR_ITEMS - 1; i >= 0; i-- )
    {
        if ( pSettings->vstrDirectoryAddress[i] != "" )
        {
            // add vector index (i) to the combobox as user data
            cbxDirectory->addItem ( pSettings->vstrDirectoryAddress[i], i );
        }
    }
}
