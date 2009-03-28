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

#include "socket.h"


/* Implementation *************************************************************/
void CSocket::Init ( const quint16 iPortNumber )
{
    // allocate memory for network receive and send buffer in samples
    vecbyRecBuf.Init ( MAX_SIZE_BYTES_NETW_BUF );

    // initialize the listening socket
    bool bSuccess = SocketDevice.bind (
        QHostAddress ( QHostAddress::Any ), iPortNumber );

    // if no success, try if server is on same machine (only for client)
    if ( ( !bSuccess ) && bIsClient )
    {
        // if server and client is on same machine, decrease port number by
        // one by definition
        bSuccess = SocketDevice.bind (
            QHostAddress( QHostAddress::Any ), iPortNumber - 1 );
    }

    if ( !bSuccess )
    {
        // we cannot bind socket, throw error
        throw CGenErr ( "Cannot bind the socket (maybe "
            "the software is already running).", "Network Error" );
    }

    // connect the "activated" signal
    QObject::connect ( &SocketDevice, SIGNAL ( readyRead() ),
        this, SLOT ( OnDataReceived() ) );
}

void CSocket::SendPacket ( const CVector<uint8_t>& vecbySendBuf,
                           const CHostAddress& HostAddr )
{
    const int iVecSizeOut = vecbySendBuf.Size();

    if ( iVecSizeOut != 0 )
    {
        // send packet through network (we have to convert the constant unsigned
        // char vector in "const char*", for this we first convert the const
        // uint8_t vector in a read/write uint8_t vector and then do the cast to
        // const char*)
        SocketDevice.writeDatagram (
            (const char*) &( (CVector<uint8_t>) vecbySendBuf )[0],
            iVecSizeOut, HostAddr.InetAddr, HostAddr.iPort );
    }
}

void CSocket::OnDataReceived()
{
    while ( SocketDevice.hasPendingDatagrams() )
    {
        QHostAddress SenderAddress;
        quint16      SenderPort;

        // read block from network interface and query address of sender
        const int iNumBytesRead =
            SocketDevice.readDatagram ( (char*) &vecbyRecBuf[0],
            MAX_SIZE_BYTES_NETW_BUF, &SenderAddress, &SenderPort );

        // check if an error occurred
        if ( iNumBytesRead < 0 )
        {
            return;
        }

        // convert address of client
        const CHostAddress RecHostAddr ( SenderAddress, SenderPort );

        if ( bIsClient )
        {
            // client
            // check if packet comes from the server we want to connect
            if ( !( pChannel->GetAddress() == RecHostAddr ) )
            {
                return;
            }

            switch ( pChannel->PutData ( vecbyRecBuf, iNumBytesRead ) )
            {
            case PS_AUDIO_OK:
                PostWinMessage ( MS_JIT_BUF_PUT, MUL_COL_LED_GREEN );
                break;

            case PS_AUDIO_ERR:
            case PS_GEN_ERROR:
                PostWinMessage ( MS_JIT_BUF_PUT, MUL_COL_LED_RED );
                break;

            case PS_PROT_ERR:
                PostWinMessage ( MS_JIT_BUF_PUT, MUL_COL_LED_YELLOW );
                break;
            }
        }
        else
        {
            // server
            if ( pChannelSet->PutData ( vecbyRecBuf, iNumBytesRead, RecHostAddr ) )
            {
                // this was an audio packet, start server
                // tell the server object to wake up if it
                // is in sleep mode (Qt will delete the event object when done)
                QCoreApplication::postEvent ( pServer,
                    new CLlconEvent ( MS_PACKET_RECEIVED, 0, 0 ) );
            }
        }
    }
}
