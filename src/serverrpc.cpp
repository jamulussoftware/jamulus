/******************************************************************************\
 * Copyright (c) 2021-2026
 *
 * Author(s):
 *  dtinth
 *  Christian Hoffmann
 *
 * As of Jamulus 3.12.1dev (commit eb172d47): All new source code contributions must be licensed
 * under AGPL 3.0 or any later version.
 *
 * Existing code: Code contributed before 3.12.1dev (commit eb172d47) was licensed under GPL 2.0+.
 * This code will be licensed under GPL 3.0 (or any later version) from
 * 3.12.1dev (commit eb172d47).  When distributed as part of Jamulus, the AGPL 3.0 terms govern
 * the combined work, including network use provisions.
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
 * ---------------------------------------------------------------------------
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 \******************************************************************************/

#include "serverrpc.h"

/* Definitions ****************************************************************/
#define INVALID_CLIENT_ID -1

CServerRpc::CServerRpc ( CServer* pServer, CRpcServer* pRpcServer, QObject* parent ) : QObject ( parent )
{
    // API doc already part of CClientRpc
    pRpcServer->HandleMethod ( "jamulus/getMode", [=] ( const QJsonObject& params, QJsonObject& response ) {
        QJsonObject result{ { "mode", "server" } };
        response["result"] = result;
        Q_UNUSED ( params );
    } );

    /// @rpc_notification jamulusserver/clientConnected
    /// @brief Emitted when a client has connected to the server.
    /// @param {number} params.id - The channel ID assigned to the client.
    /// @param {string} params.address - The client's address.
    /// @param {number} params.totalChannels - Number of total channels connected to the server.
    connect ( pServer, &CServer::ClientConnected, [=] ( const int iChanID, const QHostAddress RecHostAddr, const int iTotChans ) {
        pRpcServer->BroadcastNotification ( "jamulusserver/clientConnected",
                                            QJsonObject{
                                                { "id", iChanID },
                                                { "address", RecHostAddr.toString() },
                                                { "totalChannels", iTotChans },
                                            } );
    } );

    /// @rpc_notification jamulusserver/clientDisconnected
    /// @brief Emitted when a client has disconnected from the server.
    /// @param {number} params.id - The channel ID assigned to the client.
    connect ( pServer, &CServer::ClientDisconnected, [=] ( const int iChanID ) {
        pRpcServer->BroadcastNotification ( "jamulusserver/clientDisconnected",
                                            QJsonObject{
                                                { "id", iChanID },
                                            } );
    } );

    /// @rpc_notification jamulusserver/chatMessageReceived
    /// @brief Emitted when a chat message is received from either a Jamulus or RPC client and to be broadcast to all connected clients.
    /// @param {number} params.id - Channel ID of sending client or -1 for RPC sent messages.
    /// @param {string} params.chatMessage - Chat message text.
    connect ( pServer, &CServer::sentChatMessage, [=] ( const int iSendingChanID, const QString& strChatText ) {
        pRpcServer->BroadcastNotification ( "jamulusserver/chatMessageReceived",
                                            QJsonObject{
                                                { "id", iSendingChanID },
                                                { "chatMessage", strChatText },
                                            } );
    } );

    /// @rpc_method jamulusserver/broadcastChatMessage
    /// @brief Sends a message (as the server) to all connected clients. This can be used to broadcast messages from external sources (e.g. scripts or
    /// monitoring tools).
    /// @param {string} params.chatMessage - The chat message text.
    /// @result {string} result - Always "ok".
    pRpcServer->HandleMethod ( "jamulusserver/broadcastChatMessage", [=] ( const QJsonObject& params, QJsonObject& response ) {
        auto jsonChatMessage = params["chatMessage"];
        if ( !jsonChatMessage.isString() )
        {
            response["error"] = CRpcServer::CreateJsonRpcError ( CRpcServer::iErrInvalidParams, "Invalid params: chatMessage is not a string" );
            return;
        }

        // set invalid channel ID to make clear this message was not sent by a Jamulus client
        pServer->SendChatTextToAllConChannels ( INVALID_CLIENT_ID, jsonChatMessage.toString() );
        response["result"] = "ok";
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
    /// @result {number} result.connections - The number of active connections.
    /// @result {array}  result.clients - The list of connected clients.
    /// @result {number} result.clients[*].id - The client’s channel id.
    /// @result {string} result.clients[*].address - The client’s address (ip:port).
    /// @result {string} result.clients[*].name - The client’s name.
    /// @result {number} result.clients[*].jitterBufferSize - The client’s jitter buffer size.
    /// @result {number} result.clients[*].channels - The number of audio channels of the client.
    /// @result {number} result.clients[*].instrumentCode - The id of the instrument for this channel.
    /// @result {string} result.clients[*].city - The city name provided by the user for this channel.
    /// @result {number} result.clients[*].countryName - The text name of the country specified by the user for this channel (see QLocale::Country).
    /// @result {number} result.clients[*].skillLevelCode - The skill level id provided by the user for this channel.
    pRpcServer->HandleMethod ( "jamulusserver/getClients", [=] ( const QJsonObject& params, QJsonObject& response ) {
        QJsonArray                clients;
        CVector<CHostAddress>     vecHostAddresses;
        CVector<QString>          vecsName;
        CVector<int>              veciJitBufNumFrames;
        CVector<int>              veciNetwFrameSizeFact;
        CVector<CChannelCoreInfo> vecChanInfo;

        int connections = 0;

        pServer->GetConCliParam ( vecHostAddresses, vecsName, veciJitBufNumFrames, veciNetwFrameSizeFact, vecChanInfo );

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
                { "instrumentCode", vecChanInfo[i].iInstrument },
                { "city", vecChanInfo[i].strCity },
                { "countryName", QLocale::countryToString ( vecChanInfo[i].eCountry ) },
                { "skillLevelCode", vecChanInfo[i].eSkillLevel },
            };
            clients.append ( client );

            ++connections;
        }

        // create result object
        QJsonObject result{
            { "connections", connections },
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
    /// @result {string} result.directoryType - The directory type as a string (see EDirectoryType and SerializeDirectoryType).
    /// @result {string} result.directoryAddress - The string used to look up the directory address (only assume valid if directoryType is "custom"
    /// and registrationStatus is "registered").
    /// @result {string} result.directory - The directory with which this server requested registration, or blank if none.
    /// @result {string} result.registrationStatus - The server registration status as string (see ESvrRegStatus and SerializeRegistrationStatus).
    pRpcServer->HandleMethod ( "jamulusserver/getServerProfile", [=] ( const QJsonObject& params, QJsonObject& response ) {
        EDirectoryType directoryType    = pServer->GetDirectoryType();
        QString        directoryAddress = pServer->GetDirectoryAddress();
        QString        dsName           = ( AT_NONE == directoryType ) ? "" : NetworkUtil::GetDirectoryAddress ( directoryType, directoryAddress );

        QJsonObject result{
            { "name", pServer->GetServerName() },
            { "city", pServer->GetServerCity() },
            { "countryId", pServer->GetServerCountry() },
            { "welcomeMessage", pServer->GetWelcomeMessage() },
            { "directoryType", SerializeDirectoryType ( directoryType ) },
            { "directoryAddress", directoryAddress },
            { "directory", dsName },
            { "registrationStatus", SerializeRegistrationStatus ( pServer->GetSvrRegStatus() ) },
        };
        response["result"] = result;
        Q_UNUSED ( params );
    } );

    /// @rpc_method jamulusserver/setDirectory
    /// @brief Set the directory type and, for custom, the directory address.
    /// @param {string} params.directoryType - The directory type as a string (see EDirectoryType and DeserializeDirectoryType).
    /// @param {string} [params.directoryAddress] - (optional) The directory address, required if `directoryType` is "custom".
    /// @result {string} result - Always "ok".
    pRpcServer->HandleMethod ( "jamulusserver/setDirectory", [=] ( const QJsonObject& params, QJsonObject& response ) {
        auto           jsonDirectoryType = params["directoryType"];
        auto           directoryAddress  = params["directoryAddress"];
        EDirectoryType directoryType     = AT_NONE;

        if ( !jsonDirectoryType.isString() )
        {
            response["error"] = CRpcServer::CreateJsonRpcError ( CRpcServer::iErrInvalidParams, "Invalid params: directory type is not a string" );
            return;
        }
        else
        {
            directoryType = DeserializeDirectoryType ( jsonDirectoryType.toString().toStdString() );
        }

        if ( !directoryAddress.isUndefined() )
        {
            if ( !directoryAddress.isString() )
            {
                response["error"] =
                    CRpcServer::CreateJsonRpcError ( CRpcServer::iErrInvalidParams, "Invalid params: directory address is not a string" );
                return;
            }
        }
        else if ( AT_CUSTOM == directoryType )
        {
            response["error"] = CRpcServer::CreateJsonRpcError ( CRpcServer::iErrInvalidParams,
                                                                 "Invalid params: directory address is required when directory type is \"custom\"" );
            return;
        }

        pServer->SetDirectoryAddress ( directoryAddress.toString() ); // ignored unless AT_CUSTOM == directoryType
        pServer->SetDirectoryType ( directoryType );

        response["result"] = "ok";
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

#if defined( Q_OS_MACOS ) && QT_VERSION < QT_VERSION_CHECK( 6, 0, 0 )
const std::unordered_map<EDirectoryType, std::string, EnumClassHash<EDirectoryType>>
#else
const std::unordered_map<EDirectoryType, std::string>
#endif
    CServerRpc::sumDirectoryTypeToString = {
        { EDirectoryType::AT_NONE, "none" },
        { EDirectoryType::AT_DEFAULT, "any_genre_1" },
        { EDirectoryType::AT_ANY_GENRE2, "any_genre_2" },
        { EDirectoryType::AT_ANY_GENRE_ASIA, "any_genre_asia" },
        { EDirectoryType::AT_GENRE_ROCK, "genre_rock" },
        { EDirectoryType::AT_GENRE_JAZZ, "genre_jazz" },
        { EDirectoryType::AT_GENRE_CLASSICAL_FOLK, "genre_classical_folk" },
        { EDirectoryType::AT_GENRE_CHORAL, "genre_choral_barbershop" },
        { EDirectoryType::AT_CUSTOM, "custom" },
};

inline QJsonValue CServerRpc::SerializeDirectoryType ( EDirectoryType eAddrType )
{
    auto found = sumDirectoryTypeToString.find ( eAddrType );
    if ( found == sumDirectoryTypeToString.end() )
        return QJsonValue ( QString ( "unknown(%1)" ).arg ( eAddrType ) );

    return QJsonValue ( QString::fromStdString ( found->second ) );
}

const std::unordered_map<std::string, EDirectoryType> CServerRpc::sumStringToDirectoryType = {
    { "none", EDirectoryType::AT_NONE },
    { "any_genre_1", EDirectoryType::AT_DEFAULT },
    { "any_genre_2", EDirectoryType::AT_ANY_GENRE2 },
    { "any_genre_asia", EDirectoryType::AT_ANY_GENRE_ASIA },
    { "genre_rock", EDirectoryType::AT_GENRE_ROCK },
    { "genre_jazz", EDirectoryType::AT_GENRE_JAZZ },
    { "genre_classical_folk", EDirectoryType::AT_GENRE_CLASSICAL_FOLK },
    { "genre_choral_barbershop", EDirectoryType::AT_GENRE_CHORAL },
    { "custom", EDirectoryType::AT_CUSTOM },
};

inline EDirectoryType CServerRpc::DeserializeDirectoryType ( std::string sAddrType )
{
    auto found = sumStringToDirectoryType.find ( sAddrType );
    if ( found == sumStringToDirectoryType.end() )
        return AT_DEFAULT;

    return found->second;
}

#if defined( Q_OS_MACOS ) && QT_VERSION < QT_VERSION_CHECK( 6, 0, 0 )
const std::unordered_map<ESvrRegStatus, std::string, EnumClassHash<ESvrRegStatus>>
#else
const std::unordered_map<ESvrRegStatus, std::string>
#endif
    CServerRpc::sumSvrRegStatusToString = { { SRS_NOT_REGISTERED, "not_registered" },
                                            { SRS_BAD_ADDRESS, "bad_address" },
                                            { SRS_REQUESTED, "requested" },
                                            { SRS_TIME_OUT, "time_out" },
                                            { SRS_UNKNOWN_RESP, "unknown_resp" },
                                            { SRS_REGISTERED, "registered" },
                                            { SRS_SERVER_LIST_FULL, "directory_server_full" }, // TODO: rename to "server_list_full"
                                            { SRS_VERSION_TOO_OLD, "server_version_too_old" },
                                            { SRS_NOT_FULFILL_REQUIREMENTS, "requirements_not_fulfilled" } };

inline QJsonValue CServerRpc::SerializeRegistrationStatus ( ESvrRegStatus eSvrRegStatus )
{
    auto found = sumSvrRegStatusToString.find ( eSvrRegStatus );
    if ( found == sumSvrRegStatusToString.end() )
        return QJsonValue ( QString ( "unknown(%1)" ).arg ( eSvrRegStatus ) );

    return QJsonValue ( QString::fromStdString ( found->second ) );
}
