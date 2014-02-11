/******************************************************************************\
 * Copyright (c) 2004-2014
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
#include "server.h"


/* Implementation *************************************************************/
void CSocket::Init ( const quint16 iPortNumber )
{
#ifdef ENABLE_RECEIVE_SOCKET_IN_SEPARATE_THREAD
# ifdef _WIN32
    // for the Windows socket usage we have to start it up first
    WSADATA wsa;
    WSAStartup ( MAKEWORD(1, 0), &wsa ); // TODO check for error and exit application on error
# endif

    // create the UDP socket
    UdpSocket = socket ( AF_INET, SOCK_DGRAM, 0 );
#endif

    // allocate memory for network receive and send buffer in samples
    vecbyRecBuf.Init ( MAX_SIZE_BYTES_NETW_BUF );

    // initialize the listening socket
    bool bSuccess;

    if ( bIsClient )
    {
        // Per definition use the port number plus ten for the client to make
        // it possible to run server and client on the same computer. If the
        // port is not available, try "NUM_SOCKET_PORTS_TO_TRY" times with
        // incremented port numbers
        quint16 iClientPortIncrement = 10; // start value: port nubmer plus ten
        bSuccess                     = false; // initialization for while loop

#ifdef ENABLE_RECEIVE_SOCKET_IN_SEPARATE_THREAD
        // preinitialize socket in address (only the port number is missing)
        sockaddr_in UdpSocketInAddr;
        UdpSocketInAddr.sin_family      = AF_INET;
        UdpSocketInAddr.sin_addr.s_addr = INADDR_ANY;

        while ( !bSuccess &&
                ( iClientPortIncrement <= NUM_SOCKET_PORTS_TO_TRY ) )
        {
            UdpSocketInAddr.sin_port = htons ( iPortNumber + iClientPortIncrement );

            bSuccess = ( bind ( UdpSocket ,
                                (sockaddr*) &UdpSocketInAddr,
                                sizeof ( sockaddr_in ) ) == 0 );

            iClientPortIncrement++;
        }
#else
        while ( !bSuccess &&
                ( iClientPortIncrement <= NUM_SOCKET_PORTS_TO_TRY ) )
        {
            bSuccess = SocketDevice.bind (
                QHostAddress ( QHostAddress::Any ),
                iPortNumber + iClientPortIncrement );

            iClientPortIncrement++;
        }
#endif
    }
    else
    {
        // for the server, only try the given port number and do not try out
        // other port numbers to bind since it is imporatant that the server
        // gets the desired port number
        bSuccess = SocketDevice.bind (
            QHostAddress ( QHostAddress::Any ), iPortNumber );
    }

    if ( !bSuccess )
    {
        // we cannot bind socket, throw error
        throw CGenErr ( "Cannot bind the socket (maybe "
            "the software is already running).", "Network Error" );
    }

    // connect the "activated" signal
#ifdef ENABLE_RECEIVE_SOCKET_IN_SEPARATE_THREAD
    if ( bIsClient )
    {
// TEST We do a test where we call "waitForReadyRead" instead of even driven method.
/*
        // We have to use a blocked queued connection since in case we use a
        // separate socket thread, the "readyRead" signal would occur and our
        // "OnDataReceived" function would be run in another thread. This could
        // lead to a situation that a new "readRead" occurs while the processing
        // of the previous signal was not finished -> the error: "Multiple
        // socket notifiers for same socket" may occur.
        QObject::connect ( &SocketDevice, SIGNAL ( readyRead() ),
            this, SLOT ( OnDataReceived() ), Qt::BlockingQueuedConnection );
*/


// TEST
QObject::connect ( this,
    SIGNAL ( ParseMessageBody ( CVector<uint8_t>, int, int ) ),
    pChannel, SLOT ( OnParseMessageBody ( CVector<uint8_t>, int, int ) ) );

QObject::connect ( this,
    SIGNAL ( DetectedCLMessage ( CVector<uint8_t>, int ) ),
    pChannel, SLOT ( OnDetectedCLMessage ( CVector<uint8_t>, int ) ) );


    }
    else
    {
        // the server does not use a separate socket thread right now, in that
        // case we must not use the blocking queued connection, otherwise we
        // would get a dead lock
        QObject::connect ( &SocketDevice, SIGNAL ( readyRead() ),
            this, SLOT ( OnDataReceived() ) );
    }
#else
    QObject::connect ( &SocketDevice, SIGNAL ( readyRead() ),
        this, SLOT ( OnDataReceived() ) );
#endif
}

CSocket::~CSocket()
{
#ifdef ENABLE_RECEIVE_SOCKET_IN_SEPARATE_THREAD
# ifdef _WIN32
    // the Windows socket must be cleanup on shutdown
    WSACleanup();
# endif
#endif
}

