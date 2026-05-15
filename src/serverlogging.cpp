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
#include <juce_core/juce_core.h>

// Server logging --------------------------------------------------------------
CServerLogging::~CServerLogging()
{
    // close logging file of open
    if ( File.is_open() )
    {
        File.close();
    }
}

void CServerLogging::Start ( const QString& strLoggingFileName )
{
    File.open ( strLoggingFileName.toStdWString(), std::ios::out | std::ios::app | std::ios::binary );
    if ( File.is_open() ) { bDoLogging = true; }
}

void CServerLogging::AddNewConnection ( const CHostAddress& ClientInetAddr, const int iNumberOfConnectedClients )
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
        const QByteArray bytes = sNewStr.toUtf8();
        File.write ( bytes.constData(), static_cast<std::streamsize> ( bytes.size() ) );
        File.put ( '\n' );
        File.flush();
    }
}

QString CServerLogging::CurTimeDatetoLogString()
{
    const juce::String formatted = juce::Time::getCurrentTime().formatted ( "%Y-%m-%d %H:%M:%S" );
    return QString::fromUtf8 ( formatted.toRawUTF8() );
}
