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
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
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
        JpegHistoryGraph ( iMaxDaysHistory ),
        SvgHistoryGraph ( iMaxDaysHistory ),
        bDoLogging ( false ),
        File ( DEFAULT_LOG_FILE_NAME ) {}

    virtual ~CServerLogging();

    void Start ( const QString& strLoggingFileName );
    void EnableHistory ( const QString& strHistoryFileName );
    void AddNewConnection ( const QHostAddress& ClientInetAddr );
    void AddServerStopped();
    void ParseLogFile ( const QString& strFileName );

protected:
    void operator<< ( const QString& sNewStr );
    QString CurTimeDatetoLogString();

    CJpegHistoryGraph JpegHistoryGraph;
    CSvgHistoryGraph  SvgHistoryGraph;
    bool              bDoLogging;
    QFile             File;
};
