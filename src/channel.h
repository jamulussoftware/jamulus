/******************************************************************************\
 * Copyright (c) 2004-2008
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
#include "audiocompr.h"
#include "util.h"
#include "resample.h"
#include "protocol.h"


/* Definitions ****************************************************************/
// Set the time-out for the input buffer until the state changes from
// connected to not-connected (the actual time depends on the way the error
// correction is implemented)
#define CON_TIME_OUT_SEC_MAX        5 // seconds

// no valid channel number
#define INVALID_CHANNEL_ID          ( MAX_NUM_CHANNELS + 1 )

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
    CChannel();
    virtual ~CChannel() {}

    EPutDataStat PutData ( const CVector<unsigned char>& vecbyData,
                           int iNumBytes );
    EGetDataStat GetData ( CVector<double>& vecdData );

    CVector<unsigned char> PrepSendPacket ( const CVector<short>& vecsNPacket );

    bool IsConnected() const { return iConTimeOut > 0; }

    void SetEnable ( const bool bNEnStat );
    void SetIsServer ( const bool bNEnStat ) { bIsServer = bNEnStat; }

    void SetAddress ( const CHostAddress NAddr ) { InetAddr = NAddr; }
    bool GetAddress ( CHostAddress& RetAddr );
    CHostAddress GetAddress() { return InetAddr; }

    void SetName ( const QString sNNa ) { sName = sNNa; }
    QString GetName() { return sName; }

    void SetRemoteName ( const QString strName )
        { Protocol.CreateChanNameMes ( strName ); }

    void SetGain ( const int iNID, const double dNG ) { vecdGains[iNID] = dNG; }
    double GetGain( const int iNID ) { return vecdGains[iNID]; }

    void SetRemoteChanGain ( const int iId, const double dGain )
        { Protocol.CreateChanGainMes ( iId, dGain ); }

    void SetSockBufSize ( const int iNumBlocks );
    int GetSockBufSize() { return iCurSockBufSize; }

    void SetNetwBufSizeFactOut ( const int iNewNetwBlSiFactOut );
    int GetNetwBufSizeFactOut() { return iCurNetwOutBlSiFact; }

    int GetNetwBufSizeFactIn() { return iCurNetwInBlSiFact; }

    void SetAudioCompressionOut ( const CAudioCompression::EAudComprType eNewAudComprTypeOut );
    CAudioCompression::EAudComprType GetAudioCompressionOut() { return eAudComprTypeOut; }

    // network protocol interface
    void CreateJitBufMes ( const int iJitBufSize )
    { 
        if ( IsConnected() )
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
        if ( IsConnected() )
        {
            Protocol.CreateNetwBlSiFactMes ( iNetwBlSiFact );
        }
    }

    void CreateConClientListMes ( const CVector<CChannelShortInfo>& vecChanInfo )
    { 
        Protocol.CreateConClientListMes ( vecChanInfo );
    }

protected:
    void SetNetwInBlSiFactAndCompr ( const int iNewBlockSizeFactor,
                                     const CAudioCompression::EAudComprType eNewAudComprType );

    // audio compression
    CAudioCompression   AudioCompressionIn;
    CAudioCompression   AudioCompressionOut;
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

    // network output conversion buffer
    CConvBuf            ConvBuf;

    // network protocol
    CProtocol           Protocol;

    int                 iConTimeOut;
    int                 iConTimeOutStartVal;

    bool                bIsEnabled;
    bool                bIsServer;

    int                 iCurNetwInBlSiFact;
    int                 iCurNetwOutBlSiFact;

    QMutex              Mutex;

    struct sNetwBufferInProps
    {
        int                              iNetwInBufSize;
        int                              iBlockSizeFactor;
        CAudioCompression::EAudComprType eAudComprType;
    };
    CVector<sNetwBufferInProps> vecNetwBufferInProps;

    CAudioCompression::EAudComprType eAudComprTypeOut;

public slots:
    void OnSendProtMessage ( CVector<uint8_t> vecMessage );
    void OnJittBufSizeChange ( int iNewJitBufSize );
    void OnNetwBlSiFactChange ( int iNewNetwBlSiFact );
    void OnChangeChanGain ( int iChanID, double dNewGain );
    void OnChangeChanName ( QString strName );

signals:
    void MessReadyForSending ( CVector<uint8_t> vecMessage );
    void NewConnection();
    void ReqJittBufSize();
    void ReqConnClientsList();
    void ConClientListMesReceived ( CVector<CChannelShortInfo> vecChanInfo );
    void ProtocolStatus ( bool bOk );
    void NameHasChanged();
    void ChatTextReceived ( QString strChatText );
    void PingReceived ( int iMs );
};


// CChannelSet (for server) ----------------------------------------------------
class CChannelSet : public QObject
{
    Q_OBJECT

public:
    CChannelSet();
    virtual ~CChannelSet() {}

    bool PutData ( const CVector<unsigned char>& vecbyRecBuf,
                   const int iNumBytesRead, const CHostAddress& HostAdr );

    int GetFreeChan();

    int CheckAddr ( const CHostAddress& Addr );

    void GetBlockAllConC ( CVector<int>& vecChanID,
                           CVector<CVector<double> >& vecvecdData,
                           CVector<CVector<double> >& vecvecdGains );