void CSocket::SendPacket ( const CVector<uint8_t>& vecbySendBuf,
                           const CHostAddress&     HostAddr )
{
    QMutexLocker locker ( &Mutex );

    const int iVecSizeOut = vecbySendBuf.Size();

    if ( iVecSizeOut != 0 )
    {
        // send packet through network (we have to convert the constant unsigned
        // char vector in "const char*", for this we first convert the const
        // uint8_t vector in a read/write uint8_t vector and then do the cast to
        // const char*)
#ifdef ENABLE_RECEIVE_SOCKET_IN_SEPARATE_THREAD
        // note that the client uses the socket directly for performance reasons
        if ( bIsClient )
        {
            sockaddr_in UdpSocketOutAddr;

            UdpSocketOutAddr.sin_family      = AF_INET;
            UdpSocketOutAddr.sin_port        = htons ( HostAddr.iPort );
			UdpSocketOutAddr.sin_addr.s_addr = htonl ( HostAddr.InetAddr.toIPv4Address() );

            sendto ( UdpSocket,
                     (const char*) &( (CVector<uint8_t>) vecbySendBuf )[0],
                     iVecSizeOut,
                     0,
                     (sockaddr*) &UdpSocketOutAddr,
                     sizeof ( sockaddr_in ) );
        }
        else
        {
#endif
            SocketDevice.writeDatagram (
                (const char*) &( (CVector<uint8_t>) vecbySendBuf )[0],
                iVecSizeOut,
                HostAddr.InetAddr,
                HostAddr.iPort );
#ifdef ENABLE_RECEIVE_SOCKET_IN_SEPARATE_THREAD
        }
#endif
    }
}

bool CSocket::GetAndResetbJitterBufferOKFlag()
{
    // check jitter buffer status
    if ( !bJitterBufferOK )
    {
        // reset flag and return "not OK" status
        bJitterBufferOK = true;
        return false;
    }

    // the buffer was OK, we do not have to reset anything and just return the
    // OK status
    return true;
}

void CSocket::OnDataReceived()
{
#ifndef ENABLE_RECEIVE_SOCKET_IN_SEPARATE_THREAD
    while ( SocketDevice.hasPendingDatagrams() )
#endif
    {
        // read block from network interface and query address of sender
#ifdef ENABLE_RECEIVE_SOCKET_IN_SEPARATE_THREAD
        sockaddr_in SenderAddr;
#ifdef _WIN32
        int SenderAddrSize = sizeof ( sockaddr_in );
#else
        socklen_t SenderAddrSize = sizeof ( sockaddr_in );
#endif

        const long iNumBytesRead = recvfrom ( UdpSocket,
                                              (char*) &vecbyRecBuf[0],
                                              MAX_SIZE_BYTES_NETW_BUF,
                                              0,
                                              (sockaddr*) &SenderAddr,
                                              &SenderAddrSize );
#else
        const int iNumBytesRead =
            SocketDevice.readDatagram ( (char*) &vecbyRecBuf[0],
                                        MAX_SIZE_BYTES_NETW_BUF,
                                        &SenderAddress,
                                        &SenderPort );
#endif

        // check if an error occurred
        if ( iNumBytesRead < 0 )
        {
            return;
        }

        // convert address of client
#ifdef ENABLE_RECEIVE_SOCKET_IN_SEPARATE_THREAD
        RecHostAddr.InetAddr.setAddress ( ntohl ( SenderAddr.sin_addr.s_addr ) );
        RecHostAddr.iPort = ntohs ( SenderAddr.sin_port );
#else
        RecHostAddr.InetAddr = SenderAddress;
        RecHostAddr.iPort    = SenderPort;
#endif

        if ( bIsClient )
        {
            // client:

            // check if packet comes from the server we want to connect and that
            // the channel is enabled
            if ( ( pChannel->GetAddress() == RecHostAddr ) &&
                 pChannel->IsEnabled() )
            {
                // this network packet is valid, put it in the channel
#ifdef ENABLE_RECEIVE_SOCKET_IN_SEPARATE_THREAD
                switch ( pChannel->PutData ( vecbyRecBuf, iNumBytesRead, this ) )
#else
                switch ( pChannel->PutData ( vecbyRecBuf, iNumBytesRead ) )
#endif
                {
                case PS_AUDIO_ERR:
                case PS_GEN_ERROR:
                    bJitterBufferOK = false;
                    break;

                default:
                    // do nothing
                    break;
                }
            }
            else
            {
                // inform about received invalid packet by fireing an event
                emit InvalidPacketReceived ( vecbyRecBuf,
                                             iNumBytesRead,
                                             RecHostAddr );
            }
        }
        else
        {
            // server:

            if ( pServer->PutData ( vecbyRecBuf, iNumBytesRead, RecHostAddr ) )
            {
                // this was an audio packet, start server if it is in sleep mode
                if ( !pServer->IsRunning() )
                {
                    // (note that Qt will delete the event object when done)
                    QCoreApplication::postEvent ( pServer,
                        new CCustomEvent ( MS_PACKET_RECEIVED, 0, 0 ) );
                }
            }
        }
    }
}
