/******************************************************************************\
 * Copyright (c) 2004-2006
 *
 * Author(s):
 *	Volker Fischer
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
#define CON_TIME_OUT_SEC_MAX	5 // seconds
#define CON_TIME_OUT_CNT_MAX	( ( CON_TIME_OUT_SEC_MAX * 1000 ) / BLOCK_DURATION_MS )

/* maximum number of internet connections (channels) */
#define MAX_NUM_CHANNELS		10 /* max number channels for server */

/* no valid channel number */
#define INVALID_CHANNEL_ID		(MAX_NUM_CHANNELS + 1)


/* Classes ********************************************************************/
class CSampleOffsetEst
{
public:
	CSampleOffsetEst() {Init();}
	virtual ~CSampleOffsetEst() {}

	void Init();
	void AddTimeStampIdx(const int iTimeStampIdx);
	double GetSamRate() {return dSamRateEst;}

protected:
	QTime RefTime;
	int iAccTiStVal;
	double dSamRateEst;
	CVector<long int> veciTimeElapsed;
	CVector<long int> veciTiStIdx;
	int iInitCnt;
};


/* CChannel ----------------------------------------------------------------- */
class CChannel : public QObject
{
	Q_OBJECT

public:
	CChannel ();
	virtual ~CChannel () {}

	bool PutData ( const CVector<unsigned char>& vecbyData,
				   int iNumBytes );
	bool GetData ( CVector<double>& vecdData );

	CVector<unsigned char> PrepSendPacket ( const CVector<short>& vecsNPacket );

	bool IsConnected () const { return iConTimeOut > 0; }

	int	GetComprAudSize () { return iAudComprSize; }

	void SetAddress ( const CHostAddress NAddr ) { InetAddr = NAddr; }
	bool GetAddress ( CHostAddress& RetAddr );
	CHostAddress GetAddress () { return InetAddr; }

	void SetSockBufSize ( const int iNewBlockSize, const int iNumBlocks );
	int GetSockBufSize () { return SockBuf.GetSize(); }

	// network protocol interface
	void CreateJitBufMes ( const int iJitBufSize )
	{
		Protocol.CreateJitBufMes ( iJitBufSize );
	}

protected:
	/* audio compression */
	CAudioCompression	AudioCompression;
	int					iAudComprSize;

	/* resampling */
	CResample			ResampleObj;
	double				dSamRateOffset;
	CVector<double>		vecdResInData;
	CVector<double>		vecdResOutData;

	/* connection parameters */
	CHostAddress		InetAddr;

	/* network jitter-buffer */
	CNetBuf				SockBuf;

	/* network output conversion buffer */
	CConvBuf			ConvBuf;

	// network protocol
	CProtocol			Protocol;

	/* time stamp index counter */
	Q_UINT8				byTimeStampIdxCnt;
	int					iTimeStampActCnt;

	int					iConTimeOut;

	QMutex Mutex;

public slots:
	void OnSendProtMessage ( CVector<uint8_t> vecMessage );
	void OnJittBufSizeChange ( int iNewJitBufSize );

signals:
	void MessReadyForSending ( CVector<uint8_t> vecMessage );
};


/* CChannelSet (for server) ------------------------------------------------- */
class CChannelSet : public QObject
{
	Q_OBJECT

public:
	CChannelSet();
	virtual ~CChannelSet() {}

	bool PutData(const CVector<unsigned char>& vecbyRecBuf,
				 const int iNumBytesRead, const CHostAddress& HostAdr);

	int	GetFreeChan();
	int	CheckAddr(const CHostAddress& Addr);

	void GetBlockAllConC(CVector<int>& vecChanID,
						 CVector<CVector<double> >& vecvecdData);
	void GetConCliParam(CVector<CHostAddress>& vecHostAddresses,
						CVector<int>& veciJitBufSize);

	/* access functions for actual channels */
	bool IsConnected(const int iChanNum)
		{return vecChannels[iChanNum].IsConnected();}
	CVector<unsigned char> PrepSendPacket(const int iChanNum,
		const CVector<short>& vecsNPacket)
		{return vecChannels[iChanNum].PrepSendPacket(vecsNPacket);}
	CHostAddress GetAddress(const int iChanNum)
		{return vecChannels[iChanNum].GetAddress();}

	void SetSockBufSize ( const int iNewBlockSize, const int iNumBlocks);
	int GetSockBufSize() {return vecChannels[0].GetSockBufSize();}

protected:
	/* do not use the vector class since CChannel does not have appropriate
	   copy constructor/operator */
	CChannel	vecChannels[MAX_NUM_CHANNELS];
	QMutex		Mutex;



public slots:
// TODO better solution!!!
	void OnSendProtMessageCh0(CVector<uint8_t> mess) { emit MessReadyForSending(0,mess); }
	void OnSendProtMessageCh1(CVector<uint8_t> mess) { emit MessReadyForSending(1,mess); }
	void OnSendProtMessageCh2(CVector<uint8_t> mess) { emit MessReadyForSending(2,mess); }
	void OnSendProtMessageCh3(CVector<uint8_t> mess) { emit MessReadyForSending(3,mess); }
	void OnSendProtMessageCh4(CVector<uint8_t> mess) { emit MessReadyForSending(4,mess); }
	void OnSendProtMessageCh5(CVector<uint8_t> mess) { emit MessReadyForSending(5,mess); }
	void OnSendProtMessageCh6(CVector<uint8_t> mess) { emit MessReadyForSending(6,mess); }
	void OnSendProtMessageCh7(CVector<uint8_t> mess) { emit MessReadyForSending(7,mess); }
	void OnSendProtMessageCh8(CVector<uint8_t> mess) { emit MessReadyForSending(8,mess); }
	void OnSendProtMessageCh9(CVector<uint8_t> mess) { emit MessReadyForSending(9,mess); }


signals:
	void MessReadyForSending ( int iChID, CVector<uint8_t> vecMessage );
};


#endif /* !defined(CHANNEL_HOIH9345KJH98_3_4344_BB23945IUHF1912__INCLUDED_) */