    void GetConCliParam ( CVector<CHostAddress>& vecHostAddresses,
                          CVector<QString>& vecsName,
                          CVector<int>& veciJitBufSize,
                          CVector<int>& veciNetwOutBlSiFact,
                          CVector<int>& veciNetwInBlSiFact );

    // access functions for actual channels
    bool IsConnected ( const int iChanNum )
        { return vecChannels[iChanNum].IsConnected(); }

    CVector<unsigned char> PrepSendPacket ( const int iChanNum,
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
    CChannel    vecChannels[MAX_NUM_CHANNELS];
    QMutex      Mutex;

    // HTML file server status
    bool        bWriteStatusHTMLFile;
    QString     strServerHTMLFileListName;
    QString     strServerNameWithPort;

public slots:
    // CODE TAG: MAX_NUM_CHANNELS_TAG
    // make sure we have MAX_NUM_CHANNELS connections!!!
    // send message
    void OnSendProtMessCh0 ( CVector<uint8_t> mess ) {emit MessReadyForSending ( 0, mess ); }
    void OnSendProtMessCh1 ( CVector<uint8_t> mess ) {emit MessReadyForSending ( 1, mess ); }
    void OnSendProtMessCh2 ( CVector<uint8_t> mess ) {emit MessReadyForSending ( 2, mess ); }
    void OnSendProtMessCh3 ( CVector<uint8_t> mess ) {emit MessReadyForSending ( 3, mess ); }
    void OnSendProtMessCh4 ( CVector<uint8_t> mess ) {emit MessReadyForSending ( 4, mess ); }
    void OnSendProtMessCh5 ( CVector<uint8_t> mess ) {emit MessReadyForSending ( 5, mess ); }

    void OnNewConnectionCh0() { vecChannels[0].CreateReqJitBufMes(); }
    void OnNewConnectionCh1() { vecChannels[1].CreateReqJitBufMes(); }
    void OnNewConnectionCh2() { vecChannels[2].CreateReqJitBufMes(); }
    void OnNewConnectionCh3() { vecChannels[3].CreateReqJitBufMes(); }
    void OnNewConnectionCh4() { vecChannels[4].CreateReqJitBufMes(); }
    void OnNewConnectionCh5() { vecChannels[5].CreateReqJitBufMes(); }

    void OnReqConnClientsListCh0() { CreateAndSendChanListForThisChan ( 0 ); }
    void OnReqConnClientsListCh1() { CreateAndSendChanListForThisChan ( 1 ); }
    void OnReqConnClientsListCh2() { CreateAndSendChanListForThisChan ( 2 ); }
    void OnReqConnClientsListCh3() { CreateAndSendChanListForThisChan ( 3 ); }
    void OnReqConnClientsListCh4() { CreateAndSendChanListForThisChan ( 4 ); }
    void OnReqConnClientsListCh5() { CreateAndSendChanListForThisChan ( 5 ); }

    void OnNameHasChangedCh0() { CreateAndSendChanListForAllConChannels(); }
    void OnNameHasChangedCh1() { CreateAndSendChanListForAllConChannels(); }
    void OnNameHasChangedCh2() { CreateAndSendChanListForAllConChannels(); }
    void OnNameHasChangedCh3() { CreateAndSendChanListForAllConChannels(); }
    void OnNameHasChangedCh4() { CreateAndSendChanListForAllConChannels(); }
    void OnNameHasChangedCh5() { CreateAndSendChanListForAllConChannels(); }

    void OnChatTextReceivedCh0 ( QString strChatText ) { CreateAndSendChatTextForAllConChannels ( 0, strChatText ); }
    void OnChatTextReceivedCh1 ( QString strChatText ) { CreateAndSendChatTextForAllConChannels ( 1, strChatText ); }
    void OnChatTextReceivedCh2 ( QString strChatText ) { CreateAndSendChatTextForAllConChannels ( 2, strChatText ); }
    void OnChatTextReceivedCh3 ( QString strChatText ) { CreateAndSendChatTextForAllConChannels ( 3, strChatText ); }
    void OnChatTextReceivedCh4 ( QString strChatText ) { CreateAndSendChatTextForAllConChannels ( 4, strChatText ); }
    void OnChatTextReceivedCh5 ( QString strChatText ) { CreateAndSendChatTextForAllConChannels ( 5, strChatText ); }

    void OnPingReceivedCh0 ( int iMs ) { vecChannels[0].CreatePingMes ( iMs ); }
    void OnPingReceivedCh1 ( int iMs ) { vecChannels[1].CreatePingMes ( iMs ); }
    void OnPingReceivedCh2 ( int iMs ) { vecChannels[2].CreatePingMes ( iMs ); }
    void OnPingReceivedCh3 ( int iMs ) { vecChannels[3].CreatePingMes ( iMs ); }
    void OnPingReceivedCh4 ( int iMs ) { vecChannels[4].CreatePingMes ( iMs ); }
    void OnPingReceivedCh5 ( int iMs ) { vecChannels[5].CreatePingMes ( iMs ); }

signals:
    void MessReadyForSending ( int iChID, CVector<uint8_t> vecMessage );
};

#endif /* !defined ( CHANNEL_HOIH9345KJH98_3_4344_BB23945IUHF1912__INCLUDED_ ) */
