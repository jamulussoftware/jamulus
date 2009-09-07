/******************************************************************************\
 * Copyright (c) 2004-2009
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

#if !defined ( CHANNEL_HOIH9345KJH98_3_4344_BB23945IUHF1912__INCLUDED_ )
#define CHANNEL_HOIH9345KJH98_3_4344_BB23945IUHF1912__INCLUDED_

#include <qthread.h>
#include <qdatetime.h>
#include <qfile.h>
#include <qtextstream.h>
#include "global.h"
#include "buffer.h"
#include "util.h"
#include "protocol.h"


/* Definitions ****************************************************************/
// Set the time-out for the input buffer until the state changes from
// connected to not connected (the actual time depends on the way the error
// correction is implemented)
#define CON_TIME_OUT_SEC_MAX                30 // seconds

enum EPutDataStat
{
    PS_GEN_ERROR,
    PS_AUDIO_OK,
    PS_AUDIO_ERR,
    PS_PROT_OK,
    PS_PROT_ERR
};


/* Classes ********************************************************************/
class CChannel : public QObject
{
    Q_OBJECT

public:
    // we have to make "server" the default since I do not see a chance to
    // use constructor initialization in the server for a vector of channels
    CChannel ( const bool bNIsServer = true );
    virtual ~CChannel() {}

    EPutDataStat PutData ( const CVector<uint8_t>& vecbyData,
                           int iNumBytes );
    EGetDataStat GetData ( CVector<uint8_t>& vecbyData );

    CVector<uint8_t> PrepSendPacket ( const CVector<uint8_t>& vecbyNPacket );

    bool IsConnected() const { return iConTimeOut > 0; }

    void SetEnable ( const bool bNEnStat );

    void SetAddress ( const CHostAddress NAddr ) { InetAddr = NAddr; }
    bool GetAddress ( CHostAddress& RetAddr );
    CHostAddress GetAddress() const { return InetAddr; }

    void SetName ( const QString sNNa );
    QString GetName();

    void SetRemoteName ( const QString strName )
        { Protocol.CreateChanNameMes ( strName ); }

    void SetGain ( const int iChanID, const double dNewGain );
    double GetGain ( const int iChanID );

    void SetRemoteChanGain ( const int iId, const double dGain )
        { Protocol.CreateChanGainMes ( iId, dGain ); }

    bool SetSockBufNumFrames ( const int iNewNumFrames );
    int GetSockBufNumFrames() const { return iCurSockBufNumFrames; }

    int GetUploadRateKbps();

    double GetTimingStdDev() { return CycleTimeVariance.GetStdDev(); }

    // set/get network out buffer size and size factor
    void SetNetwFrameSizeAndFact ( const int iNewNetwFrameSize,
                                   const int iNewNetwFrameSizeFact );
    int GetNetwFrameSizeFact() const { return iNetwFrameSizeFact; }
    int GetNetwFrameSize() const { return iNetwFrameSize; }


    // network protocol interface
    void CreateJitBufMes ( const int iJitBufSize )
    { 
        if ( ProtocolIsEnabled() )
        {
            Protocol.CreateJitBufMes ( iJitBufSize );
        }
    }
    void CreateReqJitBufMes()                             { Protocol.CreateReqJitBufMes(); }
    void CreateReqConnClientsList()                       { Protocol.CreateReqConnClientsList(); }
    void CreateChatTextMes ( const QString& strChatText ) { Protocol.CreateChatTextMes ( strChatText ); }
    void CreatePingMes ( const int iMs )                  { Protocol.CreatePingMes ( iMs ); }

    void CreateConClientListMes ( const CVector<CChannelShortInfo>& vecChanInfo )
    { 
        Protocol.CreateConClientListMes ( vecChanInfo );
    }

    void CreateNetTranspPropsMessFromCurrentSettings();
    void CreateAndImmSendDisconnectionMes()
        { Protocol.CreateAndImmSendDisconnectionMes(); }

protected:
    bool ProtocolIsEnabled(); 

    // connection parameters
    CHostAddress        InetAddr;

    // channel name
    QString             sName;

    // mixer and effect settings
    CVector<double>     vecdGains;

    // network jitter-buffer
    CNetBuf             SockBuf;
    int                 iCurSockBufNumFrames;

    CCycleTimeVariance  CycleTimeVariance;

    // network output conversion buffer
    CConvBuf<uint8_t>   ConvBuf;

    // network protocol
    CProtocol           Protocol;

    int                 iConTimeOut;
    int                 iConTimeOutStartVal;

    bool                bIsEnabled;
    bool                bIsServer;

    int                 iNetwFrameSizeFact;
    int                 iNetwFrameSize;

    QMutex              Mutex;

public slots:
    void OnSendProtMessage ( CVector<uint8_t> vecMessage );
    void OnJittBufSizeChange ( int iNewJitBufSize );
    void OnChangeChanGain ( int iChanID, double dNewGain );
    void OnChangeChanName ( QString strName );
    void OnNetTranspPropsReceived ( CNetworkTransportProps NetworkTransportProps );
    void OnReqNetTranspProps();
    void OnDisconnection();

signals:
    void MessReadyForSending ( CVector<uint8_t> vecMessage );
    void NewConnection();
    void ReqJittBufSize();
    void ReqConnClientsList();
    void ConClientListMesReceived ( CVector<CChannelShortInfo> vecChanInfo );
    void NameHasChanged();
    void ChatTextReceived ( QString strChatText );
    void PingReceived ( int iMs );
    void ReqNetTranspProps();
    void Disconnected();
};

#endif /* !defined ( CHANNEL_HOIH9345KJH98_3_4344_BB23945IUHF1912__INCLUDED_ ) */
