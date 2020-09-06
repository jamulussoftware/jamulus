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
#include "initialprogram.h"
#include "clientdlg.h"

/* Implementation *************************************************************/
CServerDlg::CServerDlg(CServer *pNServP,
                       CSettings *pNSetP,
                       const bool bStartMinimized,
                       QWidget *parent,
                       Qt::WindowFlags f)
        : QDialog(parent, f),
          pServer(pNServP),
          pSettings(pNSetP),
          BitmapSystemTrayInactive(QString::fromUtf8(":/png/main/res/ui/logo.png")),
          BitmapSystemTrayActive(QString::fromUtf8(":/png/main/res/ui/logo.png")),
          navegador(new Navegador) {
    setupUi(this);
    navegador->render(this);


    ///////////////////////////////////////////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////////////////////////////////
    //Esta funcion lee el archivo de servidores que se cargo con la info de la API de Servidores Centrales
    QString savefile = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    QStringList linea;

    QFile fileIn(savefile + "/servidores.sagora");
    fileIn.open(QIODevice::ReadWrite | QIODevice::Text);

    QTextStream in(&fileIn);

    while (!in.atEnd()) {
        QString line = in.readLine();
        linea << line;
    }

    // central server address type combo box
    cbxCentServAddrType->clear();

    for (int i = 0; i < linea.size(); i++) {
        QStringList ip;
        ip << linea.at(i).split(",");

        ips << ip.at(0);
        nombreCentralServer << ip.at(1);
        puertoCentralServer << ip.at(2);

        cbxCentServAddrType->addItem(ip.at(1));
    }

    cbxCentServAddrType->setCurrentIndex(static_cast<int> ( 0 ));


    pServer->UpdateServerList();

    pServer->bEnableRecording = false;

    // Add help text to controls -----------------------------------------------
    // client list
    lvwClients->setWhatsThis(tr("<b>Client List:</b> The client list "
                                "shows all clients which are currently connected to this server. Some "
                                "information about the clients like the IP address and name are given "
                                "for each connected client."));

    lvwClients->setAccessibleName(tr("Connected clients list view"));
    chbRegisterServer->hide();
    //lblCentralServerAddress->hide();
    edtCentralServerAddress->hide();


    //cbxCentServAddrType->hide();
    // start minimized on operating system start
    chbStartOnOSStart->setWhatsThis(tr("<b>Start Minimized on Operating "
                                       "System Start:</b> If the start minimized on operating system start "
                                       "check box is checked, the ") + APP_NAME + tr(" server will be "
                                                                                     "started when the operating system starts up and is automatically "
                                                                                     "minimized to a system task bar icon."));

    // CC licence dialog switch
//    chbUseCCLicence->setWhatsThis ( tr ( "<b>Show Creative Commons Licence "
//        "Dialog:</b> If enabled, a Creative Commons BY-NC-SA 4.0 Licence "
//        "dialog is shown each time a new user connects the server." ) );

    // Make My Server Public flag
    chbRegisterServer->setWhatsThis(tr("<b>Make My Server Public:</b> If "
                                       "the Make My Server Public check box is checked, this server registers "
                                       "itself at the central server so that all ") + APP_NAME +
                                    tr(" users can see the server in the connect dialog server list and "
                                       "connect to it. The registering of the server is renewed periodically "
                                       "to make sure that all servers in the connect dialog server list are "
                                       "actually available."));

    //Quito por el momento

    // register server status label
    /* lblRegSvrStatus->setWhatsThis ( tr ( "<b>Register Server Status:</b> If "
         "the Make My Server Public check box is checked, this will show "
         "the success of registration with the central server." ) );*/

    // central server address
    QString strCentrServAddr = tr("<b>Central Server Address:</b> The "
                                  "Central server address is the IP address or URL of the central server "
                                  "at which this server is registered. With the central server address "
                                  "type either the local region can be selected of the default central "
                                  "servers or a manual address can be specified.");

    lblCentralServerAddress->setWhatsThis(strCentrServAddr);
    cbxCentServAddrType->setWhatsThis(strCentrServAddr);
    edtCentralServerAddress->setWhatsThis(strCentrServAddr);

    cbxCentServAddrType->setAccessibleName(tr("Default central server type combo box"));
    edtCentralServerAddress->setAccessibleName(tr("Central server address line edit"));

    // server name
    QString strServName = tr("<b>Server Name:</b> The server name identifies "
                             "your server in the connect dialog server list at the clients. If no "
                             "name is given, the IP address is shown instead.");

    lblServerName->setWhatsThis(strServName);
    edtServerName->setWhatsThis(strServName);

    edtServerName->setAccessibleName(tr("Server name line edit"));

    // location city
    QString strLocCity = tr("<b>Location City:</b> The city in which this "
                            "server is located can be set here. If a city name is entered, it "
                            "will be shown in the connect dialog server list at the clients.");

    lblLocationCity->setWhatsThis(strLocCity);
    edtLocationCity->setWhatsThis(strLocCity);

    edtLocationCity->setAccessibleName(tr(
            "City where the server is located line edit"));

    // location country
    QString strLocCountry = tr("<b>Location country:</b> The country in "
                               "which this server is located can be set here. If a country is "
                               "entered, it will be shown in the connect dialog server list at the "
                               "clients.");

    lblLocationCountry->setWhatsThis(strLocCountry);
    cbxLocationCountry->setWhatsThis(strLocCountry);
    lblLocationCountry->setWhatsThis(strLocCountry);
    cbxLocationCountry->setWhatsThis(strLocCountry);

    cbxLocationCountry->setAccessibleName(tr(
            "Country where the server is located combo box"));


    // check if system tray icon can be used
    bSystemTrayIconAvaialbe = SystemTrayIcon.isSystemTrayAvailable();

    // init system tray icon
    if (bSystemTrayIconAvaialbe) {
        // prepare context menu to be added to the system tray icon
        pSystemTrayIconMenu = new QMenu(this);

        pSystemTrayIconMenu->addAction(tr("E&xit"),
                                       this, SLOT(OnSysTrayMenuExit()));

        pSystemTrayIconMenu->addSeparator();

        pSystemTrayIconMenu->addAction(
                tr("&Hide ") + APP_NAME + tr(" server"),
                this, SLOT(OnSysTrayMenuHide()));

        pSystemTrayIconMenu->setDefaultAction(pSystemTrayIconMenu->addAction(
                tr("&Open ") + APP_NAME + tr(" server"),
                this, SLOT(OnSysTrayMenuOpen())));

        SystemTrayIcon.setContextMenu(pSystemTrayIconMenu);

        // set tool text
        SystemTrayIcon.setToolTip(QString(APP_NAME) + tr(" server"));

        // show icon of state "inactive"
        SystemTrayIcon.setIcon(QIcon(BitmapSystemTrayInactive));
        SystemTrayIcon.show();
    }

    // act on "start minimized" flag (note, this has to be done after setting
    // the correct value for the system tray icon availablility)
    if (bStartMinimized) {
        showMinimized();
    }

    // set text for version and application name
//    lblRecordPath->setText(QString(APP_NAME) +
//                            tr(" server ") + QString(VERSION));


    // set up list view for connected clients
    lvwClients->setColumnWidth(0, 170);
    lvwClients->setColumnWidth(1, 200);
    lvwClients->setStyleSheet("QHeaderView::section { background-color: transparent; }");
    lvwClients->clear();

    chbStartOnOSStart->hide();
// TEST workaround for resize problem of window after iconize in task bar
    lvwClients->setMinimumWidth(170 + 130 + 60 + 205);
    lvwClients->setMinimumHeight(140);


    // insert items in reverse order because in Windows all of them are
    // always visible -> put first item on the top
    vecpListViewItems.Init(MAX_NUM_CHANNELS);

    for (int i = MAX_NUM_CHANNELS - 1; i >= 0; i--) {
        vecpListViewItems[i] = new QTreeWidgetItem(lvwClients);
        vecpListViewItems[i]->setHidden(true);
    }


    // update server name line edit
    edtServerName->setText(pServer->GetServerName());

    // update server city line edit
    edtLocationCity->setText(pServer->GetServerPass());

    // load country combo box with all available countries
    cbxLocationCountry->setInsertPolicy(QComboBox::NoInsert);
    cbxLocationCountry->clear();

    for (int iCurCntry = static_cast<int> ( QLocale::AnyCountry );
         iCurCntry < static_cast<int> ( QLocale::LastCountry ); iCurCntry++) {
        // add all countries except of the "Default" country
        if (static_cast<QLocale::Country> ( iCurCntry ) != QLocale::AnyCountry) {
//             store the country enum index together with the string (this is
//             important since we sort the combo box items later on)
            cbxLocationCountry->addItem(QLocale::countryToString(
                    static_cast<QLocale::Country> ( iCurCntry )), iCurCntry);
        }
    }

//    // sort country combo box items in alphabetical order
    cbxLocationCountry->model()->sort(0, Qt::AscendingOrder);

//     select current country
    cbxLocationCountry->setCurrentIndex(
            cbxLocationCountry->findData(
                    static_cast<int> ( 10 )));

    // update register server check box
    if (pServer->GetServerListEnabled()) {
        chbRegisterServer->setCheckState(Qt::Checked);
    } else {
        chbRegisterServer->setCheckState(Qt::Unchecked);
    }

    cbxLocationCountry->hide();
    lblLocationCountry->hide();
//INICIO REGISTRADO
    OnRegisterServerStateChanged(0);

    // update show Creative Commons licence check box
//    if ( pServer->GetLicenceType() == LT_CREATIVECOMMONS )
//    {
//        chbUseCCLicence->setCheckState ( Qt::Checked );
//    }
//    else
//    {
//        chbUseCCLicence->setCheckState ( Qt::Unchecked );
//    }

    // update start minimized check box (only available for Windows)
#ifdef _WIN32
    chbStartOnOSStart->setVisible ( false );
#else
    const bool bCurAutoStartMinState = false;

    if (bCurAutoStartMinState) {
        chbStartOnOSStart->setCheckState(Qt::Unchecked);
    } else {
        chbStartOnOSStart->setCheckState(Qt::Checked);
    }

    // modify registry according to setting (this is just required in case a
    // user has changed the registry by hand)
    pServer->SetAutoRunMinimized(bCurAutoStartMinState);
    ModifyAutoStartEntry(bCurAutoStartMinState);
#endif

    // update GUI dependencies
    UpdateGUIDependencies();

    // set window title
    setWindowTitle(APP_NAME + tr(" Server"));


    // View menu  --------------------------------------------------------------
//    QMenu *pViewMenu = new QMenu("&Window", this);

//    pViewMenu->addAction(tr("E&xit"), this,
//                         SLOT(close()), QKeySequence(Qt::CTRL + Qt::Key_Q));


    // Main menu bar -----------------------------------------------------------
//    pMenu = new QMenuBar ( this );

//    pMenu->addMenu ( pViewMenu );
//    pMenu->addMenu ( new CHelpMenu ( this ) );

//    // Now tell the layout about the menu
//    layout()->setMenuBar ( pMenu );


    // Connections -------------------------------------------------------------
    // check boxes
    QObject::connect(chbRegisterServer, SIGNAL(stateChanged(int)),
                     this, SLOT(OnRegisterServerStateChanged(int)));


    QObject::connect(RecordCheckBox, SIGNAL(stateChanged(int)),
                     this, SLOT(RecordCheck(int)));

    QObject::connect(chbStartOnOSStart, SIGNAL(stateChanged(int)),
                     this, SLOT(OnStartOnOSStartStateChanged(int)));

//    QObject::connect ( chbUseCCLicence, SIGNAL ( stateChanged ( int ) ),
//        this, SLOT ( OnUseCCLicenceStateChanged ( int ) ) );

    // line edits
    QObject::connect(edtCentralServerAddress, SIGNAL(editingFinished()),
                     this, SLOT(OnCentralServerAddressEditingFinished()));

    QObject::connect(edtServerName, SIGNAL(textChanged(
    const QString& )),
    this, SLOT(OnServerNameTextChanged(
    const QString& )));

    QObject::connect(edtLocationCity, SIGNAL(textChanged(
    const QString& )),
    this, SLOT(OnLocationCityTextChanged(
    const QString& )));

//    // combo boxes
    QObject::connect(cbxLocationCountry, SIGNAL(activated(int)),
                     this, SLOT(OnLocationCountryActivated(int)));

    QObject::connect(cbxCentServAddrType, SIGNAL(activated(int)),
                     this, SLOT(OnCentServAddrTypeActivated(int)));

    QObject::connect(btn_crear_sala, SIGNAL(clicked()),
                     this, SLOT(OnClientClickedServer()));

    // timers
    QObject::connect(&Timer, SIGNAL(timeout()), this, SLOT(OnTimer()));

    // other
    QObject::connect(pServer, SIGNAL(Started()),
                     this, SLOT(OnServerStarted()));

    QObject::connect(pServer, SIGNAL(Stopped()),
                     this, SLOT(OnServerStopped()));

    QObject::connect(pServer, SIGNAL(SvrRegStatusChanged()),
                     this, SLOT(OnSvrRegStatusChanged()));

    QObject::connect(QCoreApplication::instance(), SIGNAL(aboutToQuit()),
                     this, SLOT(OnAboutToQuit()));

    QObject::connect(&SystemTrayIcon,
                     SIGNAL(activated(QSystemTrayIcon::ActivationReason)),
                     this, SLOT(OnSysTrayActivated(QSystemTrayIcon::ActivationReason)));



    // Timers ------------------------------------------------------------------
    // start timer for GUI controls
    Timer.start(GUI_CONTRL_UPDATE_TIME);
}



