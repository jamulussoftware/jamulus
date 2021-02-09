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

#include <QCoreApplication>
#include <QDir>
#include "global.h"
#ifndef HEADLESS
# include <QApplication>
# include <QMessageBox>
# include "clientdlg.h"
# include "serverdlg.h"
#endif
#include "settings.h"
#include "testbench.h"
#include "util.h"
#ifdef ANDROID
# include <QtAndroidExtras/QtAndroid>
#endif
#if defined ( Q_OS_MACX )
# include "mac/activity.h"
#endif


// Implementation **************************************************************

int main ( int argc, char** argv )
{

    QString        strArgument;
    double         rDbleArgument;
    QList<QString> CommandLineOptions;

    // initialize all flags and string which might be changed by command line
    // arguments
#if defined( SERVER_BUNDLE ) && ( defined( Q_OS_MACX ) )
    // if we are on MacOS and we are building a server bundle, starts Jamulus in server mode
    bool         bIsClient                   = false;
#else
    bool         bIsClient                   = true;
#endif
    bool         bUseGUI                     = true;
    bool         bStartMinimized             = false;
    bool         bShowComplRegConnList       = false;
    bool         bDisconnectAllClientsOnQuit = false;
    bool         bUseDoubleSystemFrameSize   = true; // default is 128 samples frame size
    bool         bUseMultithreading          = false;
    bool         bShowAnalyzerConsole        = false;
    bool         bMuteStream                 = false;
    bool         bMuteMeInPersonalMix        = false;
    bool         bDisableRecording           = false;
    bool         bNoAutoJackConnect          = false;
    bool         bUseTranslation             = true;
    bool         bCustomPortNumberGiven      = false;
    bool         bLogIP                      = false;
    bool         bEduModeEnabled             = false;
    int          iNumServerChannels          = DEFAULT_USED_NUM_CHANNELS;
    quint16      iPortNumber                 = DEFAULT_PORT_NUMBER;
    ELicenceType eLicenceType                = LT_NO_LICENCE;
    QString      strMIDISetup                = "";
    QString      strConnOnStartupAddress     = "";
    QString      strIniFileName              = "";
    QString      strHTMLStatusFileName       = "";
    QString      strLoggingFileName          = "";
    QString      strRecordingDirName         = "";
    QString      strCentralServer            = "";
    QString      strServerInfo               = "";
    QString      strServerPublicIP           = "";
    QString      strServerListFilter         = "";
    QString      strEduModePassword          = "";
    QString      strWelcomeMessage           = "";
    QString      strClientName               = "";

#if !defined(HEADLESS) && defined(_WIN32)
    if (AttachConsole(ATTACH_PARENT_PROCESS)) {
        freopen("CONOUT$", "w", stdout);
        freopen("CONOUT$", "w", stderr);
    }
#endif

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
            qInfo() << "- server mode chosen";
            CommandLineOptions << "--server";
            continue;
        }


        // Use GUI flag --------------------------------------------------------
        if ( GetFlagArgument ( argv,
                               i,
                               "-n",
                               "--nogui" ) )
        {
            bUseGUI = false;
            qInfo() << "- no GUI mode chosen";
            CommandLineOptions << "--nogui";
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
            qInfo() << "- licence required";
            CommandLineOptions << "--licence";
            continue;
        }


        // Log whole IP-Adresses on server ----------------------------------------------------
        if ( GetFlagArgument ( argv,
                               i,
                               "--logip",
                               "--logip" ) )
        {
            bLogIP = true;
            qInfo() << qUtf8Printable( QString( "- Logging full IP adresses." ) );
            CommandLineOptions << "--logip";
            continue;
        }


        // Use 64 samples frame size mode --------------------------------------
        if ( GetFlagArgument ( argv,
                               i,
                               "-F",
                               "--fastupdate" ) )
        {
            bUseDoubleSystemFrameSize = false; // 64 samples frame size
            qInfo() << qUtf8Printable( QString( "- using %1 samples frame size mode" )
                .arg( SYSTEM_FRAME_SIZE_SAMPLES ) );
            CommandLineOptions << "--fastupdate";
            continue;
        }


        // Use multithreading --------------------------------------------------
        if ( GetFlagArgument ( argv,
                               i,
                               "-T",
                               "--multithreading" ) )
        {
            bUseMultithreading = true;
            qInfo() << "- using multithreading";
            CommandLineOptions << "--multithreading";
            continue;
        }


        // Maximum number of channels ------------------------------------------
        if ( GetNumericArgument ( argc,
                                  argv,
                                  i,
                                  "-u",
                                  "--numchannels",
                                  1,
                                  MAX_NUM_CHANNELS,
                                  rDbleArgument ) )
        {
            iNumServerChannels = static_cast<int> ( rDbleArgument );

            qInfo() << qUtf8Printable( QString("- maximum number of channels: %1")
                .arg( iNumServerChannels ) );

            CommandLineOptions << "--numchannels";
            continue;
        }


        // Start minimized -----------------------------------------------------
        if ( GetFlagArgument ( argv,
                               i,
                               "-z",
                               "--startminimized" ) )
        {
            bStartMinimized = true;
            qInfo() << "- start minimized enabled";
            CommandLineOptions << "--startminimized";
            continue;
        }


        // Disconnect all clients on quit --------------------------------------
        if ( GetFlagArgument ( argv,
                               i,
                               "-d",
                               "--discononquit" ) )
        {
            bDisconnectAllClientsOnQuit = true;
            qInfo() << "- disconnect all clients on quit";
            CommandLineOptions << "--discononquit";
            continue;
        }


        // Disabling auto Jack connections -------------------------------------
        if ( GetFlagArgument ( argv,
                               i,
                               "-j",
                               "--nojackconnect" ) )
        {
            bNoAutoJackConnect = true;
            qInfo() << "- disable auto Jack connections";
            CommandLineOptions << "--nojackconnect";
            continue;
        }


        // Disable translations ------------------------------------------------
        if ( GetFlagArgument ( argv,
                               i,
                               "-t",
                               "--notranslation" ) )
        {
            bUseTranslation = false;
            qInfo() << "- translations disabled";
            CommandLineOptions << "--notranslation";
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
            qInfo() << "- show all registered servers in server list";
            CommandLineOptions << "--showallservers";
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
            qInfo() << "- show analyzer console";
            CommandLineOptions << "--showanalyzerconsole";
            continue;
        }


        // Controller MIDI channel ---------------------------------------------
        if ( GetStringArgument ( argc,
                                 argv,
                                 i,
                                 "--ctrlmidich", // no short form
                                 "--ctrlmidich",
                                 strArgument ) )
        {
            strMIDISetup = strArgument;
            qInfo() << qUtf8Printable( QString( "- MIDI controller settings: %1" )
                .arg( strMIDISetup ) );
            CommandLineOptions << "--ctrlmidich";
            continue;
        }


        // Use logging ---------------------------------------------------------
        if ( GetStringArgument ( argc,
                                 argv,
                                 i,
                                 "-l",
                                 "--log",
                                 strArgument ) )
        {
            strLoggingFileName = strArgument;
            qInfo() << qUtf8Printable( QString( "- logging file name: %1" )
                .arg( strLoggingFileName ) );
            CommandLineOptions << "--log";
            continue;
        }


        // Port number ---------------------------------------------------------
        if ( GetNumericArgument ( argc,
                                  argv,
                                  i,
                                  "-p",
                                  "--port",
                                  0,
                                  65535,
                                  rDbleArgument ) )
        {
            iPortNumber            = static_cast<quint16> ( rDbleArgument );
            bCustomPortNumberGiven = true;
            qInfo() << qUtf8Printable( QString( "- selected port number: %1" )
                .arg( iPortNumber ) );
            CommandLineOptions << "--port";
            continue;
        }


        // HTML status file ----------------------------------------------------
        if ( GetStringArgument ( argc,
                                 argv,
                                 i,
                                 "-m",
                                 "--htmlstatus",
                                 strArgument ) )
        {
            strHTMLStatusFileName = strArgument;
            qInfo() << qUtf8Printable( QString( "- HTML status file name: %1" )
                .arg( strHTMLStatusFileName ) );
            CommandLineOptions << "--htmlstatus";
            continue;
        }


        // Client Name ---------------------------------------------------------
        if ( GetStringArgument ( argc,
                                 argv,
                                 i,
                                 "--clientname", // no short form
                                 "--clientname",
                                 strArgument ) )
        {
            strClientName = strArgument;
            qInfo() << qUtf8Printable( QString( "- client name: %1" )
                .arg( strClientName ) );
            CommandLineOptions << "--clientname";
            continue;
        }


        // Recording directory -------------------------------------------------
        if ( GetStringArgument ( argc,
                                 argv,
                                 i,
                                 "-R",
                                 "--recording",
                                 strArgument ) )
        {
            strRecordingDirName = strArgument;
            qInfo() << qUtf8Printable( QString("- recording directory name: %1" )
                .arg( strRecordingDirName ) );
            CommandLineOptions << "--recording";
            continue;
        }


        // Disable recording on startup ----------------------------------------
        if ( GetFlagArgument ( argv,
                               i,
                               "--norecord", // no short form
                               "--norecord" ) )
        {
            bDisableRecording = true;
            qInfo() << "- recording will not be enabled";
            CommandLineOptions << "--norecord";
            continue;
        }


        // Central server ------------------------------------------------------
        if ( GetStringArgument ( argc,
                                 argv,
                                 i,
                                 "-e",
                                 "--centralserver",
                                 strArgument ) )
        {
            strCentralServer = strArgument;
            qInfo() << qUtf8Printable( QString( "- central server: %1" )
                .arg( strCentralServer ) );
            CommandLineOptions << "--centralserver";
            continue;
        }


        // Server Public IP --------------------------------------------------
        if ( GetStringArgument ( argc,
                                 argv,
                                 i,
                                 "--serverpublicip", // no short form
                                 "--serverpublicip",
                                 strArgument ) )
        {
            strServerPublicIP = strArgument;
            CommandLineOptions << "--serverpublicip";
            continue;
        }


        // Server info ---------------------------------------------------------
        if ( GetStringArgument ( argc,
                                 argv,
                                 i,
                                 "-o",
                                 "--serverinfo",
                                 strArgument ) )
        {
            strServerInfo = strArgument;
            qInfo() << qUtf8Printable( QString( "- server info: %1" )
                .arg( strServerInfo ) );
            CommandLineOptions << "--serverinfo";
            continue;
        }


        // Server list filter --------------------------------------------------
        if ( GetStringArgument ( argc,
                                 argv,
                                 i,
                                 "-f",
                                 "--listfilter",
                                 strArgument ) )
        {
            strServerListFilter = strArgument;
            qInfo() << qUtf8Printable( QString( "- server list filter: %1" )
                .arg( strServerListFilter ) );
            CommandLineOptions << "--listfilter";
            continue;
        }

        // Education mode password ---------------------------------------------
        if ( GetStringArgument ( argc,
                                 argv,
                                 i,
                                 "--edumodepassword", // no short argument
                                 "--edumodepassword",
                                 strArgument ) )
        {
            strEduModePassword = strArgument;
            bEduModeEnabled    = true;
            qInfo() << qUtf8Printable( QString( "- enabled Edu-Mode with password " ) );
            CommandLineOptions << "--edumodepassword";
            continue;
        }

        // Server welcome message ----------------------------------------------
        if ( GetStringArgument ( argc,
                                 argv,
                                 i,
                                 "-w",
                                 "--welcomemessage",
                                 strArgument ) )
        {
            strWelcomeMessage = strArgument;
            qInfo() << qUtf8Printable( QString( "- welcome message: %1" )
                .arg( strWelcomeMessage ) );
            CommandLineOptions << "--welcomemessage";
            continue;
        }


        // Initialization file -------------------------------------------------
        if ( GetStringArgument ( argc,
                                 argv,
                                 i,
                                 "-i",
                                 "--inifile",
                                 strArgument ) )
        {
            strIniFileName = strArgument;
            qInfo() << qUtf8Printable( QString( "- initialization file name: %1" )
                .arg( strIniFileName ) );
            CommandLineOptions << "--inifile";
            continue;
        }


        // Connect on startup --------------------------------------------------
        if ( GetStringArgument ( argc,
                                 argv,
                                 i,
                                 "-c",
                                 "--connect",
                                 strArgument ) )
        {
            strConnOnStartupAddress = NetworkUtil::FixAddress ( strArgument );
            qInfo() << qUtf8Printable( QString( "- connect on startup to address: %1" )
                .arg( strConnOnStartupAddress ) );
            CommandLineOptions << "--connect";
            continue;
        }


        // Mute stream on startup ----------------------------------------------
        if ( GetFlagArgument ( argv,
                               i,
                               "-M",
                               "--mutestream" ) )
        {
            bMuteStream = true;
            qInfo() << "- mute stream activated";
            CommandLineOptions << "--mutestream";
            continue;
        }


        // For headless client mute my own signal in personal mix --------------
        if ( GetFlagArgument ( argv,
                               i,
                               "--mutemyown", // no short form
                               "--mutemyown" ) )
        {
            bMuteMeInPersonalMix = true;
            qInfo() << "- mute me in my personal mix";
            CommandLineOptions << "--mutemyown";
            continue;
        }


        // Version number ------------------------------------------------------
        if ( ( !strcmp ( argv[i], "--version" ) ) ||
             ( !strcmp ( argv[i], "-v" ) ) )
        {
            qCritical() << qUtf8Printable( GetVersionAndNameStr ( false ) );
            exit ( 1 );
        }


        // Help (usage) flag ---------------------------------------------------
        if ( ( !strcmp ( argv[i], "--help" ) ) ||
             ( !strcmp ( argv[i], "-h" ) ) ||
             ( !strcmp ( argv[i], "-?" ) ) )
        {
            const QString strHelp = UsageArguments ( argv );
            qInfo() << qUtf8Printable( strHelp );
            exit ( 0 );
        }


        // Unknown option ------------------------------------------------------
        qCritical() << qUtf8Printable( QString( "%1: Unknown option '%2' -- use '--help' for help" )
            .arg( argv[0] ).arg( argv[i] ) );

