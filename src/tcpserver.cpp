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

#include "tcpserver.h"

#include "protocol.h"
#include "server.h"
#include "channel.h"

CTcpServer::CTcpServer ( CServer* pNServP, const QString& strServerBindIP, int iPort, bool bEnableIPv6 ) :
    pServer ( pNServP ),
    strServerBindIP ( strServerBindIP ),
    iPort ( iPort ),
    bEnableIPv6 ( bEnableIPv6 ),
    pTcpServer ( new QTcpServer ( this ) )
{
    connect ( this, &CTcpServer::ProtocolCLMessageReceived, pServer, &CServer::OnProtocolCLMessageReceived );
    connect ( pTcpServer, &QTcpServer::newConnection, this, &CTcpServer::OnNewConnection );
}

CTcpServer::~CTcpServer()
{
    if ( pTcpServer->isListening() )
    {
        qInfo() << "- stopping Jamulus-TCP server";
        pTcpServer->close();
    }
}

bool CTcpServer::Start()
{
    if ( iPort < 0 )
    {
        return false;
    }

    // default to any-address for either both IP protocols or just IPv4
    QHostAddress hostAddress = bEnableIPv6 ? QHostAddress::Any : QHostAddress::AnyIPv4;

    if ( !bEnableIPv6 )
    {
        if ( !strServerBindIP.isEmpty() )
        {
            hostAddress = QHostAddress ( strServerBindIP );
        }
    }

    if ( pTcpServer->listen ( hostAddress, iPort ) )
    {
        qInfo() << qUtf8Printable (
            QString ( "- Jamulus-TCP: Server started on %1:%2" ).arg ( pTcpServer->serverAddress().toString() ).arg ( pTcpServer->serverPort() ) );
        return true;
    }
    qInfo() << "- Jamulus-TCP: Unable to start server:" << pTcpServer->errorString();
    return false;
}

void CTcpServer::OnNewConnection()
{
    QTcpSocket* pSocket = pTcpServer->nextPendingConnection();
    if ( !pSocket )
    {
        return;
    }

    // express IPv4 address as IPv4
    CHostAddress peerAddress ( pSocket->peerAddress(), pSocket->peerPort() );

    if ( peerAddress.InetAddr.protocol() == QAbstractSocket::IPv6Protocol )
    {
        bool    ok;
        quint32 ip4 = peerAddress.InetAddr.toIPv4Address ( &ok );
        if ( ok )
        {
            peerAddress.InetAddr.setAddress ( ip4 );
        }
    }

    CTcpConnection* pTcpConnection = new CTcpConnection ( pSocket, peerAddress );

    qDebug() << "- Jamulus-TCP: received connection from:" << peerAddress.InetAddr.toString();

    // allocate memory for network receive and send buffer in samples
    CVector<uint8_t> vecbyRecBuf;
    vecbyRecBuf.Init ( MAX_SIZE_BYTES_NETW_BUF );

    connect ( pSocket, &QTcpSocket::disconnected, [this, pTcpConnection]() {
        qDebug() << "- Jamulus-TCP: connection from:" << pTcpConnection->tcpAddress.InetAddr.toString() << "closed";
        pTcpConnection->pTcpSocket->deleteLater();
        delete pTcpConnection;
    } );

    connect ( pSocket, &QTcpSocket::readyRead, [this, pTcpConnection, vecbyRecBuf]() {
        // handle received Jamulus protocol packet

        // check if this is a protocol message
        int              iRecCounter;
        int              iRecID;
        CVector<uint8_t> vecbyMesBodyData;

        long iNumBytesRead = pTcpConnection->pTcpSocket->read ( (char*) &vecbyRecBuf[0], MESS_HEADER_LENGTH_BYTE );
        if ( iNumBytesRead == -1 )
        {
            return;
        }

        if ( iNumBytesRead < MESS_HEADER_LENGTH_BYTE )
        {
            qDebug() << "-- short read: expected" << MESS_HEADER_LENGTH_BYTE << "bytes, got" << iNumBytesRead;
            return;
        }

        long iPayloadLength = CProtocol::GetBodyLength ( vecbyRecBuf );

        long iNumBytesRead2 = pTcpConnection->pTcpSocket->read ( (char*) &vecbyRecBuf[MESS_HEADER_LENGTH_BYTE], iPayloadLength );
        if ( iNumBytesRead2 == -1 )
        {
            return;
        }

        if ( iNumBytesRead2 < iPayloadLength )
        {
            qDebug() << "-- short read: expected" << iPayloadLength << "bytes, got" << iNumBytesRead2;
            return;
        }

        iNumBytesRead += iNumBytesRead2;

        qDebug() << "- Jamulus-TCP: received protocol message of length" << iNumBytesRead;

        if ( !CProtocol::ParseMessageFrame ( vecbyRecBuf, iNumBytesRead, vecbyMesBodyData, iRecCounter, iRecID ) )
        {
            qDebug() << "- Jamulus-TCP: message parsed OK, ID =" << iRecID;

            // this is a protocol message, check the type of the message
            if ( CProtocol::IsConnectionLessMessageID ( iRecID ) )
            {
                //### TODO: BEGIN ###//
                // a copy of the vector is used -> avoid malloc in real-time routine
                emit ProtocolCLMessageReceived ( iRecID, vecbyMesBodyData, pTcpConnection->tcpAddress, pTcpConnection );
                //### TODO: END ###//
            }
            else
            {
                //### TODO: BEGIN ###//
                // a copy of the vector is used -> avoid malloc in real-time routine
                // emit ProtocolMessageReceived ( iRecCounter, iRecID, vecbyMesBodyData, pTcpConnection->tcpAddress, pTcpConnection );
                //### TODO: END ###//
            }
        }
    } );
}

#if 0
void CTcpServer::Send ( QTcpSocket* pSocket ) {
    // pSocket->write ( );
}
#endif