CServerDlg::~CServerDlg() {
    if(pServer->IsRunning()){
        pServer->Stop();
    }
}

void CServerDlg::OnClientClickedServer() {

    if(edtServerName->text().isEmpty()){
        edtServerName->setStyleSheet(
                                     "background: rgb(15, 15, 15); "
                                     "color:  rgb(255,255,255); "
                                     "selection-background-color: rgb(160,160,160);"
                                     "border: 1px solid red");
        edtServerName->setFocus();
        txt_error_msj->setVisible(true);
        txt_error_msj->setText("Por favor, ingresá un nombre para la sala");
    }else{
        txt_error_msj->setVisible(false);
        edtServerName->setStyleSheet(""
                                     "background: rgb(15, 15, 15); "
                                     "color:  rgb(255,255,255); "
                                     "selection-background-color: rgb(160,160,160);"
                                     "");

        bool bShowComplRegConnList = false;
        bool bShowAnalyzerConsole = false;
        bool bNoAutoJackConnect = false;
        int iCtrlMIDIChannel = INVALID_MIDI_CH;
        quint16 iPortNumber = LLCON_DEFAULT_PORT_NUMBER;
        QString strConnOnStartupAddress = "";
        QString strIniFileName = "";
        QString strHTMLStatusFileName = "";
        QString strServerName = "";
        QString strLoggingFileName = "";
        QString strHistoryFileName = "";
        QString strRecordingDirName = "";
        QString strCentralServer = "";
        QString strServerInfo = "";
        QString strWelcomeMessage = "";
        QString strClientName = APP_NAME;
        bool bClientByServer = true;
        QString bSalaByServer = CHostAddress(NetworkUtil::GetLocalAddress().InetAddr, 22124).toString();;
        QString bPassByServer = edtServerName->text();

        CClient Client(iPortNumber,
                       strConnOnStartupAddress,
                       iCtrlMIDIChannel,
                       bNoAutoJackConnect,
                       strClientName);

        // load settings from init-file
        CSettings Settings(&Client, strIniFileName);
        Settings.Load();
        CClientDlg ClientDlg(&Client,
                             &Settings,
                             strConnOnStartupAddress,
                             bShowComplRegConnList,
                             bShowAnalyzerConsole,
                             nullptr,
                             Qt::Window,
                             bClientByServer,
                             bSalaByServer,
                             bPassByServer);

        // show dialog
    //    this->hide();
        ClientDlg.show();
        ClientDlg.exec();
    }




}