// clicking on the Mac application bundle, the actual application
// is called with weird command line args -> do not exit on these
#if !( defined ( Q_OS_MACX ) )
        exit ( 1 );
#endif
    }


    // Dependencies ------------------------------------------------------------
#ifdef HEADLESS
    if ( bUseGUI )
    {
        bUseGUI = false;
        qWarning() << "No GUI support compiled. Running in headless mode.";
    }
    Q_UNUSED ( bStartMinimized )       // avoid compiler warnings
    Q_UNUSED ( bShowComplRegConnList ) // avoid compiler warnings
    Q_UNUSED ( bShowAnalyzerConsole )  // avoid compiler warnings
    Q_UNUSED ( bMuteStream )           // avoid compiler warnings
#endif

    // the inifile is not supported for the headless server mode
    if ( !bIsClient && !bUseGUI && !strIniFileName.isEmpty() )
    {
        qWarning() << "No initialization file support in headless server mode.";
    }

    // mute my own signal in personal mix is only supported for headless mode
    if ( bIsClient && bUseGUI && bMuteMeInPersonalMix )
    {
        bMuteMeInPersonalMix = false;
        qWarning() << "Mute my own signal in my personal mix is only supported in headless mode.";
    }

    // per definition: if we are in "GUI" server mode and no central server
    // address is given, we use the default central server address
    if ( !bIsClient && bUseGUI && strCentralServer.isEmpty() )
    {
        strCentralServer = DEFAULT_SERVER_ADDRESS;
    }

    // adjust default port number for client: use different default port than the server since
    // if the client is started before the server, the server would get a socket bind error
    if ( bIsClient && !bCustomPortNumberGiven )
    {
        iPortNumber += 10; // increment by 10
    }


    // Application/GUI setup ---------------------------------------------------
    // Application object
