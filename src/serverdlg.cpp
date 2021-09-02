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

#include "serverdlg.h"

/* Implementation *************************************************************/
CServerDlg::CServerDlg ( CServer* pNServP, CServerSettings* pNSetP, const bool bStartMinimized, QWidget* parent ) :
    CBaseDlg ( parent, Qt::Window ), // use Qt::Window to get min/max window buttons
    pServer ( pNServP ),
    pSettings ( pNSetP ),
    BitmapSystemTrayInactive ( QString::fromUtf8 ( ":/png/LEDs/res/CLEDGreyArrow.png" ) ),
    BitmapSystemTrayActive ( QString::fromUtf8 ( ":/png/LEDs/res/CLEDGreenArrow.png" ) )
{
    // check if system tray icon can be used
    bSystemTrayIconAvaialbe = SystemTrayIcon.isSystemTrayAvailable();

    setupUi ( this );

    // Add help text to controls -----------------------------------------------
    // client list
    lvwClients->setWhatsThis ( "<b>" + tr ( "Client List" ) + ":</b> " +
                               tr ( "The client list shows all clients which are currently connected to this "
                                    "server. Some information about the clients like the IP address and name "
                                    "are given for each connected client." ) );

    lvwClients->setAccessibleName ( tr ( "Connected clients list view" ) );

    // start minimized on operating system start
    chbStartOnOSStart->setWhatsThis ( "<b>" +
                                      tr ( "Start Minimized on Operating "
                                           "System Start" ) +
                                      ":</b> " +
                                      tr ( "If the start minimized on operating system start "
                                           "check box is checked, the server will be "
                                           "started when the operating system starts up and is automatically "
                                           "minimized to a system task bar icon." ) );

    // Make My Server Public flag
    chbRegisterServer->setWhatsThis ( "<b>" + tr ( "Make My Server Public" ) + ":</b> " +
                                      tr ( "If the Make My Server Public check box is checked, this server registers "
                                           "itself at the directory server so that all users of the application "
                                           "can see the server in the connect dialog server list and "
                                           "connect to it. The registration of the server is renewed periodically "
                                           "to make sure that all servers in the connect dialog server list are "
                                           "actually available." ) );

    // register server status label
    lblRegSvrStatus->setWhatsThis ( "<b>" + tr ( "Register Server Status" ) + ":</b> " +
                                    tr ( "If the Make My Server Public check box is checked, this will show "
                                         "whether registration with the directory server is successful. If the "
                                         "registration failed, please choose another server list." ) );

    // custom directory server address
    QString strCentrServAddr = "<b>" + tr ( "Custom Directory Server Address" ) + ":</b> " +
                               tr ( "The custom directory server address is the IP address or URL of the directory "
                                    "server at which the server list of the connection dialog is managed." );

    lblCentralServerAddress->setWhatsThis ( strCentrServAddr );
    edtCentralServerAddress->setWhatsThis ( strCentrServAddr );
    edtCentralServerAddress->setAccessibleName ( tr ( "Directory server address line edit" ) );

    cbxCentServAddrType->setWhatsThis ( "<b>" + tr ( "Server List Selection" ) + ":</b> " +
                                        tr ( "Selects the server list (i.e. directory server address) in which your server will be added." ) );
    cbxCentServAddrType->setAccessibleName ( tr ( "Server list selection combo box" ) );

    // server name
    QString strServName = "<b>" + tr ( "Server Name" ) + ":</b> " +
                          tr ( "The server name identifies "
                               "your server in the connect dialog server list at the clients." );

    lblServerName->setWhatsThis ( strServName );
    edtServerName->setWhatsThis ( strServName );

    edtServerName->setAccessibleName ( tr ( "Server name line edit" ) );

    // location city
    QString strLocCity = "<b>" + tr ( "Location City" ) + ":</b> " +
                         tr ( "The city in which this "
                              "server is located can be set here. If a city name is entered, it "
                              "will be shown in the connect dialog server list at the clients." );

    lblLocationCity->setWhatsThis ( strLocCity );
    edtLocationCity->setWhatsThis ( strLocCity );

    edtLocationCity->setAccessibleName ( tr ( "City where the server is located line edit" ) );

    // location country
    QString strLocCountry = "<b>" + tr ( "Location country" ) + ":</b> " +
                            tr ( "The country in "
                                 "which this server is located can be set here. If a country is "
                                 "entered, it will be shown in the connect dialog server list at the "
                                 "clients." );

    lblLocationCountry->setWhatsThis ( strLocCountry );
    cbxLocationCountry->setWhatsThis ( strLocCountry );

    cbxLocationCountry->setAccessibleName ( tr ( "Country where the server is located combo box" ) );

    // recording directory
    pbtRecordingDir->setAccessibleName ( tr ( "Display dialog to select recording directory button" ) );
    pbtRecordingDir->setWhatsThis ( "<b>" + tr ( "Main Recording Directory" ) + ":</b> " +
                                    tr ( "Click the button to open the dialog that allows the main recording directory to be selected."
                                         "The chosen value must exist and be writeable (allow creation of sub-directories "
                                         "by the user Jamulus is running as). " ) );

    edtRecordingDir->setAccessibleName ( tr ( "Main recording directory text box (read-only)" ) );
    edtRecordingDir->setWhatsThis ( "<b>" + tr ( "Main Recording Directory" ) + ":</b> " +
                                    tr ( "The current value of the main recording directory. "
                                         "The chosen value must exist and be writeable (allow creation of sub-directories "
                                         "by the user Jamulus is running as). "
                                         "Click the button to open the dialog that allows the main recording directory to be selected." ) );

    tbtClearRecordingDir->setAccessibleName ( tr ( "Clear the recording directory button" ) );
    tbtClearRecordingDir->setWhatsThis ( "<b>" + tr ( "Clear Recording Directory" ) + ":</b> " +
                                         tr ( "Click the button to clear the currently selected recording directory. "
                                              "This will prevent recording until a new value is selected." ) );

    // enable recorder
    chbEnableRecorder->setAccessibleName ( tr ( "Checkbox to turn on or off server recording" ) );
    chbEnableRecorder->setWhatsThis ( "<b>" + tr ( "Enable Recorder" ) + ":</b> " +
                                      tr ( "Checked when the recorder is enabled, otherwise unchecked. "
                                           "The recorder will run when a session is in progress, if (set up correctly and) enabled." ) );

    // current session directory
    edtCurrentSessionDir->setAccessibleName ( tr ( "Current session directory text box (read-only)" ) );
    edtCurrentSessionDir->setWhatsThis ( "<b>" + tr ( "Current Session Directory" ) + ":</b> " +
                                         tr ( "Enabled during recording and holds the current recording session directory. "
                                              "Disabled after recording or when the recorder is not enabled." ) );

    // recorder status
    lblRecorderStatus->setAccessibleName ( tr ( "Recorder status label" ) );
    lblRecorderStatus->setWhatsThis (
        "<b>" + tr ( "Recorder Status" ) + ":</b> " + tr ( "Displays the current status of the recorder.  The following values are possible:" ) +
        "<dl>" + "<dt>" + SREC_NOT_INITIALISED + "</dt>" + "<dd>" + tr ( "No recording directory has been set or the value is not useable" ) +
        "</dd>" + "<dt>" + SREC_NOT_ENABLED + "</dt>" + "<dd>" + tr ( "Recording has been switched off" )
#ifdef _WIN32
        + tr ( " by the UI checkbox" )
#else
        + tr ( ", either by the UI checkbox or SIGUSR2 being received" )
#endif
        + "</dd>" + "<dt>" + SREC_NOT_RECORDING + "</dt>" + "<dd>" + tr ( "There is no one connected to the server to record" ) + "</dd>" + "<dt>" +
        SREC_RECORDING + "</dt>" + "<dd>" + tr ( "The performers are being recorded to the specified session directory" ) + "</dd>" + "</dl>" +
        "<br/><b>" + tr ( "NOTE" ) + ":</b> " +
        tr ( "If the recording directory is not useable, the problem will be displayed in place of the directory." ) );

    // new recording
    pbtNewRecording->setAccessibleName ( tr ( "Request new recording button" ) );
    pbtNewRecording->setWhatsThis ( "<b>" + tr ( "New Recording" ) + ":</b> " +
                                    tr ( "During a recording session, the button can be used to start a new recording." ) );

    // welcome message
    tedWelcomeMessage->setAccessibleName ( tr ( "Server welcome message edit box" ) );
    tedWelcomeMessage->setWhatsThis ( "<b>" + tr ( "Server Welcome Message" ) + ":</b> " +
                                      tr ( "A server welcome message text is displayed in the chat window if a "
                                           "musician enters the server. If no message is set, the server welcome is disabled." ) );

    // init system tray icon
    if ( bSystemTrayIconAvaialbe )
    {
        // prepare context menu to be added to the system tray icon
        pSystemTrayIconMenu = new QMenu ( this );

        pSystemTrayIconMenu->addAction ( tr ( "E&xit" ), this, SLOT ( OnSysTrayMenuExit() ) );

        pSystemTrayIconMenu->addSeparator();

        pSystemTrayIconMenu->addAction ( tr ( "&Hide " ) + APP_NAME + tr ( " server" ), this, SLOT ( OnSysTrayMenuHide() ) );

        pSystemTrayIconMenu->setDefaultAction (
            pSystemTrayIconMenu->addAction ( tr ( "&Open " ) + APP_NAME + tr ( " server" ), this, SLOT ( OnSysTrayMenuOpen() ) ) );

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

    // set up list view for connected clients
    lvwClients->setColumnWidth ( 0, 170 ); // 170 //  IP:port
    lvwClients->setColumnWidth ( 1, 200 ); // 200 //  Name
    lvwClients->setColumnWidth ( 2, 120 ); //  60 //  Buf-Frames
    lvwClients->setColumnWidth ( 3, 50 );  //         Channels
    lvwClients->clear();

    // clang-format off
// TEST workaround for resize problem of window after iconize in task bar
lvwClients->setMinimumWidth ( 170 + 130 + 60 + 205 );
lvwClients->setMinimumHeight ( 140 );
    // clang-format on

    // insert items in reverse order because in Windows all of them are
    // always visible -> put first item on the top
    vecpListViewItems.Init ( MAX_NUM_CHANNELS );

    for ( int i = MAX_NUM_CHANNELS - 1; i >= 0; i-- )
    {
        vecpListViewItems[i] = new QTreeWidgetItem ( lvwClients );
        vecpListViewItems[i]->setHidden ( true );
    }

    // directory server address type combo box
    cbxCentServAddrType->clear();
    cbxCentServAddrType->addItem ( csCentServAddrTypeToString ( AT_DEFAULT ) );
    cbxCentServAddrType->addItem ( csCentServAddrTypeToString ( AT_ANY_GENRE2 ) );
    cbxCentServAddrType->addItem ( csCentServAddrTypeToString ( AT_ANY_GENRE3 ) );
    cbxCentServAddrType->addItem ( csCentServAddrTypeToString ( AT_GENRE_ROCK ) );
    cbxCentServAddrType->addItem ( csCentServAddrTypeToString ( AT_GENRE_JAZZ ) );
    cbxCentServAddrType->addItem ( csCentServAddrTypeToString ( AT_GENRE_CLASSICAL_FOLK ) );
    cbxCentServAddrType->addItem ( csCentServAddrTypeToString ( AT_GENRE_CHORAL ) );
    cbxCentServAddrType->addItem ( csCentServAddrTypeToString ( AT_CUSTOM ) );
    cbxCentServAddrType->setCurrentIndex ( static_cast<int> ( pServer->GetCentralServerAddressType() ) );

    // custom directory server address
    edtCentralServerAddress->setText ( pServer->GetServerListCentralServerAddress() );

    // update server name line edit
    edtServerName->setText ( pServer->GetServerName() );

    // update server city line edit
    edtLocationCity->setText ( pServer->GetServerCity() );

    // load country combo box with all available countries
    cbxLocationCountry->setInsertPolicy ( QComboBox::NoInsert );
    cbxLocationCountry->clear();

    for ( int iCurCntry = static_cast<int> ( QLocale::AnyCountry ); iCurCntry < static_cast<int> ( QLocale::LastCountry ); iCurCntry++ )
    {
        // add all countries except of the "Default" country
        if ( static_cast<QLocale::Country> ( iCurCntry ) != QLocale::AnyCountry )
        {
            // store the country enum index together with the string (this is
            // important since we sort the combo box items later on)
            cbxLocationCountry->addItem ( QLocale::countryToString ( static_cast<QLocale::Country> ( iCurCntry ) ), iCurCntry );
        }
    }

    // sort country combo box items in alphabetical order
    cbxLocationCountry->model()->sort ( 0, Qt::AscendingOrder );

    // select current country
    cbxLocationCountry->setCurrentIndex ( cbxLocationCountry->findData ( static_cast<int> ( pServer->GetServerCountry() ) ) );

    // update register server check box
    if ( pServer->GetServerListEnabled() )
    {
        chbRegisterServer->setCheckState ( Qt::Checked );
    }
    else
    {
        chbRegisterServer->setCheckState ( Qt::Unchecked );
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

    // update delay panning check box
    if ( pServer->IsDelayPanningEnabled() )
    {
        chbEnableDelayPanning->setCheckState ( Qt::Checked );
    }
    else
    {
        chbEnableDelayPanning->setCheckState ( Qt::Unchecked );
    }

    // Recorder controls
    chbEnableRecorder->setChecked ( pServer->GetRecordingEnabled() );
    edtCurrentSessionDir->setText ( "" );
    pbtNewRecording->setAutoDefault ( false );
    pbtRecordingDir->setAutoDefault ( false );
    edtRecordingDir->setText ( pServer->GetRecordingDir() );
    tbtClearRecordingDir->setText ( u8"\u232B" );

    UpdateRecorderStatus ( QString::null );

    // language combo box (corrects the setting if language not found)
    cbxLanguage->Init ( pSettings->strLanguage );

    // setup welcome message GUI control
    tedWelcomeMessage->setPlaceholderText ( tr ( "Type a message here. If no message is set, the server welcome is disabled." ) );

    tedWelcomeMessage->setText ( pServer->GetWelcomeMessage() );

    // prepare update check info label (invisible by default)
    lblUpdateCheck->setOpenExternalLinks ( true ); // enables opening a web browser if one clicks on a html link
    lblUpdateCheck->setText (
        "<font color=\"#c94a55\"><b>" +
            APP_UPGRADE_AVAILABLE_MSG_TEXT
            .arg ( APP_NAME )
            .arg ( VERSION ) +
        "</b></font>" );
    lblUpdateCheck->hide();

    // update GUI dependencies
    UpdateGUIDependencies();

    // set window title
    setWindowTitle ( APP_NAME + tr ( " Server" ) );

    // View menu  --------------------------------------------------------------
    QMenu* pViewMenu = new QMenu ( tr ( "&Window" ), this );

    pViewMenu->addAction ( tr ( "E&xit" ), this, SLOT ( close() ), QKeySequence ( Qt::CTRL + Qt::Key_Q ) );

    // Main menu bar -----------------------------------------------------------
    pMenu = new QMenuBar ( this );

    pMenu->addMenu ( pViewMenu );
    pMenu->addMenu ( new CHelpMenu ( false, this ) );

    // Now tell the layout about the menu
    layout()->setMenuBar ( pMenu );

    // Window positions --------------------------------------------------------
    // main window
    if ( !pSettings->vecWindowPosMain.isEmpty() && !pSettings->vecWindowPosMain.isNull() )
    {
        restoreGeometry ( pSettings->vecWindowPosMain );
    }

    // Connections -------------------------------------------------------------
    // check boxes
    QObject::connect ( chbRegisterServer, &QCheckBox::stateChanged, this, &CServerDlg::OnRegisterServerStateChanged );

    QObject::connect ( chbStartOnOSStart, &QCheckBox::stateChanged, this, &CServerDlg::OnStartOnOSStartStateChanged );

    QObject::connect ( chbEnableRecorder, &QCheckBox::stateChanged, this, &CServerDlg::OnEnableRecorderStateChanged );

    // delay panning
    QObject::connect ( chbEnableDelayPanning, &QCheckBox::stateChanged, this, &CServerDlg::OnEnableDelayPanningStateChanged );

    // line edits
    QObject::connect ( edtCentralServerAddress, &QLineEdit::editingFinished, this, &CServerDlg::OnCentralServerAddressEditingFinished );

    QObject::connect ( edtServerName, &QLineEdit::textChanged, this, &CServerDlg::OnServerNameTextChanged );

    QObject::connect ( edtLocationCity, &QLineEdit::textChanged, this, &CServerDlg::OnLocationCityTextChanged );

    // combo boxes
    QObject::connect ( cbxLocationCountry,
                       static_cast<void ( QComboBox::* ) ( int )> ( &QComboBox::activated ),
                       this,
                       &CServerDlg::OnLocationCountryActivated );

    QObject::connect ( cbxCentServAddrType,
                       static_cast<void ( QComboBox::* ) ( int )> ( &QComboBox::activated ),
                       this,
                       &CServerDlg::OnCentServAddrTypeActivated );

    QObject::connect ( cbxLanguage, &CLanguageComboBox::LanguageChanged, this, &CServerDlg::OnLanguageChanged );

    // push buttons
    QObject::connect ( pbtRecordingDir, &QPushButton::released, this, &CServerDlg::OnRecordingDirClicked );

    QObject::connect ( pbtNewRecording, &QPushButton::released, this, &CServerDlg::OnNewRecordingClicked );

    // tool buttons
    QObject::connect ( tbtClearRecordingDir, &QToolButton::released, this, &CServerDlg::OnClearRecordingDirClicked );

    // timers
    QObject::connect ( &Timer, &QTimer::timeout, this, &CServerDlg::OnTimer );

    // other
    QObject::connect ( tedWelcomeMessage, &QTextEdit::textChanged, this, &CServerDlg::OnWelcomeMessageChanged );

    QObject::connect ( pServer, &CServer::Started, this, &CServerDlg::OnServerStarted );

    QObject::connect ( pServer, &CServer::Stopped, this, &CServerDlg::OnServerStopped );

    QObject::connect ( pServer, &CServer::SvrRegStatusChanged, this, &CServerDlg::OnSvrRegStatusChanged );

    QObject::connect ( pServer, &CServer::RecordingSessionStarted, this, &CServerDlg::OnRecordingSessionStarted );

    QObject::connect ( pServer, &CServer::StopRecorder, this, &CServerDlg::OnStopRecorder );

    QObject::connect ( pServer, &CServer::CLVersionAndOSReceived, this, &CServerDlg::OnCLVersionAndOSReceived );

    QObject::connect ( &SystemTrayIcon, &QSystemTrayIcon::activated, this, &CServerDlg::OnSysTrayActivated );

    // Initializations which have to be done after the signals are connected ---
    // start timer for GUI controls
    Timer.start ( GUI_CONTRL_UPDATE_TIME );

    // query the update server version number needed for update check (note
    // that the connection less message respond may not make it back but that
    // is not critical since the next time Jamulus is started we have another
    // chance and the update check is not time-critical at all)
    CHostAddress UpdateServerHostAddress;

    // Send the request to two servers for redundancy if either or both of them
    // has a higher release version number, the reply will trigger the notification.

    if ( NetworkUtil().ParseNetworkAddress ( UPDATECHECK1_ADDRESS, UpdateServerHostAddress, pServer->IsIPv6Enabled() ) )
    {
        pServer->CreateCLServerListReqVerAndOSMes ( UpdateServerHostAddress );
    }

    if ( NetworkUtil().ParseNetworkAddress ( UPDATECHECK2_ADDRESS, UpdateServerHostAddress, pServer->IsIPv6Enabled() ) )
    {
        pServer->CreateCLServerListReqVerAndOSMes ( UpdateServerHostAddress );
    }
}

void CServerDlg::closeEvent ( QCloseEvent* Event )
{
    // store window positions
    pSettings->vecWindowPosMain = saveGeometry();

    // default implementation of this event handler routine
    Event->accept();
}

void CServerDlg::OnStartOnOSStartStateChanged ( int value )
{
    const bool bCurAutoStartMinState = ( value == Qt::Checked );

    // update registry and server setting (for ini file)
    pServer->SetAutoRunMinimized ( bCurAutoStartMinState );
    ModifyAutoStartEntry ( bCurAutoStartMinState );
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
    pServer->SetServerListCentralServerAddress ( edtCentralServerAddress->text() );

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
        // text is too long, update control with shortened text
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
        // text is too long, update control with shortened text
        edtLocationCity->setText ( strNewCity.left ( MAX_LEN_SERVER_CITY ) );
    }
}

void CServerDlg::OnLocationCountryActivated ( int iCntryListItem )
{
    // apply new setting to the server and update it
    pServer->SetServerCountry ( static_cast<QLocale::Country> ( cbxLocationCountry->itemData ( iCntryListItem ).toInt() ) );

    pServer->UpdateServerList();
}

void CServerDlg::OnCentServAddrTypeActivated ( int iTypeIdx )
{

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
    if ( pServer->GetRecorderErrMsg() != QString::null )
    {
        QMessageBox::warning ( this,
                               APP_NAME,
                               tr ( "Recorder failed to start. "
                                    "Please check available disk space and permissions and try again. "
                                    "Error: " ) +
                                   pServer->GetRecorderErrMsg() );
    }
}

void CServerDlg::OnRecordingDirClicked()
{
    // get the current value from pServer
    QString currentValue    = pServer->GetRecordingDir();
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
#ifdef _WIN32
    // on single or double click on the icon, show window in foreground for windows only
    if ( ActReason == QSystemTrayIcon::Trigger || ActReason == QSystemTrayIcon::DoubleClick )
#else
    // on double click on the icon, show window in foreground for all
    if ( ActReason == QSystemTrayIcon::DoubleClick )
#endif
    {
        ShowWindowInForeground();
    }
}

void CServerDlg::OnCLVersionAndOSReceived ( CHostAddress, COSUtil::EOpSystemType, QString strVersion )
{
    // update check
#if QT_VERSION >= QT_VERSION_CHECK( 5, 6, 0 )
    int            mySuffixIndex;
    QVersionNumber myVersion = QVersionNumber::fromString ( VERSION, &mySuffixIndex );

    int            serverSuffixIndex;
    QVersionNumber serverVersion = QVersionNumber::fromString ( strVersion, &serverSuffixIndex );

    // only compare if the server version has no suffix (such as dev or beta)
    if ( strVersion.size() == serverSuffixIndex && QVersionNumber::compare ( serverVersion, myVersion ) > 0 )
    {
        lblUpdateCheck->show();
    }
#endif
}

void CServerDlg::OnTimer()
{
    CVector<CHostAddress> vecHostAddresses;
    CVector<QString>      vecsName;
    CVector<int>          veciJitBufNumFrames;
    CVector<int>          veciNetwFrameSizeFact;

    ListViewMutex.lock();
    {
        pServer->GetConCliParam ( vecHostAddresses, vecsName, veciJitBufNumFrames, veciNetwFrameSizeFact );

        // we assume that all vectors have the same length
        const int iNumChannels = vecHostAddresses.Size();

        // fill list with connected clients
        for ( int i = 0; i < iNumChannels; i++ )
        {
            if ( !( vecHostAddresses[i].InetAddr == QHostAddress ( static_cast<quint32> ( 0 ) ) ) )
            {
                // IP, port number
                vecpListViewItems[i]->setText ( 0, vecHostAddresses[i].toString ( CHostAddress::SM_IP_PORT ) );

                // name
                vecpListViewItems[i]->setText ( 1, vecsName[i] );

                // jitter buffer size (polling for updates)
                vecpListViewItems[i]->setText ( 2, QString().setNum ( veciJitBufNumFrames[i] ) );

                // show num of audio channels
                int iNumAudioChs = pServer->GetClientNumAudioChannels ( i );
                vecpListViewItems[i]->setText ( 3, QString().setNum ( iNumAudioChs ) );

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
    const bool          bCurSerListEnabled = pServer->GetServerListEnabled();
    const ESvrRegStatus eSvrRegStatus      = pServer->GetSvrRegStatus();

    // if register server is not enabled, we disable all the configuration
    // controls for the server list
    cbxCentServAddrType->setEnabled ( bCurSerListEnabled );
    grbServerInfo->setEnabled ( bCurSerListEnabled );

    QString strStatus = svrRegStatusToString ( eSvrRegStatus );

    switch ( eSvrRegStatus )
    {
    case SRS_BAD_ADDRESS:
    case SRS_TIME_OUT:
    case SRS_CENTRAL_SVR_FULL:
    case SRS_VERSION_TOO_OLD:
    case SRS_NOT_FULFILL_REQUIREMENTS:
        strStatus = "<font color=\"red\"><b>" + strStatus + "</b></font>";
        break;

    case SRS_REGISTERED:
        strStatus = "<font color=\"darkGreen\"><b>" + strStatus + "</b></font>";
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
    QSettings RegSettings ( "HKEY_CURRENT_USER\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run", QSettings::NativeFormat );

    // create start string of auto run entry
    QString strRegValue = QCoreApplication::applicationFilePath().replace ( "/", "\\" ) + " -s --startminimized";
#endif

    if ( bDoAutoStart )
    {
#ifdef _WIN32
        // ckeck if registry entry is correctly present, if not, correct
        const bool bWriteRegValue = strRegValue.compare ( RegSettings.value ( AUTORUN_SERVER_REG_NAME ).toString() );

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
    QString currentSessionDir = edtCurrentSessionDir->text();
    QString errMsg            = pServer->GetRecorderErrMsg();
    bool    bIsRecording      = false;
    QString strRecorderStatus;
    QString strRecordingDir;

    if ( pServer->GetRecorderInitialised() )
    {
        strRecordingDir = pServer->GetRecordingDir();
        chbEnableRecorder->setEnabled ( true );

        if ( pServer->GetRecordingEnabled() )
        {
            if ( pServer->IsRunning() )
            {
                edtCurrentSessionDir->setText ( sessionDir != QString::null ? sessionDir : "" );

                strRecorderStatus = SREC_RECORDING;
                bIsRecording      = true;
            }
            else
            {
                strRecorderStatus = SREC_NOT_RECORDING;
            }
        }
        else
        {
            strRecorderStatus = SREC_NOT_ENABLED;
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
        strRecorderStatus = SREC_NOT_INITIALISED;
    }

    chbEnableRecorder->blockSignals ( true );
    chbEnableRecorder->setChecked ( strRecorderStatus != SREC_NOT_ENABLED );
    chbEnableRecorder->blockSignals ( false );

    edtRecordingDir->setText ( strRecordingDir );
    edtRecordingDir->setEnabled ( !bIsRecording );
    pbtRecordingDir->setEnabled ( !bIsRecording );
    tbtClearRecordingDir->setEnabled ( !bIsRecording );
    edtCurrentSessionDir->setEnabled ( bIsRecording );
    lblRecorderStatus->setText ( strRecorderStatus );
    pbtNewRecording->setEnabled ( bIsRecording );
}

void CServerDlg::changeEvent ( QEvent* pEvent )
{
    // if we have a system tray icon, we make the window invisible if it is
    // minimized
    if ( bSystemTrayIconAvaialbe && ( pEvent->type() == QEvent::WindowStateChange ) )
    {
        if ( isMinimized() )
        {
            // we have to call the hide function from another thread -> use
            // the timer for this purpose
            QTimer::singleShot ( 0, this, SLOT ( hide() ) );
        }
        else
        {
            // we have to call the show function from another thread -> use
            // the timer for this purpose
            QTimer::singleShot ( 0, this, SLOT ( show() ) );
        }
    }
}
