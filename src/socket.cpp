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

#include "socket.h"


/* Implementation *************************************************************/
void CSocket::Init ()
{
	/* allocate memory for network receive and send buffer in samples */
	vecbyRecBuf.Init ( MAX_SIZE_BYTES_NETW_BUF );

	/* initialize the listening socket */
	bool bSuccess = SocketDevice.bind (
		QHostAddress ( (Q_UINT32) 0 ) /* INADDR_ANY */, LLCON_PORT_NUMBER );

	if ( bIsClient )
	{
		/* if no success, try if server is on same machine (only for client) */
		if ( !bSuccess )
		{
			/* if server and client is on same machine, decrease port number by
			one by definition */
			bSuccess = SocketDevice.bind (
				QHostAddress( (Q_UINT32) 0 ) /* INADDR_ANY */,
				LLCON_PORT_NUMBER - 1 );
		}
	}

	if ( !bSuccess )
	{
		/* show error message */
		QMessageBox::critical ( 0, "Network Error", "Cannot bind the socket.",
			QMessageBox::Ok, QMessageBox::NoButton );

		/* exit application */
		exit ( 1 );
	}

	QSocketNotifier* pSocketNotivRead =
		new QSocketNotifier ( SocketDevice.socket (), QSocketNotifier::Read );

	/* connect the "activated" signal */
	QObject::connect ( pSocketNotivRead, SIGNAL ( activated ( int ) ),
		this, SLOT ( OnDataReceived () ) );
}

void CSocket::SendPacket( const CVector<unsigned char>& vecbySendBuf,
						  const CHostAddress& HostAddr,
						  const int iTimeStampIdx )
{
	const int iVecSizeOut = vecbySendBuf.Size ();

	if ( iVecSizeOut != 0 )
	{
		/* send packet through network */
		SocketDevice.writeBlock (
			(const char*) &((CVector<unsigned char>) vecbySendBuf)[0],
			iVecSizeOut, HostAddr.InetAddr, HostAddr.iPort );
	}

	/* sent time stamp if required */
	if ( iTimeStampIdx != INVALID_TIME_STAMP_IDX )
	{
		/* Always one byte long */
		SocketDevice.writeBlock ( (const char*) &iTimeStampIdx, 1,
			HostAddr.InetAddr, HostAddr.iPort );
	}
}

void CSocket::OnDataReceived ()
{
	/* read block from network interface */
	const int iNumBytesRead = SocketDevice.readBlock( (char*) &vecbyRecBuf[0],
		MAX_SIZE_BYTES_NETW_BUF);

	/* check if an error occurred */
	if ( iNumBytesRead < 0 )
	{
		return;
	}

	/* get host address of client */
	CHostAddress RecHostAddr ( SocketDevice.peerAddress (),
		SocketDevice.peerPort () );

	if ( bIsClient )
	{
		/* client */
		/* check if packet comes from the server we want to connect */
		if ( ! ( pChannel->GetAddress () == RecHostAddr ) )
			return;

		if ( pChannel->PutData( vecbyRecBuf, iNumBytesRead ) )
		{
			PostWinMessage ( MS_JIT_BUF_PUT, MUL_COL_LED_GREEN );
		}
		else
		{
			PostWinMessage ( MS_JIT_BUF_PUT, MUL_COL_LED_RED );
		}
	}
	else
	{
		/* server */
		pChannelSet->PutData ( vecbyRecBuf, iNumBytesRead, RecHostAddr );
	}
}
