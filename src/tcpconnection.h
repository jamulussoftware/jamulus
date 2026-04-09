/******************************************************************************\
 * Copyright (c) 2024-2026
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
// so we get a cyclic dependency. To solve this issue, prototypes of the
// channel class and server class are defined here.
class CServer;  // forward declaration of CServer
class CChannel; // forward declaration of CChannel

#define TCP_CONNECT_TIMEOUT_MS    3000
#define TCP_KEEPALIVE_INTERVAL_MS 15000

/* Classes ********************************************************************/
class CTcpConnection : public QObject
{
    Q_OBJECT

public:
#ifndef SERVER_ONLY
    CTcpConnection ( QTcpSocket* pTcpSocket, const CHostAddress& tcpAddress, CClient* pClient, CChannel* pChannel, bool bIsSession );
#endif
    CTcpConnection ( QTcpSocket* pTcpSocket, const CHostAddress& tcpAddress, CServer* pServer );
    ~CTcpConnection() {}

    void      SetChannel ( CChannel* pChan ) { pChannel = pChan; }
    CChannel* GetChannel() { return pChannel; }

    qint64 write ( const char* data, qint64 maxSize );
    void   disconnectFromHost();

    bool IsSession() { return bIsSession; }

private:
    QTcpSocket*  pTcpSocket;
    CHostAddress tcpAddress;

    CServer*  pServer;
    CChannel* pChannel;

    const bool bIsSession;

    int              iPos;
    int              iPayloadRemain;
    CVector<uint8_t> vecbyRecBuf;

    QTimer TimerKeepalive;

signals:
    void ProtocolCLMessageReceived ( int iRecID, CVector<uint8_t> vecbyMesBodyData, CHostAddress HostAdr, CTcpConnection* pTcpConnection );
    void CLSendEmptyMes ( CHostAddress InetAddr, CTcpConnection* pTcpConnection );

private slots:
    void OnDisconnected();
    void OnReadyRead();
    void OnTimerKeepalive();
};
