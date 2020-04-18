/******************************************************************************\
 * Copyright (c) 2004-2020
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
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
\******************************************************************************/

#pragma once

#include <QMutex>
#include <QTimer>
#include <QDateTime>
#include <list>
#include "global.h"
#include "util.h"


/* Definitions ****************************************************************/
// protocol message IDs
#define PROTMESSID_ILLEGAL                     0 // illegal ID
#define PROTMESSID_ACKN                        1 // acknowledge
#define PROTMESSID_JITT_BUF_SIZE              10 // jitter buffer size
#define PROTMESSID_REQ_JITT_BUF_SIZE          11 // request jitter buffer size
#define PROTMESSID_NET_BLSI_FACTOR            12 // OLD (not used anymore)
#define PROTMESSID_CHANNEL_GAIN               13 // set channel gain for mix
#define PROTMESSID_CONN_CLIENTS_LIST_NAME     14 // OLD (not used anymore)
#define PROTMESSID_SERVER_FULL                15 // OLD (not used anymore)
#define PROTMESSID_REQ_CONN_CLIENTS_LIST      16 // request connected client list
#define PROTMESSID_CHANNEL_NAME               17 // OLD (not used anymore)
#define PROTMESSID_CHAT_TEXT                  18 // contains a chat text
#define PROTMESSID_PING_MS                    19 // OLD (not used anymore)
#define PROTMESSID_NETW_TRANSPORT_PROPS       20 // properties for network transport
#define PROTMESSID_REQ_NETW_TRANSPORT_PROPS   21 // request properties for network transport
#define PROTMESSID_DISCONNECTION              22 // OLD (not used anymore)
#define PROTMESSID_REQ_CHANNEL_INFOS          23 // request channel infos for fader tag
#define PROTMESSID_CONN_CLIENTS_LIST          24 // channel infos for connected clients
#define PROTMESSID_CHANNEL_INFOS              25 // set channel infos
#define PROTMESSID_OPUS_SUPPORTED             26 // tells that OPUS codec is supported
#define PROTMESSID_LICENCE_REQUIRED           27 // licence required
#define PROTMESSID_REQ_CHANNEL_LEVEL_LIST     28 // request the channel level list

// message IDs of connection less messages (CLM)
// DEFINITION -> start at 1000, end at 1999, see IsConnectionLessMessageID
#define PROTMESSID_CLM_PING_MS                1001 // for measuring ping time
#define PROTMESSID_CLM_PING_MS_WITHNUMCLIENTS 1002 // for ping time and num. of clients info
#define PROTMESSID_CLM_SERVER_FULL            1003 // server full message
#define PROTMESSID_CLM_REGISTER_SERVER        1004 // register server
#define PROTMESSID_CLM_UNREGISTER_SERVER      1005 // unregister server
#define PROTMESSID_CLM_SERVER_LIST            1006 // server list
#define PROTMESSID_CLM_REQ_SERVER_LIST        1007 // request server list
#define PROTMESSID_CLM_SEND_EMPTY_MESSAGE     1008 // an empty message shall be send
#define PROTMESSID_CLM_EMPTY_MESSAGE          1009 // empty message
#define PROTMESSID_CLM_DISCONNECTION          1010 // disconnection
#define PROTMESSID_CLM_VERSION_AND_OS         1011 // version number and operating system
#define PROTMESSID_CLM_REQ_VERSION_AND_OS     1012 // request version number and operating system
#define PROTMESSID_CLM_CONN_CLIENTS_LIST      1013 // channel infos for connected clients
#define PROTMESSID_CLM_REQ_CONN_CLIENTS_LIST  1014 // request the connected clients list
#define PROTMESSID_CLM_CHANNEL_LEVEL_LIST     1015 // channel level list
#define PROTMESSID_CLM_REGISTER_SERVER_RESP   1016 // status of server registration request

