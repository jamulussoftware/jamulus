/******************************************************************************\
 * Copyright (c) 2004-2010
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

#if !defined ( PROTOCOL_H__3B123453_4344_BB2392354455IUHF1912__INCLUDED_ )
#define PROTOCOL_H__3B123453_4344_BB2392354455IUHF1912__INCLUDED_

#include <qglobal.h>
#include <qmutex.h>
#include <qtimer.h>
#include <qdatetime.h>
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
#define PROTMESSID_CONN_CLIENTS_LIST          14 // connected client list
#define PROTMESSID_SERVER_FULL                15 // server full message
#define PROTMESSID_REQ_CONN_CLIENTS_LIST      16 // request connected client list
#define PROTMESSID_CHANNEL_NAME               17 // set channel name for fader tag
#define PROTMESSID_CHAT_TEXT                  18 // contains a chat text
#define PROTMESSID_PING_MS                    19 // for measuring ping time
#define PROTMESSID_NETW_TRANSPORT_PROPS       20 // properties for network transport
#define PROTMESSID_REQ_NETW_TRANSPORT_PROPS   21 // request properties for network transport
#define PROTMESSID_DISCONNECTION              22 // disconnection
#define PROTMESSID_REQ_CHANNEL_NAME           23 // request channel name for fader tag

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
    void CreateConClientListMes ( const CVector<CChannelShortInfo>& vecChanInfo );
    void CreateServerFullMes();
    void CreateReqConnClientsList();
    void CreateChanNameMes ( const QString strName );
    void CreateReqChanNameMes();
    void CreateChatTextMes ( const QString strChatText );
    void CreatePingMes ( const int iMs );
    void CreateNetwTranspPropsMes ( const CNetworkTransportProps& NetTrProps );
    void CreateReqNetwTranspPropsMes();

    void CreateAndImmSendDisconnectionMes();
    void CreateAndImmSendAcknMess ( const int& iID, const int& iCnt );

    bool ParseMessage ( const CVector<uint8_t>& vecbyData,
                        const int iNumBytes );

    bool IsProtocolMessage ( const CVector<uint8_t>& vecbyData,
                             const int               iNumBytes );

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

        CVector<uint8_t>    vecMessage;
        int                 iID, iCnt;
    };

    void EnqueueMessage ( CVector<uint8_t>& vecMessage,
                          const int iCnt,
                          const int iID );

    bool ParseMessageFrame ( const CVector<uint8_t>& vecIn,
                             const int iNumBytesIn,
                             int& iCnt,
                             int& iID,
                             CVector<uint8_t>& vecData );

    void GenMessageFrame ( CVector<uint8_t>& vecOut,
                           const int iCnt,
                           const int iID,
                           const CVector<uint8_t>& vecData );

    void PutValOnStream ( CVector<uint8_t>& vecIn,
                          unsigned int& iPos,
                          const uint32_t iVal,
                          const unsigned int iNumOfBytes );

    uint32_t GetValFromStream ( const CVector<uint8_t>& vecIn,
                                unsigned int& iPos,
                                const unsigned int iNumOfBytes );

    void SendMessage();

    void CreateAndSendMessage ( const int iID, const CVector<uint8_t>& vecData );

    bool EvaluateJitBufMes             ( const CVector<uint8_t>& vecData );
    bool EvaluateReqJitBufMes          ( const CVector<uint8_t>& vecData );
    bool EvaluateChanGainMes           ( const CVector<uint8_t>& vecData );
    bool EvaluateConClientListMes      ( const CVector<uint8_t>& vecData );
    bool EvaluateServerFullMes         ( const CVector<uint8_t>& vecData );
    bool EvaluateReqConnClientsList    ( const CVector<uint8_t>& vecData );
    bool EvaluateChanNameMes           ( const CVector<uint8_t>& vecData );
    bool EvaluateReqChanNameMes        ( const CVector<uint8_t>& vecData );
    bool EvaluateChatTextMes           ( const CVector<uint8_t>& vecData );
    bool EvaluatePingMes               ( const CVector<uint8_t>& vecData );
    bool EvaluateNetwTranspPropsMes    ( const CVector<uint8_t>& vecData );
    bool EvaluateReqNetwTranspPropsMes ( const CVector<uint8_t>& vecData );
    bool EvaluateDisconnectionMes      ( const CVector<uint8_t>& vecData );

    int                     iOldRecID, iOldRecCnt;

    // these two objects must be sequred by a mutex
    uint8_t                 iCounter;
    std::list<CSendMessage> SendMessQueue;

    QTimer                  TimerSendMess;
    QMutex                  Mutex;

public slots:
    void OnTimerSendMess() { SendMessage(); }

signals:
    // transmitting
    void MessReadyForSending ( CVector<uint8_t> vecMessage );

    // receiving
    void ChangeJittBufSize ( int iNewJitBufSize );
    void ReqJittBufSize();
    void ChangeNetwBlSiFact ( int iNewNetwBlSiFact );
    void ChangeChanGain ( int iChanID, double dNewGain );
    void ConClientListMesReceived ( CVector<CChannelShortInfo> vecChanInfo );
    void ServerFull();
    void ReqConnClientsList();
    void ChangeChanName ( QString strName );
    void ReqChanName();
    void ChatTextReceived ( QString strChatText );
    void PingReceived ( int iMs );
    void NetTranspPropsReceived ( CNetworkTransportProps NetworkTransportProps );
    void ReqNetTranspProps();
    void Disconnection();
};

#endif /* !defined ( PROTOCOL_H__3B123453_4344_BB2392354455IUHF1912__INCLUDED_ ) */
