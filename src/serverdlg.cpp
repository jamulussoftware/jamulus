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
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
\******************************************************************************/

#include "serverdlg.h"


/* Implementation *************************************************************/
CServerDlg::CServerDlg ( CServer*        pNServP,
                         CSettings*      pNSetP,
                         const bool      bStartMinimized,
                         QWidget*        parent,
                         Qt::WindowFlags f )
    : QDialog                  ( parent, f ),
      pServer                  ( pNServP ),
      pSettings                ( pNSetP ),
      BitmapSystemTrayInactive ( QString::fromUtf8 ( ":/png/LEDs/res/CLEDGreyArrow.png" ) ),
      BitmapSystemTrayActive   ( QString::fromUtf8 ( ":/png/LEDs/res/CLEDGreenArrow.png" ) )
{
    setupUi ( this );


    // Add help text to controls -----------------------------------------------
    // client list
    lvwClients->setWhatsThis ( "<b>" + tr ( "Client List" ) + ":</b> " + tr (
        "The client list shows all clients which are currently connected to this "
        "server. Some information about the clients like the IP address and name "
        "are given for each connected client." ) );

    lvwClients->setAccessibleName ( tr ( "Connected clients list view" ) );

    // start minimized on operating system start
    chbStartOnOSStart->setWhatsThis ( "<b>" + tr ( "Start Minimized on Operating "
        "System Start" ) + ":</b> " + tr ( "If the start minimized on operating system start "
        "check box is checked, the server will be "
        "started when the operating system starts up and is automatically "
        "minimized to a system task bar icon." ) );

    // CC licence dialog switch
    chbUseCCLicence->setWhatsThis ( "<b>" + tr ( "Show Creative Commons Licence "
        "Dialog" ) + ":</b> " + tr ( "If enabled, a Creative Commons BY-NC-SA 4.0 Licence "
        "dialog is shown each time a new user connects the server." ) );

    // Make My Server Public flag
    chbRegisterServer->setWhatsThis ( "<b>" + tr ( "Make My Server Public" ) + ":</b> " +
        tr ( "If the Make My Server Public check box is checked, this server registers "
        "itself at the central server so that all users of the application "
        "can see the server in the connect dialog server list and "
        "connect to it. The registration of the server is renewed periodically "
        "to make sure that all servers in the connect dialog server list are "
        "actually available." ) );

    // register server status label
    lblRegSvrStatus->setWhatsThis ( "<b>" + tr ( "Register Server Status" ) + ":</b> " +
        tr ( "If the Make My Server Public check box is checked, this will show "
        "whether registration with the central server is successful. If the "
        "registration failed, please choose another server list." ) );

    // custom central server address
    QString strCentrServAddr = "<b>" + tr ( "Custom Central Server Address" ) + ":</b> " +
        tr ( "The custom central server address is the IP address or URL of the central "
        "server at which the server list of the connection dialog is managed." );

    lblCentralServerAddress->setWhatsThis ( strCentrServAddr );
    edtCentralServerAddress->setWhatsThis ( strCentrServAddr );
    edtCentralServerAddress->setAccessibleName ( tr ( "Central server address line edit" ) );

    cbxCentServAddrType->setWhatsThis ( "<b>" + tr ( "Server List Selection" ) + ":</b> " + tr (
        "Selects the server list (i.e. central server address) in which your server will be added." ) );
    cbxCentServAddrType->setAccessibleName ( tr ( "Server list selection combo box" ) );

    // server name
    QString strServName = "<b>" + tr ( "Server Name" ) + ":</b> " + tr ( "The server name identifies "
        "your server in the connect dialog server list at the clients." );

    lblServerName->setWhatsThis ( strServName );
    edtServerName->setWhatsThis ( strServName );

    edtServerName->setAccessibleName ( tr ( "Server name line edit" ) );

    // location city
    QString strLocCity = "<b>" + tr ( "Location City" ) + ":</b> " + tr ( "The city in which this "
        "server is located can be set here. If a city name is entered, it "
        "will be shown in the connect dialog server list at the clients." );

    lblLocationCity->setWhatsThis ( strLocCity );
    edtLocationCity->setWhatsThis ( strLocCity );

    edtLocationCity->setAccessibleName ( tr ( "City where the server is located line edit" ) );

    // location country
    QString strLocCountry = "<b>" + tr ( "Location country" ) + ":</b> " + tr ( "The country in "
        "which this server is located can be set here. If a country is "
        "entered, it will be shown in the connect dialog server list at the "
        "clients." );

    lblLocationCountry->setWhatsThis ( strLocCountry );
    cbxLocationCountry->setWhatsThis ( strLocCountry );

    cbxLocationCountry->setAccessibleName ( tr (
        "Country where the server is located combo box" ) );

    // enable recorder
    chbEnableRecorder->setAccessibleName( tr ( "Checkbox to turn on or off server recording" ) );
    chbEnableRecorder->setWhatsThis( "<b>" + tr ( "Enable Recorder" ) + ":</b>"
        + tr ( "Checked when the recorder is enabled, otherwise unchecked. "
               "The recorder will run when a session is in progress, if (set up correctly and) enabled." ) );

    // current session directory
    edtCurrentSessionDir->setAccessibleName( tr ( "Current session directory text box (read-only)" ) );
    edtCurrentSessionDir->setWhatsThis( "<b>" + tr ( "Current Session Directory" ) + ":</b>"
        +  tr ( "Enabled during recording and holds the current recording session directory. "
                "Disabled after recording or when the recorder is not enabled." ) );

    // recorder status
    lblRecorderStatus->setAccessibleName ( tr ( "Recorder status label" ) );
    lblRecorderStatus->setWhatsThis ( "<b>" + tr ( "Recorder Status" ) + ":</b>"
        +  tr ( "Displays the current status of the recorder." ) );

    // new recording
    pbtNewRecording->setAccessibleName ( tr ( "Request new recording button" ) );
    pbtNewRecording->setWhatsThis ( "<b>" + tr ( "New Recording" ) + ":</b>"
        +  tr ( "During a recording session, the button can be used to start a new recording." ) );

    // recording directory
    pbtRecordingDir->setAccessibleName ( tr ( "Display dialog to select recording directory button" ) );
    pbtRecordingDir->setWhatsThis ( "<b>" + tr ( "Main Recording Directory" ) + ":</b>"
        +  tr ( "Click the button to open the dialog that allows the main recording directory to be selected."
                "The chosen value must exist and be writeable (allow creation of sub-directories "
                "by the user Jamulus is running as). " ) );

    edtRecordingDir->setAccessibleName ( tr ( "Main recording directory text box (read-only)" ) );
    edtRecordingDir->setWhatsThis ( "<b>" + tr ( "Main Recording Directory" ) + ":</b>"
        +  tr ( "The current value of the main recording directory. "
                "The chosen value must exist and be writeable (allow creation of sub-directories "
                "by the user Jamulus is running as). "
                "Click the button to open the dialog that allows the main recording directory to be selected." ) );

    tbtClearRecordingDir->setAccessibleName ( tr ( "Clear the recording directory button" ) );
    tbtClearRecordingDir->setWhatsThis ( "<b>" + tr ( "Clear Recording Directory" ) + ":</b>"
        +  tr ( "Click the button to clear the currently selected recording directory. "
                "This will prevent recording until a new value is selected.") );

    // check if system tray icon can be used
    bSystemTrayIconAvaialbe = SystemTrayIcon.isSystemTrayAvailable();

    // init system tray icon
    if ( bSystemTrayIconAvaialbe )
    {
        // prepare context menu to be added to the system tray icon
        pSystemTrayIconMenu = new QMenu ( this );

        pSystemTrayIconMenu->addAction ( tr ( "E&xit" ),
            this, SLOT ( OnSysTrayMenuExit() ) );

        pSystemTrayIconMenu->addSeparator();

        pSystemTrayIconMenu->addAction (
            tr ( "&Hide " ) + APP_NAME + tr ( " server" ),
            this, SLOT ( OnSysTrayMenuHide() ) );

        pSystemTrayIconMenu->setDefaultAction ( pSystemTrayIconMenu->addAction (
            tr ( "&Open " ) + APP_NAME + tr ( " server" ),
            this, SLOT ( OnSysTrayMenuOpen() ) ) );

        SystemTrayIcon.setContextMenu ( pSystemTrayIconMenu );

        // set tool text
        SystemTrayIcon.setToolTip ( QString ( APP_NAME ) + tr ( " server" ) );

        // show icon of state "inactive"
        SystemTrayIcon.setIcon ( QIcon ( BitmapSystemTrayInactive ) );
        SystemTrayIcon.show();
    }

    // act on "start minimized" flag (note, this has to be done after setting
    // the correct value for the system tray icon availablility)
    if ( bStartMinimized )
    {
        showMinimized();
    }

    // set text for version and application name
    lblNameVersion->setText ( QString ( APP_NAME ) +
        tr ( " server " ) + QString ( VERSION ) );

    // set up list view for connected clients
    lvwClients->setColumnWidth ( 0, 170 );
    lvwClients->setColumnWidth ( 1, 200 );
    lvwClients->clear();


// TEST workaround for resize problem of window after iconize in task bar
lvwClients->setMinimumWidth ( 170 + 130 + 60 + 205 );
lvwClients->setMinimumHeight ( 140 );


    // insert items in reverse order because in Windows all of them are
    // always visible -> put first item on the top
    vecpListViewItems.Init ( MAX_NUM_CHANNELS );

    for ( int i = MAX_NUM_CHANNELS - 1; i >= 0; i-- )
    {
        vecpListViewItems[i] = new QTreeWidgetItem ( lvwClients );
        vecpListViewItems[i]->setHidden ( true );
    }

    // central server address type combo box
    cbxCentServAddrType->clear();
    cbxCentServAddrType->addItem ( csCentServAddrTypeToString ( AT_DEFAULT ) );
    cbxCentServAddrType->addItem ( csCentServAddrTypeToString ( AT_ALL_GENRES ) );
    cbxCentServAddrType->addItem ( csCentServAddrTypeToString ( AT_GENRE_ROCK ) );
    cbxCentServAddrType->addItem ( csCentServAddrTypeToString ( AT_GENRE_JAZZ ) );
    cbxCentServAddrType->addItem ( csCentServAddrTypeToString ( AT_GENRE_CLASSICAL_FOLK ) );
    cbxCentServAddrType->addItem ( csCentServAddrTypeToString ( AT_CUSTOM ) );
    cbxCentServAddrType->setCurrentIndex ( static_cast<int> ( pServer->GetCentralServerAddressType() ) );

    // update server name line edit
    edtServerName->setText ( pServer->GetServerName() );

    // update server city line edit
    edtLocationCity->setText ( pServer->GetServerCity() );

    // load country combo box with all available countries
    cbxLocationCountry->setInsertPolicy ( QComboBox::NoInsert );
    cbxLocationCountry->clear();

    for ( int iCurCntry = static_cast<int> ( QLocale::AnyCountry );
          iCurCntry < static_cast<int> ( QLocale::LastCountry ); iCurCntry++ )
    {
        // add all countries except of the "Default" country
        if ( static_cast<QLocale::Country> ( iCurCntry ) != QLocale::AnyCountry )
        {
            // store the country enum index together with the string (this is
            // important since we sort the combo box items later on)
            cbxLocationCountry->addItem ( QLocale::countryToString (
                static_cast<QLocale::Country> ( iCurCntry ) ), iCurCntry );
        }
    }

    // sort country combo box items in alphabetical order
    cbxLocationCountry->model()->sort ( 0, Qt::AscendingOrder );

    // select current country
    cbxLocationCountry->setCurrentIndex (
        cbxLocationCountry->findData (
        static_cast<int> ( pServer->GetServerCountry() ) ) );

    // update register server check box
    if ( pServer->GetServerListEnabled() )
    {
        chbRegisterServer->setCheckState ( Qt::Checked );
    }
    else
    {
        chbRegisterServer->setCheckState ( Qt::Unchecked );
    }

    // update show Creative Commons licence check box
    if ( pServer->GetLicenceType() == LT_CREATIVECOMMONS )
    {
        chbUseCCLicence->setCheckState ( Qt::Checked );
    }
    else
    {
        chbUseCCLicence->setCheckState ( Qt::Unchecked );
    }

    // update start minimized check box (only available for Windows)
#ifndef _WIN32
    chbStartOnOSStart->setVisible ( false );
#else
    const bool bCurAutoStartMinState = pServer->GetAutoRunMinimized();

    if ( bCurAutoStartMinState )
    {
        chbStartOnOSStart->setCheckState ( Qt::Checked );
    }
    else
    {
        chbStartOnOSStart->setCheckState ( Qt::Unchecked );
    }

    // modify registry according to setting (this is just required in case a
    // user has changed the registry by hand)
    ModifyAutoStartEntry ( bCurAutoStartMinState );
#endif

    // Recorder controls
    chbEnableRecorder->setCheckState ( Qt::CheckState::Checked ); // move to settings
    edtCurrentSessionDir->setText ( "" );
    pbtNewRecording->setAutoDefault ( false );
    pbtRecordingDir->setAutoDefault ( false );
    edtRecordingDir->setText( pServer->GetRecordingDir() );
    tbtClearRecordingDir->setText ( "(X)" ); // someone make me a nice icon?

    UpdateRecorderStatus ( QString::null );

    // update GUI dependencies
    UpdateGUIDependencies();

    // set window title
    setWindowTitle ( APP_NAME + tr ( " Server" ) );


    // View menu  --------------------------------------------------------------
    QMenu* pViewMenu = new QMenu ( tr ( "&Window" ), this );

    pViewMenu->addAction ( tr ( "E&xit" ), this,
        SLOT ( close() ), QKeySequence ( Qt::CTRL + Qt::Key_Q ) );


    // Main menu bar -----------------------------------------------------------
    pMenu = new QMenuBar ( this );

    pMenu->addMenu ( pViewMenu );
    pMenu->addMenu ( new CHelpMenu ( false, this ) );

    // Now tell the layout about the menu
    layout()->setMenuBar ( pMenu );


    // Connections -------------------------------------------------------------
    // check boxes
    QObject::connect ( chbRegisterServer, SIGNAL ( stateChanged ( int ) ),
        this, SLOT ( OnRegisterServerStateChanged ( int ) ) );

    QObject::connect ( chbStartOnOSStart, SIGNAL ( stateChanged ( int ) ),
        this, SLOT ( OnStartOnOSStartStateChanged ( int ) ) );

    QObject::connect ( chbUseCCLicence, SIGNAL ( stateChanged ( int ) ),
        this, SLOT ( OnUseCCLicenceStateChanged ( int ) ) );

    QObject::connect ( chbEnableRecorder, SIGNAL ( stateChanged ( int ) ),
        this, SLOT ( OnEnableRecorderStateChanged ( int ) ) );

    // line edits
    QObject::connect ( edtCentralServerAddress, SIGNAL ( editingFinished() ),
        this, SLOT ( OnCentralServerAddressEditingFinished() ) );

    QObject::connect ( edtServerName, SIGNAL ( textChanged ( const QString& ) ),
        this, SLOT ( OnServerNameTextChanged ( const QString& ) ) );

    QObject::connect ( edtLocationCity, SIGNAL ( textChanged ( const QString& ) ),
        this, SLOT ( OnLocationCityTextChanged ( const QString& ) ) );

    // combo boxes
    QObject::connect ( cbxLocationCountry, SIGNAL ( activated ( int ) ),
        this, SLOT ( OnLocationCountryActivated ( int ) ) );

    QObject::connect ( cbxCentServAddrType, SIGNAL ( activated ( int ) ),
        this, SLOT ( OnCentServAddrTypeActivated ( int ) ) );

    // push buttons
    QObject::connect ( pbtNewRecording, SIGNAL ( released() ),
        this, SLOT ( OnNewRecordingClicked() ) );

    QObject::connect ( pbtRecordingDir, SIGNAL ( released() ),
        this, SLOT ( OnRecordingDirClicked() ) );

    // tool buttons
    QObject::connect ( tbtClearRecordingDir, SIGNAL ( released() ),
        this, SLOT ( OnClearRecordingDirClicked() ) );

    // timers
    QObject::connect ( &Timer, SIGNAL ( timeout() ), this, SLOT ( OnTimer() ) );

    // other
    QObject::connect ( pServer, SIGNAL ( Started() ),
        this, SLOT ( OnServerStarted() ) );

    QObject::connect ( pServer, SIGNAL ( Stopped() ),
        this, SLOT ( OnServerStopped() ) );

    QObject::connect ( pServer, SIGNAL ( SvrRegStatusChanged() ),
        this, SLOT ( OnSvrRegStatusChanged() ) );

    QObject::connect ( pServer, SIGNAL ( RecordingSessionStarted ( QString ) ),
        this, SLOT ( OnRecordingSessionStarted ( QString ) ) );

    QObject::connect ( pServer, SIGNAL ( StopRecorder() ),
        this, SLOT ( OnStopRecorder() ) );

    QObject::connect ( QCoreApplication::instance(), SIGNAL ( aboutToQuit() ),
        this, SLOT ( OnAboutToQuit() ) );

    QObject::connect ( &SystemTrayIcon,
        SIGNAL ( activated ( QSystemTrayIcon::ActivationReason ) ),
        this, SLOT ( OnSysTrayActivated ( QSystemTrayIcon::ActivationReason ) ) );


    // Timers ------------------------------------------------------------------
    // start timer for GUI controls
    Timer.start ( GUI_CONTRL_UPDATE_TIME );
}

