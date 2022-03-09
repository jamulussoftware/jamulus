/******************************************************************************\
 * Copyright (c) 2021-2022
 *
 * Author(s):
 *  dtinth
 *  Christian Hoffmann
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

#include "global.h"
#include "rpcserver.h"

CRpcServer::CRpcServer ( QObject* parent, int iPort, QString strSecret ) :
    QObject ( parent ),
    iPort ( iPort ),
    strSecret ( strSecret ),
    pTransportServer ( new QTcpServer ( this ) )
{
    connect ( pTransportServer, &QTcpServer::newConnection, this, &CRpcServer::OnNewConnection );

    /// @rpc_method jamulus/getVersion
    /// @brief Returns Jamulus version.
    /// @param {object} params - No parameters (empty object).
    /// @result {string} result.version - The Jamulus version.
    HandleMethod ( "jamulus/getVersion", [=] ( const QJsonObject& params, QJsonObject& response ) {
        QJsonObject result{ { "version", VERSION } };
        response["result"] = result;
        Q_UNUSED ( params );
    } );
}

CRpcServer::~CRpcServer()
{
    if ( pTransportServer->isListening() )
    {
        qInfo() << "- stopping RPC server";
        pTransportServer->close();
    }
}

bool CRpcServer::Start()
{
    if ( iPort < 0 )
    {
        return false;
    }
    if ( pTransportServer->listen ( QHostAddress ( JSON_RPC_LISTEN_ADDRESS ), iPort ) )
    {
        qInfo() << qUtf8Printable ( QString ( "- JSON-RPC: Server started on %1:%2" )
                                        .arg ( pTransportServer->serverAddress().toString() )
                                        .arg ( pTransportServer->serverPort() ) );
        return true;
    }
    qInfo() << "- JSON-RPC: Unable to start server:" << pTransportServer->errorString();
    return false;
}

QJsonObject CRpcServer::CreateJsonRpcError ( int code, QString message )
{
    QJsonObject error;
    error["code"]    = QJsonValue ( code );
    error["message"] = QJsonValue ( message );
    return error;
}

QJsonObject CRpcServer::CreateJsonRpcErrorReply ( int code, QString message )
{
    QJsonObject object;
    object["jsonrpc"] = QJsonValue ( "2.0" );
    object["error"]   = CreateJsonRpcError ( code, message );
    return object;
}

void CRpcServer::OnNewConnection()
{
    QTcpSocket* pSocket = pTransportServer->nextPendingConnection();
    if ( !pSocket )
    {
        return;
    }

    qDebug() << "- JSON-RPC: received connection from:" << pSocket->peerAddress().toString();
    vecClients.append ( pSocket );
    isAuthenticated[pSocket] = false;

    connect ( pSocket, &QTcpSocket::disconnected, [this, pSocket]() {
        qDebug() << "- JSON-RPC: connection from:" << pSocket->peerAddress().toString() << "closed";
        vecClients.removeAll ( pSocket );
        isAuthenticated.remove ( pSocket );
        pSocket->deleteLater();
    } );

    connect ( pSocket, &QTcpSocket::readyRead, [this, pSocket]() {
        while ( pSocket->canReadLine() )
        {
            QByteArray line = pSocket->readLine();
            if ( line.trimmed().isEmpty() )
            {
                Send ( pSocket, QJsonDocument ( CreateJsonRpcErrorReply ( iErrParseError, "Parse error: Blank line received" ) ) );
                continue;
            }
            QJsonParseError parseError;
            QJsonDocument   data = QJsonDocument::fromJson ( line, &parseError );

            if ( parseError.error != QJsonParseError::NoError )
            {
                Send ( pSocket, QJsonDocument ( CreateJsonRpcErrorReply ( iErrParseError, "Parse error: Invalid JSON received" ) ) );
                pSocket->disconnectFromHost();
                return;
            }

            if ( data.isArray() )
            {
                // JSON-RPC batch mode: multiple requests in an array
                QJsonArray output;
                for ( auto item : data.array() )
                {
                    if ( !item.isObject() )
                    {
                        output.append (
                            CreateJsonRpcErrorReply ( iErrInvalidRequest, "Invalid request: Non-object item encountered in a batch request array" ) );
                        pSocket->disconnectFromHost();
                        return;
                    }
                    auto        object = item.toObject();
                    QJsonObject response;
                    response["jsonrpc"] = QJsonValue ( "2.0" );
                    response["id"]      = object["id"];
                    ProcessMessage ( pSocket, object, response );
                    output.append ( response );
                }
                if ( output.size() < 1 )
                {
                    Send ( pSocket,
                           QJsonDocument ( CreateJsonRpcErrorReply ( iErrInvalidRequest, "Invalid request: Empty batch request encountered" ) ) );
                    pSocket->disconnectFromHost();
                    return;
                }
                Send ( pSocket, QJsonDocument ( output ) );
                continue;
            }

            if ( data.isObject() )
            {
                auto        object = data.object();
                QJsonObject response;
                response["jsonrpc"] = QJsonValue ( "2.0" );
                response["id"]      = object["id"];
                ProcessMessage ( pSocket, object, response );
                Send ( pSocket, QJsonDocument ( response ) );
                continue;
            }

            Send (
                pSocket,
                QJsonDocument ( CreateJsonRpcErrorReply ( iErrInvalidRequest,
                                                          "Invalid request: Unrecognized JSON; a request must be either an object or an array" ) ) );
            pSocket->disconnectFromHost();
            return;
        }
    } );
}

void CRpcServer::Send ( QTcpSocket* pSocket, const QJsonDocument& aMessage ) { pSocket->write ( aMessage.toJson ( QJsonDocument::Compact ) + "\n" ); }

void CRpcServer::HandleApiAuth ( QTcpSocket* pSocket, const QJsonObject& params, QJsonObject& response )
{
    auto userSecret = params["secret"];
    if ( !userSecret.isString() )
    {
        response["error"] = CreateJsonRpcError ( iErrInvalidParams, "Invalid params: secret is not a string" );
        return;
    }

    if ( userSecret == strSecret )
    {
        isAuthenticated[pSocket] = true;
        response["result"]       = "ok";
        qInfo() << "- JSON-RPC: accepted valid authentication secret from" << pSocket->peerAddress().toString();
        return;
    }
    response["error"] = CreateJsonRpcError ( CRpcServer::iErrAuthenticationFailed, "Authentication failed." );
    qWarning() << "- JSON-RPC: rejected invalid authentication secret from" << pSocket->peerAddress().toString();
}

void CRpcServer::HandleMethod ( const QString& strMethod, CRpcHandler pHandler ) { mapMethodHandlers[strMethod] = pHandler; }

void CRpcServer::ProcessMessage ( QTcpSocket* pSocket, QJsonObject message, QJsonObject& response )
{
    if ( !message["method"].isString() )
    {
        response["error"] = CreateJsonRpcError ( iErrInvalidRequest, "Invalid request: The `method` member is not a string" );
        return;
    }

    // Obtain the params
    auto jsonParams = message["params"];
    if ( !jsonParams.isObject() )
    {
        response["error"] = CreateJsonRpcError ( iErrInvalidParams, "Invalid params: The `params` member is not an object" );
        return;
    }
    auto params = jsonParams.toObject();

    // Obtain the method name
    auto method = message["method"].toString();

    // Authentication must be allowed when un-authed
    if ( method == "jamulus/apiAuth" )
    {
        /// @rpc_method jamulus/apiAuth
        /// @brief Authenticates the connection which is a requirement for calling further methods.
        /// @param {object} params - No parameters (empty object).
        /// @result {string} result - "ok" on success
        HandleApiAuth ( pSocket, params, response );
        return;
    }

    // Require authentication for everything else
    if ( !isAuthenticated[pSocket] )
    {
        response["error"] = CreateJsonRpcError ( iErrUnauthenticated, "Unauthenticated: Please authenticate using jamulus/apiAuth first" );
        qInfo() << "- JSON-RPC: rejected unauthenticated request from" << pSocket->peerAddress().toString();
        return;
    }

    // Obtain the method handler
    auto it = mapMethodHandlers.find ( method );
    if ( it == mapMethodHandlers.end() )
    {
        response["error"] = CreateJsonRpcError ( iErrMethodNotFound, "Method not found" );
        return;
    }

    // Call the method handler
    auto methodHandler = mapMethodHandlers[method];
    methodHandler ( params, response );
    Q_UNUSED ( pSocket );
}

void CRpcServer::BroadcastNotification ( const QString& strMethod, const QJsonObject& aParams )
{
    for ( auto socket : vecClients )
    {
        if ( !isAuthenticated[socket] )
        {
            continue;
        }
        QJsonObject notification;
        notification["jsonrpc"] = "2.0";
        notification["method"]  = strMethod;
        notification["params"]  = aParams;
        Send ( socket, QJsonDocument ( notification ) );
    }
}
