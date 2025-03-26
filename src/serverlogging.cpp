/******************************************************************************\
 * Copyright (c) 2004-2025
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

#include "serverlogging.h"

// Server logging --------------------------------------------------------------
CServerLogging::~CServerLogging()
{
    // close logging file of open
    if ( File.isOpen() )
    {
        File.close();
    }
}

void CServerLogging::Start ( const QString& strLoggingFileName )
{
    // open file
    File.setFileName ( strLoggingFileName );

    if ( File.open ( QIODevice::Append | QIODevice::Text ) )
    {
        bDoLogging = true;
    }
}

void CServerLogging::AddNewConnection ( const QHostAddress& ClientInetAddr, const int iNumberOfConnectedClients )
{
    // logging of new connected channel
    const QString strLogStr =
        CurTimeDatetoLogString() + ", " + ClientInetAddr.toString() + ", connected (" + QString::number ( iNumberOfConnectedClients ) + ")";

    qInfo() << qUtf8Printable ( strLogStr ); // on console
    *this << strLogStr;                      // in log file
}

void CServerLogging::AddServerStopped()
{
    const QString strLogStr = CurTimeDatetoLogString() + ",, server idling "
                                                         "-------------------------------------";

    qInfo() << qUtf8Printable ( strLogStr ); // on console
    *this << strLogStr;                      // in log file
}

void CServerLogging::operator<< ( const QString& sNewStr )
{
    if ( bDoLogging )
    {
        // append new line in logging file
        QTextStream out ( &File );
        out << sNewStr << '\n';
        out.flush();
    }
}

QString CServerLogging::CurTimeDatetoLogString()
{
    // time and date to string conversion
    const QDateTime curDateTime = QDateTime::currentDateTime();

    // format date and time output according to "2006-09-30 11:38:08"
    return curDateTime.toString ( "yyyy-MM-dd HH:mm:ss" );
}
