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

#include "socket.h"


/* Implementation *************************************************************/
void CSocket::Init()
{
    // allocate memory for network receive and send buffer in samples
    vecbyRecBuf.Init ( MAX_SIZE_BYTES_NETW_BUF );

    // initialize the listening socket
    bool bSuccess = SocketDevice.bind (
        QHostAddress ( (Q_UINT32) 0 ), // INADDR_ANY
        LLCON_PORT_NUMBER );

    if ( bIsClient )
    {
        // if no success, try if server is on same machine (only for client)
        if ( !bSuccess )
        {
            // if server and client is on same machine, decrease port number by
            // one by definition
            bSuccess = SocketDevice.bind (
                QHostAddress( (Q_UINT32) 0 ), // INADDR_ANY
                LLCON_PORT_NUMBER - 1 );
        }
    }

    if ( !bSuccess )
    {
        // show error message
        QMessageBox::critical ( 0, "Network Error", "Cannot bind the socket.",
            QMessageBox::Ok, QMessageBox::NoButton );

        // exit application
        exit ( 1 );
    }

    QSocketNotifier* pSocketNotivRead =
        new QSocketNotifier ( SocketDevice.socket (), QSocketNotifier::Read );

    // connect the "activated" signal
    QObject::connect ( pSocketNotivRead, SIGNAL ( activated ( int ) ),
        this, SLOT ( OnDataReceived() ) );
}

void CSocket::SendPacket ( const CVector<unsigned char>& vecbySendBuf,
                           const CHostAddress& HostAddr )
{
    const int iVecSizeOut = vecbySendBuf.Size();

    if ( iVecSizeOut != 0 )
    {
        // send packet through network (we have to convert the constant unsigned
        // char vector in "const char*", for this we first convert the const
        // unsigned char vector in a read/write unsigned char vector and then
        // do the cast to const char*)
        SocketDevice.writeBlock (
            (const char*) &( (CVector<unsigned char>) vecbySendBuf )[0],
            iVecSizeOut, HostAddr.InetAddr, HostAddr.iPort );
    }
}

void CSocket::OnDataReceived()
{
    // read block from network interface
    const int iNumBytesRead = SocketDevice.readBlock ( (char*) &vecbyRecBuf[0],
        MAX_SIZE_BYTES_NETW_BUF );

    // check if an error occurred
    if ( iNumBytesRead < 0 )
    {
        return;
    }

    // get host address of client
    CHostAddress RecHostAddr ( SocketDevice.peerAddress(),
        SocketDevice.peerPort() );

    if ( bIsClient )
    {
        // client
        // check if packet comes from the server we want to connect
        if ( ! ( pChannel->GetAddress() == RecHostAddr ) )
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
            QThread::postEvent ( pServer,
                new CLlconEvent ( MS_PACKET_RECEIVED, 0, 0 ) );
        }
    }
}