// lengths of message as defined in protocol.cpp file
#define MESS_HEADER_LENGTH_BYTE         7 // TAG (2), ID (2), cnt (1), length (2)
#define MESS_LEN_WITHOUT_DATA_BYTE      ( MESS_HEADER_LENGTH_BYTE + 2 /* CRC (2) */ )

// time out for message re-send if no acknowledgement was received
#define SEND_MESS_TIMEOUT_MS            400 // ms


/* Classes ********************************************************************/
class CProtocol : public QObject
{
    Q_OBJECT

public:
    CProtocol();

    void Reset();

    void CreateJitBufMes ( const int iJitBufSize );
    void CreateReqJitBufMes();
    void CreateChanGainMes ( const int iChanID, const double dGain );
    void CreateConClientListMes ( const CVector<CChannelInfo>& vecChanInfo );
    void CreateReqConnClientsList();
    void CreateChanInfoMes ( const CChannelCoreInfo ChanInfo );
    void CreateReqChanInfoMes();
    void CreateChatTextMes ( const QString strChatText );
    void CreateNetwTranspPropsMes ( const CNetworkTransportProps& NetTrProps );
    void CreateReqNetwTranspPropsMes();
    void CreateLicenceRequiredMes ( const ELicenceType eLicenceType );
    void CreateOpusSupportedMes();
    void CreateReqChannelLevelListMes ( const bool bRCL );

    void CreateCLPingMes               ( const CHostAddress& InetAddr, const int iMs );
    void CreateCLPingWithNumClientsMes ( const CHostAddress& InetAddr,
                                         const int           iMs,
                                         const int           iNumClients );
    void CreateCLServerFullMes         ( const CHostAddress& InetAddr );
    void CreateCLRegisterServerMes     ( const CHostAddress&    InetAddr,
                                         const CHostAddress&    LInetAddr,
                                         const CServerCoreInfo& ServerInfo );
    void CreateCLUnregisterServerMes   ( const CHostAddress& InetAddr );
    void CreateCLServerListMes         ( const CHostAddress&        InetAddr,
                                         const CVector<CServerInfo> vecServerInfo );
    void CreateCLReqServerListMes      ( const CHostAddress& InetAddr );
    void CreateCLSendEmptyMesMes       ( const CHostAddress& InetAddr,
                                         const CHostAddress& TargetInetAddr );
    void CreateCLEmptyMes              ( const CHostAddress& InetAddr );
    void CreateCLDisconnection         ( const CHostAddress& InetAddr );
    void CreateCLVersionAndOSMes       ( const CHostAddress& InetAddr );
    void CreateCLReqVersionAndOSMes    ( const CHostAddress& InetAddr );
    void CreateCLConnClientsListMes    ( const CHostAddress&          InetAddr,
                                         const CVector<CChannelInfo>& vecChanInfo );
    void CreateCLReqConnClientsListMes ( const CHostAddress& InetAddr );
    void CreateCLChannelLevelListMes   ( const CHostAddress&      InetAddr,
                                         const CVector<uint16_t>& vecLevelList,
                                         const int                iNumClients );
    void CreateCLRegisterServerResp    ( const CHostAddress& InetAddr,
                                         const ESvrRegResult eResult );

    static bool ParseMessageFrame ( const CVector<uint8_t>& vecbyData,
                                    const int               iNumBytesIn,
                                    CVector<uint8_t>&       vecbyMesBodyData,
                                    int&                    iRecCounter,
                                    int&                    iRecID );

    bool ParseMessageBody ( const CVector<uint8_t>& vecbyMesBodyData,
                            const int               iRecCounter,
                            const int               iRecID );

    bool ParseConnectionLessMessageBody ( const CVector<uint8_t>& vecbyMesBodyData,
                                          const int               iRecID,
                                          const CHostAddress&     InetAddr );

    static bool IsConnectionLessMessageID ( const int iID )
        { return ( iID >= 1000 ) && ( iID < 2000 ); }

