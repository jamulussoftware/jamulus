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
#include <QTextStream>
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
#if defined ( __APPLE__ ) || defined ( __MACOSX )
# include "mac/activity.h"
#endif


// Implementation **************************************************************

int main ( int argc, char** argv )
{

    QTextStream&   tsConsole = *( ( new ConsoleWriterFactory() )->get() );
    QString        strArgument;
    double         rDbleArgument;
    QList<QString> CommandLineOptions;

    // initialize all flags and string which might be changed by command line
    // arguments
#if defined( SERVER_BUNDLE ) && ( defined( __APPLE__ ) || defined( __MACOSX ) )
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
    QString      strServerListFilter         = "";
    QString      strWelcomeMessage           = "";
    QString      strClientName               = APP_NAME;

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
            tsConsole << "- no GUI mode chosen" << endl;
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
            tsConsole << "- licence required" << endl;
            CommandLineOptions << "--licence";
            continue;
        }


        // Use 64 samples frame size mode --------------------------------------
        if ( GetFlagArgument ( argv,
                               i,
                               "-F",
                               "--fastupdate" ) )
        {
            bUseDoubleSystemFrameSize = false; // 64 samples frame size
            tsConsole << "- using " << SYSTEM_FRAME_SIZE_SAMPLES << " samples frame size mode" << endl;
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
            tsConsole << "- using multithreading" << endl;
            CommandLineOptions << "--multithreading";
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
            tsConsole << "- start minimized enabled" << endl;
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
            tsConsole << "- disconnect all clients on quit" << endl;
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
            tsConsole << "- disable auto Jack connections" << endl;
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
            tsConsole << "- translations disabled" << endl;
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
            tsConsole << "- show all registered servers in server list" << endl;
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
            tsConsole << "- show analyzer console" << endl;
            CommandLineOptions << "--showanalyzerconsole";
            continue;
        }


        // Controller MIDI channel ---------------------------------------------
        if ( GetStringArgument ( tsConsole,
                                 argc,
                                 argv,
                                 i,
                                 "--ctrlmidich", // no short form
                                 "--ctrlmidich",
                                 strArgument ) )
        {
            strMIDISetup = strArgument;
            tsConsole << "- MIDI controller settings: " << strMIDISetup << endl;
            CommandLineOptions << "--ctrlmidich";
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
            CommandLineOptions << "--log";
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
            iPortNumber            = static_cast<quint16> ( rDbleArgument );
            bCustomPortNumberGiven = true;
            tsConsole << "- selected port number: " << iPortNumber << endl;
            CommandLineOptions << "--port";
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
            CommandLineOptions << "--htmlstatus";
            continue;
        }


        // Client Name ---------------------------------------------------------
        if ( GetStringArgument ( tsConsole,
                                 argc,
                                 argv,
                                 i,
                                 "--clientname", // no short form
                                 "--clientname",
                                 strArgument ) )
        {
            strClientName = QString ( APP_NAME ) + " " + strArgument;
            tsConsole << "- client name: " << strClientName << endl;
            CommandLineOptions << "--clientname";
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
            tsConsole << "- recording will not be enabled" << endl;
            CommandLineOptions << "--norecord";
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
            CommandLineOptions << "--centralserver";
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
            CommandLineOptions << "--serverinfo";
            continue;
        }


        // Server list filter --------------------------------------------------
        if ( GetStringArgument ( tsConsole,
                                 argc,
                                 argv,
                                 i,
                                 "-f",
                                 "--listfilter",
                                 strArgument ) )
        {
            strServerListFilter = strArgument;
            tsConsole << "- server list filter: " << strServerListFilter << endl;
            CommandLineOptions << "--listfilter";
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
            CommandLineOptions << "--welcomemessage";
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
            CommandLineOptions << "--inifile";
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
            strConnOnStartupAddress = NetworkUtil::FixAddress ( strArgument );
            tsConsole << "- connect on startup to address: " << strConnOnStartupAddress << endl;
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
            tsConsole << "- mute stream activated" << endl;
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
            tsConsole << "- mute me in my personal mix" << endl;
            CommandLineOptions << "--mutemyown";
            continue;
        }


        // Version number ------------------------------------------------------
        if ( ( !strcmp ( argv[i], "--version" ) ) ||
             ( !strcmp ( argv[i], "-v" ) ) )
        {
            tsConsole << GetVersionAndNameStr ( false ) << endl;
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
#ifdef HEADLESS
    if ( bUseGUI )
    {
        bUseGUI = false;
        tsConsole << "No GUI support compiled. Running in headless mode." << endl;
    }
    Q_UNUSED ( bStartMinimized )       // avoid compiler warnings
    Q_UNUSED ( bShowComplRegConnList ) // avoid compiler warnings
    Q_UNUSED ( bShowAnalyzerConsole )  // avoid compiler warnings
    Q_UNUSED ( bMuteStream )           // avoid compiler warnings
#endif

    // the inifile is not supported for the headless server mode
    if ( !bIsClient && !bUseGUI && !strIniFileName.isEmpty() )
    {
        tsConsole << "No initialization file support in headless server mode." << endl;
    }

    // mute my own signal in personal mix is only supported for headless mode
    if ( bIsClient && bUseGUI && bMuteMeInPersonalMix )
    {
        bMuteMeInPersonalMix = false;
        tsConsole << "Mute my own signal in my personal mix is only supported in headless mode." << endl;
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
    QCoreApplication* pApp = bUseGUI
        ? new QApplication ( argc, argv )
        : new QCoreApplication ( argc, argv );
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

#if defined ( __APPLE__ ) || defined ( __MACOSX )
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
                tsConsole << GetVersionAndNameStr ( false ) << endl;

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
                             strServerListFilter,
                             strWelcomeMessage,
                             strRecordingDirName,
                             bDisconnectAllClientsOnQuit,
                             bUseDoubleSystemFrameSize,
                             bUseMultithreading,
                             bDisableRecording,
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
                tsConsole << GetVersionAndNameStr ( false ) << endl;

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
            tsConsole << generr.GetErrorText() << endl;
        }
    }
    
#if defined ( __APPLE__ ) || defined ( __MACOSX )
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
        "  -t, --notranslation   disable translation (use englisch language)\n"
        "  -v, --version         output version information and exit\n"
        "\nServer only:\n"
        "  -d, --discononquit    disconnect all clients on quit\n"
        "  -e, --centralserver   address of the central server\n"
        "                        (or 'localhost' to be a central server)\n"
        "  -f, --listfilter      server list whitelist filter in the format:\n"
        "                        [IP address 1];[IP address 2];[IP address 3]; ...\n"
        "  -F, --fastupdate      use 64 samples frame size mode\n"
        "  -l, --log             enable logging, set file name\n"
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
        "\nClient only:\n"
        "  -M, --mutestream      starts the application in muted state\n"
        "      --mutemyown       mute me in my personal mix (headless only)\n"
        "  -c, --connect         connect to given server address on startup\n"
        "  -j, --nojackconnect   disable auto Jack connections\n"
        "  --ctrlmidich          MIDI controller channel to listen\n"
        "  --clientname          client name (window title and jack client name)\n"
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
