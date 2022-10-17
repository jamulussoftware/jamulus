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

#include "serverrpc.h"

CServerRpc::CServerRpc ( CServer* pServer, CRpcServer* pRpcServer, QObject* parent ) : QObject ( parent )
{

    /// @rpc_notification jamulusserver/chatReceived
    /// @brief Emitted when a chat text is received. Server-generated chats are not included in this notification.
    /// @param {string} params.id       - The channel ID generating the chat.
    /// @param {string} params.name     - The user name generating the chat.
    /// @param {number} params.address  - The address of the channel generating the chat.
    /// @param {string} params.stamp    - The date/time of the chat (ISO 8601 format, in server configured timezone).
    /// @param {string} params.text     - The chat text.
    connect ( pServer, &CServer::rpcChatSent, [=] ( int ChanID, QString ChanName, QString ChanAddr, QString ChatStamp, QString strChatText ) {
        pRpcServer->BroadcastNotification ( "jamulusserver/chatReceived",
                                            QJsonObject{
                                                { "id", ChanID },
                                                { "name", ChanName },
                                                { "address", ChanAddr },
                                                { "stamp", ChatStamp },
                                                { "text", strChatText },
                                            } );
    } );

    /// @rpc_notification jamulusserver/clientDisconnect
    /// @brief Emitted when a client disconnects from the server.
    /// @result {string} result.id - The client channel id.
    /// @result {string} result.name - The client’s name.
    /// @result {string} result.address - The client’s address (ip:port).
    QObject::connect ( pServer, &CServer::rpcClientDisconnected, [=] ( int ChanID, QString ChanName, CHostAddress ChanAddr ) {
        pRpcServer->BroadcastNotification ( "jamulusserver/clientDisconnect",
                                            QJsonObject{
                                                { "id", ChanID },
                                                { "name", ChanName },
                                                { "address", ChanAddr.toString ( CHostAddress::SM_IP_PORT ) },
                                            } );
    } );

    /// @rpc_notification jamulusserver/clientConnect
    /// @brief Emitted when a new client connects to the server.
    /// @result {string} result.name - The client’s name.
    /// @result {string} result.address - The client’s address (ip:port).
    /// @result {number} result.instrumentCode - The id of the user's instrument.
    /// @result {number} result.instrumentName - The text name of the user's instrument.
    /// @result {string} result.city - The user's city name.
    /// @result {number} result.countryCode - The id of the country specified by the user (see QLocale::Country).
    /// @result {number} result.countryName - The text name of the user's country (see QLocale::Country).
    /// @result {number} result.skillLevelCode - The user's skill level id.
    /// @result {number} result.skillLevelName - The user's skill level text name.
    QObject::connect ( &CRpcLogging::getInstance(), &CRpcLogging::rpcClientConnected, [=] ( CChannel& channel ) {
        CChannelCoreInfo chanInfo = channel.GetChanInfo();

        // We have to find the channel id ourselves.
        int ChanID = pServer->FindChannel ( channel.GetAddress() );

        pRpcServer->BroadcastNotification ( "jamulusserver/clientConnect",
                                            QJsonObject{
                                                { "id", ChanID },
                                                { "name", chanInfo.strName },
                                                { "address", channel.GetAddress().toString ( CHostAddress::SM_IP_PORT ) },
                                                { "instrumentCode", chanInfo.iInstrument },
                                                { "instrumentName", CInstPictures::GetName ( chanInfo.iInstrument ) },
                                                { "city", chanInfo.strCity },
                                                { "countryCode", chanInfo.eCountry },
                                                { "countryName", QLocale::countryToString ( chanInfo.eCountry ) },
                                                { "skillLevelCode", chanInfo.eSkillLevel },
                                                { "skillLevelName", SkillLevelToString ( chanInfo.eSkillLevel ) },
                                            } );
    } );

    /// @rpc_notification jamulusserver/clientInfoChanged
    /// @brief Emitted when a client changes their information (name, instrument, country, city, skill level).
    /// @result {string} result.oldName - The client’s name just prior to this change.
    /// @result {string} result.name - The client’s name (new one, if change).
    /// @result {string} result.address - The client’s address (ip:port).
    /// @result {number} result.instrumentCode - The id of the user's instrument.
    /// @result {number} result.instrumentName - The text name of the user's instrument.
    /// @result {string} result.city - The user's city name.
    /// @result {number} result.countryCode - The id of the country specified by the user (see QLocale::Country).
    /// @result {number} result.countryName - The text name of the user's country (see QLocale::Country).
    /// @result {number} result.skillLevelCode - The user's skill level id.
    /// @result {number} result.skillLevelName - The user's skill level text name.

    QObject::connect ( &CRpcLogging::getInstance(), &CRpcLogging::rpcUpdateConnection, [=] ( CChannel& channel, const QString strOldName ) {
        CChannelCoreInfo chanInfo = channel.GetChanInfo();

        // We have to find the channel id ourselves.
        int ChanID = pServer->FindChannel ( channel.GetAddress() );

        pRpcServer->BroadcastNotification ( "jamulusserver/clientInfoChanged",
                                            QJsonObject{
                                                { "id", ChanID },
                                                { "oldName", strOldName },
                                                { "name", chanInfo.strName },
                                                { "address", channel.GetAddress().toString ( CHostAddress::SM_IP_PORT ) },
                                                { "instrumentCode", chanInfo.iInstrument },
                                                { "instrumentName", CInstPictures::GetName ( chanInfo.iInstrument ) },
                                                { "city", chanInfo.strCity },
                                                { "countryCode", chanInfo.eCountry },
                                                { "countryName", QLocale::countryToString ( chanInfo.eCountry ) },
                                                { "skillLevelCode", chanInfo.eSkillLevel },
                                                { "skillLevelName", SkillLevelToString ( chanInfo.eSkillLevel ) },
                                            } );
    } );

    // API doc already part of CClientRpc
    pRpcServer->HandleMethod ( "jamulus/getMode", [=] ( const QJsonObject& params, QJsonObject& response ) {
        QJsonObject result{ { "mode", "server" } };
        response["result"] = result;
        Q_UNUSED ( params );
    } );

    /// @rpc_method jamulusserver/sendBroadcastChat
    /// @brief Send a chat message to all connected clients.  Messages from the server are not escaped and can contain HTML as defined for
    /// QTextBrowser.
    /// @param {string} params.textMessage - The chat message to be sent.
    /// @result {string} result - Always "ok".
    pRpcServer->HandleMethod ( "jamulusserver/sendBroadcastChat", [=] ( const QJsonObject& params, QJsonObject& response ) {
        auto chatMessage = params["textMessage"];

        if ( !chatMessage.isString() )
        {
            response["error"] = CRpcServer::CreateJsonRpcError ( CRpcServer::iErrInvalidParams, "Invalid params: textMessage is not a string" );
            return;
        }

        pServer->CreateAndSendChatTextForAllConChannels ( -1, QString ( chatMessage.toString() ) );

        response["result"] = "ok";
    } );

    /// @rpc_method jamulusserver/sendChat
    /// @brief Send a chat message to the channel identified by a specificc address. The chat should be pre-escaped if necessary prior to calling this
    /// method.
    /// @param {string} params.address - The full channel IP address as a string XXX.XXX.XXX.XXX:PPPPP
    /// @param {string} params.textMessage - The chat message to be sent.
    /// @result {string} result - "ok" if channel could be determined and message sent.
    pRpcServer->HandleMethod ( "jamulusserver/sendChat", [=] ( const QJsonObject& params, QJsonObject& response ) {
        auto strAddress  = params["address"];
        auto chatMessage = params["textMessage"];

        bool                      boolStatus;
        CVector<CHostAddress>     vecHostAddresses;
        CVector<QString>          vecsName;
        CVector<int>              veciJitBufNumFrames;
        CVector<int>              veciNetwFrameSizeFact;
        CVector<CChannelCoreInfo> vecChanInfo;

        if ( !chatMessage.isString() )
        {
            response["error"] = CRpcServer::CreateJsonRpcError ( CRpcServer::iErrInvalidParams, "Invalid params: textMessage is not a string" );
            return;
        }

        if ( !strAddress.isString() )
        {
            response["error"] = CRpcServer::CreateJsonRpcError ( CRpcServer::iErrInvalidParams, "Invalid params: address is not a string" );
            return;
        }

        // Find the channel number from the address provided.

        pServer->GetConCliParam ( vecHostAddresses, vecsName, veciJitBufNumFrames, veciNetwFrameSizeFact, vecChanInfo );

        const int iNumChannels = vecHostAddresses.Size();

        // fill list with connected clients
        for ( int i = 0; i < iNumChannels; i++ )
        {
            if ( vecHostAddresses[i].InetAddr == QHostAddress ( static_cast<quint32> ( 0 ) ) )
            {
                continue;
            }

            if ( QString ( vecHostAddresses[i].toString ( CHostAddress::SM_IP_PORT ) ) == QString ( strAddress.toString() ) )
            {
                // Send chat to channel

                boolStatus = pServer->CreateAndSendPreEscapedChatText ( i, QString ( chatMessage.toString() ) );

                if ( boolStatus )
                {
                    response["result"] = "ok";
                    return;
                }
                else
                    break;
            }
        }

        response["error"] = CRpcServer::CreateJsonRpcError ( CRpcServer::iErrChannelNotFound, "Could not locate channel from address" );
    } );

    /// @rpc_method jamulusserver/getRecorderStatus
    /// @brief Returns the recorder state.
    /// @param {object} params - No parameters (empty object).
    /// @result {boolean} result.initialised - True if the recorder is initialised.
    /// @result {string} result.errorMessage - The recorder error message, if any.
    /// @result {boolean} result.enabled - True if the recorder is enabled.
    /// @result {string} result.recordingDirectory - The recorder recording directory.
    pRpcServer->HandleMethod ( "jamulusserver/getRecorderStatus", [=] ( const QJsonObject& params, QJsonObject& response ) {
        QJsonObject result{
            { "initialised", pServer->GetRecorderInitialised() },
            { "errorMessage", pServer->GetRecorderErrMsg() },
            { "enabled", pServer->GetRecordingEnabled() },
            { "recordingDirectory", pServer->GetRecordingDir() },
        };

        response["result"] = result;
        Q_UNUSED ( params );
    } );

    /// @rpc_method jamulusserver/getClients
    /// @brief Returns the list of connected clients along with details about them.
    /// @param {object} params - No parameters (empty object).
    /// @result {array} result.clients - The list of connected clients.
    /// @result {number} result.clients[*].id - The client’s channel id.
    /// @result {string} result.clients[*].address - The client’s address (ip:port).
    /// @result {string} result.clients[*].name - The client’s name.
    /// @result {number} result.clients[*].jitterBufferSize - The client’s jitter buffer size.
    /// @result {number} result.clients[*].channels - The number of audio channels of the client.
    pRpcServer->HandleMethod ( "jamulusserver/getClients", [=] ( const QJsonObject& params, QJsonObject& response ) {
        QJsonArray            clients;
        CVector<CHostAddress> vecHostAddresses;
        CVector<QString>      vecsName;
        CVector<int>          veciJitBufNumFrames;
        CVector<int>          veciNetwFrameSizeFact;

        pServer->GetConCliParam ( vecHostAddresses, vecsName, veciJitBufNumFrames, veciNetwFrameSizeFact );

        // we assume that all vectors have the same length
        const int iNumChannels = vecHostAddresses.Size();

        // fill list with connected clients
        for ( int i = 0; i < iNumChannels; i++ )
        {
            if ( vecHostAddresses[i].InetAddr == QHostAddress ( static_cast<quint32> ( 0 ) ) )
            {
                continue;
            }
            QJsonObject client{
                { "id", i },
                { "address", vecHostAddresses[i].toString ( CHostAddress::SM_IP_PORT ) },
                { "name", vecsName[i] },
                { "jitterBufferSize", veciJitBufNumFrames[i] },
                { "channels", pServer->GetClientNumAudioChannels ( i ) },
            };
            clients.append ( client );
        }

        // create result object
        QJsonObject result{
            { "clients", clients },
        };
        response["result"] = result;
        Q_UNUSED ( params );
    } );

    /// @rpc_method jamulusserver/getServerProfile
    /// @brief Returns the server registration profile and status.
    /// @param {object} params - No parameters (empty object).
    /// @result {string} result.name - The server name.
    /// @result {string} result.city - The server city.
    /// @result {number} result.countryId - The server country ID (see QLocale::Country).
    /// @result {string} result.welcomeMessage - The server welcome message.
    /// @result {string} result.directoryServer - The directory server to which this server requested registration, or blank if none.
    /// @result {string} result.registrationStatus - The server registration status as string (see ESvrRegStatus and SerializeRegistrationStatus).
    pRpcServer->HandleMethod ( "jamulusserver/getServerProfile", [=] ( const QJsonObject& params, QJsonObject& response ) {
        QString dsName = "";

        if ( AT_NONE != pServer->GetDirectoryType() )
            dsName = NetworkUtil::GetDirectoryAddress ( pServer->GetDirectoryType(), pServer->GetDirectoryAddress() );

        QJsonObject result{
            { "name", pServer->GetServerName() },
            { "city", pServer->GetServerCity() },
            { "countryId", pServer->GetServerCountry() },
            { "welcomeMessage", pServer->GetWelcomeMessage() },
            { "directoryServer", dsName },
            { "registrationStatus", SerializeRegistrationStatus ( pServer->GetSvrRegStatus() ) },
        };
        response["result"] = result;
        Q_UNUSED ( params );
    } );

    /// @rpc_method jamulusserver/setServerName
    /// @brief Sets the server name.
    /// @param {string} params.serverName - The new server name.
    /// @result {string} result - Always "ok".
    pRpcServer->HandleMethod ( "jamulusserver/setServerName", [=] ( const QJsonObject& params, QJsonObject& response ) {
        auto jsonServerName = params["serverName"];
        if ( !jsonServerName.isString() )
        {
            response["error"] = CRpcServer::CreateJsonRpcError ( CRpcServer::iErrInvalidParams, "Invalid params: serverName is not a string" );
            return;
        }

        pServer->SetServerName ( jsonServerName.toString() );
        response["result"] = "ok";
    } );

    /// @rpc_method jamulusserver/setWelcomeMessage
    /// @brief Sets the server welcome message.
    /// @param {string} params.welcomeMessage - The new welcome message.
    /// @result {string} result - Always "ok".
    pRpcServer->HandleMethod ( "jamulusserver/setWelcomeMessage", [=] ( const QJsonObject& params, QJsonObject& response ) {
        auto jsonWelcomeMessage = params["welcomeMessage"];
        if ( !jsonWelcomeMessage.isString() )
        {
            response["error"] = CRpcServer::CreateJsonRpcError ( CRpcServer::iErrInvalidParams, "Invalid params: welcomeMessage is not a string" );
            return;
        }

        pServer->SetWelcomeMessage ( jsonWelcomeMessage.toString() );
        response["result"] = "ok";
    } );

    /// @rpc_method jamulusserver/setRecordingDirectory
    /// @brief Sets the server recording directory.
    /// @param {string} params.recordingDirectory - The new recording directory.
    /// @result {string} result - Always "acknowledged".
    ///  To check if the directory was changed, call `jamulusserver/getRecorderStatus` again.
    pRpcServer->HandleMethod ( "jamulusserver/setRecordingDirectory", [=] ( const QJsonObject& params, QJsonObject& response ) {
        auto jsonRecordingDirectory = params["recordingDirectory"];
        if ( !jsonRecordingDirectory.isString() )
        {
            response["error"] =
                CRpcServer::CreateJsonRpcError ( CRpcServer::iErrInvalidParams, "Invalid params: recordingDirectory is not a string" );
            return;
        }

        pServer->SetRecordingDir ( jsonRecordingDirectory.toString() );
        response["result"] = "acknowledged";
    } );

    /// @rpc_method jamulusserver/startRecording
    /// @brief Starts the server recording.
    /// @param {object} params - No parameters (empty object).
    /// @result {string} result - Always "acknowledged".
    ///  To check if the recording was enabled, call `jamulusserver/getRecorderStatus` again.
    pRpcServer->HandleMethod ( "jamulusserver/startRecording", [=] ( const QJsonObject& params, QJsonObject& response ) {
        pServer->SetEnableRecording ( true );
        response["result"] = "acknowledged";
        Q_UNUSED ( params );
    } );

    /// @rpc_method jamulusserver/stopRecording
    /// @brief Stops the server recording.
    /// @param {object} params - No parameters (empty object).
    /// @result {string} result - Always "acknowledged".
    ///  To check if the recording was disabled, call `jamulusserver/getRecorderStatus` again.
    pRpcServer->HandleMethod ( "jamulusserver/stopRecording", [=] ( const QJsonObject& params, QJsonObject& response ) {
        pServer->SetEnableRecording ( false );
        response["result"] = "acknowledged";
        Q_UNUSED ( params );
    } );

    /// @rpc_method jamulusserver/restartRecording
    /// @brief Restarts the recording into a new directory.
    /// @param {object} params - No parameters (empty object).
    /// @result {string} result - Always "acknowledged".
    ///  To check if the recording was restarted or if there is any error, call `jamulusserver/getRecorderStatus` again.
    pRpcServer->HandleMethod ( "jamulusserver/restartRecording", [=] ( const QJsonObject& params, QJsonObject& response ) {
        pServer->RequestNewRecording();
        response["result"] = "acknowledged";
        Q_UNUSED ( params );
    } );
}

QJsonValue CServerRpc::SerializeRegistrationStatus ( ESvrRegStatus eSvrRegStatus )
{
    switch ( eSvrRegStatus )
    {
    case SRS_NOT_REGISTERED:
        return "not_registered";

    case SRS_BAD_ADDRESS:
        return "bad_address";

    case SRS_REQUESTED:
        return "requested";

    case SRS_TIME_OUT:
        return "time_out";

    case SRS_UNKNOWN_RESP:
        return "unknown_resp";

    case SRS_REGISTERED:
        return "registered";

    case SRS_SERVER_LIST_FULL:
        return "directory_server_full";

    case SRS_VERSION_TOO_OLD:
        return "server_version_too_old";

    case SRS_NOT_FULFILL_REQUIREMENTS:
        return "requirements_not_fulfilled";
    }

    return QString ( "unknown(%1)" ).arg ( eSvrRegStatus );
}
