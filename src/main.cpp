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

//////////////////////////////////////
//Libraries para la webview
#include <QtCore/QUrl>
#include <QtCore/QCommandLineOption>
#include <QtCore/QCommandLineParser>
#include <QGuiApplication>
#include <QStyleHints>
#include <QScreen>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QtQml/QQmlContext>
#include <QtWebView/QtWebView>
#include <QtWebEngineWidgets/QWebEngineView>

//////////////////////////////////////

#include <QApplication>
#include <QMessageBox>
#include <QDir>
#include <QTextStream>
#include <QTranslator>
#include <QCoreApplication>
#include <QtCore>
#include <QtNetwork/QNetworkReply>
#include <QtNetwork/QNetworkAccessManager>
//////////////////////////////////////
#include "global.h"
#include "clientdlg.h"
#include "serverdlg.h"
#include "settings.h"
#include "testbench.h"
#include "util.h"
#include "initialprogram.h"


//Implementation **************************************************************
int main ( int argc, char** argv )
{
    QTextStream& tsConsole = *( ( new ConsoleWriterFactory() )->get() );
    QString      strArgument;
    double       rDbleArgument;
    // initialize all flags and string which might be changed by command line
    // arguments

#if defined( SERVER_BUNDLE ) && ( defined( __APPLE__ ) || defined( __MACOSX ) )
    // if we are on MacOS and we are building a server bundle, starts Jamulus in server mode
    bool         bIsClient                 = false;
#else
    bool         bIsInicial                = true;
    bool         bIsClient                 = false;
    bool         bIsServer                = false;
#endif

    bool         bUseGUI                   = true;
    bool         bStartMinimized           = false;
    bool         bShowComplRegConnList     = false;
    bool         bDisconnectAllClients     = false;
    bool         bUseDoubleSystemFrameSize = true; // default is 128 samples frame size
    bool         bShowAnalyzerConsole      = false;
    bool         bCentServPingServerInList = false;
    bool         bNoAutoJackConnect        = false;
    int          iNumServerChannels        = DEFAULT_USED_NUM_CHANNELS;
    int          iMaxDaysHistory           = DEFAULT_DAYS_HISTORY;
    int          iCtrlMIDIChannel          = INVALID_MIDI_CH;
    quint16      iPortNumber               = LLCON_DEFAULT_PORT_NUMBER;
    ELicenceType eLicenceType              = LT_NO_LICENCE;
    QString      strConnOnStartupAddress   = "";
    QString      strIniFileName            = "";
    QString      strHTMLStatusFileName     = "";
    QString      strServerName             = "";
    QString      strLoggingFileName        = "";
    QString      strHistoryFileName        = "";
    QString      strRecordingDirName       = "";
    QString      strCentralServer          = "";
    QString      strServerInfo             = "";
    QString      strWelcomeMessage         = "";
    QString      strClientName             = APP_NAME;



    ////////////////////////////////////////////////////////////////////////////////////////
    //Archivos para el guardado y lectura de servidores centrales
    QString savefile = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    QFile file(savefile+"/servidores.sagora");
    file.open(QIODevice::ReadWrite | QIODevice::Text);
    QFile fileIn(savefile+"/servidores.sagora");
    fileIn.open(QIODevice::ReadWrite | QIODevice::Text);
    ////////////////////////////////////////////////////////////////////////////////////////


    // QT docu: argv()[0] is the program name, argv()[1] is the first
    // argument and argv()[argc()-1] is the last argument.
    // Start with first argument, therefore "i = 1"
    for ( int i = 1; i < argc; i++ )
    {

        // Server mode flag ----------------------------------------------------
        if ( GetFlagArgument ( argv,
                               i,
                               "-s",
                               "--server" ) )
        {
            bIsClient = false;
            tsConsole << "- server mode chosen" << endl;
            continue;
        }


        // Use GUI flag --------------------------------------------------------
        if ( GetFlagArgument ( argv,
                               i,
                               "-n",
                               "--nogui" ) )
        {
            bUseGUI = false;
            tsConsole << "- no GUI mode chosen" << endl;
            continue;
        }


        // Use licence flag ----------------------------------------------------
        if ( GetFlagArgument ( argv,
                               i,
                               "-L",
                               "--licence" ) )
        {
            // right now only the creative commons licence is supported
            eLicenceType = LT_CREATIVECOMMONS;
            tsConsole << "- licence required" << endl;
            continue;
        }


        // Use 64 samples frame size mode ----------------------------------------------------
        if ( GetFlagArgument ( argv,
                               i,
                               "-F",
                               "--fastupdate" ) )
        {
            bUseDoubleSystemFrameSize = false; // 64 samples frame size
            tsConsole << "- using " << SYSTEM_FRAME_SIZE_SAMPLES << " samples frame size mode" << endl;
            continue;
        }


        // Maximum number of channels ------------------------------------------
        if ( GetNumericArgument ( tsConsole,
                                  argc,
                                  argv,
                                  i,
                                  "-u",
                                  "--numchannels",
                                  1,
                                  MAX_NUM_CHANNELS,
                                  rDbleArgument ) )
        {
            iNumServerChannels = static_cast<int> ( rDbleArgument );

            tsConsole << "- maximum number of channels: "
                << iNumServerChannels << endl;

            continue;
        }


        // Maximum days in history display -------------------------------------
        if ( GetNumericArgument ( tsConsole,
                                  argc,
                                  argv,
                                  i,
                                  "-D",
                                  "--histdays",
                                  1,
                                  366,
                                  rDbleArgument ) )
        {
            iMaxDaysHistory = static_cast<int> ( rDbleArgument );

            tsConsole << "- maximum days in history display: "
                << iMaxDaysHistory << endl;

            continue;
        }


        // Start minimized -----------------------------------------------------
        if ( GetFlagArgument ( argv,
                               i,
                               "-z",
                               "--startminimized" ) )
        {
            bStartMinimized = true;
            tsConsole << "- start minimized enabled" << endl;
            continue;
        }


        // Ping servers in list for central server -----------------------------
        if ( GetFlagArgument ( argv,
                               i,
                               "-g",
                               "--pingservers" ) )
        {
            bCentServPingServerInList = true;
            tsConsole << "- ping servers in slave server list" << endl;
            continue;
        }


        // Disabling auto Jack connections -------------------------------------
        if ( GetFlagArgument ( argv,
                               i,
                               "-j",
                               "--nojackconnect" ) )
        {
            bNoAutoJackConnect = true;
            tsConsole << "- disable auto Jack connections" << endl;
            continue;
        }


        // Show all registered servers in the server list ----------------------
        // Undocumented debugging command line argument: Show all registered
        // servers in the server list regardless if a ping to the server is
        // possible or not.
        if ( GetFlagArgument ( argv,
                               i,
                               "--showallservers", // no short form
                               "--showallservers" ) )
        {
            bShowComplRegConnList = true;
            tsConsole << "- show all registered servers in server list" << endl;
            continue;
        }


        // Disconnect all clients (emergency mode) -----------------------------
        // Undocumented debugging command line argument: Needed to disconnect
        // an unwanted client.
        if ( GetFlagArgument ( argv,
                               i,
                               "--disconnectall", // no short form
                               "--disconnectall" ) )
        {
            bDisconnectAllClients = true;
            tsConsole << "- disconnect all clients" << endl;
            continue;
        }


        // Show analyzer console -----------------------------------------------
        // Undocumented debugging command line argument: Show the analyzer
        // console to debug network buffer properties.
        if ( GetFlagArgument ( argv,
                               i,
                               "--showanalyzerconsole", // no short form
                               "--showanalyzerconsole" ) )
        {
            bShowAnalyzerConsole = true;
            tsConsole << "- show analyzer console" << endl;
            continue;
        }


        // Controller MIDI channel ---------------------------------------------
        if ( GetNumericArgument ( tsConsole,
                                  argc,
                                  argv,
                                  i,
                                  "--ctrlmidich", // no short form
                                  "--ctrlmidich",
                                  0,
                                  15,
                                  rDbleArgument ) )
        {
            iCtrlMIDIChannel = static_cast<int> ( rDbleArgument );
            tsConsole << "- selected controller MIDI channel: " << iCtrlMIDIChannel << endl;
            continue;
        }


        // Use logging ---------------------------------------------------------
        if ( GetStringArgument ( tsConsole,
                                 argc,
                                 argv,
                                 i,
                                 "-l",
                                 "--log",
                                 strArgument ) )
        {
            strLoggingFileName = strArgument;
            tsConsole << "- logging file name: " << strLoggingFileName << endl;
            continue;
        }


        // Port number ---------------------------------------------------------
        if ( GetNumericArgument ( tsConsole,
                                  argc,
                                  argv,
                                  i,
                                  "-p",
                                  "--port",
                                  0,
                                  65535,
                                  rDbleArgument ) )
        {
            iPortNumber = static_cast<quint16> ( rDbleArgument );
            tsConsole << "- selected port number: " << iPortNumber << endl;
            continue;
        }


        // HTML status file ----------------------------------------------------
        if ( GetStringArgument ( tsConsole,
                                 argc,
                                 argv,
                                 i,
                                 "-m",
                                 "--htmlstatus",
                                 strArgument ) )
        {
            strHTMLStatusFileName = strArgument;
            tsConsole << "- HTML status file name: " << strHTMLStatusFileName << endl;
            continue;
        }

        if ( GetStringArgument ( tsConsole,
                                 argc,
                                 argv,
                                 i,
                                 "-a",
                                 "--servername",
                                 strArgument ) )
        {
            strServerName = strArgument;
            tsConsole << "- server name for HTML status file: " << strServerName << endl;
            continue;
        }


        // Client Name ---------------------------------------------------------
        if ( GetStringArgument ( tsConsole,
                                 argc,
                                 argv,
                                 i,
                                 "--clientname",
                                 "--clientname",
                                 strArgument ) )
        {
            strClientName = QString ( APP_NAME ) + " " + strArgument;
            tsConsole << "- client name: " << strClientName << endl;
            continue;
        }


        // Server history file name --------------------------------------------
        if ( GetStringArgument ( tsConsole,
                                 argc,
                                 argv,
                                 i,
                                 "-y",
                                 "--history",
                                 strArgument ) )
        {
            strHistoryFileName = strArgument;
            tsConsole << "- history file name: " << strHistoryFileName << endl;
            continue;
        }


        // Recording directory -------------------------------------------------
        if ( GetStringArgument ( tsConsole,
                                 argc,
                                 argv,
                                 i,
                                 "-R",
                                 "--recording",
                                 strArgument ) )
        {
            strRecordingDirName = strArgument;
            tsConsole << "- recording directory name: " << strRecordingDirName << endl;
            continue;
        }


        // Central server ------------------------------------------------------
        if ( GetStringArgument ( tsConsole,
                                 argc,
                                 argv,
                                 i,
                                 "-e",
                                 "--centralserver",
                                 strArgument ) )
        {
            strCentralServer = strArgument;
            tsConsole << "- central server: " << strCentralServer << endl;
            continue;
        }


        // Server info ---------------------------------------------------------
        if ( GetStringArgument ( tsConsole,
                                 argc,
                                 argv,
                                 i,
                                 "-o",
                                 "--serverinfo",
                                 strArgument ) )
        {
            strServerInfo = strArgument;
            tsConsole << "- server info: " << strServerInfo << endl;
            continue;
        }


        // Server welcome message ----------------------------------------------
        if ( GetStringArgument ( tsConsole,
                                 argc,
                                 argv,
                                 i,
                                 "-w",
                                 "--welcomemessage",
                                 strArgument ) )
        {
            strWelcomeMessage = strArgument;
            tsConsole << "- welcome message: " << strWelcomeMessage << endl;
            continue;
        }


        // Initialization file -------------------------------------------------
        if ( GetStringArgument ( tsConsole,
                                 argc,
                                 argv,
                                 i,
                                 "-i",
                                 "--inifile",
                                 strArgument ) )
        {
            strIniFileName = strArgument;
            tsConsole << "- initialization file name: " << strIniFileName << endl;
            continue;
        }


        // Connect on startup --------------------------------------------------
        if ( GetStringArgument ( tsConsole,
                                 argc,
                                 argv,
                                 i,
                                 "-c",
                                 "--connect",
                                 strArgument ) )
        {
            strConnOnStartupAddress = strArgument;
            tsConsole << "- connect on startup to address: " << strConnOnStartupAddress << endl;
            continue;
        }


        // Version number ------------------------------------------------------
        if ( ( !strcmp ( argv[i], "--version" ) ) ||
             ( !strcmp ( argv[i], "-v" ) ) )
        {
            tsConsole << CAboutDlg::GetVersionAndNameStr ( false ) << endl;
            exit ( 1 );
        }


        // Help (usage) flag ---------------------------------------------------
        if ( ( !strcmp ( argv[i], "--help" ) ) ||
             ( !strcmp ( argv[i], "-h" ) ) ||
             ( !strcmp ( argv[i], "-?" ) ) )
        {
            const QString strHelp = UsageArguments ( argv );
            tsConsole << strHelp << endl;
            exit ( 1 );
        }


        // Unknown option ------------------------------------------------------
        tsConsole << argv[0] << ": ";
        tsConsole << "Unknown option '" <<
            argv[i] << "' -- use '--help' for help" << endl;

// clicking on the Mac application bundle, the actual application
// is called with weird command line args -> do not exit on these
#if !( defined ( __APPLE__ ) || defined ( __MACOSX ) )
        exit ( 1 );
#endif
    }



    // Dependencies ------------------------------------------------------------
    // per definition: if we are in "GUI" server mode and no central server
    // address is given, we use the default central server address
    if ( !bIsClient && bUseGUI && strCentralServer.isEmpty() )
    {
        strCentralServer = DEFAULT_SERVER_ADDRESS;
    }


    // Application/GUI setup ---------------------------------------------------
    // Application object
    if ( !bUseGUI && !strHistoryFileName.isEmpty() )
    {
        tsConsole << "Qt5 requires a windowing system to paint a JPEG image; image will use SVG" << endl;
    }

//    QCoreApplication* pApp = bUseGUI
//            ? new QApplication ( argc, argv )
//            : new QCoreApplication ( argc, argv );

 QApplication* pApp = new QApplication ( argc, argv );

#ifdef _WIN32
    // set application priority class -> high priority
    SetPriorityClass ( GetCurrentProcess(), HIGH_PRIORITY_CLASS );

    // For accessible support we need to add a plugin to qt. The plugin has to
    // be located in the install directory of the software by the installer.
    // Here, we set the path to our application path.
    QDir ApplDir ( QApplication::applicationDirPath() );
    pApp->addLibraryPath ( QString ( ApplDir.absolutePath() ) );
#endif

    // init resources
    Q_INIT_RESOURCE(resources);


// TODO translation loading does not yet work
//    // load translations
//    if ( bUseGUI )
//    {
//        QTranslator myappTranslator;
//        bool ret = myappTranslator.load ( "src/res/translation_" + QLocale::system().name() );
//qDebug() << "translation successfully loaded: " << ret << "   " << "src/res/translation_" + QLocale::system().name();
//        pApp->installTranslator ( &myappTranslator );
//    }


// TEST -> activate the following line to activate the test bench,
//CTestbench Testbench ( "127.0.0.1", LLCON_DEFAULT_PORT_NUMBER );


    try
    {
        ///////////////////////////////
        //Pantalla inicial de Sagora
        if(bIsInicial){

                  initialprogram w;
                  w.show();
                  w.setWindowTitle("SAGORA");

                  ////////////////////////////////////////////////////////////////////
                  //WEBVIEW para notificaciones y comunicación

                  QtWebView::initialize();
//                    QtWebEngine::initialize();
     //             const QString initialUrl = QStringLiteral("https://sagora.org/notificaciones28365462.html");
                  const QString initialUrl = QStringLiteral("https://sagora.org/notificaciones.html");

                  QQmlApplicationEngine engine;

                  QQmlContext *context = engine.rootContext();
                  context->setContextProperty(QStringLiteral("initialUrl"), initialUrl);

                  QRect geometry = QGuiApplication::primaryScreen()->availableGeometry();

                      const QSize size = geometry.size() * 3/ 6;
                      const QSize offset = (geometry.size() - size) / 2;
                      const QPoint pos = geometry.topLeft() + QPoint(offset.width(), offset.height());
                      geometry = QRect(pos, size);

                  context->setContextProperty(QStringLiteral("initialX"), geometry.x());
                  context->setContextProperty(QStringLiteral("initialY"), geometry.y());
                  context->setContextProperty(QStringLiteral("initialWidth"), geometry.width());
                  context->setContextProperty(QStringLiteral("initialHeight"), geometry.height());

                  engine.load(QUrl(QStringLiteral("qrc:/main.qml")));

                  //engine.connect(mybutton, SIGNAL(mySignal()),
                    //                &myClass, SLOT(cppSlot()));


                  ////////////////////////////////////////////////////////////////////
                  ///////////////////////////////////////////////////////////////////
                  //Este script escribe un archivo actualizando los servidores desde
                  //la pagina de sagora. Esto permite agregar servidores sin necesidad
                  //de reinstalar el programa.
                  QNetworkAccessManager manager;
                  QNetworkRequest request(QUrl("https://sagora.org/servidores.html"));
                  QNetworkReply *reply(manager.get(request));
                  QEventLoop loop;
                  QObject::connect(reply, SIGNAL(finished()), &loop, SLOT(quit()));

                  loop.exec();

                  //qDebug(reply->readAll());

                  QString test = reply->readAll();
                  QStringList ips;
                  QStringList nombreCentralServer;
                  QStringList puertoCentralServer;

                  QJsonDocument document = QJsonDocument::fromJson(test.toUtf8());
                  QJsonArray bolsa = document.array();

                  QTextStream out(&file);

                  for(int i = 0; i < bolsa.size(); i++)
                     {
                         ips << bolsa[i].toObject().value("ip").toString();
                         nombreCentralServer<< bolsa[i].toObject().value("nombre").toString();
                         puertoCentralServer << bolsa[i].toObject().value("puerto").toString();

                         out << ips.at(i) << ", " << nombreCentralServer.at(i) << "," << puertoCentralServer.at(i) << "\n";
                     }

                  QTextStream in(&fileIn);
                  while (!in.atEnd())
                     {
                       QString line = in.readLine();

                       //qInfo() << line;
                     }

                  fileIn.close();
                  file.close();
                  ///////////////////////////////////////////////////////////////////


                  //Ejecución de las pantallas principales
                  pApp->setQuitOnLastWindowClosed(false);
                  pApp->exec();


        }
        if ( bIsClient )
        {
            // Client:
            // actual client object
            CClient Client ( iPortNumber,
                             strConnOnStartupAddress,
                             iCtrlMIDIChannel,
                             bNoAutoJackConnect,
                             strClientName );

            // load settings from init-file
            CSettings Settings ( &Client, strIniFileName );
            Settings.Load();

            if ( bUseGUI )
            {
                // GUI object
                CClientDlg ClientDlg ( &Client,
                                       &Settings,
                                       strConnOnStartupAddress,
                                       bShowComplRegConnList,
                                       bShowAnalyzerConsole,
                                       nullptr,
                                       Qt::Window );

                // show dialog
                ClientDlg.show();
                pApp->exec();
            }
            else
            {
                // only start application without using the GUI
                tsConsole << CAboutDlg::GetVersionAndNameStr ( false ) << endl;

                pApp->exec();
            }
        }
              if ( bIsServer )
        {

            // Server:
            // actual server object
            CServer Server ( iNumServerChannels,
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
                             eLicenceType );
            if ( bUseGUI )
            {
                // load settings from init-file
                CSettings Settings ( &Server, strIniFileName );
                Settings.Load();

                // update server list AFTER restoring the settings from the
                // settings file
                Server.UpdateServerList();

                // GUI object for the server
                CServerDlg ServerDlg ( &Server,
                                       &Settings,
                                       bStartMinimized,
                                       nullptr,
                                       Qt::Window );

                // show dialog (if not the minimized flag is set)
                if ( !bStartMinimized )
                {
                    ServerDlg.show();
                }

                pApp->exec();
            }
            else
            {
                // only start application without using the GUI
                tsConsole << CAboutDlg::GetVersionAndNameStr ( false ) << endl;

                // update serverlist
                Server.UpdateServerList();

                pApp->exec();
            }
        }
    }

    catch ( CGenErr generr )
    {
        // show generic error
        if ( bUseGUI )
        {
            QMessageBox::critical ( nullptr,
                                    APP_NAME,
                                    generr.GetErrorText(),
                                    "Quit",
                                    nullptr );
        }
        else
        {
            tsConsole << generr.GetErrorText() << endl;
        }
    }





    return 0;
}