void CServerDlg::closeEvent(QCloseEvent *Event) {
    // if server was registered at the central server, unregister on shutdown
    if (pServer->GetServerListEnabled()) {
        pServer->UnregisterSlaveServer();
    }


    // default implementation of this event handler routine
    Event->accept();
}

void CServerDlg::OnStartOnOSStartStateChanged(int value) {
    const bool bCurAutoStartMinState = (value == Qt::Unchecked);

    // update registry and server setting (for ini file)
    pServer->SetAutoRunMinimized(bCurAutoStartMinState);
    ModifyAutoStartEntry(bCurAutoStartMinState);
}

void CServerDlg::OnUseCCLicenceStateChanged(int value) {
    if (value == Qt::Checked) {
        pServer->SetLicenceType(LT_CREATIVECOMMONS);
    } else {
        pServer->SetLicenceType(LT_NO_LICENCE);
    }
}

void CServerDlg::RecordCheck(int value) {
    if (value == 2) {
        pServer->bEnableRecording = true;

        RecordCheckBox->setText("La transmisión se grabará en : "+QStandardPaths::writableLocation(QStandardPaths::DesktopLocation));
        RecordCheckBox->setStyleSheet("QCheckBox { color: white; background: rgb(13,204,135);"
                                      "}");
    }

    if (value == 0) {
        pServer->bEnableRecording = false;
        pServer->JamRecorder.OnEnd();
        RecordCheckBox->setText("Grabar!");
        RecordCheckBox->setStyleSheet("QCheckBox { color: white; "
                                      "background: transparent"
                                      "}");
    }
}

