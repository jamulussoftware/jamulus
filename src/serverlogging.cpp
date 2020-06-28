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

void CServerLogging::EnableHistory ( const QString& strHistoryFileName )
{
#ifndef HEADLESS
    if ( strHistoryFileName.right ( 4 ).compare ( ".svg", Qt::CaseInsensitive ) != 0 )
    {
        JpegHistoryGraph.Start ( strHistoryFileName );
    }
    else
#endif
    {
        SvgHistoryGraph.Start ( strHistoryFileName );
    }
}

void CServerLogging::AddNewConnection ( const QHostAddress& ClientInetAddr )
{
    // logging of new connected channel
    const QString strLogStr = CurTimeDatetoLogString() + ", " +
        ClientInetAddr.toString() + ", connected";

    QTextStream& tsConsoleStream = *( ( new ConsoleWriterFactory() )->get() );
    tsConsoleStream << strLogStr << endl; // on console
    *this << strLogStr; // in log file

    // add element to history
#ifndef HEADLESS
    JpegHistoryGraph.Add ( QDateTime::currentDateTime(), ClientInetAddr );
#endif
    SvgHistoryGraph.Add ( QDateTime::currentDateTime(), ClientInetAddr );
}

void CServerLogging::AddDisconnection ( const QHostAddress& ClientInetAddr )
{
    // logging of disconnected channel
    const QString strLogStr = CurTimeDatetoLogString() + ", " +
        ClientInetAddr.toString() + ", Disconnected";

    QTextStream& tsConsoleStream = *( ( new ConsoleWriterFactory() )->get() );
    tsConsoleStream << strLogStr << endl; // on console
    *this << strLogStr; // in log file

    // add element to history
#ifndef HEADLESS
    JpegHistoryGraph.Add ( QDateTime::currentDateTime(), ClientInetAddr );
#endif
    SvgHistoryGraph.Add ( QDateTime::currentDateTime(), ClientInetAddr );
}

void CServerLogging::AddServerStopped()
{
    const QString strLogStr = CurTimeDatetoLogString() + ",, server stopped "
        "-------------------------------------";

    QTextStream& tsConsoleStream = *( ( new ConsoleWriterFactory() )->get() );
    tsConsoleStream << strLogStr << endl; // on console
    *this << strLogStr; // in log file

    // add element to history and update on server stop
#ifndef HEADLESS
    JpegHistoryGraph.Add ( QDateTime::currentDateTime(), AHistoryGraph::HIT_SERVER_STOP );
#endif
    SvgHistoryGraph.Add ( QDateTime::currentDateTime(), AHistoryGraph::HIT_SERVER_STOP );

#ifndef HEADLESS
    JpegHistoryGraph.Update();
#endif
    SvgHistoryGraph.Update();
}

void CServerLogging::operator<< ( const QString& sNewStr )
{
    if ( bDoLogging )
    {
        // append new line in logging file
        QTextStream out ( &File );
        out << sNewStr << endl;
        File.flush();
    }
}

void CServerLogging::ParseLogFile ( const QString& strFileName )
{
    // open file for reading
    QFile LogFile ( strFileName );
    LogFile.open ( QIODevice::ReadOnly | QIODevice::Text );

    QTextStream inStream ( &LogFile );

    // read all content from file
    while ( !inStream.atEnd() )
    {
        // get a new line from log file
        QString strCurLine = inStream.readLine();

        // parse log file line
        QStringList strlistCurLine = strCurLine.split ( "," );

        // check number of separated strings condition
        if ( strlistCurLine.size() == 3 )
        {
            // first entry
            QDateTime curDateTime =
                QDateTime::fromString ( strlistCurLine.at ( 0 ).trimmed(),
                "yyyy-MM-dd HH:mm:ss" );

            if ( curDateTime.isValid() )
            {
                // check if server stop or new client connection
                QString strAddress = strlistCurLine.at ( 1 ).trimmed();

                if ( strAddress.isEmpty() )
                {
                    // server stop
#ifndef HEADLESS
                    JpegHistoryGraph.Add ( curDateTime, AHistoryGraph::HIT_SERVER_STOP );
#endif
                    SvgHistoryGraph.Add ( curDateTime, CSvgHistoryGraph::HIT_SERVER_STOP );
                }
                else
                {
                    QHostAddress curAddress;
                    
                    // second entry is IP address
                    if ( curAddress.setAddress ( strlistCurLine.at ( 1 ).trimmed() ) )
                    {
                        // new client connection
#ifndef HEADLESS
                        JpegHistoryGraph.Add ( curDateTime, curAddress );
#endif
                        SvgHistoryGraph.Add ( curDateTime, curAddress );
                    }
                }
            }
        }
    }

#ifndef HEADLESS
    JpegHistoryGraph.Update();
#endif
    SvgHistoryGraph.Update();
}

QString CServerLogging::CurTimeDatetoLogString()
{
    // time and date to string conversion
    const QDateTime curDateTime = QDateTime::currentDateTime();

    // format date and time output according to "2006-09-30 11:38:08"
    return curDateTime.toString("yyyy-MM-dd HH:mm:ss");
}