/******************************************************************************\
* Command Line Argument Parsing                                                *
\******************************************************************************/
QString UsageArguments ( char **argv )
{
    return
        "Usage: " + QString ( argv[0] ) + " [option] [argument]\n"
        "\nRecognized options:\n"
        "  -a, --servername      server name, required for HTML status (server\n"
        "                        only)\n"
        "  -c, --connect         connect to given server address on startup\n"
        "                        (client only)\n"
        "  -e, --centralserver   address of the central server (server only)\n"
        "  -F, --fastupdate      use 64 samples frame size mode (server only)\n"
        "  -g, --pingservers     ping servers in list to keep NAT port open\n"
        "                        (central server only)\n"
        "  -h, -?, --help        display this help text and exit\n"
        "  -i, --inifile         initialization file name\n"
        "  -j, --nojackconnect   disable auto Jack connections (client only)\n"
        "  -l, --log             enable logging, set file name\n"
        "  -L, --licence         a licence must be accepted on a new\n"
        "                        connection (server only)\n"
        "  -m, --htmlstatus      enable HTML status file, set file name (server\n"
        "                        only)\n"
        "  -n, --nogui           disable GUI\n"
        "  -o, --serverinfo      infos of the server(s) in the format:\n"
        "                        [name];[city];[country as QLocale ID]; ...\n"
        "                        [server1 address];[server1 name]; ...\n"
        "                        [server1 city]; ...\n"
        "                        [server1 country as QLocale ID]; ...\n"
        "                        [server2 address]; ... (server only)\n"
        "  -p, --port            local port number (server only)\n"
        "  -R, --recording       enables recording and sets directory to contain\n"
        "                        recorded jams (server only)\n"
        "  -s, --server          start server\n"
        "  -u, --numchannels     maximum number of channels (server only)\n"
        "  -v, --version         output version information and exit\n"
        "  -w, --welcomemessage  welcome message on connect (server only)\n"
        "  -y, --history         enable connection history and set file\n"
        "                        name (server only)\n"
        "  -D, --histdays        number of days of history to display (server only)\n"
        "  -z, --startminimized  start minimizied (server only)\n"
        "  --ctrlmidich          MIDI controller channel to listen (client only)\n"
        "  --clientname          client name (window title and jack client name)\n"
        "\nExample: " + QString ( argv[0] ) + " -l -inifile myinifile.ini\n";
}

