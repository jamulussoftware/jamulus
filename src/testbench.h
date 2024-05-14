/******************************************************************************\
 * Copyright (c) 2004-2024
 *
 * Author(s):
 *  Volker Fischer
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
#include <QTimer>
#include <QDateTime>
#include <QUdpSocket>
#include <QHostAddress>
#include "global.h"
#include "socket.h"
#include "protocol.h"
#include "util.h"

/* Classes ********************************************************************/
class CTestbench : public QObject
{
    Q_OBJECT

public:
    CTestbench ( QString sNewAddress, quint16 iNewPort ) : sAddress ( sNewAddress ), iPort ( iNewPort )
    {
        sLAddress = GenRandomIPv4Address().toString();
        iLPort    = static_cast<quint16> ( GenRandomIntInRange ( -2, 10000 ) );

        // bind socket (try 100 port numbers)
        quint16 iPortIncrement = 0;     // start value: port number plus ten
        bool    bSuccess       = false; // initialization for while loop

        while ( !bSuccess && ( iPortIncrement <= 100 ) )
        {
            bSuccess = UdpSocket.bind ( QHostAddress ( QHostAddress::Any ), 22222 + iPortIncrement );

            iPortIncrement++;
        }

        // connect protocol signals
        QObject::connect ( &Protocol, &CProtocol::MessReadyForSending, this, &CTestbench::OnSendProtMessage );

        QObject::connect ( &Protocol, &CProtocol::CLMessReadyForSending, this, &CTestbench::OnSendCLMessage );

        // connect and start the timer (testbench heartbeat)
        QObject::connect ( &Timer, &QTimer::timeout, this, &CTestbench::OnTimer );

        Timer.start ( 1 ); // 1 ms
    }

protected:
    int GenRandomIntInRange ( const int iStart, const int iEnd ) const
    {
        return static_cast<int> ( iStart + ( ( static_cast<double> ( iEnd - iStart + 1 ) * rand() ) / RAND_MAX ) );
    }

    QString GenRandomString() const
    {
        const int iLen      = GenRandomIntInRange ( 0, 111 );
        QString   strReturn = "";

        for ( int i = 0; i < iLen; i++ )
        {
            strReturn += static_cast<char> ( GenRandomIntInRange ( 0, 255 ) );
        }

        return strReturn;
    }

    QHostAddress GenRandomIPv4Address() const
    {
        quint32 a = static_cast<quint32> ( 192 );
        quint32 b = static_cast<quint32> ( 168 );
        quint32 c = static_cast<quint32> ( GenRandomIntInRange ( 1, 253 ) );
        quint32 d = static_cast<quint32> ( GenRandomIntInRange ( 1, 253 ) );
        return QHostAddress ( a << 24 | b << 16 | c << 8 | d );
    }

