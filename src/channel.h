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
#define CON_TIME_OUT_SEC_MAX                60 // seconds

// no valid channel number
#define INVALID_CHANNEL_ID                  ( MAX_NUM_CHANNELS + 1 )

enum EPutDataStat
{
    PS_GEN_ERROR,
    PS_AUDIO_OK,
    PS_AUDIO_ERR,
    PS_PROT_OK,
    PS_PROT_ERR
};

enum EGetDataStat
{
    GS_BUFFER_OK,
    GS_BUFFER_UNDERRUN,
    GS_CHAN_NOW_DISCONNECTED,
    GS_CHAN_NOT_CONNECTED
};


/* Classes ********************************************************************/
// CChannel --------------------------------------------------------------------
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

    CVector<uint8_t> PrepSendPacket ( const CVector<short>& vecsNPacket );

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

    bool SetSockBufSize ( const int iNumBlocks );
    int GetSockBufSize() const { return iCurSockBufSize; }

    int GetAudioBlockSizeIn() { return NetwBufferInProps.iAudioBlockSize; }
    int GetUploadRateKbps();

    double GetTimingStdDev() { return CycleTimeVariance.GetStdDev(); }

    void SetNetwBufSizeFactOut ( const int iNewNetwBlSiFactOut );
    int GetNetwBufSizeFactOut() const { return iCurNetwOutBlSiFact; }

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

    void CreateNetwBlSiFactMes ( const int iNetwBlSiFact )
    { 
        if ( ProtocolIsEnabled() )
        {
            Protocol.CreateNetwBlSiFactMes ( iNetwBlSiFact );
        }
    }

    void CreateConClientListMes ( const CVector<CChannelShortInfo>& vecChanInfo )
    { 
        Protocol.CreateConClientListMes ( vecChanInfo );
    }

    void CreateNetTranspPropsMessFromCurrentSettings();
    void CreateDisconnectionMes() { Protocol.CreateDisconnectionMes(); }

protected:
    bool ProtocolIsEnabled(); 

    // audio compression
    int                 iAudComprSizeOut;

    // connection parameters
    CHostAddress        InetAddr;

    // channel name
    QString             sName;

    // mixer and effect settings
    CVector<double>     vecdGains;

    // network jitter-buffer
    CNetBuf             SockBuf;
    int                 iCurSockBufSize;

    CCycleTimeVariance  CycleTimeVariance;

    // network output conversion buffer
    CConvBuf            ConvBuf;

    // network protocol
    CProtocol           Protocol;

    int                 iConTimeOut;
    int                 iConTimeOutStartVal;

    bool                bIsEnabled;
    bool                bIsServer;

    int                 iCurNetwOutBlSiFact;

    QMutex              Mutex;

    struct sNetwProperties
    {
        int iNetwInBufSize;
        int iAudioBlockSize;
    };
    sNetwProperties NetwBufferInProps;

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
};


// CChannelSet (for server) ----------------------------------------------------
class CChannelSet : public QObject
{
    Q_OBJECT

public:
    CChannelSet();
    virtual ~CChannelSet() {}

    bool PutData ( const CVector<uint8_t>& vecbyRecBuf,
                   const int iNumBytesRead, const CHostAddress& HostAdr );

    int GetFreeChan();

    int CheckAddr ( const CHostAddress& Addr );

    void GetBlockAllConC ( CVector<int>& vecChanID,
                           CVector<CVector<double> >& vecvecdData,
                           CVector<CVector<double> >& vecvecdGains );

    void GetConCliParam ( CVector<CHostAddress>& vecHostAddresses,
                          CVector<QString>& vecsName,
                          CVector<int>& veciJitBufSize,
                          CVector<int>& veciNetwOutBlSiFact );

    // access functions for actual channels
    bool IsConnected ( const int iChanNum )
        { return vecChannels[iChanNum].IsConnected(); }

    CVector<uint8_t> PrepSendPacket ( const int iChanNum,
                                      const CVector<short>& vecsNPacket )
        { return vecChannels[iChanNum].PrepSendPacket ( vecsNPacket ); }

    CHostAddress GetAddress ( const int iChanNum )
        { return vecChannels[iChanNum].GetAddress(); }