void CServerDlg::OnStartOnOSStartStateChanged ( int value )
{
    const bool bCurAutoStartMinState = ( value == Qt::Checked );

    // update registry and server setting (for ini file)
    pServer->SetAutoRunMinimized ( bCurAutoStartMinState );
    ModifyAutoStartEntry         ( bCurAutoStartMinState );
}

void CServerDlg::OnUseCCLicenceStateChanged ( int value )
{
    if ( value == Qt::Checked )
    {
        pServer->SetLicenceType ( LT_CREATIVECOMMONS );
    }
    else
    {
        pServer->SetLicenceType ( LT_NO_LICENCE );
    }
}

void CServerDlg::OnRegisterServerStateChanged ( int value )
{
    const bool bRegState = ( value == Qt::Checked );

    // apply new setting to the server and update it
    pServer->SetServerListEnabled ( bRegState );

    // if registering is disabled, unregister slave server
    if ( !bRegState )
    {
        pServer->UnregisterSlaveServer();
    }

    pServer->UpdateServerList();

    // update GUI dependencies
    UpdateGUIDependencies();
}

void CServerDlg::OnCentralServerAddressEditingFinished()
{
    // apply new setting to the server and update it
    pServer->SetServerListCentralServerAddress (
        edtCentralServerAddress->text() );

    pServer->UpdateServerList();
}