void CServerDlg::OnRegisterServerStateChanged(int value) {
    const bool bRegState = (value == Qt::Unchecked);

    // apply new setting to the server and update it
    pServer->SetServerListEnabled(bRegState);

    // if registering is disabled, unregister slave server
    if (!bRegState) {
        pServer->UnregisterSlaveServer();
        pServer->UpdateServerList();
        edtServerName->setText("");
        edtLocationCity->setText("");
        QString strStatus = "Recuerde compartir su IP + Puerto y NO colocar contraseña al conectarse como cliente";
        chbRegisterServer->setText(strStatus);
    }
    if (bRegState) {
        chbRegisterServer->setText("Hacer Privada mi Sala de Ensayo ");
    }
    pServer->UpdateServerList();

    // update GUI dependencies
    UpdateGUIDependencies();
}

void CServerDlg::OnCentralServerAddressEditingFinished() {
    // apply new setting to the server and update it
    pServer->SetServerListCentralServerAddress(
            edtCentralServerAddress->text());

    pServer->UpdateServerList();
}

void CServerDlg::OnServerNameTextChanged(const QString &strNewName) {
    // check length
    if (strNewName.length() <= MAX_LEN_SERVER_NAME) {
        // apply new setting to the server and update it
        pServer->SetServerName(strNewName);
        pServer->UpdateServerList();
    } else {
        // text is too long, update control with shortend text
        edtServerName->setText(strNewName.left(MAX_LEN_SERVER_NAME));
    }
}

