/******************************************************************************\
 * Copyright (c) 2004-2023
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

/* Classes ********************************************************************/
class CServerLogging
{
public:
    CServerLogging() : bDoLogging ( false ), File ( DEFAULT_LOG_FILE_NAME ) {}

    virtual ~CServerLogging();

    void Start ( const QString& strLoggingFileName );
    void AddServerStopped();

    void AddNewConnection ( const QHostAddress& ClientInetAddr, const int iNumberOfConnectedClients );

protected:
    void    operator<< ( const QString& sNewStr );
    QString CurTimeDatetoLogString();

    bool  bDoLogging;
    QFile File;
};