void CServerDlg::OnServerNameTextChanged ( const QString& strNewName )
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
        edtServerName->setText ( strNewName.left ( MAX_LEN_SERVER_NAME ) );
    }
}

void CServerDlg::OnLocationCityTextChanged ( const QString& strNewCity )
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
        edtLocationCity->setText ( strNewCity.left ( MAX_LEN_SERVER_CITY ) );
    }
}

void CServerDlg::OnLocationCountryActivated ( int iCntryListItem )
{
    // apply new setting to the server and update it
    pServer->SetServerCountry ( static_cast<QLocale::Country> (
        cbxLocationCountry->itemData ( iCntryListItem ).toInt() ) );

    pServer->UpdateServerList();
}

void CServerDlg::OnCentServAddrTypeActivated ( int iTypeIdx )
{
    // if server was registered, unregister first
    if ( pServer->GetServerListEnabled() )
    {
        pServer->UnregisterSlaveServer();
    }

    // apply new setting to the server and update it
    pServer->SetCentralServerAddressType ( static_cast<ECSAddType> ( iTypeIdx ) );
    pServer->UpdateServerList();

    // update GUI dependencies
    UpdateGUIDependencies();
}

void CServerDlg::OnServerStarted()
{
     UpdateSystemTrayIcon ( true );
     UpdateRecorderStatus ( QString::null );
}

