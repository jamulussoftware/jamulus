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

#include "cmdline.h"
#include <QDebug>

/************************************************
  Statics to be assigned from main ()
 ************************************************/

int    CCommandline::argc = 0;
char** CCommandline::argv = NULL;

CCommandline::CCommandline ( tOnArgumentError* onArgumentErrorFunc ) : onArgumentError ( onArgumentErrorFunc )
{
    if ( ( argc == 0 ) || ( argv == NULL ) )
    {
        QString errmsg ( "CCommandline::argc and CCommandline::argv are not initialized from main() !" );

        if ( onArgumentErrorFunc )
        {
            onArgumentErrorFunc ( errmsg );
        }
        else
        {
            qCritical() << errmsg;
            exit ( 1 );
        }
    }

    reset();
}

/************************************************
 Get full command line as a single string
 ************************************************/

QString CCommandline::Commandline()
{
    QString cmdline = argv[0];

    for ( int i = 1; i < argc; i++ )
    {
        cmdline += " ";
        cmdline += argv[i];
    }

    return cmdline;
}

QString CCommandline::Argument ( int i ) // i = 1..ArgumentCount()
{
    if ( ( i >= 0 ) && ( i < argc ) )
    {
        return QString ( argv[i] );
    }

    return QString();
}

/************************************************
 sequencial parse functions
 using the argument index: 1 <= i < argc
 ************************************************/

bool CCommandline::GetFlagArgument ( int& i, const QString& strShortOpt, const QString& strLongOpt )
{
    if ( ( !strShortOpt.compare ( argv[i] ) ) || ( !strLongOpt.compare ( argv[i] ) ) )
    {
        return true;
    }
    else
    {
        return false;
    }
}

bool CCommandline::GetStringArgument ( int& i, const QString& strShortOpt, const QString& strLongOpt, QString& strArg )
{
    if ( ( !strShortOpt.compare ( argv[i] ) ) || ( !strLongOpt.compare ( argv[i] ) ) )
    {
        if ( ++i >= argc )
        {
            if ( onArgumentError )
            {
                onArgumentError ( QString ( "%1: '%2' needs a string argument." ).arg ( argv[0] ).arg ( argv[i - 1] ) );
            }

            return false;
        }

        strArg = argv[i];

        return true;
    }
    else
    {
        return false;
    }
}

bool CCommandline::GetNumericArgument ( int&           i,
                                        const QString& strShortOpt,
                                        const QString& strLongOpt,
                                        double         rRangeStart,
                                        double         rRangeStop,
                                        double&        rValue )
{
    if ( ( !strShortOpt.compare ( argv[i] ) ) || ( !strLongOpt.compare ( argv[i] ) ) )
    {
        QString errmsg = "%1: '%2' needs a numeric argument from '%3' to '%4'.";

        if ( ++i >= argc )
        {
            if ( onArgumentError )
            {
                onArgumentError ( errmsg.arg ( argv[0] ).arg ( argv[i - 1] ).arg ( rRangeStart ).arg ( rRangeStop ) );
            }

            return false;
        }

        char* p;
        rValue = strtod ( argv[i], &p );
        if ( *p || ( rValue < rRangeStart ) || ( rValue > rRangeStop ) )
        {
            if ( onArgumentError )
            {
                onArgumentError ( errmsg.arg ( argv[0] ).arg ( argv[i - 1] ).arg ( rRangeStart ).arg ( rRangeStop ) );
            }

            if ( rValue < rRangeStart )
            {
                rValue = rRangeStart;
            }
            else if ( rValue > rRangeStop )
            {
                rValue = rRangeStop;
            }

            return true;
        }

        return true;
    }
    else
    {
        return false;
    }
}

/************************************************
 find and get a specific argument:
 ************************************************/

bool CCommandline::GetFlagArgument ( const QString& strShortOpt, const QString& strLongOpt )
{
    for ( int i = 1; i < argc; i++ )
    {
        if ( GetFlagArgument ( i, strShortOpt, strLongOpt ) )
        {
            return true;
        }
    }

    return false;
}

bool CCommandline::GetStringArgument ( const QString& strShortOpt, const QString& strLongOpt, QString& strArg )
{
    for ( int i = 1; i < argc; i++ )
    {
        if ( GetStringArgument ( i, strShortOpt, strLongOpt, strArg ) )
        {
            return true;
        }
    }

    return false;
}

bool CCommandline::GetNumericArgument ( const QString& strShortOpt, const QString& strLongOpt, double rRangeStart, double rRangeStop, double& rValue )
{
    for ( int i = 1; i < argc; i++ )
    {
        if ( GetNumericArgument ( i, strShortOpt, strLongOpt, rRangeStart, rRangeStop, rValue ) )
        {
            return true;
        }
    }

    return false;
}