#ifdef HEADLESS
    QCoreApplication* pApp = new QCoreApplication ( argc, argv );
#else
# if defined ( Q_OS_IOS )
    bIsClient = false;
    bUseGUI = true;

    // bUseMultithreading = true;
    QApplication* pApp =  new QApplication ( argc, argv );
# else
    QCoreApplication* pApp = bUseGUI
        ? new QApplication ( argc, argv )
        : new QCoreApplication ( argc, argv );
# endif
#endif

#ifdef ANDROID
    // special Android coded needed for record audio permission handling
    auto result = QtAndroid::checkPermission ( QString ( "android.permission.RECORD_AUDIO" ) );

    if ( result == QtAndroid::PermissionResult::Denied )
    {
        QtAndroid::PermissionResultMap resultHash = QtAndroid::requestPermissionsSync ( QStringList ( { "android.permission.RECORD_AUDIO" } ) );

        if ( resultHash["android.permission.RECORD_AUDIO"] == QtAndroid::PermissionResult::Denied )
        {
            return 0;
        }
    }
#endif

#ifdef _WIN32
    // set application priority class -> high priority
    SetPriorityClass ( GetCurrentProcess(), HIGH_PRIORITY_CLASS );

    // For accessible support we need to add a plugin to qt. The plugin has to
    // be located in the install directory of the software by the installer.
    // Here, we set the path to our application path.
    QDir ApplDir ( QApplication::applicationDirPath() );
    pApp->addLibraryPath ( QString ( ApplDir.absolutePath() ) );
