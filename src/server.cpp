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

#include "server.h"


/* Implementation *************************************************************/
CServer::CServer () : Socket ( &ChannelSet, this )
{
	vecsSendData.Init ( MIN_BLOCK_SIZE_SAMPLES );

	/* init moving average buffer for response time evaluation */
	RespTimeMoAvBuf.Init ( LEN_MOV_AV_RESPONSE );

	/* connect timer timeout signal */
	QObject::connect ( &Timer, SIGNAL ( timeout () ),
		this, SLOT ( OnTimer () ) );

#ifdef _WIN32
	// event handling of custom events seems not to work under Windows in this
	// class, do not use automatic start/stop of server in Windows version
	Start ();
#endif
}

void CServer::Start ()
{
	if ( !IsRunning () )
	{
		/* start main timer */
		Timer.start ( MIN_BLOCK_DURATION_MS );

		/* init time for response time evaluation */
		TimeLastBlock = QTime::currentTime ();

		qDebug("Server started");
	}
}

void CServer::Stop ()
{
	/* stop main timer */
	Timer.stop ();

	qDebug("Server stopped");
}

void CServer::OnTimer ()
{
	CVector<int>				vecChanID;
	CVector<CVector<double> >	vecvecdData ( MIN_BLOCK_SIZE_SAMPLES );

	/* get data from all connected clients */
	ChannelSet.GetBlockAllConC ( vecChanID, vecvecdData );
	const int iNumClients = vecvecdData.Size ();

	/* Check if at least one client is connected. If not, stop server until
	   one client is connected */
	if ( iNumClients != 0 )
	{
		/* actual processing of audio data -> mix */
		vecsSendData = ProcessData ( vecvecdData );

		/* send the same data to all connected clients */
		for ( int i = 0; i < iNumClients; i++ )
		{
			Socket.SendPacket (
				ChannelSet.PrepSendPacket ( vecChanID[i], vecsSendData ),
				ChannelSet.GetAddress ( vecChanID[i] ),
				ChannelSet.GetTimeStampIdx ( vecChanID[i] ) );
		}


		/* update response time measurement --------------------------------- */
		/* add time difference */
		const QTime CurTime = QTime::currentTime ();

		/* we want to calculate the standard deviation (we assume that the mean
		   is correct at the block period time) */
		const double dCurAddVal = ( (double) TimeLastBlock.msecsTo ( CurTime ) -
			MIN_BLOCK_DURATION_MS );

		RespTimeMoAvBuf.Add ( dCurAddVal * dCurAddVal ); /* add squared value */

		/* store old time value */
		TimeLastBlock = CurTime;
	}
	else
	{
		// Disable server if no clients are connected. In this case the server
		// does not consume any significant CPU when no client is connected.
#ifndef _WIN32
		// event handling of custom events seems not to work under Windows in this
		// class, do not use automatic start/stop of server in Windows version
		Stop ();
#endif
	}
}

CVector<short> CServer::ProcessData ( CVector<CVector<double> >& vecvecdData )
{
	CVector<short> vecsOutData;
	vecsOutData.Init ( MIN_BLOCK_SIZE_SAMPLES );

	const int iNumClients = vecvecdData.Size ();

	/* we normalize with sqrt() of N to avoid that the level drops too much
	   in case that a new client connects */
	const double dNorm = sqrt ( (double) iNumClients );

	/* mix all audio data from all clients together */
	for ( int i = 0; i < MIN_BLOCK_SIZE_SAMPLES; i++ )
	{
		double dMixedData = 0.0;

		for ( int j = 0; j < iNumClients; j++ )
		{
			dMixedData += vecvecdData[j][i];
		}

		/* normalization and truncating to short */
		vecsOutData[i] = Double2Short ( dMixedData / dNorm );
	}

	return vecsOutData;
}

bool CServer::GetTimingStdDev ( double& dCurTiStdDev )
{
	dCurTiStdDev = 0.0; /* init return value */

	/* only return value if server is active and the actual measurement is
	   updated */
	if ( IsRunning () )
	{
		/* we want to return the standard deviation, for that we need to calculate
		   the sqaure root */
		dCurTiStdDev = sqrt ( RespTimeMoAvBuf.GetAverage () );

		return true;
	}
	else
	{
		return false;
	}
}

void CServer::customEvent(QCustomEvent* Event)
{
	if (Event->type() == QEvent::User + 11)
	{
		const int iMessType = ((CLlconEvent*) Event)->iMessType;

		switch(iMessType)
		{
		case MS_PACKET_RECEIVED:
			// wake up the server if a packet was received
			// if the server is still running, the call to Start() will have
			// no effect
			Start ();
			break;
		}
	}
}