void CServerDlg::OnLocationCityTextChanged(const QString &strNewCity) {
    // check length
    if (strNewCity.length() <= MAX_LEN_SERVER_CITY) {
        // apply new setting to the server and update it
        pServer->SetServerCity(strNewCity);
        pServer->UpdateServerList();
    } else {
        // text is too long, update control with shortend text
        edtLocationCity->setText(strNewCity.left(MAX_LEN_SERVER_CITY));
    }
}

void CServerDlg::OnLocationCountryActivated(int iCntryListItem) {
    // apply new setting to the server and update it
    pServer->SetServerCountry(static_cast<QLocale::Country> (
                                      cbxLocationCountry->itemData(iCntryListItem).toInt())
    );

    pServer->UpdateServerList();
}

void CServerDlg::OnCentServAddrTypeActivated(int iTypeIdx) {
    // if server was registered, unregister first
    if (pServer->GetServerListEnabled()) {
        pServer->UnregisterSlaveServer();
    }

    // apply new setting to the server and update it
    pServer->SetCentralServerAddressType(static_cast<ECSAddType> ( iTypeIdx ));
    pServer->UpdateServerList();

    // update GUI dependencies
    UpdateGUIDependencies();
}

void CServerDlg::OnSysTrayActivated(QSystemTrayIcon::ActivationReason ActReason) {
    // on double click on the icon, show window in fore ground
    if (ActReason == QSystemTrayIcon::DoubleClick) {
        ShowWindowInForeground();
    }
}

void CServerDlg::OnTimer() {
    CVector < CHostAddress > vecHostAddresses;
    CVector < QString > vecsName;
    CVector<int> veciJitBufNumFrames;
    CVector<int> veciNetwFrameSizeFact;

    ListViewMutex.lock();
    {
        pServer->GetConCliParam(vecHostAddresses,
                                vecsName,
                                veciJitBufNumFrames,
                                veciNetwFrameSizeFact);

        // we assume that all vectors have the same length
        const int iNumChannels = vecHostAddresses.Size();

        // fill list with connected clients
        for (int i = 0; i < iNumChannels; i++) {
            if (!(vecHostAddresses[i].InetAddr == QHostAddress(static_cast<quint32> ( 0 )))) {
                // IP, port number
                vecpListViewItems[i]->setText(0,
                                              vecHostAddresses[i].toString(CHostAddress::SM_IP_PORT));

                // name
                vecpListViewItems[i]->setText(1, vecsName[i]);

                // jitter buffer size (polling for updates)
                vecpListViewItems[i]->setText(2,
                                              QString().setNum(veciJitBufNumFrames[i]));

                vecpListViewItems[i]->setHidden(false);
            } else {
                vecpListViewItems[i]->setHidden(true);
            }
        }
    }
    ListViewMutex.unlock();
}

