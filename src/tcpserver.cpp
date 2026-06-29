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

CTcpServer::CTcpServer ( CServer* pNServP, const QString& strServerBindIP, int iPort ) :
    pServer ( pNServP ),
    strServerBindIP ( strServerBindIP ),
    iPort ( iPort ),
    pTcpServer ( new QTcpServer ( this ) )
{
    connect ( pTcpServer, &QTcpServer::newConnection, this, &CTcpServer::OnNewConnection );
}

CTcpServer::~CTcpServer()
{
    if ( pTcpServer->isListening() )
    {
        qInfo() << "- stopping Jamulus-TCP server";
        pTcpServer->close();
    }
    pTcpServer->deleteLater();
}

bool CTcpServer::Start()
{
    if ( iPort < 0 )
    {
        return false;
    }

    // default to any-address for either both IP protocols or just IPv4
    QHostAddress hostAddress = pServer->IsIPv6Available() ? QHostAddress::Any : QHostAddress::AnyIPv4;

    if ( !pServer->IsIPv6Available() )
    {
        if ( !strServerBindIP.isEmpty() )
        {
            hostAddress = QHostAddress ( strServerBindIP );
        }
    }

    if ( pTcpServer->listen ( hostAddress, iPort ) )
    {
        qInfo() << qUtf8Printable ( QString ( "- Jamulus-TCP: server started on port %1" ).arg ( pTcpServer->serverPort() ) );
        return true;
    }
    qWarning() << qUtf8Printable (
        QString ( "- Jamulus-TCP: unable to start server on port %1: %2" ).arg ( pTcpServer->serverPort() ).arg ( pTcpServer->errorString() ) );
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

    qDebug() << "- Jamulus-TCP: received connection from:" << peerAddress.toString();

    new CTcpConnection ( pSocket, peerAddress, pServer ); // will auto-delete on disconnect
}
