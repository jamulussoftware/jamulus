/******************************************************************************\
 * Copyright (c) 2004-2006
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

#if !defined(CHANNEL_HOIH9345KJH98_3_4344_BB23945IUHF1912__INCLUDED_)
#define CHANNEL_HOIH9345KJH98_3_4344_BB23945IUHF1912__INCLUDED_

#include <qthread.h>
#include <qdatetime.h>
#include "global.h"
#include "buffer.h"
#include "audiocompr.h"
#include "util.h"
#include "resample.h"
#include "protocol.h"


/* Definitions ****************************************************************/
/* Set the time-out for the input buffer until the state changes from
   connected to not-connected (the actual time depends on the way the error
   correction is implemented) */
#define CON_TIME_OUT_SEC_MAX    5 // seconds

/* maximum number of internet connections (channels) */
// if you want to change this paramter, change the connections in this class, too!
#define MAX_NUM_CHANNELS        10 /* max number channels for server */

/* no valid channel number */
#define INVALID_CHANNEL_ID      (MAX_NUM_CHANNELS + 1)

enum EPutDataStat
{
    PS_GEN_ERROR,
    PS_AUDIO_OK,
    PS_AUDIO_ERR,
    PS_PROT_OK,
    PS_PROT_ERR
};


/* Classes ********************************************************************/
/* CChannel ----------------------------------------------------------------- */
class CChannel : public QObject
{
    Q_OBJECT

public:
    CChannel();
    virtual ~CChannel() {}

    EPutDataStat PutData ( const CVector<unsigned char>& vecbyData,
                           int iNumBytes );
    bool GetData ( CVector<double>& vecdData );

    CVector<unsigned char> PrepSendPacket ( const CVector<short>& vecsNPacket );

    bool IsConnected() const { return iConTimeOut > 0; }

    void SetAddress ( const CHostAddress NAddr ) { InetAddr = NAddr; }
    bool GetAddress ( CHostAddress& RetAddr );
    CHostAddress GetAddress () { return InetAddr; }

    void SetName ( const std::string sNNa ) { sName = sNNa; }
    std::string GetName() { return sName; }

    void SetGain ( const int iNID, const double dNG ) { vecdGains[iNID] = dNG; }
    double GetGain( const int iNID ) { return vecdGains[iNID]; }

    void SetSockBufSize ( const int iNumBlocks );
    int GetSockBufSize() { return iCurSockBufSize; }

    void SetNetwBufSizeFactOut ( const int iNewNetwBlSiFactOut );
    int GetNetwBufSizeFactOut() { return iCurNetwOutBlSiFact; }

    int GetNetwBufSizeFactIn() { return iCurNetwInBlSiFact; }

    // network protocol interface
    void CreateJitBufMes ( const int iJitBufSize )
    { 
        if ( IsConnected() )
        {
            Protocol.CreateJitBufMes ( iJitBufSize );
        }
    }
    void CreateReqJitBufMes() { Protocol.CreateReqJitBufMes(); }

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
    void SetNetwInBlSiFact ( const int iNewBlockSizeFactor );

    // audio compression
    CAudioCompression   AudioCompressionIn;
    int                 iAudComprSizeIn;
    CAudioCompression   AudioCompressionOut;
    int                 iAudComprSizeOut;

    // resampling
    CResample           ResampleObj;
    double              dSamRateOffset;
    CVector<double>     vecdResInData;
    CVector<double>     vecdResOutData;

    // connection parameters
    CHostAddress        InetAddr;

    // channel name
    std::string         sName;

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

    int                 vecNetwInBufSizes[MAX_NET_BLOCK_SIZE_FACTOR];

    int                 iCurNetwInBlSiFact;
    int                 iCurNetwOutBlSiFact;

    QMutex              Mutex;

public slots:
    void OnSendProtMessage ( CVector<uint8_t> vecMessage );
    void OnJittBufSizeChange ( int iNewJitBufSize );
    void OnNetwBlSiFactChange ( int iNewNetwBlSiFact );
    void OnChangeChanGain ( int iChanID, double dNewGain );

signals:
    void MessReadyForSending ( CVector<uint8_t> vecMessage );
    void NewConnection();
    void ReqJittBufSize();
    void ConClientListMesReceived ( CVector<CChannelShortInfo> vecChanInfo );
    void ProtocolStatus ( bool bOk );
};