    void StartStatusHTMLFileWriting ( const QString& strNewFileName,
                                      const QString& strNewServerNameWithPort );

protected:
    CVector<CChannelShortInfo> CreateChannelList();
    void CreateAndSendChanListForAllConChannels();
    void CreateAndSendChanListForAllExceptThisChan ( const int iCurChanID );
    void CreateAndSendChanListForThisChan ( const int iCurChanID );
    void CreateAndSendChatTextForAllConChannels ( const int iCurChanID, const QString& strChatText );
    void WriteHTMLChannelList();

    /* do not use the vector class since CChannel does not have appropriate
       copy constructor/operator */
    CChannel         vecChannels[MAX_NUM_CHANNELS];
    QMutex           Mutex;

    CVector<QString> vstrChatColors;

    // HTML file server status
    bool             bWriteStatusHTMLFile;
    QString          strServerHTMLFileListName;
    QString          strServerNameWithPort;

public slots:
    // CODE TAG: MAX_NUM_CHANNELS_TAG
    // make sure we have MAX_NUM_CHANNELS connections!!!
    // send message
    void OnSendProtMessCh0 ( CVector<uint8_t> mess ) { emit MessReadyForSending ( 0, mess ); }
    void OnSendProtMessCh1 ( CVector<uint8_t> mess ) { emit MessReadyForSending ( 1, mess ); }
    void OnSendProtMessCh2 ( CVector<uint8_t> mess ) { emit MessReadyForSending ( 2, mess ); }
    void OnSendProtMessCh3 ( CVector<uint8_t> mess ) { emit MessReadyForSending ( 3, mess ); }
    void OnSendProtMessCh4 ( CVector<uint8_t> mess ) { emit MessReadyForSending ( 4, mess ); }
    void OnSendProtMessCh5 ( CVector<uint8_t> mess ) { emit MessReadyForSending ( 5, mess ); }
    void OnSendProtMessCh6 ( CVector<uint8_t> mess ) { emit MessReadyForSending ( 6, mess ); }
    void OnSendProtMessCh7 ( CVector<uint8_t> mess ) { emit MessReadyForSending ( 7, mess ); }
    void OnSendProtMessCh8 ( CVector<uint8_t> mess ) { emit MessReadyForSending ( 8, mess ); }
    void OnSendProtMessCh9 ( CVector<uint8_t> mess ) { emit MessReadyForSending ( 9, mess ); }

    void OnNewConnectionCh0() { vecChannels[0].CreateReqJitBufMes(); }
    void OnNewConnectionCh1() { vecChannels[1].CreateReqJitBufMes(); }
    void OnNewConnectionCh2() { vecChannels[2].CreateReqJitBufMes(); }
    void OnNewConnectionCh3() { vecChannels[3].CreateReqJitBufMes(); }
    void OnNewConnectionCh4() { vecChannels[4].CreateReqJitBufMes(); }
    void OnNewConnectionCh5() { vecChannels[5].CreateReqJitBufMes(); }
    void OnNewConnectionCh6() { vecChannels[6].CreateReqJitBufMes(); }
    void OnNewConnectionCh7() { vecChannels[7].CreateReqJitBufMes(); }
    void OnNewConnectionCh8() { vecChannels[8].CreateReqJitBufMes(); }
    void OnNewConnectionCh9() { vecChannels[9].CreateReqJitBufMes(); }

    void OnReqConnClientsListCh0() { CreateAndSendChanListForThisChan ( 0 ); }
    void OnReqConnClientsListCh1() { CreateAndSendChanListForThisChan ( 1 ); }
    void OnReqConnClientsListCh2() { CreateAndSendChanListForThisChan ( 2 ); }
    void OnReqConnClientsListCh3() { CreateAndSendChanListForThisChan ( 3 ); }
    void OnReqConnClientsListCh4() { CreateAndSendChanListForThisChan ( 4 ); }
    void OnReqConnClientsListCh5() { CreateAndSendChanListForThisChan ( 5 ); }
    void OnReqConnClientsListCh6() { CreateAndSendChanListForThisChan ( 6 ); }
    void OnReqConnClientsListCh7() { CreateAndSendChanListForThisChan ( 7 ); }
    void OnReqConnClientsListCh8() { CreateAndSendChanListForThisChan ( 8 ); }
    void OnReqConnClientsListCh9() { CreateAndSendChanListForThisChan ( 9 ); }

