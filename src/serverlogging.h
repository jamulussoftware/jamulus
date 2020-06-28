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

#pragma once

#include <QDateTime>
#include <QHostAddress>
#include <QFile>
#include <QString>
#include <QTimer>
#include "global.h"
#include "util.h"

#include "historygraph.h"

/* Classes ********************************************************************/
class CServerLogging
{
public:
    CServerLogging ( const int iMaxDaysHistory ) :
#ifndef HEADLESS
        JpegHistoryGraph ( iMaxDaysHistory ),
#endif
        SvgHistoryGraph ( iMaxDaysHistory ),
        bDoLogging ( false ),
        File ( DEFAULT_LOG_FILE_NAME ) {}

    virtual ~CServerLogging();

    void Start ( const QString& strLoggingFileName );
    void EnableHistory ( const QString& strHistoryFileName );
    void AddNewConnection ( const QHostAddress& ClientInetAddr );
    void AddDisconnection ( const QHostAddress& ClientInetAddr );
    void AddServerStopped();
    void ParseLogFile ( const QString& strFileName );

protected:
    void operator<< ( const QString& sNewStr );
    QString CurTimeDatetoLogString();

#ifndef HEADLESS
    CJpegHistoryGraph JpegHistoryGraph;
#endif
    CSvgHistoryGraph  SvgHistoryGraph;
    bool              bDoLogging;
    QFile             File;
};
