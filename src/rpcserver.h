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

#pragma once

#include <QObject>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTimer>
#include <QMap>
#include <QVector>
#include <memory>

typedef std::function<void ( const QJsonObject&, QJsonObject& )> CRpcHandler;

/* Classes ********************************************************************/
class CRpcServer : public QObject
{
    Q_OBJECT

public:
    CRpcServer ( QObject* parent, int iPort, QString secret );
    virtual ~CRpcServer();

    bool Start();
    void HandleMethod ( const QString& strMethod, CRpcHandler pHandler );
    void BroadcastNotification ( const QString& strMethod, const QJsonObject& aParams );

    static QJsonObject CreateJsonRpcError ( int code, QString message );

    // JSON-RPC standard error codes
    static const int iErrInvalidRequest = -32600;
    static const int iErrMethodNotFound = -32601;
    static const int iErrInvalidParams  = -32602;
    static const int iErrParseError     = -32700;

    // Our errors
    static const int iErrAuthenticationFailed = 400;
    static const int iErrUnauthenticated      = 401;

private:
    int         iPort;
    QString     strSecret;
    QTcpServer* pTransportServer;

    // A map from method name to handler functions
    QMap<QString, CRpcHandler> mapMethodHandlers;
    QMap<QTcpSocket*, bool>    isAuthenticated;
    QVector<QTcpSocket*>       vecClients;

    void HandleApiAuth ( QTcpSocket* pSocket, const QJsonObject& params, QJsonObject& response );
    void ProcessMessage ( QTcpSocket* pSocket, QJsonObject message, QJsonObject& response );
    void Send ( QTcpSocket* pSocket, const QJsonDocument& aMessage );

    static QJsonObject CreateJsonRpcErrorReply ( int code, QString message );

protected slots:
    void OnNewConnection();
};