#endif

#if defined ( Q_OS_MACX )
    // On OSX we need to declare an activity to ensure the process doesn't get
    // throttled by OS level Nap, Sleep, and Thread Priority systems.
    CActivity activity;

    activity.BeginActivity();
#endif

    // init resources
    Q_INIT_RESOURCE(resources);


// TEST -> activate the following line to activate the test bench,
//CTestbench Testbench ( "127.0.0.1", DEFAULT_PORT_NUMBER );


    try
    {
        if ( bIsClient )
        {
            // Client:
            // actual client object
            CClient Client ( iPortNumber,
                             strConnOnStartupAddress,
                             strMIDISetup,
                             bNoAutoJackConnect,
                             strClientName,
                             bMuteMeInPersonalMix );

            // load settings from init-file (command line options override)
            CClientSettings Settings ( &Client, strIniFileName );
            Settings.Load ( CommandLineOptions );

            // load translation
            if ( bUseGUI && bUseTranslation )
            {
                CLocale::LoadTranslation ( Settings.strLanguage, pApp );
                CInstPictures::UpdateTableOnLanguageChange();
            }

#ifndef HEADLESS
            if ( bUseGUI )
            {
                // GUI object
                CClientDlg ClientDlg ( &Client,
                                       &Settings,
                                       strConnOnStartupAddress,
                                       strMIDISetup,
                                       bShowComplRegConnList,
                                       bShowAnalyzerConsole,
                                       bMuteStream,
                                       nullptr );

                // show dialog
                ClientDlg.show();
                pApp->exec();
            }
            else
#endif
            {
                // only start application without using the GUI
                qInfo() << qUtf8Printable( GetVersionAndNameStr ( false ) );

                pApp->exec();
            }
        }
        else
        {
            // Server:
            // actual server object
            CServer Server ( iNumServerChannels,
                             strLoggingFileName,
                             iPortNumber,
                             strHTMLStatusFileName,
                             strCentralServer,
                             strServerInfo,
                             strServerPublicIP,
                             strServerListFilter,
                             strWelcomeMessage,
                             strRecordingDirName,
                             strEduModePassword,
                             bDisconnectAllClientsOnQuit,
                             bUseDoubleSystemFrameSize,
                             bUseMultithreading,
                             bLogIP,
                             bDisableRecording,
                             bEduModeEnabled,
                             eLicenceType );

#ifndef HEADLESS
            if ( bUseGUI )
            {
                // load settings from init-file (command line options override)
                CServerSettings Settings ( &Server, strIniFileName );
                Settings.Load ( CommandLineOptions );

                // load translation
                if ( bUseGUI && bUseTranslation )
                {
                    CLocale::LoadTranslation ( Settings.strLanguage, pApp );
                }

                // update server list AFTER restoring the settings from the
                // settings file
                Server.UpdateServerList();

                // GUI object for the server
                CServerDlg ServerDlg ( &Server,
                                       &Settings,
                                       bStartMinimized,
                                       nullptr );

                // show dialog (if not the minimized flag is set)
                if ( !bStartMinimized )
                {
                    ServerDlg.show();
                }

                pApp->exec();
            }
            else
#endif
            {
                // only start application without using the GUI
                qInfo() << qUtf8Printable( GetVersionAndNameStr ( false ) );

                // update serverlist
                Server.UpdateServerList();

                pApp->exec();
            }
        }
    }

    catch ( const CGenErr& generr )
    {
        // show generic error
#ifndef HEADLESS
        if ( bUseGUI )
        {
            QMessageBox::critical ( nullptr,
                                    APP_NAME,
                                    generr.GetErrorText(),
                                    "Quit",
                                    nullptr );
        }
        else
#endif
        {
            qCritical() << qUtf8Printable( QString( "%1: %2" )
                .arg( APP_NAME ).arg( generr.GetErrorText() ) );
            exit ( 1 );
        }
    }

