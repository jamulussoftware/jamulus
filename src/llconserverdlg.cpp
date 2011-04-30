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

#include "llconserverdlg.h"


/* Implementation *************************************************************/
CLlconServerDlg::CLlconServerDlg ( CServer* pNServP, QWidget* parent )
    : QDialog ( parent ), pServer ( pNServP )
{
    setupUi ( this );


    // Add help text to controls -----------------------------------------------
    // client list
    ListViewClients->setWhatsThis ( tr ( "<b>Client List:</b> The client list "
        "shows all clients which are currently connected to this server. Some "
        "informations about the clients like the IP address, name, buffer "
        "state are given for each connected client." ) );

    ListViewClients->setAccessibleName ( tr ( "Connected clients list view" ) );

    // register server flag
    cbRegisterServer->setWhatsThis ( tr ( "<b>Register Server Status:</b> If "
        "the register server check box is checked, this server registers "
        "itself at the central server so that all " ) + APP_NAME +
        tr ( " users can see the server in the connect dialog server list and "
        "connect to it. The registering of the server is renewed periodically "
        "to make sure that all servers in the connect dialog server list are "
        "actually available." ) );

    // central server address
    QString strCentrServAddr = tr ( "<b>Central Server Address:</b> The "
        "Central server address is the IP address or URL of the central server "
        "at which this server is registered. If the Default check box is "
        "checked, the default central server address is shown read-only." );

    LabelCentralServerAddress->setWhatsThis    ( strCentrServAddr );
    cbDefaultCentralServer->setWhatsThis       ( strCentrServAddr );
    LineEditCentralServerAddress->setWhatsThis ( strCentrServAddr );

    cbDefaultCentralServer->setAccessibleName (
        tr ( "Default central server check box" ) );

    LineEditCentralServerAddress->setAccessibleName (
        tr ( "Central server address line edit" ) );

    // server name
    QString strServName = tr ( "<b>Server Name:</b> The server name identifies "
        "your server in the connect dialog server list at the clients. If no "
        "name is given, the IP address is shown instead." );

    LabelServerName->setWhatsThis    ( strServName );
    LineEditServerName->setWhatsThis ( strServName );

    LineEditServerName->setAccessibleName ( tr ( "Server name line edit" ) );

    // location city
    QString strLocCity = tr ( "<b>Location City:</b> The city in which this "
        "server is located can be set here. If a city name is entered, it "
        "will be shown in the connect dialog server list at the clients." );

    LabelLocationCity->setWhatsThis    ( strLocCity );
    LineEditLocationCity->setWhatsThis ( strLocCity );

    LineEditLocationCity->setAccessibleName ( tr (
        "City where the server is located line edit" ) );

    // location country
    QString strLocCountry = tr ( "<b>Location country:</b> The country in "
        "which this server is located can be set here. If a country is "
        "entered, it will be shown in the connect dialog server list at the "
        "clients." );

    LabelLocationCountry->setWhatsThis    ( strLocCountry );
    ComboBoxLocationCountry->setWhatsThis ( strLocCountry );

    ComboBoxLocationCountry->setAccessibleName ( tr (
        "Country where the server is located combo box" ) );


    // set text for version and application name
    TextLabelNameVersion->setText ( QString ( APP_NAME ) +
        tr ( " server " ) + QString ( VERSION ) );

    // set up list view for connected clients
    ListViewClients->setColumnWidth ( 0, 170 );
    ListViewClients->setColumnWidth ( 1, 130 );
    ListViewClients->setColumnWidth ( 2, 60 );
    ListViewClients->clear();

    // insert items in reverse order because in Windows all of them are
    // always visible -> put first item on the top
    vecpListViewItems.Init ( USED_NUM_CHANNELS );
    for ( int i = USED_NUM_CHANNELS - 1; i >= 0; i-- )
    {
        vecpListViewItems[i] = new CServerListViewItem ( ListViewClients );
        vecpListViewItems[i]->setHidden ( true );
    }

    // update central server name line edit
    LineEditCentralServerAddress->setText (
        pServer->GetServerListCentralServerAddress() );

    // update default central server address check box
    if ( pServer->GetUseDefaultCentralServerAddress() )
    {
        cbDefaultCentralServer->setCheckState ( Qt::Checked );
    }
    else
    {
        cbDefaultCentralServer->setCheckState ( Qt::Unchecked );
    }

    // update server name line edit
    LineEditServerName->setText ( pServer->GetServerName() );

    // update server city line edit
    LineEditLocationCity->setText ( pServer->GetServerCity() );

    // load country combo box with all available countries
    ComboBoxLocationCountry->setInsertPolicy ( QComboBox::NoInsert );
    ComboBoxLocationCountry->clear();

    for ( int iCurCntry = static_cast<int> ( QLocale::AnyCountry );
          iCurCntry < static_cast<int> ( QLocale::LastCountry ); iCurCntry++ )
    {
        // add all countries except of the "Default" country
        if ( static_cast<QLocale::Country> ( iCurCntry ) != QLocale::AnyCountry )
        {
            // store the country enum index together with the string (this is
            // important since we sort the combo box items later on)
            ComboBoxLocationCountry->addItem ( QLocale::countryToString (
                static_cast<QLocale::Country> ( iCurCntry ) ), iCurCntry );
        }
    }

    // sort country combo box items in alphabetical order
    ComboBoxLocationCountry->model()->sort ( 0, Qt::AscendingOrder );

    // select current country
    ComboBoxLocationCountry->setCurrentIndex (
        ComboBoxLocationCountry->findData (
        static_cast<int> ( pServer->GetServerCountry() ) ) );

    // update register server check box
    if ( pServer->GetServerListEnabled() )
    {
        cbRegisterServer->setCheckState ( Qt::Checked );
    }
    else
    {
        cbRegisterServer->setCheckState ( Qt::Unchecked );
    }

    // update GUI dependencies
    UpdateGUIDependencies();


    // Main menu bar -----------------------------------------------------------
    pMenu = new QMenuBar ( this );
    pMenu->addMenu ( new CLlconHelpMenu ( this ) );

    // Now tell the layout about the menu
    layout()->setMenuBar ( pMenu );


    // Connections -------------------------------------------------------------
    // check boxes
    QObject::connect ( cbRegisterServer, SIGNAL ( stateChanged ( int ) ),
        this, SLOT ( OnRegisterServerStateChanged ( int ) ) );

    QObject::connect ( cbDefaultCentralServer, SIGNAL ( stateChanged ( int ) ),
        this, SLOT ( OnDefaultCentralServerStateChanged ( int ) ) );

    // line edits
    QObject::connect ( LineEditCentralServerAddress,
        SIGNAL ( editingFinished() ),
        this, SLOT ( OnLineEditCentralServerAddressEditingFinished() ) );

    QObject::connect ( LineEditServerName, SIGNAL ( textChanged ( const QString& ) ),
        this, SLOT ( OnLineEditServerNameTextChanged ( const QString& ) ) );

    QObject::connect ( LineEditLocationCity, SIGNAL ( textChanged ( const QString& ) ),
        this, SLOT ( OnLineEditLocationCityTextChanged ( const QString& ) ) );

    // combo boxes
    QObject::connect ( ComboBoxLocationCountry, SIGNAL ( activated ( int ) ),
        this, SLOT ( OnComboBoxLocationCountryActivated ( int ) ) );

    // timers
    QObject::connect ( &Timer, SIGNAL ( timeout() ), this, SLOT ( OnTimer() ) );


    // Timers ------------------------------------------------------------------
    // start timer for GUI controls
    Timer.start ( GUI_CONTRL_UPDATE_TIME );
}

