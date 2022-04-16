#pragma once

/******************************************************************************\
 * Copyright (c) 2022
 *
 * Author(s):
 *  Peter Goderie (pgScorpio)
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

#include <QString>

/******************************************************************************
 CCommandline class:
   Note that passing commandline arguments to classes is no longer required,
   since via this class we can get commandline options anywhere.
 ******************************************************************************/

class CCommandline
{
public:
    typedef void tOnArgumentError ( QString errMsg );

    CCommandline ( tOnArgumentError* onArgumentErrorFunc = NULL );

private:
    friend int main ( int argc, char** argv );

    /************************************************
     Statics to be assigned in main ()
     ************************************************/
    static int    argc;
    static char** argv;

protected:
    tOnArgumentError* onArgumentError;

public:
    static QString GetProgramPath() { return QString ( *argv ); }

    static int     ArgumentCount() { return ( argc - 1 ); } // excludes program path
    static QString Argument ( int i );                      // i = 1..ArgumentCount()

    static QString Commandline();

public:
    // sequencial parse functions using the argument index:

    bool GetFlagArgument ( int& i, const QString& strShortOpt, const QString& strLongOpt );

    bool GetStringArgument ( int& i, const QString& strShortOpt, const QString& strLongOpt, QString& strArg );

    bool GetNumericArgument ( int& i, const QString& strShortOpt, const QString& strLongOpt, double rRangeStart, double rRangeStop, double& rValue );

public:
    // find and get a specific argument:
    bool GetFlagArgument ( const QString& strShortOpt, const QString& strLongOpt );
    bool GetStringArgument ( const QString& strShortOpt, const QString& strLongOpt, QString& strArg );
    bool GetNumericArgument ( const QString& strShortOpt, const QString& strLongOpt, double rRangeStart, double rRangeStop, double& rValue );

    /************************************************
     parse bare arguments
     ************************************************/
protected:
    int    currentIndex;
    char** currentArgv;

    void reset()
    {
        currentArgv  = argv;
        currentIndex = 0;
    }

public:
    QString GetFirstArgument()
    {
        reset();
        // Skipping program path
        return GetNextArgument();
    }

    QString GetNextArgument()
    {
        if ( currentIndex < argc )
        {
            currentArgv++;
            currentIndex++;

            if ( currentIndex < argc )
            {
                return QString ( *currentArgv );
            }
        }

        return QString();
    }
};

/********************************************************************
 defines for commandline options in the style "shortopt", "longopt"
 Name is standard CMDLN_LONGOPTNAME
 These defines can be used for strShortOpt, strLongOpt parameters
 of the CCommandline functions.
 ********************************************************************/

// clang-format off

#define CMDLN_INIFILE             "-i",                    "--inifile"
#define CMDLN_NOGUI               "-n",                    "--nogui"
#define CMDLN_PORT                "-p",                    "--port"
#define CMDLN_JSONRPCPORT         "--jsonrpcport",         "--jsonrpcport"
#define CMDLN_JSONRPCSECRETFILE   "--jsonrpcsecretfile",   "--jsonrpcsecretfile"
#define CMDLN_QOS                 "-Q",                    "--qos"
#define CMDLN_NOTRANSLATION       "-t",                    "--notranslation"
#define CMDLN_ENABLEIPV6          "-6",                    "--enableipv6"
#define CMDLN_DISCONONQUIT        "-d",                    "--discononquit"
#define CMDLN_DIRECTORYSERVER     "-e",                    "--directoryserver"
#define CMDLN_DIRECTORYFILE       "--directoryfile",       "--directoryfile"
#define CMDLN_LISTFILTER          "-f",                    "--listfilter"
#define CMDLN_FASTUPDATE          "-F",                    "--fastupdate"
#define CMDLN_LOG                 "-l",                    "--log"
#define CMDLN_LICENCE             "-L",                    "--licence"
#define CMDLN_HTMLSTATUS          "-m",                    "--htmlstatus"
#define CMDLN_SERVERINFO          "-o",                    "--serverinfo"
#define CMDLN_SERVERPUBLICIP      "--serverpublicip",      "--serverpublicip"
#define CMDLN_DELAYPAN            "-P",                    "--delaypan"
#define CMDLN_RECORDING           "-R",                    "--recording"
#define CMDLN_NORECORD            "--norecord",            "--norecord"
#define CMDLN_SERVER              "-s",                    "--server"
#define CMDLN_SERVERBINDIP        "--serverbindip",        "--serverbindip"
#define CMDLN_MULTITHREADING      "-T",                    "--multithreading"
#define CMDLN_NUMCHANNELS         "-u",                    "--numchannels"
#define CMDLN_WELCOMEMESSAGE      "-w",                    "--welcomemessage"
#define CMDLN_STARTMINIMIZED      "-z",                    "--startminimized"
#define CMDLN_CONNECT             "-c",                    "--connect"
#define CMDLN_NOJACKCONNECT       "-j",                    "--nojackconnect"
#define CMDLN_MUTESTREAM          "-M",                    "--mutestream"
#define CMDLN_MUTEMYOWN           "--mutemyown",           "--mutemyown"
#define CMDLN_CLIENTNAME          "--clientname",          "--clientname"
#define CMDLN_CTRLMIDICH          "--ctrlmidich",          "--ctrlmidich"

// Backwards compatibility options:
#define CMDLN_CENTRALSERVER       "--centralserver",       "--centralserver"

// Debug options: (not in help)
#define CMDLN_SHOWALLSERVERS      "--showallservers",      "--showallservers"
#define CMDLN_SHOWANALYZERCONSOLE "--showanalyzerconsole", "--showanalyzerconsole"

// CMDLN_SPECIAL: Used for debugging, should NOT be in help, nor documented elsewhere!
// any option after --special is accepted
#define CMDLN_SPECIAL             "--special",             "--special"

// Special options for sound-redesign testing
#define CMDLN_JACKINPUTS          "--jackinputs",          "--jackinputs"

// clang-format on