    // this function is public because we need it in the test bench
    void CreateAndImmSendAcknMess ( const int& iID,
                                    const int& iCnt );

protected:
    class CSendMessage
    {
    public:
        CSendMessage() : vecMessage ( 0 ), iID ( PROTMESSID_ILLEGAL ),
            iCnt ( 0 ) {}
        CSendMessage ( const CVector<uint8_t>& nMess, const int iNCnt,
            const int iNID ) : vecMessage ( nMess ), iID ( iNID ),
            iCnt ( iNCnt ) {}

        CSendMessage& operator= ( const CSendMessage& NewSendMess )
        {
            vecMessage.Init ( NewSendMess.vecMessage.Size() );
            vecMessage = NewSendMess.vecMessage;

            iID  = NewSendMess.iID;
            iCnt = NewSendMess.iCnt;
            return *this; 
        }

        CVector<uint8_t> vecMessage;
        int              iID, iCnt;
    };

    void EnqueueMessage ( CVector<uint8_t>& vecMessage,
                          const int         iCnt,
                          const int         iID );

    void GenMessageFrame ( CVector<uint8_t>&       vecOut,
                           const int               iCnt,
                           const int               iID,
                           const CVector<uint8_t>& vecData );

    void PutValOnStream ( CVector<uint8_t>& vecIn,
                          int&              iPos,
                          const uint32_t    iVal,
                          const int         iNumOfBytes );

    void PutStringUTF8OnStream ( CVector<uint8_t>& vecIn,
                                 int&              iPos,
                                 const QByteArray& sStringUTF8 );

    static uint32_t GetValFromStream ( const CVector<uint8_t>& vecIn,
                                       int&                    iPos,
                                       const int               iNumOfBytes );

    bool GetStringFromStream ( const CVector<uint8_t>& vecIn,
                               int&                    iPos,
                               const int               iMaxStringLen,
                               QString&                strOut );

    void SendMessage();

    void CreateAndSendMessage ( const int               iID,
                                const CVector<uint8_t>& vecData );

    void CreateAndImmSendConLessMessage ( const int               iID,
                                          const CVector<uint8_t>& vecData,
                                          const CHostAddress&     InetAddr );

    bool EvaluateJitBufMes              ( const CVector<uint8_t>& vecData );
    bool EvaluateReqJitBufMes();
    bool EvaluateChanGainMes            ( const CVector<uint8_t>& vecData );
    bool EvaluateConClientListMes       ( const CVector<uint8_t>& vecData );
    bool EvaluateReqConnClientsList();
    bool EvaluateChanInfoMes            ( const CVector<uint8_t>& vecData );
    bool EvaluateReqChanInfoMes();
    bool EvaluateChatTextMes            ( const CVector<uint8_t>& vecData );
    bool EvaluateNetwTranspPropsMes     ( const CVector<uint8_t>& vecData );
    bool EvaluateReqNetwTranspPropsMes();
    bool EvaluateLicenceRequiredMes     ( const CVector<uint8_t>& vecData );
    bool EvaluateReqChannelLevelListMes ( const CVector<uint8_t>& vecData );