void CLlconServerDlg::OnDefaultCentralServerStateChanged ( int value )
{
    // apply new setting to the server and update it
    pServer->SetUseDefaultCentralServerAddress ( value == Qt::Checked );
    pServer->UpdateServerList();

    // update GUI dependencies
    UpdateGUIDependencies();
}

void CLlconServerDlg::OnRegisterServerStateChanged ( int value )
{
    // apply new setting to the server and update it
    pServer->SetServerListEnabled ( value == Qt::Checked );
    pServer->UpdateServerList();

    // update GUI dependencies
    UpdateGUIDependencies();
}

void CLlconServerDlg::OnLineEditCentralServerAddressEditingFinished()
{
    // apply new setting to the server and update it
    pServer->SetServerListCentralServerAddress (
        LineEditCentralServerAddress->text() );

    pServer->UpdateServerList();
}

void CLlconServerDlg::OnLineEditServerNameTextChanged ( const QString& strNewName )
{
    // check length
    if ( strNewName.length() <= MAX_LEN_SERVER_NAME )
    {
        // apply new setting to the server and update it
        pServer->SetServerName ( strNewName );
        pServer->UpdateServerList();
    }
    else
    {
        // text is too long, update control with shortend text
        LineEditServerName->setText ( strNewName.left ( MAX_LEN_SERVER_NAME ) );
    }
}

