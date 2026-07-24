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

#include "clientrpc.h"

CClientRpc::CClientRpc ( CClient* pClient, CClientSettings* pSettings, CRpcServer* pRpcServer, QObject* parent ) :
    QObject ( parent ),
    m_pSettings ( pSettings )
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
              [=] ( CHostAddress InetAddr, CVector<CServerInfo> vecServerInfo ) {
                  QJsonArray    arrServerInfo;
                  QSet<QString> setServerAddresses;
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
                      setServerAddresses.insert ( serverInfo.HostAddr.toString() );
                      pClient->CreateCLServerListPingMes ( serverInfo.HostAddr );
                  }
                  // remember which servers this directory listed, so jamulusclient/connect can tell
                  // a delisted server apart from one that was never seen in the first place
                  m_mapDirectoryServers[InetAddr.toString()] = setServerAddresses;
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
    connect ( pClient, &CClient::Disconnected, [=]() {
        // keep our record of the current connection (used by jamulusclient/disconnect) in sync,
        // whether the disconnect was initiated by us, the server, or the desktop UI
        m_strConnectedDirectory.clear();
        m_strConnectedServer.clear();
        m_strConnectStatus.clear();
        pRpcServer->BroadcastNotification ( "jamulusclient/disconnected", QJsonObject{} );
    } );

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

        // Allow IPv4 only for communicating with Directories
        if ( NetworkUtil::ParseNetworkAddress ( jsonDirectoryIp.toString(), haDirectoryAddress, false ) )
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

    /// @rpc_method jamulusclient/getDirectories
    /// @brief Returns the list of directories in the same order as presented in Jamulus.
    /// @param {object} params - No parameters (empty object).
    /// @result {array} result - Array of directory socket address strings, usable as params.directory in jamulusclient/pollServerList.
    pRpcServer->HandleMethod ( "jamulusclient/getDirectories", [=] ( const QJsonObject& params, QJsonObject& response ) {
        QJsonArray arrDirectories;
        // built-in directories in UI order
        for ( int i = AT_DEFAULT; i < AT_CUSTOM; i++ )
        {
            arrDirectories.append ( NetworkUtil::GetDirectoryAddress ( static_cast<EDirectoryType> ( i ), "" ) );
        }
        // custom directories — stored newest-first, displayed oldest-first (reverse iteration)
        for ( int i = MAX_NUM_SERVER_ADDR_ITEMS - 1; i >= 0; i-- )
        {
            if ( !m_pSettings->vstrDirectoryAddress[i].isEmpty() )
            {
                arrDirectories.append ( m_pSettings->vstrDirectoryAddress[i] );
            }
        }
        response["result"] = arrDirectories;
        Q_UNUSED ( params );
    } );

    /// @rpc_method jamulusclient/connect
    /// @brief Connects to a server previously discovered via jamulusclient/pollServerList on the given directory.
    ///  Blocks for a short time (up to a few seconds) while the outcome of the connection attempt is determined.
    /// @param {string} params.directory - Socket address of the directory the server was discovered from. Must
    ///  have already been queried via jamulusclient/pollServerList.
    /// @param {string} params.server - Socket address of the server to connect to, as returned in
    ///  params.servers[*].address of jamulusclient/serverListReceived.
    /// @result {string} result - "ok"; "Not Found" if the directory hasn't been polled, or the server address is
    ///  invalid; "Unauthorized" if the server requires accepting a licence, which isn't supported over this API;
    ///  "Gone (server no longer listed)" if the server is not, or is no longer, present in the directory's server
    ///  list; "Upgrade Required (obsolete protocol, upgrade Jamulus)" if the server requires a newer protocol
    ///  version than this client supports; or "Insufficient Storage (Full)" if the server is full. If the local
    ///  audio interface fails to start, an error is returned instead of a result.
    pRpcServer->HandleMethod ( "jamulusclient/connect", [=] ( const QJsonObject& params, QJsonObject& response ) {
        auto jsonDirectory = params["directory"];
        auto jsonServer    = params["server"];
        if ( !jsonDirectory.isString() || !jsonServer.isString() )
        {
            response["error"] =
                CRpcServer::CreateJsonRpcError ( CRpcServer::iErrInvalidParams, "Invalid params: directory and server must be strings" );
            return;
        }

        // Resolve the directory the same way jamulusclient/pollServerList does, so it matches the key that the
        // jamulusclient/serverListReceived handler cached the server list under.
        CHostAddress haDirectoryAddress;
        const QString strDirectoryKey =
            NetworkUtil::ParseNetworkAddress ( jsonDirectory.toString(), haDirectoryAddress, false ) ? haDirectoryAddress.toString() : QString();

        if ( strDirectoryKey.isEmpty() || !m_mapDirectoryServers.contains ( strDirectoryKey ) )
        {
            response["result"] = "Not Found";
            return;
        }

        const QString strServer = jsonServer.toString();

        if ( !m_mapDirectoryServers[strDirectoryKey].contains ( strServer ) )
        {
            response["result"] = "Gone (server no longer listed)";
            return;
        }

        // Delegate the actual connection attempt to the same method the desktop UI's "Connect" button uses
        // (CClientDlg::Connect), so both agree on what constitutes success vs. each failure mode.
        QString                       strErrorMessage;
        const CClient::EConnectResult eResult = pClient->ConnectToServer ( strServer, &strErrorMessage );

        if ( eResult == CClient::CR_SOUND_DEVICE_ERROR )
        {
            response["error"] = CRpcServer::CreateJsonRpcError ( 1, "Could not start the audio interface: " + strErrorMessage );
            return;
        }

        switch ( eResult )
        {
        case CClient::CR_OK:
            response["result"] = "ok";
            break;
        case CClient::CR_INVALID_ADDRESS:
            response["result"] = "Not Found";
            break;
        case CClient::CR_UNAUTHORIZED:
            response["result"] = "Unauthorized";
            break;
        case CClient::CR_GONE:
            response["result"] = "Gone (server no longer listed)";
            break;
        case CClient::CR_UPGRADE_REQUIRED:
            response["result"] = "Upgrade Required (obsolete protocol, upgrade Jamulus)";
            break;
        case CClient::CR_FULL:
            response["result"] = "Insufficient Storage (Full)";
            break;
        default:
            break;
        }

        // Unlike the desktop UI (which keeps the connection alive on CR_UNAUTHORIZED/CR_UPGRADE_REQUIRED so
        // the user can interact with the licence dialog, or simply because it never blocked on stale
        // versions), there is no interactive fallback here, so any non-ok outcome ends the attempt.
        if ( ( eResult != CClient::CR_OK ) && pClient->IsRunning() )
        {
            pClient->Stop();
        }

        m_strConnectedDirectory = jsonDirectory.toString();
        m_strConnectedServer    = strServer;
        m_strConnectStatus      = response["result"].toString();
    } );

    /// @rpc_method jamulusclient/disconnect
    /// @brief Disconnects from the server that was connected via jamulusclient/connect.
    /// @param {string} params.directory - Socket address of the directory that was passed to jamulusclient/connect.
    /// @param {string} params.server - Socket address of the server that was passed to jamulusclient/connect.
    /// @result {string} result - "ok"; "Not Found" if params.directory/params.server do not match the connection
    ///  established by the most recent jamulusclient/connect call; or, reflecting the outcome of that call,
    ///  "Unauthorized", "Gone (server no longer listed)", or "Upgrade Required (obsolete protocol, upgrade
    ///  Jamulus)".
    pRpcServer->HandleMethod ( "jamulusclient/disconnect", [=] ( const QJsonObject& params, QJsonObject& response ) {
        auto jsonDirectory = params["directory"];
        auto jsonServer    = params["server"];
        if ( !jsonDirectory.isString() || !jsonServer.isString() )
        {
            response["error"] =
                CRpcServer::CreateJsonRpcError ( CRpcServer::iErrInvalidParams, "Invalid params: directory and server must be strings" );
            return;
        }

        if ( ( jsonDirectory.toString() != m_strConnectedDirectory ) || ( jsonServer.toString() != m_strConnectedServer ) )
        {
            response["result"] = "Not Found";
            return;
        }

        // capture before Stop(): it synchronously emits CClient::Disconnected, which our own handler
        // above (kept in sync with the desktop UI's cleanup) reacts to by clearing these fields
        const QString strPreviousStatus = m_strConnectStatus;

        if ( pClient->IsRunning() )
        {
            pClient->Stop();
        }

        response["result"] = strPreviousStatus;
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
    /// @result {number} result.id - The channel ID assigned by the server, or -1 if not currently connected.
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
            { "id", pClient->GetMyChannelID() },
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

    /// @rpc_method jamulusclient/setFaderLevel
    /// @brief Sets the fader level. Example: {"id":1,"jsonrpc":"2.0","method":"jamulusclient/setFaderLevel","params":{"channelIndex": 0,"level":
    /// 50}}.
    /// @param {number} params.channelIndex - The channel index of the fader to be set.
    /// @param {number} params.level - The fader level in range 0..100.
    /// @result {string} result - Always "ok".
    pRpcServer->HandleMethod ( "jamulusclient/setFaderLevel", [=] ( const QJsonObject& params, QJsonObject& response ) {
        auto jsonChannelIndex = params["channelIndex"];
        if ( !jsonChannelIndex.isDouble() || ( jsonChannelIndex.toInt() < 0 ) || ( jsonChannelIndex.toInt() > MAX_NUM_CHANNELS ) )
        {
            response["error"] =
                CRpcServer::CreateJsonRpcError ( CRpcServer::iErrInvalidParams, "Invalid params: channelIndex is not a number or out-of-range" );
            return;
        }

        auto jsonLevel = params["level"];
        if ( !jsonLevel.isDouble() || ( jsonLevel.toInt() < 0 ) || ( jsonLevel.toInt() > 100 ) )
        {
            response["error"] =
                CRpcServer::CreateJsonRpcError ( CRpcServer::iErrInvalidParams, "Invalid params: level is not a number or out-of-range" );
            return;
        }

        pClient->SetControllerInFaderLevel ( jsonChannelIndex.toInt(), jsonLevel.toInt() );
        response["result"] = "ok";
    } );

    /// @rpc_method jamulusclient/getMidiSettings
    /// @brief Returns all MIDI controller settings.
    /// @param {object} params - No parameters (empty object).
    /// @result {object} result - MIDI settings object.
    pRpcServer->HandleMethod ( "jamulusclient/getMidiSettings", [=] ( const QJsonObject& params, QJsonObject& response ) {
        QJsonObject jsonMidiParams{ { "bUseMIDIController", m_pSettings->bUseMIDIController },
                                    { "midiDevice", m_pSettings->strMidiDevice },
                                    { "midiChannel", m_pSettings->iMidiChannel },
                                    { "midiMuteMyself", m_pSettings->iMidiMuteMyself },
                                    { "midiFaderOffset", m_pSettings->iMidiFaderOffset },
                                    { "midiFaderCount", m_pSettings->iMidiFaderCount },
                                    { "midiPanOffset", m_pSettings->iMidiPanOffset },
                                    { "midiPanCount", m_pSettings->iMidiPanCount },
                                    { "midiSoloOffset", m_pSettings->iMidiSoloOffset },
                                    { "midiSoloCount", m_pSettings->iMidiSoloCount },
                                    { "midiMuteOffset", m_pSettings->iMidiMuteOffset },
                                    { "midiMuteCount", m_pSettings->iMidiMuteCount },
                                    { "bMidiFaderEnabled", m_pSettings->bMidiFaderEnabled },
                                    { "bMidiPanEnabled", m_pSettings->bMidiPanEnabled },
                                    { "bMidiSoloEnabled", m_pSettings->bMidiSoloEnabled },
                                    { "bMidiMuteEnabled", m_pSettings->bMidiMuteEnabled },
                                    { "bMidiMuteMyselfEnabled", m_pSettings->bMidiMuteMyselfEnabled },
                                    { "bMIDIPickupMode", m_pSettings->bMIDIPickupMode } };
        response["result"] = jsonMidiParams;
        Q_UNUSED ( params );
    } );

    /// @rpc_method jamulusclient/setMidiSettings
    /// @brief Sets one or more MIDI controller settings.
    /// @param {object} params - Any subset of MIDI settings fields to set.
    /// @result {string} result - Always "ok".
    pRpcServer->HandleMethod ( "jamulusclient/setMidiSettings", [=] ( const QJsonObject& params, QJsonObject& response ) {
        bool bPreviousMIDIState = m_pSettings->bUseMIDIController;

        QHash<QString, std::function<void ( const QJsonValue& )>> setters = {
            { "bUseMIDIController", [this] ( const QJsonValue& v ) { m_pSettings->bUseMIDIController = v.toBool(); } },
            { "midiDevice", [this] ( const QJsonValue& v ) { m_pSettings->strMidiDevice = v.toString(); } },
            { "midiChannel", [this] ( const QJsonValue& v ) { m_pSettings->iMidiChannel = v.toInt(); } },
            { "midiMuteMyself", [this] ( const QJsonValue& v ) { m_pSettings->iMidiMuteMyself = v.toInt(); } },
            { "midiFaderOffset", [this] ( const QJsonValue& v ) { m_pSettings->iMidiFaderOffset = v.toInt(); } },
            { "midiFaderCount", [this] ( const QJsonValue& v ) { m_pSettings->iMidiFaderCount = v.toInt(); } },
            { "midiPanOffset", [this] ( const QJsonValue& v ) { m_pSettings->iMidiPanOffset = v.toInt(); } },
            { "midiPanCount", [this] ( const QJsonValue& v ) { m_pSettings->iMidiPanCount = v.toInt(); } },
            { "midiSoloOffset", [this] ( const QJsonValue& v ) { m_pSettings->iMidiSoloOffset = v.toInt(); } },
            { "midiSoloCount", [this] ( const QJsonValue& v ) { m_pSettings->iMidiSoloCount = v.toInt(); } },
            { "midiMuteOffset", [this] ( const QJsonValue& v ) { m_pSettings->iMidiMuteOffset = v.toInt(); } },
            { "midiMuteCount", [this] ( const QJsonValue& v ) { m_pSettings->iMidiMuteCount = v.toInt(); } },
            { "bMidiFaderEnabled", [this] ( const QJsonValue& v ) { m_pSettings->bMidiFaderEnabled = v.toBool(); } },
            { "bMidiPanEnabled", [this] ( const QJsonValue& v ) { m_pSettings->bMidiPanEnabled = v.toBool(); } },
            { "bMidiSoloEnabled", [this] ( const QJsonValue& v ) { m_pSettings->bMidiSoloEnabled = v.toBool(); } },
            { "bMidiMuteEnabled", [this] ( const QJsonValue& v ) { m_pSettings->bMidiMuteEnabled = v.toBool(); } },
            { "bMidiMuteMyselfEnabled", [this] ( const QJsonValue& v ) { m_pSettings->bMidiMuteMyselfEnabled = v.toBool(); } },
            { "bMIDIPickupMode", [this] ( const QJsonValue& v ) { m_pSettings->bMIDIPickupMode = v.toBool(); } } };

        for ( auto it = setters.constBegin(); it != setters.constEnd(); ++it )
        {
            if ( params.contains ( it.key() ) )
            {
                it.value() ( params[it.key()] );
            }
        }

        // If midiDevice was changed and MIDI is currently enabled, restart MIDI to reconnect
        bool bDeviceChanged = params.contains ( "midiDevice" );
        if ( bDeviceChanged && m_pSettings->bUseMIDIController && pClient->IsMIDIEnabled() )
        {
            // Disable MIDI
            pClient->EnableMIDI ( false );

            // Set the new device
            pClient->SetMIDIDevice ( m_pSettings->strMidiDevice );

            // Re-enable MIDI
            pClient->EnableMIDI ( true );

            // Check if reconnection was successful
            if ( !pClient->IsMIDIEnabled() )
            {
                response["error"] = CRpcServer::CreateJsonRpcError ( 1, "Failed to connect to MIDI device" );
                return;
            }
        }
        else if ( bDeviceChanged )
        {
            // Just update the device setting for next time MIDI is enabled
            pClient->SetMIDIDevice ( m_pSettings->strMidiDevice );
        }

        // Apply other settings to actually enable/disable MIDI
        pClient->SetSettings ( m_pSettings );

        // Check if MIDI was requested but failed to enable
        if ( m_pSettings->bUseMIDIController && !pClient->IsMIDIEnabled() )
        {
            // Restore previous state on failure
            m_pSettings->bUseMIDIController = bPreviousMIDIState;
            response["error"]               = CRpcServer::CreateJsonRpcError ( 1, "Failed to open MIDI port" );
            return;
        }

        response["result"] = "ok";
    } );

    /// @rpc_method jamulusclient/getMidiDevices
    /// @brief Returns a list of available MIDI input devices.
    /// @param {object} params - No parameters (empty object).
    /// @result {array} result - Array of MIDI device name strings.
    pRpcServer->HandleMethod ( "jamulusclient/getMidiDevices", [=] ( const QJsonObject& params, QJsonObject& response ) {
        QStringList deviceNames = pClient->GetMIDIDevNames();
        QJsonArray  jsonDevices;
        for ( const QString& deviceName : deviceNames )
        {
            jsonDevices.append ( deviceName );
        }
        response["result"] = jsonDevices;
        Q_UNUSED ( params );
    } );

    /// @rpc_method jamulusclient/setMuted
    /// @brief Mutes or unmutes the client.
    /// @param {boolean} params.muted - muted (true or false).
    /// @result {string} result - Always "ok".
    pRpcServer->HandleMethod ( "jamulusclient/setMuted", [=] ( const QJsonObject& params, QJsonObject& response ) {
        auto muted = params["muted"];
        if ( !muted.isBool() )
        {
            response["error"] = CRpcServer::CreateJsonRpcError ( CRpcServer::iErrInvalidParams, "Invalid params: muted is not a boolean" );
            return;
        }

        pClient->OnRPCInMuteMyself ( muted.toBool() );

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
