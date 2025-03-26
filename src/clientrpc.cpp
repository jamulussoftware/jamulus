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

#include "clientrpc.h"

CClientRpc::CClientRpc ( CClient* pClient, CRpcServer* pRpcServer, QObject* parent ) : QObject ( parent )
{
    /// @rpc_notification jamulusclient/chatTextReceived
    /// @brief Emitted when a chat text is received.
    /// @param {string} params.chatText - The chat text.
    connect ( pClient, &CClient::ChatTextReceived, [=] ( QString strChatText ) {
        pRpcServer->BroadcastNotification ( "jamulusclient/chatTextReceived",
                                            QJsonObject{
                                                { "chatText", strChatText },
                                            } );
    } );

    /// @rpc_notification jamulusclient/connected
    /// @brief Emitted when the client is connected to the server.
    /// @param {number} params.id - The channel ID assigned to the client.
    connect ( pClient, &CClient::ClientIDReceived, [=] ( int iChanID ) {
        pRpcServer->BroadcastNotification ( "jamulusclient/connected",
                                            QJsonObject{
                                                { "id", iChanID },
                                            } );
    } );

    /// @rpc_notification jamulusclient/clientListReceived
    /// @brief Emitted when the client list is received.
    /// @param {array} params.clients - The client list.
    /// @param {number} params.clients[*].id - The channel ID.
    /// @param {string} params.clients[*].name - The musician’s name.
    /// @param {string} params.clients[*].skillLevel - The musician’s skill level (beginner, intermediate, expert, or null).
    /// @param {number} params.clients[*].countryId - The musician’s country ID (see QLocale::Country).
    /// @param {string} params.clients[*].country - The musician’s country.
    /// @param {string} params.clients[*].city - The musician’s city.
    /// @param {number} params.clients[*].instrumentId - The musician’s instrument ID (see CInstPictures::GetTable).
    /// @param {string} params.clients[*].instrument - The musician’s instrument.
    connect ( pClient, &CClient::ConClientListMesReceived, [=] ( CVector<CChannelInfo> vecChanInfo ) {
        QJsonArray arrChanInfo;
        for ( const auto& chanInfo : vecChanInfo )
        {
            QJsonObject objChanInfo{
                { "id", chanInfo.iChanID },
                { "name", chanInfo.strName },
                { "skillLevel", SerializeSkillLevel ( chanInfo.eSkillLevel ) },
                { "countryId", chanInfo.eCountry },
                { "country", QLocale::countryToString ( chanInfo.eCountry ) },
                { "city", chanInfo.strCity },
                { "instrumentId", chanInfo.iInstrument },
                { "instrument", CInstPictures::GetName ( chanInfo.iInstrument ) },
            };
            arrChanInfo.append ( objChanInfo );
        }
        pRpcServer->BroadcastNotification ( "jamulusclient/clientListReceived",
                                            QJsonObject{
                                                { "clients", arrChanInfo },
                                            } );
        arrStoredChanInfo = arrChanInfo;
    } );

    /// @rpc_notification jamulusclient/channelLevelListReceived
    /// @brief Emitted when the channel level list is received.
    /// @param {array} params.channelLevelList - The channel level list.
    ///  Each item corresponds to the respective client retrieved from the jamulusclient/clientListReceived notification.
    /// @param {number} params.channelLevelList[*] - The channel level, an integer between 0 and 9.
    connect ( pClient, &CClient::CLChannelLevelListReceived, [=] ( CHostAddress /* unused */, CVector<uint16_t> vecLevelList ) {
        QJsonArray arrLevelList;
        for ( const auto& level : vecLevelList )
        {
            arrLevelList.append ( level );
        }
        pRpcServer->BroadcastNotification ( "jamulusclient/channelLevelListReceived",
                                            QJsonObject{
                                                { "channelLevelList", arrLevelList },
                                            } );
    } );

    /// @rpc_notification jamulusclient/serverListReceived
    /// @brief Emitted when the server list is received.
    /// @param {array} params.servers - The server list.
    /// @param {string} params.servers[*].address - Socket address (ip_address:port).
    /// @param {string} params.servers[*].name - Server name.
    /// @param {number} params.servers[*].countryId - Server country ID (see QLocale::Country).
    /// @param {string} params.servers[*].country - Server country.
    /// @param {string} params.servers[*].city - Server city.
    connect ( pClient->getConnLessProtocol(),
              &CProtocol::CLServerListReceived,
              [=] ( CHostAddress /* unused */, CVector<CServerInfo> vecServerInfo ) {
                  QJsonArray arrServerInfo;
                  for ( const auto& serverInfo : vecServerInfo )
                  {
                      QJsonObject objServerInfo{
                          { "address", serverInfo.HostAddr.toString() },
                          { "name", serverInfo.strName },
                          { "countryId", serverInfo.eCountry },
                          { "country", QLocale::countryToString ( serverInfo.eCountry ) },
                          { "city", serverInfo.strCity },
                      };
                      arrServerInfo.append ( objServerInfo );
                      pClient->CreateCLServerListPingMes ( serverInfo.HostAddr );
                  }
                  pRpcServer->BroadcastNotification ( "jamulusclient/serverListReceived",
                                                      QJsonObject{
                                                          { "servers", arrServerInfo },
                                                      } );
              } );

    /// @rpc_notification jamulusclient/serverInfoReceived
    /// @brief Emitted when a server info is received.
    /// @param {string} params.address - The server socket address.
    /// @param {number} params.pingtime - The round-trip ping time, in milliseconds.
    /// @param {number} params.numClients - The number of clients connected to the server.
    connect ( pClient, &CClient::CLPingTimeWithNumClientsReceived, [=] ( CHostAddress InetAddr, int iPingTime, int iNumClients ) {
        pRpcServer->BroadcastNotification (
            "jamulusclient/serverInfoReceived",
            QJsonObject{ { "address", InetAddr.toString() }, { "pingTime", iPingTime }, { "numClients", iNumClients } } );
    } );

    /// @rpc_notification jamulusclient/disconnected
    /// @brief Emitted when the client is disconnected from the server.
    /// @param {object} params - No parameters (empty object).
    connect ( pClient, &CClient::Disconnected, [=]() { pRpcServer->BroadcastNotification ( "jamulusclient/disconnected", QJsonObject{} ); } );

    /// @rpc_notification jamulusclient/recorderState
    /// @brief Emitted when the client is connected to a server whose recorder state changes.
    /// @param {number} params.state - The recorder state.
    connect ( pClient, &CClient::RecorderStateReceived, [=] ( const ERecorderState newRecorderState ) {
        pRpcServer->BroadcastNotification ( "jamulusclient/recorderState", QJsonObject{ { "state", newRecorderState } } );
    } );

    /// @rpc_method jamulusclient/pollServerList
    /// @brief Request list of servers in a directory.
    /// @param {string} params.directory - Socket address of directory to query. Example: anygenre1.jamulus.io:22124
    /// @result {string} result - "ok" or "error" if bad arguments.
    pRpcServer->HandleMethod ( "jamulusclient/pollServerList", [=] ( const QJsonObject& params, QJsonObject& response ) {
        auto jsonDirectoryIp = params["directory"];
        if ( !jsonDirectoryIp.isString() )
        {
            response["error"] = CRpcServer::CreateJsonRpcError ( CRpcServer::iErrInvalidParams, "Invalid params: directory is not a string" );
            return;
        }

        CHostAddress haDirectoryAddress;
        if ( NetworkUtil().ParseNetworkAddress ( jsonDirectoryIp.toString(), haDirectoryAddress, false ) )
        {
            // send the request for the server list
            pClient->CreateCLReqServerListMes ( haDirectoryAddress );
            response["result"] = "ok";
        }
        else
        {
            response["error"] =
                CRpcServer::CreateJsonRpcError ( CRpcServer::iErrInvalidParams, "Invalid params: directory is not a valid socket address" );
        }

        response["result"] = "ok";
    } );

    /// @rpc_method jamulus/getMode
    /// @brief Returns the current mode, i.e. whether Jamulus is running as a server or client.
    /// @param {object} params - No parameters (empty object).
    /// @result {string} result.mode - The current mode (server or client).
    pRpcServer->HandleMethod ( "jamulus/getMode", [=] ( const QJsonObject& params, QJsonObject& response ) {
        QJsonObject result{ { "mode", "client" } };
        response["result"] = result;
        Q_UNUSED ( params );
    } );

    /// @rpc_method jamulusclient/getClientInfo
    /// @brief Returns the client information.
    /// @param {object} params - No parameters (empty object).
    /// @result {boolean} result.connected - Whether the client is connected to the server.
    pRpcServer->HandleMethod ( "jamulusclient/getClientInfo", [=] ( const QJsonObject& params, QJsonObject& response ) {
        QJsonObject result{ { "connected", pClient->IsConnected() } };
        response["result"] = result;
        Q_UNUSED ( params );
    } );

    /// @rpc_method jamulusclient/getChannelInfo
    /// @brief Returns the client's profile information.
    /// @param {object} params - No parameters (empty object).
    /// @result {number} result.id - The channel ID.
    /// @result {string} result.name - The musician’s name.
    /// @result {string} result.skillLevel - The musician’s skill level (beginner, intermediate, expert, or null).
    /// @result {number} result.countryId - The musician’s country ID (see QLocale::Country).
    /// @result {string} result.country - The musician’s country.
    /// @result {string} result.city - The musician’s city.
    /// @result {number} result.instrumentId - The musician’s instrument ID (see CInstPictures::GetTable).
    /// @result {string} result.instrument - The musician’s instrument.
    /// @result {string} result.skillLevel - Your skill level (beginner, intermediate, expert, or null).
    pRpcServer->HandleMethod ( "jamulusclient/getChannelInfo", [=] ( const QJsonObject& params, QJsonObject& response ) {
        QJsonObject result{
            // TODO: We cannot include "id" here is pClient->ChannelInfo is a CChannelCoreInfo which lacks that field.
            { "name", pClient->ChannelInfo.strName },
            { "countryId", pClient->ChannelInfo.eCountry },
            { "country", QLocale::countryToString ( pClient->ChannelInfo.eCountry ) },
            { "city", pClient->ChannelInfo.strCity },
            { "instrumentId", pClient->ChannelInfo.iInstrument },
            { "instrument", CInstPictures::GetName ( pClient->ChannelInfo.iInstrument ) },
            { "skillLevel", SerializeSkillLevel ( pClient->ChannelInfo.eSkillLevel ) },
        };
        response["result"] = result;
        Q_UNUSED ( params );
    } );

    /// @rpc_method jamulusclient/getClientList
    /// @brief Returns the client list.
    /// @param {object} params - No parameters (empty object).
    /// @result {array} result.clients - The client list. See jamulusclient/clientListReceived for the format.
    pRpcServer->HandleMethod ( "jamulusclient/getClientList", [=] ( const QJsonObject& params, QJsonObject& response ) {
        if ( !pClient->IsConnected() )
        {
            response["error"] = CRpcServer::CreateJsonRpcError ( 1, "Client is not connected" );
            return;
        }

        QJsonObject result{
            { "clients", arrStoredChanInfo },
        };
        response["result"] = result;
        Q_UNUSED ( params );
    } );

    /// @rpc_method jamulusclient/setName
    /// @brief Sets your name.
    /// @param {string} params.name - The new name.
    /// @result {string} result - Always "ok".
    pRpcServer->HandleMethod ( "jamulusclient/setName", [=] ( const QJsonObject& params, QJsonObject& response ) {
        auto jsonName = params["name"];
        if ( !jsonName.isString() )
        {
            response["error"] = CRpcServer::CreateJsonRpcError ( CRpcServer::iErrInvalidParams, "Invalid params: name is not a string" );
            return;
        }

        pClient->ChannelInfo.strName = TruncateString ( jsonName.toString(), MAX_LEN_FADER_TAG );
        pClient->SetRemoteInfo();

        response["result"] = "ok";
    } );

    /// @rpc_method jamulusclient/setSkillLevel
    /// @brief Sets your skill level.
    /// @param {string} params.skillLevel - The new skill level (beginner, intermediate, expert, or null).
    /// @result {string} result - Always "ok".
    pRpcServer->HandleMethod ( "jamulusclient/setSkillLevel", [=] ( const QJsonObject& params, QJsonObject& response ) {
        auto jsonSkillLevel = params["skillLevel"];
        if ( jsonSkillLevel.isNull() )
        {
            pClient->ChannelInfo.eSkillLevel = SL_NOT_SET;
            pClient->SetRemoteInfo();
            return;
        }

        if ( !jsonSkillLevel.isString() )
        {
            response["error"] = CRpcServer::CreateJsonRpcError ( CRpcServer::iErrInvalidParams, "Invalid params: skillLevel is not a string" );
            return;
        }

        auto strSkillLevel = jsonSkillLevel.toString();
        if ( strSkillLevel == "beginner" )
        {
            pClient->ChannelInfo.eSkillLevel = SL_BEGINNER;
        }
        else if ( strSkillLevel == "intermediate" )
        {
            pClient->ChannelInfo.eSkillLevel = SL_INTERMEDIATE;
        }
        else if ( strSkillLevel == "expert" )
        {
            pClient->ChannelInfo.eSkillLevel = SL_PROFESSIONAL;
        }
        else
        {
            response["error"] = CRpcServer::CreateJsonRpcError ( CRpcServer::iErrInvalidParams,
                                                                 "Invalid params: skillLevel is not beginner, intermediate or expert" );
            return;
        }

        pClient->SetRemoteInfo();
        response["result"] = "ok";
    } );

    /// @rpc_method jamulusclient/sendChatText
    /// @brief Sends a chat text message.
    /// @param {string} params.chatText - The chat text message.
    /// @result {string} result - Always "ok".
    pRpcServer->HandleMethod ( "jamulusclient/sendChatText", [=] ( const QJsonObject& params, QJsonObject& response ) {
        auto jsonMessage = params["chatText"];
        if ( !jsonMessage.isString() )
        {
            response["error"] = CRpcServer::CreateJsonRpcError ( CRpcServer::iErrInvalidParams, "Invalid params: chatText is not a string" );
            return;
        }
        if ( !pClient->IsConnected() )
        {
            response["error"] = CRpcServer::CreateJsonRpcError ( 1, "Client is not connected" );
            return;
        }

        pClient->CreateChatTextMes ( jsonMessage.toString() );

        response["result"] = "ok";
    } );
}

QJsonValue CClientRpc::SerializeSkillLevel ( ESkillLevel eSkillLevel )
{
    switch ( eSkillLevel )
    {
    case SL_BEGINNER:
        return QJsonValue ( "beginner" );

    case SL_INTERMEDIATE:
        return QJsonValue ( "intermediate" );

    case SL_PROFESSIONAL:
        return QJsonValue ( "expert" );

    default:
        return QJsonValue ( QJsonValue::Null );
    }
}
