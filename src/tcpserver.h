/******************************************************************************\
 * Copyright (c) 2024
 *
 * Author(s):
 *  Tony Mountifield
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

#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTimer>
#include <QVector>
#include <memory>

#include "global.h"
#include "util.h"

// The header files channel.h and server.h require to include this header file
// so we get a cyclic dependency. To solve this issue, a prototype of the
// channel class and server class is defined here.
class CServer;        // forward declaration of CServer
class CChannel;       // forward declaration of CChannel
class CTcpConnection; // forward declaration of CTcpConnection

/* Classes ********************************************************************/
class CTcpServer : public QObject
{
    Q_OBJECT

public:
    CTcpServer ( CServer* pNServP, const QString& strServerBindIP, int iPort, bool bEnableIPv6 );
    virtual ~CTcpServer();

    bool Start();

private:
    CServer*      pServer; // for server
    const QString strServerBindIP;
    const int     iPort;
    const bool    bEnableIPv6;
    QTcpServer*   pTcpServer;

signals:
    void ProtocolCLMessageReceived ( int iRecID, CVector<uint8_t> vecbyMesBodyData, CHostAddress HostAdr, CTcpConnection* pTcpConnection );

protected slots:
    void OnNewConnection();
};

class CTcpConnection
{
public:
    CTcpConnection ( QTcpSocket* pTcpSocket, const CHostAddress& tcpAddress ) : pTcpSocket ( pTcpSocket ), tcpAddress ( tcpAddress ) {}
    ~CTcpConnection() {}

    QTcpSocket*  pTcpSocket;
    CHostAddress tcpAddress;
    CHostAddress udpAddress;
};