#if defined ( Q_OS_MACX )
    activity.EndActivity();
#endif

    return 0;
}


/******************************************************************************\
* Command Line Argument Parsing                                                *
\******************************************************************************/
QString UsageArguments ( char **argv )
{
    return
        "Usage: " + QString ( argv[0] ) + " [option] [optional argument]\n"
        "\nRecognized options:\n"
        "  -h, -?, --help        display this help text and exit\n"
        "  -i, --inifile         initialization file name (not\n"
        "                        supported for headless server mode)\n"
        "  -n, --nogui           disable GUI\n"
        "  -p, --port            set your local port number\n"
        "  -t, --notranslation   disable translation (use English language)\n"
        "  -v, --version         output version information and exit\n"
        "\nServer only:\n"
        "  -d, --discononquit    disconnect all clients on quit\n"
        "  -e, --centralserver   address of the server list on which to register\n"
        "                        (or 'localhost' to be a server list)\n"
        "  -f, --listfilter      server list whitelist filter in the format:\n"
        "                        [IP address 1];[IP address 2];[IP address 3]; ...\n"
        "  --edumodepassword     enable Edu-Mode and set following admin password\n"
        "  -F, --fastupdate      use 64 samples frame size mode\n"
        "  -l, --log             enable logging, set file name\n"
        "   --logip              Log full IP adresses\n"
        "  -L, --licence         show an agreement window before users can connect\n"
        "  -m, --htmlstatus      enable HTML status file, set file name\n"
        "  -o, --serverinfo      infos of this server in the format:\n"
        "                        [name];[city];[country as QLocale ID]\n"
        "  -R, --recording       sets directory to contain recorded jams\n"
        "      --norecord        disables recording (when enabled by default by -R)\n"
        "  -s, --server          start server\n"
        "  -T, --multithreading  use multithreading to make better use of\n"
        "                        multi-core CPUs and support more clients\n"
        "  -u, --numchannels     maximum number of channels\n"
        "  -w, --welcomemessage  welcome message on connect\n"
        "  -z, --startminimized  start minimizied\n"
        "      --serverpublicip  specify your public IP address when\n"
        "                        running a slave and your own central server\n"
        "                        behind the same NAT\n"
        "\nClient only:\n"
        "  -M, --mutestream      starts the application in muted state\n"
        "      --mutemyown       mute me in my personal mix (headless only)\n"
        "  -c, --connect         connect to given server address on startup\n"
        "  -j, --nojackconnect   disable auto Jack connections\n"
        "      --ctrlmidich      MIDI controller channel to listen\n"
        "      --clientname      client name (window title and jack client name)\n"
        "\nExample: " + QString ( argv[0] ) + " -s --inifile myinifile.ini\n";
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

bool GetStringArgument ( int          argc,
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
            qCritical() << qUtf8Printable( QString( "%1: '%2' needs a string argument." )
                .arg( argv[0] ).arg( strLongOpt ) );
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

bool GetNumericArgument ( int          argc,
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
        QString errmsg = "%1: '%2' needs a numeric argument between '%3' and '%4'.";
        if ( ++i >= argc )
        {
            qCritical() << qUtf8Printable( errmsg
                .arg( argv[0] ).arg( strLongOpt ).arg( rRangeStart ).arg( rRangeStop ) );
            exit ( 1 );
        }

        char *p;
        rValue = strtod ( argv[i], &p );
        if ( *p ||
             ( rValue < rRangeStart ) ||
             ( rValue > rRangeStop ) )
        {
            qCritical() << qUtf8Printable( errmsg
                .arg( argv[0] ).arg( strLongOpt ).arg( rRangeStart ).arg( rRangeStop ) );
            exit ( 1 );
        }

        return true;
    }
    else
    {
        return false;
    }
}