void CServerDlg::OnServerStopped()
{
    UpdateSystemTrayIcon ( false );
    UpdateRecorderStatus ( QString::null );
}

void CServerDlg::OnStopRecorder()
{
    UpdateRecorderStatus ( QString::null );
}

void CServerDlg::OnRecordingDirClicked()
{
    // get the current value from pServer
    QString currentValue = pServer->GetRecordingDir();
    QString newRecordingDir = QFileDialog::getExistingDirectory ( this,
                                                                  tr ( "Select Main Recording Directory" ),
                                                                  currentValue,
                                                                  QFileDialog::ShowDirsOnly | QFileDialog::DontUseNativeDialog );
    if ( newRecordingDir != currentValue )
    {
        pServer->SetRecordingDir ( newRecordingDir );
        UpdateRecorderStatus ( QString::null );
    }
}

void CServerDlg::OnClearRecordingDirClicked()
{
    if ( pServer->GetRecorderErrMsg() != QString::null || pServer->GetRecordingDir() != "" )
    {
        pServer->SetRecordingDir ( "" );
        UpdateRecorderStatus ( QString::null );
    }
}

void CServerDlg::OnSysTrayActivated ( QSystemTrayIcon::ActivationReason ActReason )
{
    // on double click on the icon, show window in fore ground
    if ( ActReason == QSystemTrayIcon::DoubleClick )
    {
        ShowWindowInForeground();
    }
}