void CServerDlg::UpdateGUIDependencies() {
    // get the states which define the GUI dependencies from the server
    const bool bCurSerListEnabled = pServer->GetServerListEnabled();

    const bool bCurUseDefCentServAddr = (pServer->GetCentralServerAddressType() != AT_DEFAULT);

    const ESvrRegStatus eSvrRegStatus = pServer->GetSvrRegStatus();

    // if register server is not enabled, we disable all the configuration
    // controls for the server list
    cbxCentServAddrType->setEnabled(bCurSerListEnabled);
    grbServerInfo->setEnabled(bCurSerListEnabled);

    // make sure the line edit does not fire signals when we update the text
    edtCentralServerAddress->blockSignals(true);
    {
        if (bCurUseDefCentServAddr) {
            // if the default central server is used, just show a text of the
            // server name
            edtCentralServerAddress->setText(DEFAULT_SERVER_NAME);
        } else {
            // show the current user defined server address
            edtCentralServerAddress->setText(
                    pServer->GetServerListCentralServerAddress());
        }
    }
    edtCentralServerAddress->blockSignals(false);

    // the line edit of the central server address is only enabled, if the
    // server list is enabled and not the default address is used
    edtCentralServerAddress->setEnabled(
            !bCurUseDefCentServAddr && bCurSerListEnabled);

    QString strStatus = svrRegStatusToString(eSvrRegStatus);

    txt_error_msj->setVisible(false);
    txt_error_msj->setStyleSheet("color:red;"
                                 "background-color:transparent;");

//    switch ( eSvrRegStatus )
//    {
//    case SRS_BAD_ADDRESS:
//    case SRS_TIME_OUT:
//    case SRS_CENTRAL_SVR_FULL:
//        strStatus = "<font color=""red""><b>" + strStatus + "</b></font>";
//        break;

//    case SRS_REGISTERED:
//        strStatus = "<font color=""blue""><b>" + strStatus + "</b></font>";
//        break;

//    default:
//        break;
//    }

//    lblRegSvrStatus->setText ( strStatus );
}

void CServerDlg::UpdateSystemTrayIcon(const bool bIsActive) {
    if (bSystemTrayIconAvaialbe) {
        if (bIsActive) {
            SystemTrayIcon.setIcon(QIcon(BitmapSystemTrayActive));
        } else {
            SystemTrayIcon.setIcon(QIcon(BitmapSystemTrayInactive));
        }
    }
}

void CServerDlg::ModifyAutoStartEntry(const bool bDoAutoStart) {
// auto start is currently only supported for Windows
#ifdef _WIN32
    // init settings object so that it points to the correct place in the
    // Windows registry for the auto run entry
    QSettings RegSettings ( "HKEY_CURRENT_USER\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run",
                            QSettings::NativeFormat );

    // create start string of auto run entry
    QString strRegValue =
        QCoreApplication::applicationFilePath().replace ( "/", "\\" );
#endif

    if (bDoAutoStart) {
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
    } else {
#ifdef _WIN32
        // delete reg key if present
        if ( RegSettings.contains ( AUTORUN_SERVER_REG_NAME ) )
        {
            RegSettings.remove ( AUTORUN_SERVER_REG_NAME );
        }
#endif
    }
}

void CServerDlg::changeEvent(QEvent *pEvent) {
    // if we have a system tray icon, we make the window invisible if it is
    // minimized
    if (bSystemTrayIconAvaialbe &&
        (pEvent->type() == QEvent::WindowStateChange)) {
        if (isMinimized()) {
            // we have to call the hide function from another thread -> use
            // the timer for this purpose
//            QTimer::singleShot(5, this, SLOT(hide()));
        }
    }
}

void CServerDlg::mousePressEvent(QMouseEvent *event) {
    m_nMouseClick_X_Coordinate = event->QMouseEvent::x();
    m_nMouseClick_Y_Coordinate = event->QMouseEvent::y();
}

void CServerDlg::mouseMoveEvent(QMouseEvent *event) {
    move(event->globalX() - m_nMouseClick_X_Coordinate, event->QMouseEvent::globalY() - m_nMouseClick_Y_Coordinate);
}