    QString    sAddress;
    quint16    iPort;
    QString    sLAddress;
    quint16    iLPort;
    QTimer     Timer;
    CProtocol  Protocol;
    QUdpSocket UdpSocket;

public slots:
    void OnTimer()
    {
        CVector<CChannelInfo>  vecChanInfo ( 1 );
        CNetworkTransportProps NetTrProps;
        CServerCoreInfo        ServerInfo;
        CVector<CServerInfo>   vecServerInfo ( 1 );
        CVector<uint16_t>      vecLevelList ( 1 );
        CHostAddress           CurHostAddress ( QHostAddress ( sAddress ), iPort );
        CHostAddress           CurLocalAddress ( QHostAddress ( sLAddress ), iLPort );
        CChannelCoreInfo       ChannelCoreInfo;
        ELicenceType           eLicenceType;
        ESvrRegResult          eSvrRegResult;

        // generate random protocol message
        switch ( GenRandomIntInRange ( 0, 34 ) )
        {
        case 0: // PROTMESSID_JITT_BUF_SIZE
            Protocol.CreateJitBufMes ( GenRandomIntInRange ( 0, 10 ) );
            break;

        case 1: // PROTMESSID_REQ_JITT_BUF_SIZE
            Protocol.CreateReqJitBufMes();
            break;

        case 2: // PROTMESSID_CHANNEL_GAIN
            Protocol.CreateChanGainMes ( GenRandomIntInRange ( 0, 20 ), GenRandomIntInRange ( -100, 100 ) );
            break;

        case 4: // PROTMESSID_CONN_CLIENTS_LIST
            vecChanInfo[0].iChanID = GenRandomIntInRange ( -2, 20 );
            vecChanInfo[0].strName = GenRandomString();

            Protocol.CreateConClientListMes ( vecChanInfo );
            break;

        case 5: // PROTMESSID_REQ_CONN_CLIENTS_LIST
            Protocol.CreateReqConnClientsList();
            break;

        case 7: // PROTMESSID_CHANNEL_INFOS
            ChannelCoreInfo.eCountry    = static_cast<QLocale::Country> ( GenRandomIntInRange ( 0, 100 ) );
            ChannelCoreInfo.eSkillLevel = static_cast<ESkillLevel> ( GenRandomIntInRange ( 0, 3 ) );
            ChannelCoreInfo.iInstrument = GenRandomIntInRange ( 0, 100 );
            ChannelCoreInfo.strCity     = GenRandomString();
            ChannelCoreInfo.strName     = GenRandomString();

            Protocol.CreateChanInfoMes ( ChannelCoreInfo );
            break;

        case 8: // PROTMESSID_REQ_CHANNEL_INFOS
            Protocol.CreateReqChanInfoMes();
            break;

        case 9: // PROTMESSID_CHAT_TEXT
            Protocol.CreateChatTextMes ( GenRandomString() );
            break;

        case 10: // PROTMESSID_LICENCE_REQUIRED
            eLicenceType = static_cast<ELicenceType> ( GenRandomIntInRange ( 0, 1 ) );

            Protocol.CreateLicenceRequiredMes ( eLicenceType );
            break;

        case 11: // PROTMESSID_NETW_TRANSPORT_PROPS
            NetTrProps.eAudioCodingType       = static_cast<EAudComprType> ( GenRandomIntInRange ( 0, 2 ) );
            NetTrProps.iAudioCodingArg        = GenRandomIntInRange ( -100, 100 );
            NetTrProps.iBaseNetworkPacketSize = GenRandomIntInRange ( -2, 1000 );
            NetTrProps.iBlockSizeFact         = GenRandomIntInRange ( -2, 100 );
            NetTrProps.iNumAudioChannels      = GenRandomIntInRange ( -2, 10 );
            NetTrProps.iSampleRate            = GenRandomIntInRange ( -2, 10000 );
            NetTrProps.eFlags                 = static_cast<ENetwFlags> ( GenRandomIntInRange ( 0, 1 ) );

            Protocol.CreateNetwTranspPropsMes ( NetTrProps );
            break;

        case 12: // PROTMESSID_REQ_NETW_TRANSPORT_PROPS
            Protocol.CreateReqNetwTranspPropsMes();
            break;

        case 14: // PROTMESSID_CLM_PING_MS
            Protocol.CreateCLPingMes ( CurHostAddress, GenRandomIntInRange ( -2, 1000 ) );
            break;

        case 15: // PROTMESSID_CLM_PING_MS_WITHNUMCLIENTS
            Protocol.CreateCLPingWithNumClientsMes ( CurHostAddress, GenRandomIntInRange ( -2, 1000 ), GenRandomIntInRange ( -2, 1000 ) );
            break;

        case 16: // PROTMESSID_CLM_SERVER_FULL
            Protocol.CreateCLServerFullMes ( CurHostAddress );
            break;

        case 17: // PROTMESSID_CLM_REGISTER_SERVER
            ServerInfo.bPermanentOnline = static_cast<bool> ( GenRandomIntInRange ( 0, 1 ) );
            ServerInfo.eCountry         = static_cast<QLocale::Country> ( GenRandomIntInRange ( 0, 100 ) );
            ServerInfo.iMaxNumClients   = GenRandomIntInRange ( -2, 10000 );
            ServerInfo.strCity          = GenRandomString();
            ServerInfo.strName          = GenRandomString();

            Protocol.CreateCLRegisterServerMes ( CurHostAddress, CurLocalAddress, ServerInfo );
            break;

        case 18: // PROTMESSID_CLM_UNREGISTER_SERVER
            Protocol.CreateCLUnregisterServerMes ( CurHostAddress );
            break;

        case 19: // PROTMESSID_CLM_SERVER_LIST
            vecServerInfo[0].bPermanentOnline = static_cast<bool> ( GenRandomIntInRange ( 0, 1 ) );
            vecServerInfo[0].eCountry         = static_cast<QLocale::Country> ( GenRandomIntInRange ( 0, 100 ) );
            vecServerInfo[0].HostAddr         = CurHostAddress;
            vecServerInfo[0].LHostAddr        = CurLocalAddress;
            vecServerInfo[0].iMaxNumClients   = GenRandomIntInRange ( -2, 10000 );
            vecServerInfo[0].strCity          = GenRandomString();
            vecServerInfo[0].strName          = GenRandomString();

            Protocol.CreateCLServerListMes ( CurHostAddress, vecServerInfo );
            break;

        case 20: // PROTMESSID_CLM_REQ_SERVER_LIST
            Protocol.CreateCLReqServerListMes ( CurHostAddress );
            break;

        case 21: // PROTMESSID_CLM_SEND_EMPTY_MESSAGE
            Protocol.CreateCLSendEmptyMesMes ( CurHostAddress, CurHostAddress );
            break;

        case 22: // PROTMESSID_CLM_EMPTY_MESSAGE
            Protocol.CreateCLEmptyMes ( CurHostAddress );
            break;

        case 23: // PROTMESSID_CLM_DISCONNECTION
            Protocol.CreateCLDisconnection ( CurHostAddress );
            break;

        case 24: // PROTMESSID_CLM_VERSION_AND_OS
            Protocol.CreateCLVersionAndOSMes ( CurHostAddress );
            break;

        case 25: // PROTMESSID_CLM_REQ_VERSION_AND_OS
            Protocol.CreateCLReqVersionAndOSMes ( CurHostAddress );
            break;

        case 26: // PROTMESSID_ACKN
            Protocol.CreateAndImmSendAcknMess ( GenRandomIntInRange ( -10, 100 ), GenRandomIntInRange ( -100, 100 ) );
            break;

        case 27:
        {
            // arbitrary "audio" packet (with random sizes)
            CVector<uint8_t> vecMessage ( GenRandomIntInRange ( 1, 1000 ) );
            OnSendProtMessage ( vecMessage );
            break;
        }

        case 28: // PROTMESSID_CLM_CONN_CLIENTS_LIST
            vecChanInfo[0].iChanID = GenRandomIntInRange ( -2, 20 );
            vecChanInfo[0].strName = GenRandomString();

            Protocol.CreateCLConnClientsListMes ( CurHostAddress, vecChanInfo );
            break;

        case 29: // PROTMESSID_CLM_REQ_CONN_CLIENTS_LIST
            Protocol.CreateCLReqConnClientsListMes ( CurHostAddress );
            break;

        case 30: // PROTMESSID_CLM_CHANNEL_LEVEL_LIST
            vecLevelList[0] = GenRandomIntInRange ( 0, 0xF );

            Protocol.CreateCLChannelLevelListMes ( CurHostAddress, vecLevelList, 1 );
            break;

        case 31: // PROTMESSID_CLM_REGISTER_SERVER_RESP
            eSvrRegResult = static_cast<ESvrRegResult> ( GenRandomIntInRange ( 0, 1 ) );

            Protocol.CreateCLRegisterServerResp ( CurHostAddress, eSvrRegResult );
            break;

        case 32: // PROTMESSID_CHANNEL_PAN
            Protocol.CreateChanPanMes ( GenRandomIntInRange ( -2, 20 ), GenRandomIntInRange ( 0, 32767 ) );
            break;

        case 33: // PROTMESSID_MUTE_STATE_CHANGED
            Protocol.CreateMuteStateHasChangedMes ( GenRandomIntInRange ( -2, 20 ), GenRandomIntInRange ( 0, 1 ) );
            break;

        case 34: // PROTMESSID_CLIENT_ID
            Protocol.CreateClientIDMes ( GenRandomIntInRange ( -2, 20 ) );
            break;
        }
    }

    void OnSendProtMessage ( CVector<uint8_t> vecMessage )
    {
        UdpSocket.writeDatagram ( (const char*) &( (CVector<uint8_t>) vecMessage )[0], vecMessage.Size(), QHostAddress ( sAddress ), iPort );

        // reset protocol so that we do not have to wait for an acknowledge to
        // send the next message
        Protocol.Reset();
    }

    void OnSendCLMessage ( CHostAddress, CVector<uint8_t> vecMessage ) { OnSendProtMessage ( vecMessage ); }
};