void CServerDlg::OnTimer()
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

        // we assume that all vectors have the same length
        const int iNumChannels = vecHostAddresses.Size();

        // fill list with connected clients
        for ( int i = 0; i < iNumChannels; i++ )
        {
            if ( !( vecHostAddresses[i].InetAddr == QHostAddress ( static_cast<quint32> ( 0 ) ) ) )
            {
                // IP, port number
                vecpListViewItems[i]->setText ( 0,
                    vecHostAddresses[i].toString ( CHostAddress::SM_IP_PORT ) );

                // name
                vecpListViewItems[i]->setText ( 1, vecsName[i] );

                // jitter buffer size (polling for updates)
                vecpListViewItems[i]->setText ( 2,
                    QString().setNum ( veciJitBufNumFrames[i] ) );

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

void CServerDlg::UpdateGUIDependencies()
{
    // get the states which define the GUI dependencies from the server
    const bool bCurSerListEnabled = pServer->GetServerListEnabled();

    const bool bCurUseDefCentServAddr = ( pServer->GetCentralServerAddressType() != AT_CUSTOM );

    const ESvrRegStatus eSvrRegStatus = pServer->GetSvrRegStatus();

    // if register server is not enabled, we disable all the configuration
    // controls for the server list
    cbxCentServAddrType->setEnabled ( bCurSerListEnabled );
    grbServerInfo->setEnabled       ( bCurSerListEnabled );

    // make sure the line edit does not fire signals when we update the text
    edtCentralServerAddress->blockSignals ( true );
    {
        if ( bCurUseDefCentServAddr )
        {
            // if the default central server is used, just show a text of the
            // server name
            edtCentralServerAddress->setText ( tr ( "Predefined Address" ) );
        }
        else
        {
            // show the current user defined server address
            edtCentralServerAddress->setText (
                pServer->GetServerListCentralServerAddress() );
        }
    }
    edtCentralServerAddress->blockSignals ( false );

    // the line edit of the central server address is only enabled, if the
    // server list is enabled and not the default address is used
    edtCentralServerAddress->setEnabled (
        !bCurUseDefCentServAddr && bCurSerListEnabled );

    QString strStatus = svrRegStatusToString ( eSvrRegStatus );

    switch ( eSvrRegStatus )
    {
    case SRS_BAD_ADDRESS:
    case SRS_TIME_OUT:
    case SRS_CENTRAL_SVR_FULL:
        strStatus = "<font color=""red""><b>" + strStatus + "</b></font>";
        break;

    case SRS_REGISTERED:
        strStatus = "<font color=""darkGreen""><b>" + strStatus + "</b></font>";
        break;

    default:
        break;
    }

    lblRegSvrStatus->setText ( strStatus );
}

void CServerDlg::UpdateSystemTrayIcon ( const bool bIsActive )
{
    if ( bSystemTrayIconAvaialbe )
    {
        if ( bIsActive )
        {
            SystemTrayIcon.setIcon ( QIcon ( BitmapSystemTrayActive ) );
        }
        else
        {
            SystemTrayIcon.setIcon ( QIcon ( BitmapSystemTrayInactive ) );
        }
    }
}

void CServerDlg::ModifyAutoStartEntry ( const bool bDoAutoStart )
{
// auto start is currently only supported for Windows
#ifdef _WIN32
    // init settings object so that it points to the correct place in the
    // Windows registry for the auto run entry
    QSettings RegSettings ( "HKEY_CURRENT_USER\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run",
                            QSettings::NativeFormat );

    // create start string of auto run entry
    QString strRegValue =
        QCoreApplication::applicationFilePath().replace ( "/", "\\" ) +
        " -s --startminimized";
#endif

    if ( bDoAutoStart )
    {
#ifdef _WIN32
        // ckeck if registry entry is correctly present, if not, correct
        const bool bWriteRegValue = strRegValue.compare (
            RegSettings.value ( AUTORUN_SERVER_REG_NAME ).toString() );

        if ( bWriteRegValue )
        {
            // write reg key in the registry
            RegSettings.setValue ( AUTORUN_SERVER_REG_NAME, strRegValue );
        }
#endif
    }
    else
    {
#ifdef _WIN32
        // delete reg key if present
        if ( RegSettings.contains ( AUTORUN_SERVER_REG_NAME ) )
        {
            RegSettings.remove ( AUTORUN_SERVER_REG_NAME );
        }
#endif
    }
}

void CServerDlg::UpdateRecorderStatus ( QString sessionDir )
{
    QString        currentSessionDir = edtCurrentSessionDir->text();
    QString        errMsg            = pServer->GetRecorderErrMsg();
    bool           bIsRecording      = false;
    QString        strRecorderStatus;
    QString        strRecordingDir;

    if ( pServer->GetRecorderInitialised() )
    {
        strRecordingDir = pServer->GetRecordingDir();
        chbEnableRecorder->setEnabled ( true );
        if ( pServer->GetRecordingEnabled() )
        {
            if ( pServer->IsRunning() )
            {
                edtCurrentSessionDir->setText ( sessionDir != QString::null ? sessionDir : "" );
                strRecorderStatus = tr ( "Recording" );
                bIsRecording      = true;
            }
            else
            {
                strRecorderStatus = tr ( "Not recording" );
            }
        }
        else
        {
            strRecorderStatus = tr ( "Not enabled" );
        }
    }
    else
    {
        strRecordingDir = pServer->GetRecorderErrMsg();
        if ( strRecordingDir == QString::null )
        {
            strRecordingDir = pServer->GetRecordingDir();
        }
        else
        {
            strRecordingDir = tr ( "ERROR" ) + ": " + strRecordingDir;
        }
        chbEnableRecorder->setEnabled ( false );
        strRecorderStatus = tr ( "Not initialised" );
    }

    edtCurrentSessionDir->setEnabled ( bIsRecording );
    lblRecorderStatus->setText ( strRecorderStatus );
    pbtNewRecording->setEnabled ( bIsRecording );
    edtRecordingDir->setText( strRecordingDir );
}

void CServerDlg::changeEvent ( QEvent* pEvent )
{
    // if we have a system tray icon, we make the window invisible if it is
    // minimized
    if ( bSystemTrayIconAvaialbe &&
         ( pEvent->type() == QEvent::WindowStateChange ) )
    {
        if ( isMinimized() )
        {
            // we have to call the hide function from another thread -> use
            // the timer for this purpose
            QTimer::singleShot ( 0, this, SLOT ( hide() ) );
        }
    }
}
