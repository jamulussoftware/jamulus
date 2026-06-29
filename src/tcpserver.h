/******************************************************************************\
 * Copyright (c) 2024-2026
 *
 * Author(s):
 *  Tony Mountifield
 *
 ******************************************************************************
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
\******************************************************************************/

#pragma once

#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTimer>
#include <QVector>
#include <memory>

#include "tcpconnection.h"

#include "global.h"
#include "util.h"

// The header file server.h requires to include this header file
// so we get a cyclic dependency. To solve this issue, a prototype of the
// server class is defined here.
class CServer; // forward declaration of CServer

/* Classes ********************************************************************/
class CTcpServer : public QObject
{
    Q_OBJECT

public:
    CTcpServer ( CServer* pNServP, const QString& strServerBindIP, int iPort );
    ~CTcpServer();

    bool Start();

private:
    CServer*      pServer; // for server
    const QString strServerBindIP;
    const int     iPort;
    QTcpServer*   pTcpServer;

private slots:
    void OnNewConnection();
};