bool GetFlagArgument ( char**  argv,
                       int&    i,
                       QString strShortOpt,
                       QString strLongOpt )
{
    if ( ( !strShortOpt.compare ( argv[i] ) ) ||
         ( !strLongOpt.compare ( argv[i] ) ) )
    {
        return true;
    }
    else
    {
        return false;
    }
}

bool GetStringArgument ( QTextStream& tsConsole,
                         int          argc,
                         char**       argv,
                         int&         i,
                         QString      strShortOpt,
                         QString      strLongOpt,
                         QString&     strArg )
{
    if ( ( !strShortOpt.compare ( argv[i] ) ) ||
         ( !strLongOpt.compare ( argv[i] ) ) )
    {
        if ( ++i >= argc )
        {
            tsConsole << argv[0] << ": ";
            tsConsole << "'" << strLongOpt << "' needs a string argument" << endl;
            exit ( 1 );
        }

        strArg = argv[i];

        return true;
    }
    else
    {
        return false;
    }
}

bool GetNumericArgument ( QTextStream& tsConsole,
                          int          argc,
                          char**       argv,
                          int&         i,
                          QString      strShortOpt,
                          QString      strLongOpt,
                          double       rRangeStart,
                          double       rRangeStop,
                          double&      rValue )
{
    if ( ( !strShortOpt.compare ( argv[i] ) ) ||
         ( !strLongOpt.compare ( argv[i] ) ) )
    {
        if ( ++i >= argc )
        {
            tsConsole << argv[0] << ": ";

            tsConsole << "'" <<
                strLongOpt << "' needs a numeric argument between " <<
                rRangeStart << " and " << rRangeStop << endl;

            exit ( 1 );
        }

        char *p;
        rValue = strtod ( argv[i], &p );
        if ( *p ||
             ( rValue < rRangeStart ) ||
             ( rValue > rRangeStop ) )
        {
            tsConsole << argv[0] << ": ";

            tsConsole << "'" <<
                strLongOpt << "' needs a numeric argument between " <<
                rRangeStart << " and " << rRangeStop << endl;

            exit ( 1 );
        }

        return true;
    }
    else
    {
        return false;
    }
}