void CLlconServerDlg::OnLineEditLocationCityTextChanged ( const QString& strNewCity )
{
    // check length
    if ( strNewCity.length() <= MAX_LEN_SERVER_CITY )
    {
        // apply new setting to the server and update it
        pServer->SetServerCity ( strNewCity );
        pServer->UpdateServerList();
    }
    else
    {
        // text is too long, update control with shortend text
        LineEditLocationCity->setText ( strNewCity.left ( MAX_LEN_SERVER_CITY ) );
    }
}

void CLlconServerDlg::OnComboBoxLocationCountryActivated ( int iCntryListItem )
{
    // apply new setting to the server and update it
    pServer->SetServerCountry ( static_cast<QLocale::Country> (
        ComboBoxLocationCountry->itemData ( iCntryListItem ).toInt() ) );

    pServer->UpdateServerList();
}

void CLlconServerDlg::OnTimer()
{
    CVector<CHostAddress> vecHostAddresses;
    CVector<QString>      vecsName;
    CVector<int>          veciJitBufNumFrames;
    CVector<int>          veciNetwFrameSizeFact;

    ListViewMutex.lock();
    {
        pServer->GetConCliParam ( vecHostAddresses,
                                  vecsName,
                                  veciJitBufNumFrames,
                                  veciNetwFrameSizeFact );

        // fill list with connected clients
        for ( int i = 0; i < USED_NUM_CHANNELS; i++ )
        {
            if ( !( vecHostAddresses[i].InetAddr == QHostAddress ( (quint32) 0 ) ) )
            {
                // IP, port number
                vecpListViewItems[i]->setText ( 0, QString("%1 : %2" ).
                    arg ( vecHostAddresses[i].InetAddr.toString() ).
                    arg ( vecHostAddresses[i].iPort ) );

                // name
                vecpListViewItems[i]->setText ( 1, vecsName[i] );

                // jitter buffer size (polling for updates)
                vecpListViewItems[i]->setText ( 3,
                    QString().setNum ( veciJitBufNumFrames[i] ) );

                // out network block size
                vecpListViewItems[i]->setText ( 4,
                    QString().setNum ( static_cast<double> (
                    veciNetwFrameSizeFact[i] * SYSTEM_BLOCK_DURATION_MS_FLOAT
                    ), 'f', 2 ) );

                vecpListViewItems[i]->setHidden ( false );
            }
            else
            {
                vecpListViewItems[i]->setHidden ( true );
            }
        }
    }
    ListViewMutex.unlock();
}

void CLlconServerDlg::UpdateGUIDependencies()
{
    // get the states which define the GUI dependencies from the server
    const bool bCurSerListEnabled = pServer->GetServerListEnabled();

    const bool bCurUseDefCentServAddr =
        pServer->GetUseDefaultCentralServerAddress();

    // if register server is not enabled, we disable all the configuration
    // controls for the server list
    cbDefaultCentralServer->setEnabled       ( bCurSerListEnabled );
    LineEditCentralServerAddress->setEnabled ( bCurSerListEnabled );
    GroupBoxServerInfo->setEnabled           ( bCurSerListEnabled );

    // If the default central server address is enabled, the line edit shows
    // the default server and is not editable. Make sure the line edit does not
    // fire signals when we update the text.
    LineEditCentralServerAddress->blockSignals ( true );
    {
        if ( bCurUseDefCentServAddr )
        {
            LineEditCentralServerAddress->setText ( DEFAULT_SERVER_ADDRESS );
        }
        else
        {
            LineEditCentralServerAddress->setText (
                pServer->GetServerListCentralServerAddress() );
        }
    }
    LineEditCentralServerAddress->blockSignals ( false );

    // the line edit of the central server address is only enabled, if the
    // server list is enabled and not the default address is used
    LineEditCentralServerAddress->setEnabled (
        !bCurUseDefCentServAddr && bCurSerListEnabled );
}

void CLlconServerDlg::customEvent ( QEvent* Event )
{
    if ( Event->type() == QEvent::User + 11 )
    {
        ListViewMutex.lock();
        {
            const int iMessType = ( (CLlconEvent*) Event )->iMessType;
            const int iStatus   = ( (CLlconEvent*) Event )->iStatus;
            const int iChanNum  = ( (CLlconEvent*) Event )->iChanNum;

            switch(iMessType)
            {
            case MS_JIT_BUF_PUT:
            case MS_JIT_BUF_GET:
                vecpListViewItems[iChanNum]->SetLight ( iStatus );
                break;
            }
        }
        ListViewMutex.unlock();
    }
}
