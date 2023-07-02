/******************************************************************************\
 * Copyright (c) 2004-2023
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
    BitmapSystemTrayInactive ( QString::fromUtf8 ( ":/png/main/res/servertrayiconinactive.png" ) ),
    BitmapSystemTrayActive ( QString::fromUtf8 ( ":/png/main/res/servertrayiconactive.png" ) )
{
    // check if system tray icon can be used
    bSystemTrayIconAvailable = SystemTrayIcon.isSystemTrayAvailable();

    setupUi ( this );

    // always start on the main tab
    tabWidget->setCurrentIndex ( 0 );

    // set window title
    setWindowTitle ( tr ( "%1 Server", "%1 is the name of the main application" ).arg ( APP_NAME ) );

    // Add help text to controls -----------------------------------------------

    // Tab: Server setup

    // client list
    lvwClients->setWhatsThis ( "<b>" + tr ( "Client List" ) + ":</b> " +
                               tr ( "The client list shows all clients which are currently connected to this "
                                    "server. Some information about the clients like the IP address and name "
                                    "are given for each connected client." ) );

    lvwClients->setAccessibleName ( tr ( "Connected clients list view" ) );

    // Server list selection combo box
    QString strDirectoryTypeAN = tr ( "Directory Type combo box" );
    lblDirectoryType->setAccessibleName ( strDirectoryTypeAN );
    cbxDirectoryType->setAccessibleName ( strDirectoryTypeAN );

    QString strDirectoryTypeWT = tr ( "<b>Directory:</b> "
                                      "Select '%1' not to register your server with a directory.<br>"
                                      "Or select one of the genres to register with that directory.<br>"
                                      "Or select '%2' and specify a Custom Directory address on the "
                                      "Options tab to register with a custom directory.<br><br>"
                                      "For any value except '%1', this server registers "
                                      "with a directory so that a %3 user can select this server "
                                      "in the client connect dialog server list when they choose that directory.<br><br>"
                                      "The registration of the server is renewed periodically "
                                      "to make sure that all servers in the connect dialog server list are "
                                      "actually available.",
                                      "%1: directory type NONE; %2: directory type CUSTOM; %3 app name, Jamulus" )
                                     .arg ( DirectoryTypeToString ( AT_NONE ) )
                                     .arg ( DirectoryTypeToString ( AT_CUSTOM ) )
                                     .arg ( APP_NAME );
    lblDirectoryType->setWhatsThis ( strDirectoryTypeWT );
    cbxDirectoryType->setWhatsThis ( strDirectoryTypeWT );

    // server registration status label
    lblRegSvrStatus->setWhatsThis ( "<b>" + tr ( "Register Server Status" ) + ":</b> " +
                                    tr ( "When a value other than \"%1\" is chosen for Directory, this will show "
                                         "whether registration is successful. If the registration failed, "
                                         "please choose a different directory." )
                                        .arg ( DirectoryTypeToString ( AT_NONE ) ) );

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
    QString strLocCountry = "<b>" + tr ( "Country/Region" ) + ":</b> " +
                            tr ( "Set the country or region where the server is running. "
                                 "Clients will show this location in their connect dialog's server "
                                 "list." );

    lblLocationCountry->setWhatsThis ( strLocCountry );
    cbxLocationCountry->setWhatsThis ( strLocCountry );

    cbxLocationCountry->setAccessibleName ( tr ( "Combo box for location of this server" ) );

    // enable recorder
    chbJamRecorder->setAccessibleName ( tr ( "Checkbox to turn on or off server recording" ) );
    chbJamRecorder->setWhatsThis ( "<b>" + tr ( "Jam Recorder" ) + ":</b> " +
                                   tr ( "When checked, the recorder will run while a session is in progress (if set up correctly)." ) );

    // new recording
    pbtNewRecording->setAccessibleName ( tr ( "Request new recording button" ) );
    pbtNewRecording->setWhatsThis ( "<b>" + tr ( "New Recording" ) + ":</b> " +
                                    tr ( "During a recording session, the button can be used to start a new recording." ) );

    // recorder status
    lblRecorderStatus->setAccessibleName ( tr ( "Recorder status label" ) );
    lblRecorderStatus->setWhatsThis (
        "<b>" + tr ( "Recorder Status" ) + ":</b> " + tr ( "Displays the current status of the recorder.  The following values are possible:" ) +
        "<dl>" + "<dt>" + SREC_NOT_INITIALISED + "</dt>" + "<dd>" +
        tr ( "No recording directory has been set or the value is not useable. "
             "Check the value in the Options tab." ) +
        "</dd>" + "<dt>" + SREC_NOT_ENABLED + "</dt>" + "<dd>"
#ifdef _WIN32
        + tr ( "Recording has been switched off by the UI checkbox or JSON-RPC." )
#else
        + tr ( "Recording has been switched off, by the UI checkbox, SIGUSR2 or JSON-RPC." )
#endif
        + "</dd>" + "<dt>" + SREC_NOT_RECORDING + "</dt>" + "<dd>" + tr ( "There is no one connected to the server to record." ) + "</dd>" + "<dt>" +
        SREC_RECORDING + "</dt>" + "<dd>" + tr ( "The performers are being recorded to the specified session directory." ) + "</dd>" + "</dl>" +
        "<br/><b>" + tr ( "NOTE" ) + ":</b> " +
        tr ( "If the recording directory is not useable, the problem will be displayed in place of the session directory." ) );

    // current session directory
    QString strCurrentSessionDirAN = tr ( "Current session directory text box (read-only)" );
    lblCurrentSessionDir->setAccessibleName ( strCurrentSessionDirAN );
    edtCurrentSessionDir->setAccessibleName ( strCurrentSessionDirAN );

    QString strCurrentSessionDirWT = "<b>" + tr ( "Current Session Directory" ) + ":</b> " +
                                     tr ( "Enabled during recording and holds the current recording session directory. "
                                          "Disabled after recording or when the recorder is not enabled." );
    lblCurrentSessionDir->setWhatsThis ( strCurrentSessionDirWT );
    edtCurrentSessionDir->setWhatsThis ( strCurrentSessionDirWT );

    // welcome message
    tedWelcomeMessage->setAccessibleName ( tr ( "Server welcome message edit box" ) );
    tedWelcomeMessage->setWhatsThis ( "<b>" + tr ( "Server Welcome Message" ) + ":</b> " +
                                      tr ( "A server welcome message text is displayed in the chat window if a "
                                           "musician enters the server. If no message is set, the server welcome is disabled." ) );

    // Tab: options

    // Interface Language
    QString strWTLanguage = "<b>" + tr ( "Language" ) + ":</b> " + tr ( "Select the language to be used for the user interface." );
    lblLanguage->setWhatsThis ( strWTLanguage );
    cbxLanguage->setWhatsThis ( strWTLanguage );

    cbxLanguage->setAccessibleName ( tr ( "Language combo box" ) );

    // recording directory
    pbtRecordingDir->setAccessibleName ( tr ( "Display dialog to select recording directory button" ) );
    pbtRecordingDir->setWhatsThis ( "<b>" + tr ( "Main Recording Directory" ) + ":</b> " +
                                    tr ( "Click the button to open the dialog that allows the main recording directory to be selected.  "
                                         "The chosen value must exist and be writeable (allow creation of sub-directories "
                                         "by the user %1 is running as)." )
                                        .arg ( APP_NAME ) );

    edtRecordingDir->setAccessibleName ( tr ( "Main recording directory text box (read-only)" ) );
    edtRecordingDir->setWhatsThis ( "<b>" + tr ( "Main Recording Directory" ) + ":</b> " +
                                    tr ( "The current value of the main recording directory. "
                                         "The chosen value must exist and be writeable (allow creation of sub-directories "
                                         "by the user %1 is running as). "
                                         "Click the button to open the dialog that allows the main recording directory to be selected." )
                                        .arg ( APP_NAME ) );

    tbtClearRecordingDir->setAccessibleName ( tr ( "Clear the recording directory button" ) );
    tbtClearRecordingDir->setWhatsThis ( "<b>" + tr ( "Clear Recording Directory" ) + ":</b> " +
                                         tr ( "Click the button to clear the currently selected recording directory. "
                                              "This will prevent recording until a new value is selected." ) );

    // custom directory
    QString strCustomDirectory = "<b>" + tr ( "Custom Directory address" ) + ":</b> " +
                                 tr ( "The Custom Directory address is the address of the directory "
                                      "holding the server list to which this server should be added." );

    lblCustomDirectory->setWhatsThis ( strCustomDirectory );
    edtCustomDirectory->setWhatsThis ( strCustomDirectory );
    edtCustomDirectory->setAccessibleName ( tr ( "Custom Directory line edit" ) );

    // server list persistence file name (directory only)
    pbtServerListPersistence->setAccessibleName ( tr ( "Server List Filename dialog push button" ) );
    pbtServerListPersistence->setWhatsThis ( "<b>" + tr ( "Server List Filename" ) + ":</b> " +
                                             tr ( "Click the button to open the dialog that allows the "
                                                  "server list persistence file name to be set. The user %1 is running as "
                                                  "needs to be able to create the file name specified "
                                                  "although it may already exist (it will get overwritten on save)." )
                                                 .arg ( APP_NAME ) );

    edtServerListPersistence->setAccessibleName ( tr ( "Server List Filename text box (read-only)" ) );
    edtServerListPersistence->setWhatsThis ( "<b>" + tr ( "Server List Filename" ) + ":</b> " +
                                             tr ( "The current value of server list persistence file name. The user %1 is running as "
                                                  "needs to be able to create the file name specified "
                                                  "although it may already exist (it will get overwritten on save). "
                                                  "Click the button to open the dialog that allows the "
                                                  "server list persistence file name to be set." )
                                                 .arg ( APP_NAME ) );

    tbtClearServerListPersistence->setAccessibleName ( tr ( "Clear the server list file name button" ) );
    tbtClearServerListPersistence->setWhatsThis ( "<b>" + tr ( "Clear Server List Filename" ) + ":</b> " +
                                                  tr ( "Click the button to clear the currently selected server list persistence file name. "
                                                       "This will prevent persisting the server list until a new value is selected." ) );

    // start minimized on operating system start
    chbStartOnOSStart->setWhatsThis ( "<b>" + tr ( "Start Minimized on Operating System Start" ) + ":</b> " +
                                      tr ( "If the start minimized on operating system start "
                                           "check box is checked, the server will be "
                                           "started when the operating system starts up and is automatically "
                                           "minimized to a system task bar icon." ) );

    // Application initialisation

    // init system tray icon
    if ( bSystemTrayIconAvailable )
    {
        // prepare context menu to be added to the system tray icon
        pSystemTrayIconMenu = new QMenu ( this );

        pSystemTrayIconMenu->addAction ( tr ( "E&xit" ), this, SLOT ( OnSysTrayMenuExit() ) );

        pSystemTrayIconMenu->addSeparator();

        pSystemTrayIconMenu->addAction ( tr ( "&Hide %1 server" ).arg ( APP_NAME ), this, SLOT ( OnSysTrayMenuHide() ) );

        pSystemTrayIconMenu->setDefaultAction (
            pSystemTrayIconMenu->addAction ( tr ( "&Show %1 server" ).arg ( APP_NAME ), this, SLOT ( OnSysTrayMenuOpen() ) ) );

        SystemTrayIcon.setContextMenu ( pSystemTrayIconMenu );

        // set tool text
        SystemTrayIcon.setToolTip ( tr ( "%1 server", "%1 is the name of the main application" ).arg ( APP_NAME ) );

        // show icon of state "inactive"
        SystemTrayIcon.setIcon ( QIcon ( BitmapSystemTrayInactive ) );
        SystemTrayIcon.show();
    }

    // act on "start minimized" flag (note, this has to be done after setting
    // and acting on the correct value for the system tray icon availablility)
    if ( bStartMinimized )
    {
        showMinimized();
    }

    // UI initialisation

    // set up list view for connected clients
    lvwClients->setColumnWidth ( 0, 170 ); // 170 //  IP:port
    lvwClients->setColumnWidth ( 1, 200 ); // 200 //  Name
    lvwClients->setColumnWidth ( 2, 120 ); //  60 //  Buf-Frames
    lvwClients->setColumnWidth ( 3, 50 );  //         Channels
    lvwClients->clear();

    //### TODO: BEGIN ###//
    // workaround for resize problem of window after iconize in task bar
    lvwClients->setMinimumWidth ( 170 + 130 + 60 + 205 );
    lvwClients->setMinimumHeight ( 140 );
    //### TODO: END ###//

    // insert items in reverse order because in Windows all of them are
    // always visible -> put first item on the top
    vecpListViewItems.Init ( MAX_NUM_CHANNELS );

    for ( int i = MAX_NUM_CHANNELS - 1; i >= 0; i-- )
    {
        vecpListViewItems[i] = new QTreeWidgetItem ( lvwClients );
        vecpListViewItems[i]->setHidden ( true );
    }

    // directory type combo box
    cbxDirectoryType->clear();
    cbxDirectoryType->addItem ( DirectoryTypeToString ( AT_NONE ) );
    cbxDirectoryType->addItem ( DirectoryTypeToString ( AT_DEFAULT ) );
    cbxDirectoryType->addItem ( DirectoryTypeToString ( AT_ANY_GENRE2 ) );
    cbxDirectoryType->addItem ( DirectoryTypeToString ( AT_ANY_GENRE3 ) );
    cbxDirectoryType->addItem ( DirectoryTypeToString ( AT_GENRE_ROCK ) );
    cbxDirectoryType->addItem ( DirectoryTypeToString ( AT_GENRE_JAZZ ) );
    cbxDirectoryType->addItem ( DirectoryTypeToString ( AT_GENRE_CLASSICAL_FOLK ) );
    cbxDirectoryType->addItem ( DirectoryTypeToString ( AT_GENRE_CHORAL ) );
    cbxDirectoryType->addItem ( DirectoryTypeToString ( AT_CUSTOM ) );

    // server info max lengths
    edtServerName->setMaxLength ( MAX_LEN_SERVER_NAME );
    edtLocationCity->setMaxLength ( MAX_LEN_SERVER_CITY );

    // load country combo box with all available countries
    cbxLocationCountry->setInsertPolicy ( QComboBox::NoInsert );
    cbxLocationCountry->clear();

    for ( int iCurCntry = static_cast<int> ( QLocale::AnyCountry ); iCurCntry < static_cast<int> ( QLocale::LastCountry ); iCurCntry++ )
    {
        // add all countries except of the "Default" country
        if ( static_cast<QLocale::Country> ( iCurCntry ) == QLocale::AnyCountry )
        {
            continue;
        }

        if ( !CLocale::IsCountryCodeSupported ( iCurCntry ) )
        {
            // The current Qt version which is the base for the loop may support
            // more country codes than our protocol does. Therefore, skip
            // the unsupported options to avoid surprises.
            continue;
        }

        // store the country enum index together with the string (this is
        // important since we sort the combo box items later on)
        cbxLocationCountry->addItem ( QLocale::countryToString ( static_cast<QLocale::Country> ( iCurCntry ) ), iCurCntry );
    }

    // sort country combo box items in alphabetical order
    cbxLocationCountry->model()->sort ( 0, Qt::AscendingOrder );

    // setup welcome message GUI control
    tedWelcomeMessage->setPlaceholderText ( tr ( "Type a message here. If no message is set, the server welcome is disabled." ) );

    // language combo box (corrects the setting if language not found)
    cbxLanguage->Init ( pSettings->strLanguage );

    // recorder options
    pbtRecordingDir->setAutoDefault ( false );
    tbtClearRecordingDir->setText ( u8"\u232B" );

    // server list persistence file
    pbtServerListPersistence->setAutoDefault ( false );
    tbtClearServerListPersistence->setText ( u8"\u232B" );

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
        chbDelayPanning->setCheckState ( Qt::Checked );
    }
    else
    {
        chbDelayPanning->setCheckState ( Qt::Unchecked );
    }

    // prepare update check info label (invisible by default)
    lblUpdateCheck->setOpenExternalLinks ( true ); // enables opening a web browser if one clicks on a html link
    lblUpdateCheck->setText ( "<font color=\"red\"><b>" + APP_UPGRADE_AVAILABLE_MSG_TEXT.arg ( APP_NAME ).arg ( VERSION ) + "</b></font>" );
    lblUpdateCheck->hide();

    // update GUI dependencies
    UpdateGUIDependencies();

    UpdateRecorderStatus ( QString() );

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
    QObject::connect ( chbJamRecorder, &QCheckBox::stateChanged, this, &CServerDlg::OnEnableRecorderStateChanged );

    QObject::connect ( chbStartOnOSStart, &QCheckBox::stateChanged, this, &CServerDlg::OnStartOnOSStartStateChanged );

    QObject::connect ( chbDelayPanning, &QCheckBox::stateChanged, this, &CServerDlg::OnEnableDelayPanningStateChanged );

    // line edits
    QObject::connect ( edtServerName, &QLineEdit::editingFinished, this, &CServerDlg::OnServerNameEditingFinished );

    QObject::connect ( edtLocationCity, &QLineEdit::editingFinished, this, &CServerDlg::OnLocationCityEditingFinished );

    QObject::connect ( edtCustomDirectory, &QLineEdit::editingFinished, this, &CServerDlg::OnCustomDirectoryEditingFinished );

    // combo boxes
    QObject::connect ( cbxDirectoryType,
                       static_cast<void ( QComboBox::* ) ( int )> ( &QComboBox::currentIndexChanged ),
                       this,
                       &CServerDlg::OnDirectoryTypeCurrentIndexChanged );

    QObject::connect ( cbxLocationCountry,
                       static_cast<void ( QComboBox::* ) ( int )> ( &QComboBox::currentIndexChanged ),
                       this,
                       &CServerDlg::OnLocationCountryCurrentIndexChanged );

    QObject::connect ( cbxLanguage, &CLanguageComboBox::LanguageChanged, this, &CServerDlg::OnLanguageChanged );

    // push buttons
    QObject::connect ( pbtNewRecording, &QPushButton::released, this, &CServerDlg::OnNewRecordingClicked );

    QObject::connect ( pbtRecordingDir, &QPushButton::released, this, &CServerDlg::OnRecordingDirClicked );

    QObject::connect ( pbtServerListPersistence, &QPushButton::released, this, &CServerDlg::OnServerListPersistenceClicked );

    // tool buttons
    QObject::connect ( tbtClearRecordingDir, &QToolButton::released, this, &CServerDlg::OnClearRecordingDirClicked );

    QObject::connect ( tbtClearServerListPersistence, &QToolButton::released, this, &CServerDlg::OnClearServerListPersistenceClicked );

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

void CServerDlg::OnServerNameEditingFinished()
{
    QString strNewName = edtServerName->text();

    // check length
    if ( strNewName.length() <= MAX_LEN_SERVER_NAME )
    {
        // apply new setting to the server and update it
        pServer->SetServerName ( strNewName );
    }
    else
    {
        // text is too long, update control with shortened text
        edtServerName->setText ( strNewName.left ( MAX_LEN_SERVER_NAME ) );
    }
}

void CServerDlg::OnLocationCityEditingFinished()
{
    QString strNewCity = edtLocationCity->text();

    // check length
    if ( strNewCity.length() <= MAX_LEN_SERVER_CITY )
    {
        // apply new setting to the server and update it
        pServer->SetServerCity ( strNewCity );
    }
    else
    {
        // text is too long, update control with shortened text
        edtLocationCity->setText ( strNewCity.left ( MAX_LEN_SERVER_CITY ) );
    }
}

void CServerDlg::OnCustomDirectoryEditingFinished()
{
    // apply new setting to the server and update it
    pServer->SetDirectoryAddress ( edtCustomDirectory->text() );

    // update GUI dependencies
    UpdateGUIDependencies();
}

void CServerDlg::OnLocationCountryCurrentIndexChanged ( int iCntryListItem )
{
    // apply new setting to the server and update it
    pServer->SetServerCountry ( static_cast<QLocale::Country> ( cbxLocationCountry->itemData ( iCntryListItem ).toInt() ) );
}

void CServerDlg::OnDirectoryTypeCurrentIndexChanged ( int iTypeIdx )
{
    // apply new setting to the server and update it
    pServer->SetDirectoryType ( static_cast<EDirectoryType> ( iTypeIdx - 1 ) );

    // update GUI dependencies
    UpdateGUIDependencies();
}

void CServerDlg::OnServerStarted()
{
    UpdateSystemTrayIcon ( true );
    UpdateRecorderStatus ( QString() );
}

void CServerDlg::OnServerStopped()
{
    UpdateSystemTrayIcon ( false );
    UpdateRecorderStatus ( QString() );
}

void CServerDlg::OnStopRecorder()
{
    UpdateRecorderStatus ( QString() );
    if ( pServer->GetRecorderErrMsg() != QString() )
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
    QString currentValue = pServer->GetRecordingDir();
    QString newRecordingDir =
        QFileDialog::getExistingDirectory ( this, tr ( "Select Main Recording Directory" ), currentValue, QFileDialog::ShowDirsOnly );

    if ( newRecordingDir != currentValue )
    {
        pServer->SetRecordingDir ( newRecordingDir );
        UpdateRecorderStatus ( QString() );
    }
}

void CServerDlg::OnClearRecordingDirClicked()
{
    if ( pServer->GetRecorderErrMsg() != QString() || pServer->GetRecordingDir() != "" )
    {
        pServer->SetRecordingDir ( "" );
        UpdateRecorderStatus ( QString() );
    }
}

void CServerDlg::OnServerListPersistenceClicked()
{
    // get the current value from pServer
    QString     currentValue = pServer->GetServerListFileName();
    QFileDialog fileDialog;
    fileDialog.setAcceptMode ( QFileDialog::AcceptSave );
    fileDialog.setOptions ( QFileDialog::HideNameFilterDetails );
    fileDialog.selectFile ( currentValue );
    if ( fileDialog.exec() && fileDialog.selectedFiles().size() == 1 )
    {
        QString newFileName = fileDialog.selectedFiles().takeFirst();

        if ( newFileName != currentValue )
        {
            pServer->SetServerListFileName ( newFileName );
            UpdateGUIDependencies();
        }
    }
}

void CServerDlg::OnClearServerListPersistenceClicked()
{
    if ( pServer->GetServerListFileName() != "" )
    {
        pServer->SetServerListFileName ( "" );
        UpdateGUIDependencies();
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
    CVector<CHostAddress>     vecHostAddresses;
    CVector<QString>          vecsName;
    CVector<int>              veciJitBufNumFrames;
    CVector<int>              veciNetwFrameSizeFact;
    CVector<CChannelCoreInfo> vecChanInfo;

    ListViewMutex.lock();
    {
        pServer->GetConCliParam ( vecHostAddresses, vecsName, veciJitBufNumFrames, veciNetwFrameSizeFact, vecChanInfo );

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
    const EDirectoryType directoryType = pServer->GetDirectoryType();
    cbxDirectoryType->setCurrentIndex ( static_cast<int> ( directoryType ) + 1 );

    const ESvrRegStatus eSvrRegStatus = pServer->GetSvrRegStatus();
    QString             strStatus     = svrRegStatusToString ( eSvrRegStatus );
    QString             strFontColour = "darkGreen";

    if ( pServer->IsDirectory() )
    {
        strStatus = tr ( "Now a directory" );
    }
    else
    {
        switch ( eSvrRegStatus )
        {
        case SRS_BAD_ADDRESS:
        case SRS_TIME_OUT:
        case SRS_SERVER_LIST_FULL:
        case SRS_VERSION_TOO_OLD:
        case SRS_NOT_FULFILL_REQUIREMENTS:
            strFontColour = "red";
            break;

        default:
            break;
        }
    }

    if ( eSvrRegStatus == SRS_NOT_REGISTERED )
    {
        lblRegSvrStatus->setText ( strStatus );
    }
    else
    {
        lblRegSvrStatus->setText ( "<font color=\"" + strFontColour + "\"><b>" + strStatus + "</b></font>" );
    }

    edtServerName->setText ( pServer->GetServerName() );
    edtLocationCity->setText ( pServer->GetServerCity() );
    cbxLocationCountry->setCurrentIndex ( cbxLocationCountry->findData ( static_cast<int> ( pServer->GetServerCountry() ) ) );

    tedWelcomeMessage->setPlainText ( pServer->GetWelcomeMessage() );

    edtCustomDirectory->setText ( pServer->GetDirectoryAddress() );

    edtServerListPersistence->setText ( pServer->GetServerListFileName() );
}

void CServerDlg::UpdateSystemTrayIcon ( const bool bIsActive )
{
    if ( bSystemTrayIconAvailable )
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
        chbJamRecorder->setEnabled ( true );

        if ( pServer->GetRecordingEnabled() )
        {
            if ( pServer->IsRunning() )
            {
                edtCurrentSessionDir->setText ( sessionDir != QString() ? sessionDir : "" );

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

        if ( strRecordingDir == QString() )
        {
            strRecordingDir = pServer->GetRecordingDir();
        }
        else
        {
            strRecordingDir = tr ( "ERROR" ) + ": " + strRecordingDir;
        }

        chbJamRecorder->setEnabled ( false );
        strRecorderStatus = SREC_NOT_INITIALISED;
    }

    chbJamRecorder->blockSignals ( true );
    chbJamRecorder->setChecked ( strRecorderStatus != SREC_NOT_ENABLED );
    chbJamRecorder->blockSignals ( false );

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
    if ( bSystemTrayIconAvailable && ( pEvent->type() == QEvent::WindowStateChange ) )
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
