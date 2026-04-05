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

/* Classes ********************************************************************/
class CTcpConnection : public QObject
{
    Q_OBJECT

public:
    CTcpConnection ( QTcpSocket* pTcpSocket, const CHostAddress& tcpAddress, CServer* pServer, CChannel* pChannel, bool bDisconAfterRecv = false );
    ~CTcpConnection() {}

    void      SetChannel ( CChannel* pChan ) { pChannel = pChan; }
    CChannel* GetChannel() { return pChannel; }

    qint64 write ( const char* data, qint64 maxSize );
    void   disconnectFromHost();

private:
    QTcpSocket*  pTcpSocket;
    CHostAddress tcpAddress;
    CHostAddress udpAddress;

    CServer*  pServer;
    CChannel* pChannel;

    const bool bDisconAfterRecv;

    int              iPos;
    int              iPayloadRemain;
    CVector<uint8_t> vecbyRecBuf;

signals:
    void ProtocolCLMessageReceived ( int iRecID, CVector<uint8_t> vecbyMesBodyData, CHostAddress HostAdr, CTcpConnection* pTcpConnection );

private slots:
    void OnDisconnected();
    void OnReadyRead();
};
