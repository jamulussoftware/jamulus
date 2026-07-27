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

#include "tcpserver.h"

#include "server.h"

CTcpServer::CTcpServer ( CServer*       pNServP,
                         const QString& strServerBindIP4,
                         const QString& strServerBindIP6,
                         int            iPort,
                         bool&          bTCPv4Available,
                         bool&          bTCPv6Available ) :
    pServer ( pNServP ),
    strServerBindIP4 ( strServerBindIP4 ),
    strServerBindIP6 ( strServerBindIP6 ),
    iPort ( iPort ),
    bTCPv4Available ( bTCPv4Available ),
    bTCPv6Available ( bTCPv6Available ),
    pTcpServer4 ( new QTcpServer ( this ) ),
    pTcpServer6 ( new QTcpServer ( this ) )
{
    connect ( pTcpServer4, &QTcpServer::newConnection, this, [this]() { AcceptConnections ( pTcpServer4 ); } );
    connect ( pTcpServer6, &QTcpServer::newConnection, this, [this]() { AcceptConnections ( pTcpServer6 ); } );
}

CTcpServer::~CTcpServer()
{
    if ( pTcpServer4->isListening() )
    {
        bTCPv4Available = false; // this is a reference to CServer::bTCPv4Available
        qInfo() << "- stopping Jamulus-TCP IPv4 server";
        pTcpServer4->close();
    }

    if ( pTcpServer6->isListening() )
    {
        bTCPv6Available = false; // this is a reference to CServer::bTCPv4Available
        qInfo() << "- stopping Jamulus-TCP IPv6 server";
        pTcpServer6->close();
    }
}

bool CTcpServer::Start()
{
    if ( iPort < 0 )
    {
        return false;
    }

    QHostAddress hostAddress;

    // IPv4 socket
    if ( !strServerBindIP4.isEmpty() )
    {
        hostAddress = QHostAddress ( strServerBindIP4 );
    }
    else
    {
        hostAddress = QHostAddress::AnyIPv4;
    }

    if ( hostAddress.protocol() == QAbstractSocket::IPv4Protocol && pTcpServer4->listen ( hostAddress, iPort ) )
    {
        bTCPv4Available = true; // this is a reference to CServer::bTCPv4Available

        qInfo() << qUtf8Printable (
            QString ( "- Jamulus-TCP: IPv4 server started on %1:%2" ).arg ( hostAddress.toString() ).arg ( pTcpServer4->serverPort() ) );
    }
    else
    {
        qWarning() << qUtf8Printable ( QString ( "- Jamulus-TCP: unable to start IPv4 server on %1:%2 - %3" )
                                           .arg ( hostAddress.toString() )
                                           .arg ( iPort )
                                           .arg ( pTcpServer4->errorString() ) );
    }

    if ( pServer->IsIPv6Available() )
    {
        // IPv6 socket
        if ( !strServerBindIP6.isEmpty() )
        {
            hostAddress = QHostAddress ( strServerBindIP6 );
        }
        else
        {
            hostAddress = QHostAddress::AnyIPv6;
        }

        if ( hostAddress.protocol() == QAbstractSocket::IPv6Protocol && pTcpServer6->listen ( hostAddress, iPort ) )
        {
            bTCPv6Available = true; // this is a reference to CServer::bTCPv6Available

            qInfo() << qUtf8Printable (
                QString ( "- Jamulus-TCP: IPv6 server started on [%1]:%2" ).arg ( hostAddress.toString() ).arg ( pTcpServer6->serverPort() ) );
        }
        else
        {
            qWarning() << qUtf8Printable ( QString ( "- Jamulus-TCP: unable to start IPv6 server on [%1]:%2 - %3" )
                                               .arg ( hostAddress.toString() )
                                               .arg ( iPort )
                                               .arg ( pTcpServer6->errorString() ) );
        }
    }
    return false;
}

void CTcpServer::AcceptConnections ( QTcpServer* pTcpServer )
{
    while ( pTcpServer->hasPendingConnections() )
    {
        QTcpSocket* const pSocket = pTcpServer->nextPendingConnection();
        if ( pSocket )
        {
            CHostAddress peerAddress ( pSocket->peerAddress(), pSocket->peerPort() );

            qDebug() << "- Jamulus-TCP: received connection from:" << peerAddress.toString();

            new CTcpConnection ( pSocket, peerAddress, pServer ); // will auto-delete on disconnect
        }
    }
}