    bool EvaluateCLPingMes               ( const CHostAddress&     InetAddr,
                                           const CVector<uint8_t>& vecData );
    bool EvaluateCLPingWithNumClientsMes ( const CHostAddress&     InetAddr,
                                           const CVector<uint8_t>& vecData );
    bool EvaluateCLServerFullMes();
    bool EvaluateCLRegisterServerMes     ( const CHostAddress&     InetAddr,
                                           const CVector<uint8_t>& vecData );
    bool EvaluateCLUnregisterServerMes   ( const CHostAddress&     InetAddr );
    bool EvaluateCLServerListMes         ( const CHostAddress&     InetAddr,
                                           const CVector<uint8_t>& vecData );
    bool EvaluateCLReqServerListMes      ( const CHostAddress&     InetAddr );
    bool EvaluateCLSendEmptyMesMes       ( const CVector<uint8_t>& vecData );
    bool EvaluateCLDisconnectionMes      ( const CHostAddress&     InetAddr );
    bool EvaluateCLVersionAndOSMes       ( const CHostAddress&     InetAddr,
                                           const CVector<uint8_t>& vecData );
    bool EvaluateCLReqVersionAndOSMes    ( const CHostAddress&     InetAddr );
    bool EvaluateCLConnClientsListMes    ( const CHostAddress&     InetAddr,
                                           const CVector<uint8_t>& vecData );
    bool EvaluateCLReqConnClientsListMes ( const CHostAddress&     InetAddr );
    bool EvaluateCLChannelLevelListMes   ( const CHostAddress&     InetAddr,
                                           const CVector<uint8_t>& vecData );
    bool EvaluateCLRegisterServerResp    ( const CHostAddress&     InetAddr,
                                           const CVector<uint8_t>& vecData );

    int                     iOldRecID;
    int                     iOldRecCnt;

    // these two objects must be sequred by a mutex
    uint8_t                 iCounter;
    std::list<CSendMessage> SendMessQueue;

    QTimer                  TimerSendMess;
    QMutex                  Mutex;

public slots:
    void OnTimerSendMess() { SendMessage(); }

signals:
    // transmitting
    void MessReadyForSending   ( CVector<uint8_t> vecMessage );
    void CLMessReadyForSending ( CHostAddress     InetAddr,
                                 CVector<uint8_t> vecMessage );

    // receiving
    void ChangeJittBufSize ( int iNewJitBufSize );
    void ReqJittBufSize();
    void ChangeNetwBlSiFact ( int iNewNetwBlSiFact );
    void ChangeChanGain ( int iChanID, double dNewGain );
    void ConClientListMesReceived ( CVector<CChannelInfo> vecChanInfo );
    void ServerFullMesReceived();
    void ReqConnClientsList();
    void ChangeChanInfo ( CChannelCoreInfo ChanInfo );
    void ReqChanInfo();
    void ChatTextReceived ( QString strChatText );
    void NetTranspPropsReceived ( CNetworkTransportProps NetworkTransportProps );
    void ReqNetTranspProps();
    void LicenceRequired ( ELicenceType eLicenceType );
    void ReqChannelLevelList ( bool bOptIn );

    void CLPingReceived               ( CHostAddress           InetAddr,
                                        int                    iMs );
    void CLPingWithNumClientsReceived ( CHostAddress           InetAddr,
                                        int                    iMs,
                                        int                    iNumClients );
    void CLRegisterServerReceived     ( CHostAddress           InetAddr,
                                        CHostAddress           LInetAddr,
                                        CServerCoreInfo        ServerInfo );
    void CLUnregisterServerReceived   ( CHostAddress           InetAddr );
    void CLServerListReceived         ( CHostAddress           InetAddr,
                                        CVector<CServerInfo>   vecServerInfo );
    void CLReqServerList              ( CHostAddress           InetAddr );
    void CLSendEmptyMes               ( CHostAddress           TargetInetAddr );
    void CLDisconnection              ( CHostAddress           InetAddr );
    void CLVersionAndOSReceived       ( CHostAddress           InetAddr,
                                        COSUtil::EOpSystemType eOSType,
                                        QString                strVersion );
    void CLReqVersionAndOS            ( CHostAddress           InetAddr );
    void CLConnClientsListMesReceived ( CHostAddress           InetAddr,
                                        CVector<CChannelInfo>  vecChanInfo );
    void CLReqConnClientsList         ( CHostAddress           InetAddr );
    void CLChannelLevelListReceived   ( CHostAddress           InetAddr,
                                        CVector<uint16_t>      vecLevelList );
    void CLRegisterServerResp         ( CHostAddress           InetAddr,
                                        ESvrRegResult          eStatus );
};
