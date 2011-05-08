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

#include <qapplication.h>
#include <qmessagebox.h>
#include <qdir.h>
#include <qtextstream.h>
#include "global.h"
#include "llconclientdlg.h"
#include "llconserverdlg.h"
#include "settings.h"
#include "testbench.h"


// Implementation **************************************************************
// these pointers are only used for the post-event routine
QApplication* pApp        = NULL;
QDialog*      pMainWindow = NULL;


int main ( int argc, char** argv )
{
#ifdef _WIN32
    // no console on windows -> just write in string and dump it
    QString     strDummySink;
    QTextStream tsConsole ( &strDummySink );
#else
    QTextStream tsConsole ( stdout );
#endif
    QString strArgument;
    double  rDbleArgument;

    // initialize all flags and string which might be changed by command line
    // arguments
    bool    bIsClient             = true;
    bool    bUseGUI               = true;
    bool    bStartMinimized       = false;
    bool    bConnectOnStartup     = false;
    bool    bDisalbeLEDs          = false;
    quint16 iPortNumber           = LLCON_DEFAULT_PORT_NUMBER;
    QString strIniFileName        = "";
    QString strHTMLStatusFileName = "";
    QString strServerName         = "";
    QString strLoggingFileName    = "";
    QString strHistoryFileName    = "";
    QString strCentralServer      = "";
    QString strServerInfo         = "";

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


        // Disable LEDs flag ---------------------------------------------------
        if ( GetFlagArgument ( argv,
                               i,
                               "-d",
                               "--disableleds" ) )
        {
            bDisalbeLEDs = true;
            tsConsole << "- disable LEDs in main window" << endl;
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


        // HTML status file ----------------------------------------------------
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
        if ( GetFlagArgument ( argv,
                               i,
                               "-c",
                               "--connect" ) )
        {
            bConnectOnStartup = true;
            tsConsole << "- connect on startup enabled" << endl;
            continue;
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
    QApplication app ( argc, argv, bUseGUI );

#ifdef _WIN32
    // Set application priority class -> high priority
    SetPriorityClass ( GetCurrentProcess(), HIGH_PRIORITY_CLASS );

    // For accessible support we need to add a plugin to qt. The plugin has to
    // be located in the install directory of llcon by the installer. Here, we
    // set the path to our application
    QDir ApplDir ( QApplication::applicationDirPath() );
    app.addLibraryPath ( QString ( ApplDir.absolutePath() ) );
#endif

    // init resources
#ifdef _IS_QMAKE_CONFIG
    Q_INIT_RESOURCE(resources);
#else
    extern int qInitResources();
    qInitResources();
#endif


// TEST -> activate the following line to activate the test bench,
//CTestbench Testbench ( "127.0.0.1", LLCON_DEFAULT_PORT_NUMBER );


    try
    {
        if ( bIsClient )
        {
            // Client:
            // actual client object
            CClient Client ( iPortNumber );

            // load settings from init-file
            CSettings Settings ( &Client );
            Settings.Load ( strIniFileName );

            // GUI object
            CLlconClientDlg ClientDlg (
                &Client,
                bConnectOnStartup,
                bDisalbeLEDs,
                0,
                Qt::Window );

            // set main window
            pMainWindow = &ClientDlg;
            pApp        = &app; // Needed for post-event routine

            // show dialog
            ClientDlg.show();
            app.exec();

            // save settings to init-file
            Settings.Save ( strIniFileName );
        }
        else
        {
            // Server:
            // actual server object
            CServer Server ( strLoggingFileName,
                             iPortNumber,
                             strHTMLStatusFileName,
                             strHistoryFileName,
                             strServerName,
                             strCentralServer,
                             strServerInfo );

            if ( bUseGUI )
            {
                // special case for the GUI mode: as the default we want to use
                // the default central server address (if not given in the
                // settings file)
                Server.SetUseDefaultCentralServerAddress ( true );

                // load settings from init-file
                CSettings Settings ( &Server );
                Settings.Load ( strIniFileName );

                // update server list AFTER restoring the settings from the
                // settings file
                Server.UpdateServerList();

                // GUI object for the server
                CLlconServerDlg ServerDlg (
                    &Server,
                    bStartMinimized,
                    0,
                    Qt::Window );

                // set main window
                pMainWindow = &ServerDlg;
                pApp        = &app; // needed for post-event routine

                // show dialog
                ServerDlg.show();
                app.exec();

                // save settings to init-file
                Settings.Save ( strIniFileName );
            }
            else
            {
                // update serverlist
                Server.UpdateServerList();

                // only start application without using the GUI
                tsConsole << CAboutDlg::GetVersionAndNameStr ( false ) << endl;

                app.exec();
            }
        }
    }

    catch ( CGenErr generr )
    {
        // show generic error
        if ( bUseGUI )
        {
            QMessageBox::critical ( 0,
                                    APP_NAME,
                                    generr.GetErrorText(),
                                    "Quit",
                                    0 );
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
        "  -s, --server          start server\n"
        "  -n, --nogui           disable GUI (server only)\n"
        "  -z, --startminimized  start minimizied (server only)\n"
        "  -l, --log             enable logging, set file name\n"
        "  -i, --inifile         initialization file name (client only)\n"
        "  -p, --port            local port number (server only)\n"
        "  -m, --htmlstatus      enable HTML status file, set file name (server\n"
        "                        only)\n"
        "  -a, --servername      server name, required for HTML status (server\n"
        "                        only)\n"
        "  -y, --history         enable connection history and set file\n"
        "                        name (server only)\n"
        "  -e, --centralserver   address of the central server (server only)\n"
        "  -o, --serverinfo      infos of the server(s) in the format:\n"
        "                        [name];[city];[country as QLocale ID]; ...\n"
        "                        [server1 address];[server1 name]; ...\n"
        "                        [server1 city]; ...\n"
        "                        [server1 country as QLocale ID]; ...\n"
        "                        [server2 address]; ... (server only)\n"
        "  -c, --connect         connect to last server on startup (client\n"
        "                        only)\n"
        "  -d, --disableleds     disable LEDs in main window (client only)\n"
        "  -h, -?, --help        this help text\n"
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


/******************************************************************************\
* Window Message System                                                        *
\******************************************************************************/
void PostWinMessage ( const _MESSAGE_IDENT MessID,
                      const int            iMessageParam,
                      const int            iChanNum )
{
    // first check if application is initialized
    if ( pApp != NULL )
    {
        CLlconEvent* LlconEv =
            new CLlconEvent ( MessID, iMessageParam, iChanNum );

        // Qt will delete the event object when done
        QCoreApplication::postEvent ( pMainWindow, LlconEv );
    }
}