    void OnNameHasChangedCh0() { CreateAndSendChanListForAllConChannels(); }
    void OnNameHasChangedCh1() { CreateAndSendChanListForAllConChannels(); }
    void OnNameHasChangedCh2() { CreateAndSendChanListForAllConChannels(); }
    void OnNameHasChangedCh3() { CreateAndSendChanListForAllConChannels(); }
    void OnNameHasChangedCh4() { CreateAndSendChanListForAllConChannels(); }
    void OnNameHasChangedCh5() { CreateAndSendChanListForAllConChannels(); }
    void OnNameHasChangedCh6() { CreateAndSendChanListForAllConChannels(); }
    void OnNameHasChangedCh7() { CreateAndSendChanListForAllConChannels(); }
    void OnNameHasChangedCh8() { CreateAndSendChanListForAllConChannels(); }
    void OnNameHasChangedCh9() { CreateAndSendChanListForAllConChannels(); }

    void OnChatTextReceivedCh0 ( QString strChatText ) { CreateAndSendChatTextForAllConChannels ( 0, strChatText ); }
    void OnChatTextReceivedCh1 ( QString strChatText ) { CreateAndSendChatTextForAllConChannels ( 1, strChatText ); }
    void OnChatTextReceivedCh2 ( QString strChatText ) { CreateAndSendChatTextForAllConChannels ( 2, strChatText ); }
    void OnChatTextReceivedCh3 ( QString strChatText ) { CreateAndSendChatTextForAllConChannels ( 3, strChatText ); }
    void OnChatTextReceivedCh4 ( QString strChatText ) { CreateAndSendChatTextForAllConChannels ( 4, strChatText ); }
    void OnChatTextReceivedCh5 ( QString strChatText ) { CreateAndSendChatTextForAllConChannels ( 5, strChatText ); }
    void OnChatTextReceivedCh6 ( QString strChatText ) { CreateAndSendChatTextForAllConChannels ( 6, strChatText ); }
    void OnChatTextReceivedCh7 ( QString strChatText ) { CreateAndSendChatTextForAllConChannels ( 7, strChatText ); }
    void OnChatTextReceivedCh8 ( QString strChatText ) { CreateAndSendChatTextForAllConChannels ( 8, strChatText ); }
    void OnChatTextReceivedCh9 ( QString strChatText ) { CreateAndSendChatTextForAllConChannels ( 9, strChatText ); }

    void OnPingReceivedCh0 ( int iMs ) { vecChannels[0].CreatePingMes ( iMs ); }
    void OnPingReceivedCh1 ( int iMs ) { vecChannels[1].CreatePingMes ( iMs ); }
    void OnPingReceivedCh2 ( int iMs ) { vecChannels[2].CreatePingMes ( iMs ); }
    void OnPingReceivedCh3 ( int iMs ) { vecChannels[3].CreatePingMes ( iMs ); }
    void OnPingReceivedCh4 ( int iMs ) { vecChannels[4].CreatePingMes ( iMs ); }
    void OnPingReceivedCh5 ( int iMs ) { vecChannels[5].CreatePingMes ( iMs ); }
    void OnPingReceivedCh6 ( int iMs ) { vecChannels[6].CreatePingMes ( iMs ); }
    void OnPingReceivedCh7 ( int iMs ) { vecChannels[7].CreatePingMes ( iMs ); }
    void OnPingReceivedCh8 ( int iMs ) { vecChannels[8].CreatePingMes ( iMs ); }
    void OnPingReceivedCh9 ( int iMs ) { vecChannels[9].CreatePingMes ( iMs ); }

signals:
    void MessReadyForSending ( int iChID, CVector<uint8_t> vecMessage );
    void ChannelConnected ( CHostAddress ChanAddr );
};

#endif /* !defined ( CHANNEL_HOIH9345KJH98_3_4344_BB23945IUHF1912__INCLUDED_ ) */
