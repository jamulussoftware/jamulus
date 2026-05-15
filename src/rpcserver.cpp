/******************************************************************************\
 * Copyright (c) 2021-2025
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

CRpcServer::CRpcServer ( QObject* parent, QString strBindIP, int iPort, QString strSecret ) :
    QObject ( parent ),
    strBindIP ( strBindIP ),
    iPort ( iPort ),
    strSecret ( strSecret )
#if defined( HEADLESS ) || defined( JAMULUS_USE_JUCE_NET )
    , pJuceServer ( new JUCE_RpcServer ( this ) )
#else
    , pTransportServer ( new QTcpServer ( this ) )
#endif
{
#if !defined( HEADLESS ) && !defined( JAMULUS_USE_JUCE_NET )
    connect ( pTransportServer, &QTcpServer::newConnection, this, &CRpcServer::OnNewConnection );
#endif

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
#if defined( HEADLESS ) || defined( JAMULUS_USE_JUCE_NET )
    pJuceServer->serverSocket.close();
    pJuceServer->stopThread ( 2000 );
#else
    if ( pTransportServer->isListening() )
    {
        qInfo() << "- stopping RPC server";
        pTransportServer->close();
    }
#endif
}

bool CRpcServer::Start()
{
    if ( iPort < 0 )
    {
        return false;
    }
#if defined( HEADLESS ) || defined( JAMULUS_USE_JUCE_NET )
    if ( pJuceServer->serverSocket.createListener ( iPort, strBindIP.toStdString() ) )
    {
        qInfo() << qUtf8Printable ( QString ( "- JSON-RPC (JUCE): Server started on %1:%2" )
                                        .arg ( strBindIP )
                                        .arg ( iPort ) );
        pJuceServer->startThread();
        return true;
    }
#else
    if ( pTransportServer->listen ( QHostAddress ( strBindIP ), iPort ) )
    {
        qInfo() << qUtf8Printable ( QString ( "- JSON-RPC: Server started on %1:%2" )
                                        .arg ( pTransportServer->serverAddress().toString() )
                                        .arg ( pTransportServer->serverPort() ) );
        return true;
    }
#endif
    qInfo() << "- JSON-RPC: Unable to start server";
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

#if defined( HEADLESS ) || defined( JAMULUS_USE_JUCE_NET )
namespace
{
    static QJsonValue VarToQJsonValue ( const juce::var& v )
    {
        if ( v.isVoid() || v.isUndefined() )
        {
            return QJsonValue();
        }
        if ( v.isBool() )
        {
            return QJsonValue ( static_cast<bool> ( v ) );
        }
        if ( v.isInt() || v.isInt64() || v.isDouble() )
        {
            return QJsonValue ( static_cast<double> ( v ) );
        }
        if ( v.isString() )
        {
            juce::String s = v.toString();
            return QJsonValue ( QString::fromUtf8 ( s.toRawUTF8() ) );
        }
        if ( auto* arr = v.getArray() )
        {
            QJsonArray qArr;
            for ( auto& item : *arr )
            {
                qArr.append ( VarToQJsonValue ( item ) );
            }
            return qArr;
        }
        if ( auto* obj = v.getDynamicObject() )
        {
            QJsonObject qObj;
            const juce::NamedValueSet& props = obj->getProperties();
            for ( int i = 0; i < props.size(); ++i )
            {
                const juce::Identifier& name = props.getName ( i );
                const juce::var&        val  = props.getValueAt ( i );
                qObj.insert ( QString::fromUtf8 ( name.toString().toRawUTF8() ), VarToQJsonValue ( val ) );
            }
            return qObj;
        }
        return QJsonValue();
    }

    static juce::var QJsonValueToVar ( const QJsonValue& v )
    {
        switch ( v.type() )
        {
        case QJsonValue::Null:
        case QJsonValue::Undefined:
            return juce::var();
        case QJsonValue::Bool:
            return juce::var ( v.toBool() );
        case QJsonValue::Double:
            return juce::var ( v.toDouble() );
        case QJsonValue::String:
        {
            QByteArray utf8 = v.toString().toUtf8();
            return juce::var ( juce::String::fromUTF8 ( utf8.constData(), utf8.size() ) );
        }
        case QJsonValue::Array:
        {
            juce::Array<juce::var> arr;
            const QJsonArray       qArr = v.toArray();
            for ( const auto& item : qArr )
            {
                arr.add ( QJsonValueToVar ( item ) );
            }
            return juce::var ( arr );
        }
        case QJsonValue::Object:
        {
            auto*                    dyn  = new juce::DynamicObject();
            const QJsonObject        qObj = v.toObject();
            const auto               keys = qObj.keys();
            for ( const auto& key : keys )
            {
                QByteArray      utf8Key = key.toUtf8();
                juce::Identifier id ( juce::String::fromUTF8 ( utf8Key.constData(), utf8Key.size() ) );
                dyn->setProperty ( id, QJsonValueToVar ( qObj.value ( key ) ) );
            }
            return juce::var ( dyn );
        }
        }
        return juce::var();
    }
}

void CRpcServer::JUCE_RpcServer::run()
{
    while ( !threadShouldExit() )
    {
        auto* clientSocket = serverSocket.waitForNextConnection();
        if ( clientSocket != nullptr )
        {
            juce::MessageManager::callAsync ( [this, clientSocket]() {
                CRpcServer* pOwnerLocal = this->pOwner;
                void*       pSocket     = (void*) clientSocket;
                pOwnerLocal->vecClients.append ( pSocket );
                pOwnerLocal->isAuthenticated[pSocket] = false;

                auto* poller = new JUCE_PollTimer ( pOwnerLocal, clientSocket );
                poller->startTimer ( 10 );
            } );
        }
    }
}

CRpcServer::JUCE_PollTimer::JUCE_PollTimer ( CRpcServer* pOwnerIn, juce::StreamingSocket* pSocketIn ) :
    pOwner ( pOwnerIn ),
    pSocket ( pSocketIn )
{
}

void CRpcServer::JUCE_PollTimer::timerCallback()
{
    if ( !pSocket->isConnected() )
    {
        pOwner->vecClients.removeAll ( (void*) pSocket );
        pOwner->isAuthenticated.remove ( (void*) pSocket );
        delete pSocket;
        stopTimer();
        delete this;
        return;
    }

    if ( pSocket->waitUntilReady ( true, 0 ) > 0 )
    {
        char buffer[4096];
        int  bytesRead = pSocket->read ( buffer, sizeof ( buffer ) - 1, false );
        if ( bytesRead > 0 )
        {
            buffer[bytesRead] = '\0';
            juce::String dataStr = juce::String::fromUTF8 ( buffer, bytesRead );
            juce::StringArray lines;
            lines.addLines ( dataStr );
            for ( const auto& line : lines )
            {
                if ( line.isEmpty() )
                {
                    continue;
                }

                juce::var parsed = juce::JSON::parse ( line );
                if ( parsed.isObject() )
                {
                    QJsonObject request = VarToQJsonValue ( parsed ).toObject();
                    QJsonObject response;
                    response["jsonrpc"] = "2.0";
                    response["id"]      = request["id"];
                    pOwner->ProcessMessage ( (void*) pSocket, request, response );
                    QJsonDocument responseDoc ( response );
                    pOwner->Send ( (void*) pSocket, responseDoc );
                }
            }
        }
    }
}
#endif

void CRpcServer::OnNewConnection()
#if defined( HEADLESS ) || defined( JAMULUS_USE_JUCE_NET )
{}
#else
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
#endif

#if defined( HEADLESS ) || defined( JAMULUS_USE_JUCE_NET )
void CRpcServer::Send ( void* pSocket, const QJsonDocument& aMessage )
{
    juce::var root;
    if ( aMessage.isObject() )
    {
        root = QJsonValueToVar ( QJsonValue ( aMessage.object() ) );
    }
    else if ( aMessage.isArray() )
    {
        root = QJsonValueToVar ( QJsonValue ( aMessage.array() ) );
    }

    juce::String jsonText = juce::JSON::toString ( root, true );
    juce::String withNewline = jsonText + "\n";
    std::string  utf8 = withNewline.toStdString();
    ((juce::StreamingSocket*) pSocket)->write ( utf8.c_str(), (int) utf8.size() );
}
#else
void CRpcServer::Send ( QTcpSocket* pSocket, const QJsonDocument& aMessage ) { pSocket->write ( aMessage.toJson ( QJsonDocument::Compact ) + "\n" ); }
#endif

#if defined( HEADLESS ) || defined( JAMULUS_USE_JUCE_NET )
void CRpcServer::HandleApiAuth ( void* pSocket, const QJsonObject& params, QJsonObject& response )
#else
void CRpcServer::HandleApiAuth ( QTcpSocket* pSocket, const QJsonObject& params, QJsonObject& response )
#endif
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
#if defined( HEADLESS ) || defined( JAMULUS_USE_JUCE_NET )
        qInfo() << "- JSON-RPC: accepted valid authentication secret";
#else
        qInfo() << "- JSON-RPC: accepted valid authentication secret from" << pSocket->peerAddress().toString();
#endif
        return;
    }
    response["error"] = CreateJsonRpcError ( CRpcServer::iErrAuthenticationFailed, "Authentication failed." );
#if defined( HEADLESS ) || defined( JAMULUS_USE_JUCE_NET )
    qWarning() << "- JSON-RPC: rejected invalid authentication secret";
#else
    qWarning() << "- JSON-RPC: rejected invalid authentication secret from" << pSocket->peerAddress().toString();
#endif
}

void CRpcServer::HandleMethod ( const QString& strMethod, CRpcHandler pHandler ) { mapMethodHandlers[strMethod] = pHandler; }

#if defined( HEADLESS ) || defined( JAMULUS_USE_JUCE_NET )
void CRpcServer::ProcessMessage ( void* pSocket, QJsonObject message, QJsonObject& response )
#else
void CRpcServer::ProcessMessage ( QTcpSocket* pSocket, QJsonObject message, QJsonObject& response )
#endif
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
        /// @param {string} params.secret - The preshared secret key.
        /// @result {string} result - "ok" on success
        HandleApiAuth ( pSocket, params, response );
        return;
    }

    // Require authentication for everything else
    if ( !isAuthenticated[pSocket] )
    {
        response["error"] = CreateJsonRpcError ( iErrUnauthenticated, "Unauthenticated: Please authenticate using jamulus/apiAuth first" );
#if !defined( HEADLESS ) && !defined( JAMULUS_USE_JUCE_NET )
        qInfo() << "- JSON-RPC: rejected unauthenticated request from" << pSocket->peerAddress().toString();
#endif
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