/* CChannelSet (for server) ------------------------------------------------- */
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
                          CVector<int>& veciJitBufSize,
                          CVector<int>& veciNetwOutBlSiFact,
                          CVector<int>& veciNetwInBlSiFact );

    /* access functions for actual channels */
    bool IsConnected ( const int iChanNum )
        { return vecChannels[iChanNum].IsConnected(); }

    CVector<unsigned char> PrepSendPacket ( const int iChanNum,
                                            const CVector<short>& vecsNPacket )
        { return vecChannels[iChanNum].PrepSendPacket ( vecsNPacket ); }

    CHostAddress GetAddress ( const int iChanNum )
        { return vecChannels[iChanNum].GetAddress(); }

protected:
    void CreateAndSendChanListForAllConClients();

    /* do not use the vector class since CChannel does not have appropriate
       copy constructor/operator */
    CChannel    vecChannels[MAX_NUM_CHANNELS];
    QMutex      Mutex;

public slots:
    // make sure we have MAX_NUM_CHANNELS connections!!!
    // send message
    void OnSendProtMessCh0(CVector<uint8_t> mess) {emit MessReadyForSending(0,mess);}
    void OnSendProtMessCh1(CVector<uint8_t> mess) {emit MessReadyForSending(1,mess);}
    void OnSendProtMessCh2(CVector<uint8_t> mess) {emit MessReadyForSending(2,mess);}
    void OnSendProtMessCh3(CVector<uint8_t> mess) {emit MessReadyForSending(3,mess);}
    void OnSendProtMessCh4(CVector<uint8_t> mess) {emit MessReadyForSending(4,mess);}
    void OnSendProtMessCh5(CVector<uint8_t> mess) {emit MessReadyForSending(5,mess);}
    void OnSendProtMessCh6(CVector<uint8_t> mess) {emit MessReadyForSending(6,mess);}
    void OnSendProtMessCh7(CVector<uint8_t> mess) {emit MessReadyForSending(7,mess);}
    void OnSendProtMessCh8(CVector<uint8_t> mess) {emit MessReadyForSending(8,mess);}
    void OnSendProtMessCh9(CVector<uint8_t> mess) {emit MessReadyForSending(9,mess);}

    void OnNewConnectionCh0() {vecChannels[0].CreateReqJitBufMes();}
    void OnNewConnectionCh1() {vecChannels[1].CreateReqJitBufMes();}
    void OnNewConnectionCh2() {vecChannels[2].CreateReqJitBufMes();}
    void OnNewConnectionCh3() {vecChannels[3].CreateReqJitBufMes();}
    void OnNewConnectionCh4() {vecChannels[4].CreateReqJitBufMes();}
    void OnNewConnectionCh5() {vecChannels[5].CreateReqJitBufMes();}
    void OnNewConnectionCh6() {vecChannels[6].CreateReqJitBufMes();}
    void OnNewConnectionCh7() {vecChannels[7].CreateReqJitBufMes();}
    void OnNewConnectionCh8() {vecChannels[8].CreateReqJitBufMes();}
    void OnNewConnectionCh9() {vecChannels[9].CreateReqJitBufMes();}

signals:
    void MessReadyForSending ( int iChID, CVector<uint8_t> vecMessage );
};


/* Sample rate offset estimation -------------------------------------------- */
class CSampleOffsetEst
{
public:
    CSampleOffsetEst() { Init(); }
    virtual ~CSampleOffsetEst() {}

    void Init();
    void AddTimeStampIdx ( const int iTimeStampIdx );
    double GetSamRate() { return dSamRateEst; }

protected:
    QTime               RefTime;
    int                 iAccTiStVal;
    double              dSamRateEst;
    CVector<long int>   veciTimeElapsed;
    CVector<long int>   veciTiStIdx;
    int                 iInitCnt;
};


#endif /* !defined(CHANNEL_HOIH9345KJH98_3_4344_BB23945IUHF1912__INCLUDED_) */
