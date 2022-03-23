/******************************************************************************\
 * Copyright (c) 2004-2022
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
#include <iostream>
#include "global.h"
#ifndef HEADLESS
#    include <QApplication>
#    include <QMessageBox>
#    include "clientdlg.h"
#    include "serverdlg.h"
#endif
#include "settings.h"
#include "testbench.h"
#include "util.h"
#ifdef ANDROID
#    include <QtAndroidExtras/QtAndroid>
#endif
#if defined( Q_OS_MACX )
#    include "mac/activity.h"
extern void qt_set_sequence_auto_mnemonic ( bool bEnable );
#endif
#include <memory>
#include "rpcserver.h"
#include "serverrpc.h"
#include "clientrpc.h"

// Implementation **************************************************************

int    CCommandlineOptions::appArgc = 0;
char** CCommandlineOptions::appArgv = NULL;

QDialog* CMsgBoxes::pMainForm       = NULL;
QString  CMsgBoxes::strMainFormName = APP_NAME;


int main ( int argc, char** argv )
{
    CCommandlineOptions::appArgc = argc;
    CCommandlineOptions::appArgv = argv;

    CCommandlineOptions cmdLine; // We don't really need an instance here, but using one improves the readability of the code.

#if defined( Q_OS_MACX )
    // Mnemonic keys are default disabled in Qt for MacOS. The following function enables them.
    // Qt will not show these with underline characters in the GUI on MacOS.
    qt_set_sequence_auto_mnemonic ( true );
#endif

    QString        strArgument;
    double         rDbleArgument;
    QList<QString> CommandLineOptions;
    QList<QString> ServerOnlyOptions;
    QList<QString> ClientOnlyOptions;

    // initialize all flags and string which might be changed by command line
    // arguments
#if defined( SERVER_BUNDLE ) && ( defined( Q_OS_MACX ) )
    // if we are on MacOS and we are building a server bundle, starts Jamulus in server mode
    bool bIsClient = false;
#else
    bool bIsClient = true;
#endif
    bool         bSpecialOptions             = false; // Any options after this option will be accepted ! (mostly used for debugging purpouses)
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
    bool         bDelayPan                   = false;
    bool         bNoAutoJackConnect          = false;
    bool         bUseTranslation             = true;
    bool         bCustomPortNumberGiven      = false;
    bool         bEnableIPv6                 = false;
    int          iNumServerChannels          = DEFAULT_USED_NUM_CHANNELS;
    quint16      iPortNumber                 = DEFAULT_PORT_NUMBER;
    int          iJsonRpcPortNumber          = INVALID_PORT;
    quint16      iQosNumber                  = DEFAULT_QOS_NUMBER;
    ELicenceType eLicenceType                = LT_NO_LICENCE;
    QString      strMIDISetup                = "";
    QString      strConnOnStartupAddress     = "";
    QString      strIniFileName              = "";
    QString      strHTMLStatusFileName       = "";
    QString      strLoggingFileName          = "";
    QString      strRecordingDirName         = "";
    QString      strDirectoryServer          = "";
    QString      strServerListFileName       = "";
    QString      strServerInfo               = "";
    QString      strServerPublicIP           = "";
    QString      strServerBindIP             = "";
    QString      strServerListFilter         = "";
    QString      strWelcomeMessage           = "";
    QString      strClientName               = "";
    QString      strJsonRpcSecretFileName    = "";

#if !defined( HEADLESS ) && defined( _WIN32 )
    if ( AttachConsole ( ATTACH_PARENT_PROCESS ) )
    {
        freopen ( "CONOUT$", "w", stdout );
        freopen ( "CONOUT$", "w", stderr );
    }
#endif

    // When adding new options, follow the same order as --help output

    // QT docu: argv()[0] is the program name, argv()[1] is the first
    // argument and argv()[argc()-1] is the last argument.
    // Start with first argument, therefore "i = 1"

    // clang-format off
// pgScorio TODO:
// Extra Checks on parameters:
//      If given, CMDLN_SERVER MUST be FIRST parameter.
//      And then only check parameters valid for common, server or client !
    // clang-format on

    for ( int i = 1; i < argc; i++ )
    {
        // Help (usage) flag ---------------------------------------------------
        if ( ( !strcmp ( argv[i], "--help" ) ) || ( !strcmp ( argv[i], "-h" ) ) || ( !strcmp ( argv[i], "-?" ) ) )
        {
            std::cout << qUtf8Printable ( UsageArguments ( argv ) );
            exit ( 0 );
        }

        // Version number ------------------------------------------------------
        if ( ( !strcmp ( argv[i], "--version" ) ) || ( !strcmp ( argv[i], "-v" ) ) )
        {
            std::cout << qUtf8Printable ( GetVersionAndNameStr ( false ) );
            exit ( 0 );
        }

        // Common options:

        // Initialization file -------------------------------------------------
        if ( cmdLine.GetStringArgument ( i, CMDLN_INIFILE, strArgument ) )
        {
            strIniFileName = strArgument;
            qInfo() << qUtf8Printable ( QString ( "- initialization file name: %1" ).arg ( strIniFileName ) );
            CommandLineOptions << "--inifile";
            continue;
        }

        // Disable GUI flag ----------------------------------------------------
        if ( cmdLine.GetFlagArgument ( i, CMDLN_NOGUI ) )
        {
            bUseGUI = false;
            qInfo() << "- no GUI mode chosen";
            CommandLineOptions << "--nogui";
            continue;
        }

        // Port number ---------------------------------------------------------
        if ( cmdLine.GetNumericArgument ( i, CMDLN_PORT, 0, 65535, rDbleArgument ) )
        {
            iPortNumber            = static_cast<quint16> ( rDbleArgument );
            bCustomPortNumberGiven = true;
            qInfo() << qUtf8Printable ( QString ( "- selected port number: %1" ).arg ( iPortNumber ) );
            CommandLineOptions << "--port";
            continue;
        }

        // Quality of Service --------------------------------------------------
        if ( cmdLine.GetNumericArgument ( i, CMDLN_QOS, 0, 255, rDbleArgument ) )
        {
            iQosNumber = static_cast<quint16> ( rDbleArgument );
            qInfo() << qUtf8Printable ( QString ( "- selected QoS value: %1" ).arg ( iQosNumber ) );
            CommandLineOptions << "--qos";
            continue;
        }

        // Disable translations ------------------------------------------------
        if ( cmdLine.GetFlagArgument ( i, CMDLN_NOTRANSLATION ) )
        {
            bUseTranslation = false;
            qInfo() << "- translations disabled";
            CommandLineOptions << "--notranslation";
            continue;
        }

        // Enable IPv6 ---------------------------------------------------------
        if ( cmdLine.GetFlagArgument ( i, CMDLN_ENABLEIPV6 ) )
        {
            bEnableIPv6 = true;
            qInfo() << "- IPv6 enabled";
            CommandLineOptions << "--enableipv6";
            continue;
        }

        // Server only:

        // Disconnect all clients on quit --------------------------------------
        if ( cmdLine.GetFlagArgument ( i, CMDLN_DISCONONQUIT ) )
        {
            bDisconnectAllClientsOnQuit = true;
            qInfo() << "- disconnect all clients on quit";
            CommandLineOptions << "--discononquit";
            ServerOnlyOptions << "--discononquit";
            continue;
        }

        // Directory server ----------------------------------------------------
        if ( cmdLine.GetStringArgument ( i, CMDLN_DIRECTORYSERVER, strArgument ) ||
             cmdLine.GetStringArgument ( i, CMDLN_CENTRALSERVER, strArgument ) // ** D E P R E C A T E D **
        )
        {
            strDirectoryServer = strArgument;
            qInfo() << qUtf8Printable ( QString ( "- directory server: %1" ).arg ( strDirectoryServer ) );
            CommandLineOptions << "--directoryserver";
            ServerOnlyOptions << "--directoryserver";
            continue;
        }

        // Directory file ------------------------------------------------------
        if ( cmdLine.GetStringArgument ( i, CMDLN_DIRECTORYFILE, strArgument ) )
        {
            strServerListFileName = strArgument;
            qInfo() << qUtf8Printable ( QString ( "- directory server persistence file: %1" ).arg ( strServerListFileName ) );
            CommandLineOptions << "--directoryfile";
            ServerOnlyOptions << "--directoryfile";
            continue;
        }

        // Server list filter --------------------------------------------------
        if ( cmdLine.GetStringArgument ( i, CMDLN_LISTFILTER, strArgument ) )
        {
            strServerListFilter = strArgument;
            qInfo() << qUtf8Printable ( QString ( "- server list filter: %1" ).arg ( strServerListFilter ) );
            CommandLineOptions << "--listfilter";
            ServerOnlyOptions << "--listfilter";
            continue;
        }

        // Use 64 samples frame size mode --------------------------------------
        if ( cmdLine.GetFlagArgument ( i, CMDLN_FASTUPDATE ) )
        {
            bUseDoubleSystemFrameSize = false; // 64 samples frame size
            qInfo() << qUtf8Printable ( QString ( "- using %1 samples frame size mode" ).arg ( SYSTEM_FRAME_SIZE_SAMPLES ) );
            CommandLineOptions << "--fastupdate";
            ServerOnlyOptions << "--fastupdate";
            continue;
        }

        // Use logging ---------------------------------------------------------
        if ( cmdLine.GetStringArgument ( i, CMDLN_LOG, strArgument ) )
        {
            strLoggingFileName = strArgument;
            qInfo() << qUtf8Printable ( QString ( "- logging file name: %1" ).arg ( strLoggingFileName ) );
            CommandLineOptions << "--log";
            ServerOnlyOptions << "--log";
            continue;
        }

        // Use licence flag ----------------------------------------------------
        if ( cmdLine.GetFlagArgument ( i, CMDLN_LICENCE ) )
        {
            // LT_CREATIVECOMMONS is now used just to enable the pop up
            eLicenceType = LT_CREATIVECOMMONS;
            qInfo() << "- licence required";
            CommandLineOptions << "--licence";
            ServerOnlyOptions << "--licence";
            continue;
        }

        // HTML status file ----------------------------------------------------
        if ( cmdLine.GetStringArgument ( i, CMDLN_HTMLSTATUS, strArgument ) )
        {
            strHTMLStatusFileName = strArgument;
            qInfo() << qUtf8Printable ( QString ( "- HTML status file name: %1" ).arg ( strHTMLStatusFileName ) );
            CommandLineOptions << "--htmlstatus";
            ServerOnlyOptions << "--htmlstatus";
            continue;
        }

        // Server info ---------------------------------------------------------
        if ( cmdLine.GetStringArgument ( i, CMDLN_SERVERINFO, strArgument ) )
        {
            strServerInfo = strArgument;
            qInfo() << qUtf8Printable ( QString ( "- server info: %1" ).arg ( strServerInfo ) );
            CommandLineOptions << "--serverinfo";
            ServerOnlyOptions << "--serverinfo";
            continue;
        }

        // Server Public IP ----------------------------------------------------
        if ( cmdLine.GetStringArgument ( i, CMDLN_SERVERPUBLICIP, strArgument ) )
        {
            strServerPublicIP = strArgument;
            qInfo() << qUtf8Printable ( QString ( "- server public IP: %1" ).arg ( strServerPublicIP ) );
            CommandLineOptions << "--serverpublicip";
            ServerOnlyOptions << "--serverpublicip";
            continue;
        }

        // Enable delay panning on startup -------------------------------------
        if ( cmdLine.GetFlagArgument ( i, CMDLN_DELAYPAN ) )
        {
            bDelayPan = true;
            qInfo() << "- starting with delay panning";
            CommandLineOptions << "--delaypan";
            ServerOnlyOptions << "--delaypan";
            continue;
        }

        // Recording directory -------------------------------------------------
        if ( cmdLine.GetStringArgument ( i, CMDLN_RECORDING, strArgument ) )
        {
            strRecordingDirName = strArgument;
            qInfo() << qUtf8Printable ( QString ( "- recording directory name: %1" ).arg ( strRecordingDirName ) );
            CommandLineOptions << "--recording";
            ServerOnlyOptions << "--recording";
            continue;
        }

        // Disable recording on startup ----------------------------------------
        if ( cmdLine.GetFlagArgument ( i, CMDLN_NORECORD ) )
        {
            bDisableRecording = true;
            qInfo() << "- recording will not take place until enabled";
            CommandLineOptions << "--norecord";
            ServerOnlyOptions << "--norecord";
            continue;
        }

        // Server mode flag ----------------------------------------------------
        if ( cmdLine.GetFlagArgument ( i, CMDLN_SERVER ) )
        {
            bIsClient = false;
            qInfo() << "- server mode chosen";
            CommandLineOptions << "--server";
            ServerOnlyOptions << "--server";
            continue;
        }

        // Server Bind IP --------------------------------------------------
        if ( cmdLine.GetStringArgument ( i, CMDLN_SERVERBINDIP, strArgument ) )
        {
            strServerBindIP = strArgument;
            qInfo() << qUtf8Printable ( QString ( "- server bind IP: %1" ).arg ( strServerBindIP ) );
            CommandLineOptions << "--serverbindip";
            ServerOnlyOptions << "--serverbindip";
            continue;
        }

        // Use multithreading --------------------------------------------------
        if ( cmdLine.GetFlagArgument ( i, CMDLN_MULTITHREADING ) )
        {
            bUseMultithreading = true;
            qInfo() << "- using multithreading";
            CommandLineOptions << "--multithreading";
            ServerOnlyOptions << "--multithreading";
            continue;
        }

        // Maximum number of channels ------------------------------------------
        if ( cmdLine.GetNumericArgument ( i, CMDLN_NUMCHANNELS, 1, MAX_NUM_CHANNELS, rDbleArgument ) )
        {
            iNumServerChannels = static_cast<int> ( rDbleArgument );

            qInfo() << qUtf8Printable ( QString ( "- maximum number of channels: %1" ).arg ( iNumServerChannels ) );

            CommandLineOptions << "--numchannels";
            ServerOnlyOptions << "--numchannels";
            continue;
        }

        // Server welcome message ----------------------------------------------
        if ( cmdLine.GetStringArgument ( i, CMDLN_WELCOMEMESSAGE, strArgument ) )
        {
            strWelcomeMessage = strArgument;
            qInfo() << qUtf8Printable ( QString ( "- welcome message: %1" ).arg ( strWelcomeMessage ) );
            CommandLineOptions << "--welcomemessage";
            ServerOnlyOptions << "--welcomemessage";
            continue;
        }

        // Start minimized -----------------------------------------------------
        if ( cmdLine.GetFlagArgument ( i, CMDLN_STARTMINIMIZED ) )
        {
            bStartMinimized = true;
            qInfo() << "- start minimized enabled";
            CommandLineOptions << "--startminimized";
            ServerOnlyOptions << "--startminimized";
            continue;
        }

        // Client only:

        // Connect on startup --------------------------------------------------
        if ( cmdLine.GetStringArgument ( i, CMDLN_CONNECT, strArgument ) )
        {
            strConnOnStartupAddress = NetworkUtil::FixAddress ( strArgument );
            qInfo() << qUtf8Printable ( QString ( "- connect on startup to address: %1" ).arg ( strConnOnStartupAddress ) );
            CommandLineOptions << "--connect";
            ClientOnlyOptions << "--connect";
            continue;
        }

        // Disabling auto Jack connections -------------------------------------
        if ( cmdLine.GetFlagArgument ( i, CMDLN_NOJACKCONNECT ) )
        {
            bNoAutoJackConnect = true;
            qInfo() << "- disable auto Jack connections";
            CommandLineOptions << "--nojackconnect";
            ClientOnlyOptions << "--nojackconnect";
            continue;
        }

        // Mute stream on startup ----------------------------------------------
        if ( cmdLine.GetFlagArgument ( i, CMDLN_MUTESTREAM ) )
        {
            bMuteStream = true;
            qInfo() << "- mute stream activated";
            CommandLineOptions << "--mutestream";
            ClientOnlyOptions << "--mutestream";
            continue;
        }

        // For headless client mute my own signal in personal mix --------------
        if ( cmdLine.GetFlagArgument ( i, CMDLN_MUTEMYOWN ) )
        {
            bMuteMeInPersonalMix = true;
            qInfo() << "- mute me in my personal mix";
            CommandLineOptions << "--mutemyown";
            ClientOnlyOptions << "--mutemyown";
            continue;
        }

        // Client Name ---------------------------------------------------------
        if ( cmdLine.GetStringArgument ( i, CMDLN_CLIENTNAME, strArgument ) )
        {
            strClientName = strArgument;
            qInfo() << qUtf8Printable ( QString ( "- client name: %1" ).arg ( strClientName ) );
            CommandLineOptions << "--clientname";
            ClientOnlyOptions << "--clientname";
            continue;
        }

        // Controller MIDI channel ---------------------------------------------
        if ( cmdLine.GetStringArgument ( i, CMDLN_CTRLMIDICH, strArgument ) )
        {
            strMIDISetup = strArgument;
            qInfo() << qUtf8Printable ( QString ( "- MIDI controller settings: %1" ).arg ( strMIDISetup ) );
            CommandLineOptions << "--ctrlmidich";
            ClientOnlyOptions << "--ctrlmidich";
            continue;
        }

        // Undocumented:

        // Show all registered servers in the server list ----------------------
        // Undocumented debugging command line argument: Show all registered
        // servers in the server list regardless if a ping to the server is
        // possible or not.
        if ( cmdLine.GetFlagArgument ( i, CMDLN_SHOWALLSERVERS ) )
        {
            bShowComplRegConnList = true;
            qInfo() << "- show all registered servers in server list";
            CommandLineOptions << "--showallservers";
            ClientOnlyOptions << "--showallservers";
            continue;
        }

        // Show analyzer console -----------------------------------------------
        // Undocumented debugging command line argument: Show the analyzer
        // console to debug network buffer properties.
        if ( cmdLine.GetFlagArgument ( i, CMDLN_SHOWANALYZERCONSOLE ) )
        {
            bShowAnalyzerConsole = true;
            qInfo() << "- show analyzer console";
            CommandLineOptions << "--showanalyzerconsole";
            ClientOnlyOptions << "--showanalyzerconsole";
            continue;
        }

        // Enable Special Options ----------------------------------------------
        if ( cmdLine.GetFlagArgument ( i, CMDLN_SPECIAL ) )
        {
            bSpecialOptions = true;
            qInfo() << "- Special options enabled !";
            continue;
        }

        // Unknown option ------------------------------------------------------
        qCritical() << qUtf8Printable ( QString ( "%1: Unknown option '%2' -- use '--help' for help" ).arg ( argv[0] ).arg ( argv[i] ) );

        // pgScorpio: No exit for options after the "--special" option.
        // Used for debugging and testing new options...
        if ( !bSpecialOptions )
        {
// clicking on the Mac application bundle, the actual application
// is called with weird command line args -> do not exit on these
#if !( defined( Q_OS_MACX ) )
            exit ( 1 );
#endif
        }
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

#ifdef SERVER_ONLY
    if ( bIsClient )
    {
        qCritical() << "Only --server mode is supported in this build with nosound.";
        exit ( 1 );
    }
#endif

    // TODO create settings in default state, if loading from file do that next, then come back here to
    //      override from command line options, then create client or server, letting them do the validation

    if ( bIsClient )
    {
        if ( ServerOnlyOptions.size() != 0 )
        {
            qCritical() << qUtf8Printable ( QString ( "%1: Server only option(s) '%2' used.  Did you omit '--server'?" )
                                                .arg ( CCommandlineOptions::GetProgramPath(), ServerOnlyOptions.join ( ", " ) ) );
            exit ( 1 );
        }

        // mute my own signal in personal mix is only supported for headless mode
        if ( bUseGUI && bMuteMeInPersonalMix )
        {
            bMuteMeInPersonalMix = false;
            qWarning() << "Mute my own signal in my personal mix is only supported in headless mode.";
        }

        // adjust default port number for client: use different default port than the server since
        // if the client is started before the server, the server would get a socket bind error
        if ( !bCustomPortNumberGiven )
        {
            iPortNumber += 10; // increment by 10
            qInfo() << qUtf8Printable ( QString ( "- allocated port number: %1" ).arg ( iPortNumber ) );
        }
    }
    else
    {
        if ( ClientOnlyOptions.size() != 0 )
        {
            qCritical() << qUtf8Printable (
                QString ( "%1: Client only option(s) '%2' used.  See '--help' for help" ).arg ( argv[0] ).arg ( ClientOnlyOptions.join ( ", " ) ) );
            exit ( 1 );
        }

        if ( bUseGUI )
        {
            // by definition, when running with the GUI we always default to registering somewhere but
            // until the settings are loaded we do not know where, so we cannot be prescriptive here

            if ( !strServerListFileName.isEmpty() )
            {
                qInfo() << "Note:"
                        << "Server list persistence file will only take effect when running as a directory server.";
            }

            if ( !strServerListFilter.isEmpty() )
            {
                qInfo() << "Note:"
                        << "Server list filter will only take effect when running as a directory server.";
            }
        }
        else
        {
            // the inifile is not supported for the headless server mode
            if ( !strIniFileName.isEmpty() )
            {
                qWarning() << "No initialization file support in headless server mode.";
                strIniFileName = "";
            }
            // therefore we know everything based on command line options

            if ( strDirectoryServer.compare ( "localhost", Qt::CaseInsensitive ) == 0 || strDirectoryServer.compare ( "127.0.0.1" ) == 0 )
            {
                if ( !strServerListFileName.isEmpty() )
                {
                    QFileInfo serverListFileInfo ( strServerListFileName );
                    if ( !serverListFileInfo.exists() )
                    {
                        QFile strServerListFile ( strServerListFileName );
                        if ( !strServerListFile.open ( QFile::OpenModeFlag::ReadWrite ) )
                        {
                            qWarning() << qUtf8Printable (
                                QString ( "Cannot create %1 for reading and writing.  Please check permissions." ).arg ( strServerListFileName ) );
                            strServerListFileName = "";
                        }
                    }
                    else if ( !serverListFileInfo.isFile() )
                    {
                        qWarning() << qUtf8Printable (
                            QString ( "Server list file %1 must be a plain file.  Please check the name." ).arg ( strServerListFileName ) );
                        strServerListFileName = "";
                    }
                    else if ( !serverListFileInfo.isReadable() || !serverListFileInfo.isWritable() )
                    {
                        qWarning() << qUtf8Printable (
                            QString ( "Server list file %1 must be readable and writeable.  Please check the permissions." )
                                .arg ( strServerListFileName ) );
                        strServerListFileName = "";
                    }
                }

                if ( !strServerListFilter.isEmpty() )
                {
                    QStringList slWhitelistAddresses = strServerListFilter.split ( ";" );
                    for ( int iIdx = 0; iIdx < slWhitelistAddresses.size(); iIdx++ )
                    {
                        // check for special case: [version]
                        if ( ( slWhitelistAddresses.at ( iIdx ).length() > 2 ) && ( slWhitelistAddresses.at ( iIdx ).left ( 1 ) == "[" ) &&
                             ( slWhitelistAddresses.at ( iIdx ).right ( 1 ) == "]" ) )
                        {
                            // Good case - it seems QVersionNumber isn't fussy
                        }
                        else if ( slWhitelistAddresses.at ( iIdx ).isEmpty() )
                        {
                            qWarning() << "There is empty entry in the server list filter that will be ignored";
                        }
                        else
                        {
                            QHostAddress InetAddr;
                            if ( !InetAddr.setAddress ( slWhitelistAddresses.at ( iIdx ) ) )
                            {
                                qWarning() << qUtf8Printable (
                                    QString ( "%1 is not a valid server list filter entry. Only plain IP addresses are supported" )
                                        .arg ( slWhitelistAddresses.at ( iIdx ) ) );
                            }
                        }
                    }
                }
            }
            else
            {
                if ( !strServerListFileName.isEmpty() )
                {
                    qWarning() << "Server list persistence file will only take effect when running as a directory server.";
                    strServerListFileName = "";
                }

                if ( !strServerListFilter.isEmpty() )
                {
                    qWarning() << "Server list filter will only take effect when running as a directory server.";
                    strServerListFileName = "";
                }
            }

            if ( strDirectoryServer.isEmpty() )
            {
                if ( !strServerPublicIP.isEmpty() )
                {
                    qWarning() << "Server Public IP will only take effect when registering a server with a directory server.";
                    strServerPublicIP = "";
                }
            }
            else
            {
                if ( !strServerPublicIP.isEmpty() )
                {
                    QHostAddress InetAddr;
                    if ( !InetAddr.setAddress ( strServerPublicIP ) )
                    {
                        qWarning() << "Server Public IP is invalid. Only plain IP addresses are supported.";
                        strServerPublicIP = "";
                    }
                }
            }
        }

        if ( !strServerBindIP.isEmpty() )
        {
            QHostAddress InetAddr;
            if ( !InetAddr.setAddress ( strServerBindIP ) )
            {
                qWarning() << "Server Bind IP is invalid. Only plain IP addresses are supported.";
                strServerBindIP = "";
            }
        }
    }

    // Application/GUI setup ---------------------------------------------------
    // Application object
#ifdef HEADLESS
    QCoreApplication* pApp = new QCoreApplication ( argc, argv );
#else
#    if defined( Q_OS_IOS )
    bUseGUI        = true;
    bIsClient      = true; // Client only - TODO: maybe a switch in interface to change to server?

    // bUseMultithreading = true;
    QApplication* pApp = new QApplication ( argc, argv );
#    else
    QCoreApplication* pApp = bUseGUI ? new QApplication ( argc, argv ) : new QCoreApplication ( argc, argv );
#    endif
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

#if defined( Q_OS_MACX )
    // On OSX we need to declare an activity to ensure the process doesn't get
    // throttled by OS level Nap, Sleep, and Thread Priority systems.
    CActivity activity;

    activity.BeginActivity();
#endif

    // init resources
    Q_INIT_RESOURCE ( resources );

    // clang-format off
// TEST -> activate the following line to activate the test bench,
//CTestbench Testbench ( "127.0.0.1", DEFAULT_PORT_NUMBER );
    // clang-format on

    CRpcServer* pRpcServer = nullptr;

    if ( iJsonRpcPortNumber != INVALID_PORT )
    {
        if ( strJsonRpcSecretFileName.isEmpty() )
        {
            qCritical() << qUtf8Printable ( QString ( "- JSON-RPC: --jsonrpcsecretfile is required. Exiting." ) );
            exit ( 1 );
        }

        QFile qfJsonRpcSecretFile ( strJsonRpcSecretFileName );
        if ( !qfJsonRpcSecretFile.open ( QFile::OpenModeFlag::ReadOnly ) )
        {
            qCritical() << qUtf8Printable ( QString ( "- JSON-RPC: Unable to open secret file %1. Exiting." ).arg ( strJsonRpcSecretFileName ) );
            exit ( 1 );
        }
        QTextStream qtsJsonRpcSecretStream ( &qfJsonRpcSecretFile );
        QString     strJsonRpcSecret = qtsJsonRpcSecretStream.readLine();
        if ( strJsonRpcSecret.length() < JSON_RPC_MINIMUM_SECRET_LENGTH )
        {
            qCritical() << qUtf8Printable ( QString ( "JSON-RPC: Refusing to run with secret of length %1 (required: %2). Exiting." )
                                                .arg ( strJsonRpcSecret.length() )
                                                .arg ( JSON_RPC_MINIMUM_SECRET_LENGTH ) );
            exit ( 1 );
        }

        qWarning() << "- JSON-RPC: This interface is experimental and is subject to breaking changes even on patch versions "
                      "(not subject to semantic versioning) during the initial phase.";

        pRpcServer = new CRpcServer ( pApp, iJsonRpcPortNumber, strJsonRpcSecret );
        if ( !pRpcServer->Start() )
        {
            qCritical() << qUtf8Printable ( QString ( "- JSON-RPC: Server failed to start. Exiting." ) );
            exit ( 1 );
        }
    }

    try
    {
        if ( bIsClient )
        {
            // Client:
            // actual client object
            CClient Client ( iPortNumber,
                             iQosNumber,
                             strConnOnStartupAddress,
                             strMIDISetup,
                             bNoAutoJackConnect,
                             strClientName,
                             bEnableIPv6,
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

            if ( pRpcServer )
            {
                new CClientRpc ( &Client, pRpcServer, pRpcServer );
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
                                       bEnableIPv6,
                                       nullptr );

                // initialise message boxes
                CMsgBoxes::init ( &ClientDlg, strClientName.isEmpty() ? QString ( APP_NAME ) : QString ( APP_NAME ) + " " + strClientName );

                // show dialog
                ClientDlg.show();
                pApp->exec();
            }
            else
#endif
            {
                // only start application without using the GUI
                qInfo() << qUtf8Printable ( GetVersionAndNameStr ( false ) );

                pApp->exec();
            }
        }
        else
        {
            // Server:
            // actual server object
            CServer Server ( iNumServerChannels,
                             strLoggingFileName,
                             strServerBindIP,
                             iPortNumber,
                             iQosNumber,
                             strHTMLStatusFileName,
                             strDirectoryServer,
                             strServerListFileName,
                             strServerInfo,
                             strServerPublicIP,
                             strServerListFilter,
                             strWelcomeMessage,
                             strRecordingDirName,
                             bDisconnectAllClientsOnQuit,
                             bUseDoubleSystemFrameSize,
                             bUseMultithreading,
                             bDisableRecording,
                             bDelayPan,
                             bEnableIPv6,
                             eLicenceType );

            if ( pRpcServer )
            {
                new CServerRpc ( &Server, pRpcServer, pRpcServer );
            }

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

                // GUI object for the server
                CServerDlg ServerDlg ( &Server, &Settings, bStartMinimized, nullptr );

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
                qInfo() << qUtf8Printable ( GetVersionAndNameStr ( false ) );

                // CServerListManager defaults to AT_NONE, so need to switch if
                // strDirectoryServer is wanted
                if ( !strDirectoryServer.isEmpty() )
                {
                    Server.SetDirectoryType ( AT_CUSTOM );
                }

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
            CMsgBoxes::ShowError( generr.GetErrorText() );
        }
        else
#endif
        {
            qCritical() << qUtf8Printable ( QString ( "%1: %2" ).arg ( APP_NAME ).arg ( generr.GetErrorText() ) );
            exit ( 1 );
        }
    }

#if defined( Q_OS_MACX )
    activity.EndActivity();
#endif

    return 0;
}

/******************************************************************************\
* Command Line Argument Parsing                                                *
\******************************************************************************/
QString UsageArguments ( char** argv )
{
    // clang-format off
    return QString (
           "\n"
           "Usage: %1 [option] [option argument] ...\n"
           "\n"
           "  -h, -?, --help        display this help text and exit\n"
           "  -v, --version         display version information and exit\n"
           "\n"
           "Common options:\n"
           "  -i, --inifile         initialization file name\n"
           "                        (not supported for headless server mode)\n"
           "  -n, --nogui           disable GUI (\"headless\")\n"
           "  -p, --port            set the local port number\n"
           "      --jsonrpcport     enable JSON-RPC server, set TCP port number\n"
           "                        (EXPERIMENTAL, APIs might still change;\n"
           "                        only accessible from localhost)\n"
           "      --jsonrpcsecretfile\n"
           "                        path to a single-line file which contains a freely\n"
           "                        chosen secret to authenticate JSON-RPC users.\n"
           "  -Q, --qos             set the QoS value. Default is 128. Disable with 0\n"
           "                        (see the Jamulus website to enable QoS on Windows)\n"
           "  -t, --notranslation   disable translation (use English language)\n"
           "  -6, --enableipv6      enable IPv6 addressing (IPv4 is always enabled)\n"
           "\n"
           "Server only:\n"
           "  -d, --discononquit    disconnect all clients on quit\n"
           "  -e, --directoryserver address of the directory server with which to register\n"
           "                        (or 'localhost' to host a server list on this server)\n"
           "      --directoryfile   enable server list persistence, set file name\n"
           "  -f, --listfilter      server list whitelist filter.  Format:\n"
           "                        [IP address 1];[IP address 2];[IP address 3]; ...\n"
           "  -F, --fastupdate      use 64 samples frame size mode\n"
           "  -l, --log             enable logging, set file name\n"
           "  -L, --licence         show an agreement window before users can connect\n"
           "  -m, --htmlstatus      enable HTML status file, set file name\n"
           "  -o, --serverinfo      registration info for this server.  Format:\n"
           "                        [name];[city];[country as QLocale ID]\n"
           "      --serverpublicip  public IP address for this server.  Needed when\n"
           "                        registering with a server list hosted\n"
           "                        behind the same NAT\n"
           "  -P, --delaypan        start with delay panning enabled\n"
           "  -R, --recording       sets directory to contain recorded jams\n"
           "      --norecord        disables recording (when enabled by default by -R)\n"
           "  -s, --server          start server\n"
           "      --serverbindip    IP address the server will bind to (rather than all)\n"
           "  -T, --multithreading  use multithreading to make better use of\n"
           "                        multi-core CPUs and support more clients\n"
           "  -u, --numchannels     maximum number of channels\n"
           "  -w, --welcomemessage  welcome message to display on connect\n"
           "                        (string or filename)\n"
           "  -z, --startminimized  start minimizied\n"
           "\n"
           "Client only:\n"
           "  -c, --connect         connect to given server address on startup\n"
           "  -j, --nojackconnect   disable auto Jack connections\n"
           "  -M, --mutestream      starts the application in muted state\n"
           "      --mutemyown       mute me in my personal mix (headless only)\n"
           "      --clientname      client name (window title and jack client name)\n"
           "      --ctrlmidich      MIDI controller channel to listen\n"
           "\n"
           "Example: %1 -s --inifile myinifile.ini\n"
           "\n"
           "For more information and localized help see:\n"
           "https://jamulus.io/wiki/Command-Line-Options\n"
    ).arg( argv[0] );
    // clang-format on
}

