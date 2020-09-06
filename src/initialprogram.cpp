#include <QApplication>
#include <QMessageBox>
#include <QDir>
#include <QTextStream>
#include <QTranslator>

#include "initialprogram.h"
#include "ui_initialprogram.h"
#include "global.h"
#include "clientdlg.h"
#include "serverdlg.h"
#include "settings.h"
#include "testbench.h"
#include "util.h"
#include <QDebug>
#include <QtWidgets>
#include <QtConcurrent>
#include <functional>
#include "navegador.h"


using namespace QtConcurrent;

bool ServerRunState = false;

initialprogram::initialprogram(QWidget *parent) :
        QWidget(parent),
        ui(new Ui::initialprogram),
        navegador(new Navegador) {
    // Setea los elementos iniciales de la UI
    ui->setupUi(this);

    // Setea y renderiza los botones de navegacion
    // formatea la ventana
    navegador->render(this);
}


initialprogram::~initialprogram() {
    delete ui;
}

void initialprogram::receiveFromQml() {
    //  qInfo() << "Hola";

}


void initialprogram::on_client_clicked() {
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
    bool bClientByServer = false;
    QString bSalaByServer;
    QString bPassByServer = "";


    iPortNumber += 10; // increment by 10

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

void initialprogram::on_server_clicked() {


    if (ServerRunState) {


    } else {
        ServerRunState = true;


        //
        const int iterations = 10;
        // Prepare the vector.
        QVector<int> vector;
        for (int i = 0; i < iterations; ++i)
            vector.append(i);
        // Create a progress dialog.
        QProgressDialog dialog;
        dialog.setLabelText(QString("Creando Servidor SAGORA ...").arg(QThread::idealThreadCount()));

        // Create a QFutureWatcher and connect signals and slots.
        QFutureWatcher<void> futureWatcher;
        QObject::connect(&futureWatcher, &QFutureWatcher<void>::finished, &dialog, &QProgressDialog::reset);
        QObject::connect(&dialog, &QProgressDialog::canceled, &futureWatcher, &QFutureWatcher<void>::cancel);
        QObject::connect(&futureWatcher, &QFutureWatcher<void>::progressRangeChanged, &dialog,
                         &QProgressDialog::setRange);
        QObject::connect(&futureWatcher, &QFutureWatcher<void>::progressValueChanged, &dialog,
                         &QProgressDialog::setValue);

        // Our function to compute
        std::function<void(int &)> spin = [](int &iteration) {
            const int work = 1000 * 1000 * 60;
            volatile int v = 0;
            for (int j = 0; j < work; ++j)
                ++v;

            //qDebug() << "iteration" << iteration << "in thread" << QThread::currentThreadId();
        };

        // Start the computation.
        futureWatcher.setFuture(QtConcurrent::map(vector, spin));

        // Display the dialog and start the event loop.
        dialog.exec();

        futureWatcher.waitForFinished();

        // Query the future to check if was canceled.
        qDebug() << "Canceled?" << futureWatcher.future().isCanceled();

#ifdef _WIN32
        // set application priority class -> high priority
        SetPriorityClass ( GetCurrentProcess(), HIGH_PRIORITY_CLASS );

        // For accessible support we need to add a plugin to qt. The plugin has to
        // be located in the install directory of the software by the installer.
        // Here, we set the path to our application path.
        QDir ApplDir ( QApplication::applicationDirPath() );
        pApp->addLibraryPath ( QString ( ApplDir.absolutePath() ) );
#endif

        bool bStartMinimized = false;
        bool bDisconnectAllClients = false;
        bool bUseDoubleSystemFrameSize = true; // default is 128 samples frame size
        bool bCentServPingServerInList = false;
        int iNumServerChannels = DEFAULT_USED_NUM_CHANNELS;
        int iMaxDaysHistory = DEFAULT_DAYS_HISTORY;
        quint16 iPortNumber = LLCON_DEFAULT_PORT_NUMBER;
        ELicenceType eLicenceType = LT_NO_LICENCE;
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




        /////////////////////////////////////////////////////////////////////////////////
        //QString savefile = QDir::currentPath().split("Sagora.app")[0]+"Sagora-Session";

        //QString savefile = QDir::currentPath();
        //QString savefile = QStandardPaths::DesktopLocation;
        QString savefile = QStandardPaths::writableLocation(QStandardPaths::DesktopLocation);
        strRecordingDirName = savefile;
        qInfo() << QStandardPaths::writableLocation(QStandardPaths::DesktopLocation);

        //QFile(savefile).setPermissions(QFileDevice::ReadUser | QFileDevice::WriteUser);
        /////////////////////////////////////////////////////////////////////////////////

        CServer Server(iNumServerChannels,
                       iMaxDaysHistory,
                       strLoggingFileName,
                       iPortNumber,
                       strHTMLStatusFileName,
                       strHistoryFileName,
                       strServerName,
                       strCentralServer,
                       strServerInfo,
                       strWelcomeMessage,
                       strRecordingDirName,
                       bCentServPingServerInList,
                       bDisconnectAllClients,
                       bUseDoubleSystemFrameSize,
                       eLicenceType);

        CSettings Settings(&Server, strIniFileName);
        Settings.Load();

        // update server list AFTER restoring the settings from the
        // settings file
        Server.UpdateServerList();

        // GUI object for the server
        CServerDlg ServerDlg(&Server,
                             &Settings,
                             bStartMinimized,
                             nullptr,
                             Qt::Window);

        ServerDlg.show();
        ServerDlg.exec();
    }
    ServerRunState = false;

}


void initialprogram::mousePressEvent(QMouseEvent *event) {
    m_nMouseClick_X_Coordinate = event->QMouseEvent::x();
    m_nMouseClick_Y_Coordinate = event->QMouseEvent::y();
}

void initialprogram::mouseMoveEvent(QMouseEvent *event) {
    move(event->globalX() - m_nMouseClick_X_Coordinate, event->QMouseEvent::globalY() - m_nMouseClick_Y_Coordinate);
}