bool CCommandlineOptions::GetFlagArgument ( int& i, const QString& strShortOpt, const QString& strLongOpt )
{
    if ( ( !strShortOpt.compare ( appArgv[i] ) ) || ( !strLongOpt.compare ( appArgv[i] ) ) )
    {
        return true;
    }
    else
    {
        return false;
    }
}

bool CCommandlineOptions::GetStringArgument ( int& i, const QString& strShortOpt, const QString& strLongOpt, QString& strArg )
{
    if ( ( !strShortOpt.compare ( appArgv[i] ) ) || ( !strLongOpt.compare ( appArgv[i] ) ) )
    {
        if ( ++i >= appArgc )
        {
            qCritical() << qUtf8Printable ( QString ( "%1: '%2' needs a string argument." ).arg ( appArgv[0] ).arg ( appArgv[i - 1] ) );
            exit ( 1 );
        }

        strArg = appArgv[i];

        return true;
    }
    else
    {
        return false;
    }
}

bool CCommandlineOptions::GetNumericArgument ( int&           i,
                                               const QString& strShortOpt,
                                               const QString& strLongOpt,
                                               double         rRangeStart,
                                               double         rRangeStop,
                                               double&        rValue )
{
    if ( ( !strShortOpt.compare ( appArgv[i] ) ) || ( !strLongOpt.compare ( appArgv[i] ) ) )
    {
        QString errmsg = "%1: '%2' needs a numeric argument from '%3' to '%4'.";
        if ( ++i >= appArgc )
        {
            qCritical() << qUtf8Printable ( errmsg.arg ( appArgv[0] ).arg ( appArgv[i - 1] ).arg ( rRangeStart ).arg ( rRangeStop ) );
            exit ( 1 );
        }

        char* p;
        rValue = strtod ( appArgv[i], &p );
        if ( *p || ( rValue < rRangeStart ) || ( rValue > rRangeStop ) )
        {
            qCritical() << qUtf8Printable ( errmsg.arg ( appArgv[0] ).arg ( appArgv[i - 1] ).arg ( rRangeStart ).arg ( rRangeStop ) );
            exit ( 1 );
        }

        return true;
    }
    else
    {
        return false;
    }
}
