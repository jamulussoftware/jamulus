/******************************************************************************\
 * Copyright (c) 2004-2020
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
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 *
\******************************************************************************/

#include "socket.h"
#include "server.h"

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <arpa/inet.h>
#endif


/* Implementation *************************************************************/
void CSocket::Init ( const quint16 iPortNumber, const quint16 iQosNumber, const QString& strServerBindIP )
{
    bool bUseIPV6 = true;

#ifdef _WIN32
    // for the Windows socket usage we have to start it up first

    // clang-format off
// TODO check for error and exit application on error
    // clang-format on

    WSADATA wsa;
    WSAStartup ( MAKEWORD ( 1, 0 ), &wsa );
#endif

    // create the UDP socket
    UdpSocket = socket ( AF_INET6, SOCK_DGRAM, 0 );
    if ( UdpSocket == -1 )
    {
        // IPv6 not available; fall back to IPv4
        bUseIPV6 = false;

        UdpSocket = socket ( AF_INET, SOCK_DGRAM, 0 );
        //
        const char tos = (char) iQosNumber; // Quality of Service
        setsockopt ( UdpSocket, IPPROTO_IP, IP_TOS, &tos, sizeof ( tos ) );
    }
    else
    {
        // The IPV6_V6ONLY socket option must be false in order for the socket to listen on both protocols.
        // On Linux it's false by default on most (all?) distros, but on Windows it is true by default
        const uint8_t no = 0;
        setsockopt( UdpSocket, IPPROTO_IPV6, IPV6_V6ONLY, (const char *)&no, sizeof( no ) );
        //
        const char tos = (char) iQosNumber; // Quality of Service
        setsockopt ( UdpSocket, IPPROTO_IPV6, IPV6_TCLASS, &tos, sizeof ( tos ) );
    }

    // allocate memory for network receive and send buffer in samples
    vecbyRecBuf.Init ( MAX_SIZE_BYTES_NETW_BUF );

    // preinitialize socket in address (only the port number is missing)
    sockaddr_in UdpSocketInAddr;
    UdpSocketInAddr.sin_family      = AF_INET;
    UdpSocketInAddr.sin_addr.s_addr = INADDR_ANY;

    struct sockaddr_in6 UdpSocketIn6Addr;
    UdpSocketIn6Addr.sin6_family = AF_INET6;
    UdpSocketIn6Addr.sin6_addr   = in6addr_any;

    // TODO - ALLOw IPV6 ADDRESS
    if ( !strServerBindIP.isEmpty() )
    {
        UdpSocketInAddr.sin_addr.s_addr = htonl ( QHostAddress ( strServerBindIP ).toIPv4Address() );
    }
    // END TODO - ALLOw IPV6 ADDRESS

    // initialize the listening socket
    bool bSuccess;

    if ( bIsClient )
    {
        if ( iPortNumber == 0 )
        {
            // if port number is 0, bind the client to a random available port
            if (bUseIPV6) {
                UdpSocketIn6Addr.sin6_port = htons ( 0 );
                bSuccess = ( ::bind ( UdpSocket ,
                            (sockaddr*) &UdpSocketIn6Addr,
                            sizeof ( struct sockaddr_in6 ) ) == 0 );
            } else {
                UdpSocketInAddr.sin_port = htons ( 0 );
                bSuccess = ( ::bind ( UdpSocket ,
                            (sockaddr*) &UdpSocketInAddr,
                            sizeof ( sockaddr_in ) ) == 0 );
            }

        }
        else
        {
            // If the port is not available, try "NUM_SOCKET_PORTS_TO_TRY" times
            // with incremented port numbers. Randomize the start port, in case a
            // faulty router gets stuck and confused by a particular port (like
            // the starting port). Might work around frustrating "cannot connect"
            // problems (#568)
            const quint16 startingPortNumber = iPortNumber + rand() % NUM_SOCKET_PORTS_TO_TRY;

            quint16 iClientPortIncrement = 0;
            bSuccess                     = false; // initialization for while loop

            while ( !bSuccess && ( iClientPortIncrement <= NUM_SOCKET_PORTS_TO_TRY ) )
            {
                if (bUseIPV6) {
                    UdpSocketIn6Addr.sin6_port = htons ( startingPortNumber + iClientPortIncrement );
                    bSuccess = ( ::bind ( UdpSocket ,
                                (sockaddr*) &UdpSocketIn6Addr,
                                sizeof ( struct sockaddr_in6 ) ) == 0 );
                } else {
                    UdpSocketInAddr.sin_port = htons ( startingPortNumber + iClientPortIncrement );
                    bSuccess = ( ::bind ( UdpSocket ,
                                (sockaddr*) &UdpSocketInAddr,
                                sizeof ( sockaddr_in ) ) == 0 );
                }

                iClientPortIncrement++;
            }
        }
    }
    else
    {
        // for the server, only try the given port number and do not try out
        // other port numbers to bind since it is important that the server
        // gets the desired port number
        if (bUseIPV6) {
            UdpSocketIn6Addr.sin6_port = htons ( iPortNumber );
            bSuccess = ( ::bind ( UdpSocket ,
                        (sockaddr*) &UdpSocketIn6Addr,
                        sizeof ( struct sockaddr_in6 ) ) == 0 );
        } else {
            UdpSocketInAddr.sin_port = htons ( iPortNumber );
            bSuccess = ( ::bind ( UdpSocket ,
                        (sockaddr*) &UdpSocketInAddr,
                        sizeof ( sockaddr_in ) ) == 0 );
        }
    }

    if ( !bSuccess )
    {
        // we cannot bind socket, throw error
        throw CGenErr ( "Cannot bind the socket (maybe "
                        "the software is already running).",
                        "Network Error" );
    }

    // Connections -------------------------------------------------------------
    // it is important to do the following connections in this class since we
    // have a thread transition

    // we have different connections for client and server
    if ( bIsClient )
    {
        // client connections:

        QObject::connect ( this, &CSocket::ProtcolMessageReceived, pChannel, &CChannel::OnProtcolMessageReceived );

        QObject::connect ( this, &CSocket::ProtcolCLMessageReceived, pChannel, &CChannel::OnProtcolCLMessageReceived );

        QObject::connect ( this, static_cast<void ( CSocket::* )()> ( &CSocket::NewConnection ), pChannel, &CChannel::OnNewConnection );
    }
    else
    {
        // server connections:

        QObject::connect ( this, &CSocket::ProtcolMessageReceived, pServer, &CServer::OnProtcolMessageReceived );

        QObject::connect ( this, &CSocket::ProtcolCLMessageReceived, pServer, &CServer::OnProtcolCLMessageReceived );

        QObject::connect ( this,
                           static_cast<void ( CSocket::* ) ( int, CHostAddress )> ( &CSocket::NewConnection ),
                           pServer,
                           &CServer::OnNewConnection );

        QObject::connect ( this, &CSocket::ServerFull, pServer, &CServer::OnServerFull );
    }
}

void CSocket::Close()
{
#ifdef _WIN32
    // closesocket will cause recvfrom to return with an error because the
    // socket is closed -> then the thread can safely be shut down
    closesocket ( UdpSocket );
#elif defined( __APPLE__ ) || defined( __MACOSX )
    // on Mac the general close has the same effect as closesocket on Windows
    close ( UdpSocket );
#else
    // on Linux the shutdown call cancels the recvfrom
    shutdown ( UdpSocket, SHUT_RDWR );
#endif
}

CSocket::~CSocket()
{
    // cleanup the socket (on Windows the WSA cleanup must also be called)
#ifdef _WIN32
    closesocket ( UdpSocket );
    WSACleanup();
#else
    close ( UdpSocket );
#endif
}

void CSocket::SendPacket ( const CVector<uint8_t>& vecbySendBuf, const CHostAddress& HostAddr )
{
    QMutexLocker locker ( &Mutex );

    const int iVecSizeOut = vecbySendBuf.Size();

    if ( iVecSizeOut > 0 )
    {
        // send packet through network (we have to convert the constant unsigned
        // char vector in "const char*", for this we first convert the const
        // uint8_t vector in a read/write uint8_t vector and then do the cast to
        // const char *)
        if (HostAddr.InetAddr.protocol() == QAbstractSocket::IPv4Protocol) {

            sockaddr_in UdpSocketOutAddr;
            UdpSocketOutAddr.sin_family      = AF_INET;
            UdpSocketOutAddr.sin_port        = htons ( HostAddr.iPort );
            UdpSocketOutAddr.sin_addr.s_addr = htonl ( HostAddr.InetAddr.toIPv4Address() );

            sendto ( UdpSocket,
                     (const char*) &( (CVector<uint8_t>) vecbySendBuf )[0],
                     iVecSizeOut,
                     0,
                     (sockaddr*) &UdpSocketOutAddr,
                     sizeof ( UdpSocketOutAddr ) );

        } else {

            sockaddr_in6 UdpSocketOutAddr;

            UdpSocketOutAddr.sin6_family = AF_INET6;
            UdpSocketOutAddr.sin6_port   = htons ( HostAddr.iPort );
            inet_pton(AF_INET6, HostAddr.InetAddr.toString().toLocal8Bit().constData(), &UdpSocketOutAddr.sin6_addr);

            sendto ( UdpSocket,
                     (const char*) &( (CVector<uint8_t>) vecbySendBuf )[0],
                     iVecSizeOut,
                     0,
                     (sockaddr*) &UdpSocketOutAddr,
                     sizeof ( UdpSocketOutAddr ) );
        }
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
    /*
        The strategy of this function is that only the "put audio" function is
        called directly (i.e. the high thread priority is used) and all other less
        important things like protocol parsing and acting on protocol messages is
        done in the low priority thread. To get a thread transition, we have to
        use the signal/slot mechanism (i.e. we use messages for that).
    */

    // read block from network interface and query address of sender
    struct sockaddr_storage addrstorage;
#ifdef _WIN32
    int SenderAddrSize = sizeof ( struct sockaddr_storage );
#else
    socklen_t SenderAddrSize = sizeof ( struct sockaddr_storage );
#endif

    const long iNumBytesRead = recvfrom ( UdpSocket,
                                          (char*) &vecbyRecBuf[0],
                                          MAX_SIZE_BYTES_NETW_BUF,
                                          0,
                                          (sockaddr*) &addrstorage,
                                          &SenderAddrSize );

    // check if an error occurred or no data could be read
    if ( iNumBytesRead <= 0 )
    {
        return;
    }

    if ( addrstorage.ss_family == AF_INET6) {
        struct sockaddr_in6 *in6 = reinterpret_cast<struct sockaddr_in6 *>(&addrstorage);
        if (IN6_IS_ADDR_V4MAPPED(&(in6->sin6_addr))) {
            RecHostAddr.InetAddr.setAddress ( ntohl (*((uint32_t*)in6->sin6_addr.s6_addr + 3) ) );
        } else {
            RecHostAddr.InetAddr.setAddress( in6->sin6_addr.s6_addr );
        }
        RecHostAddr.iPort = ntohs ( in6->sin6_port );
    } else {
        struct sockaddr_in *in = reinterpret_cast<struct sockaddr_in *>(&addrstorage);
        // convert address of client
        RecHostAddr.InetAddr.setAddress ( ntohl ( in->sin_addr.s_addr ) );
        RecHostAddr.iPort = ntohs ( in->sin_port );
    }

    // check if this is a protocol message
    int              iRecCounter;
    int              iRecID;
    CVector<uint8_t> vecbyMesBodyData;

    if ( !CProtocol::ParseMessageFrame ( vecbyRecBuf, iNumBytesRead, vecbyMesBodyData, iRecCounter, iRecID ) )
    {
        // this is a protocol message, check the type of the message
        if ( CProtocol::IsConnectionLessMessageID ( iRecID ) )
        {

            // clang-format off
// TODO a copy of the vector is used -> avoid malloc in real-time routine
            // clang-format on

            emit ProtcolCLMessageReceived ( iRecID, vecbyMesBodyData, RecHostAddr );
        }
        else
        {

            // clang-format off
// TODO a copy of the vector is used -> avoid malloc in real-time routine
            // clang-format on

            emit ProtcolMessageReceived ( iRecCounter, iRecID, vecbyMesBodyData, RecHostAddr );
        }
    }
    else
    {
        // this is most probably a regular audio packet
        if ( bIsClient )
        {
            // client:

            switch ( pChannel->PutAudioData ( vecbyRecBuf, iNumBytesRead, RecHostAddr ) )
            {
            case PS_AUDIO_ERR:
            case PS_GEN_ERROR:
                bJitterBufferOK = false;
                break;

            case PS_NEW_CONNECTION:
                // inform other objects that new connection was established
                emit NewConnection();
                break;

            case PS_AUDIO_INVALID:
                // inform about received invalid packet by fireing an event
                emit InvalidPacketReceived ( RecHostAddr );
                break;

            default:
                // do nothing
                break;
            }
        }
        else
        {
            // server:

            int iCurChanID;

            if ( pServer->PutAudioData ( vecbyRecBuf, iNumBytesRead, RecHostAddr, iCurChanID ) )
            {
                // we have a new connection, emit a signal
                emit NewConnection ( iCurChanID, RecHostAddr );

                // this was an audio packet, start server if it is in sleep mode
                if ( !pServer->IsRunning() )
                {
                    // (note that Qt will delete the event object when done)
                    QCoreApplication::postEvent ( pServer, new CCustomEvent ( MS_PACKET_RECEIVED, 0, 0 ) );
                }
            }

            // check if no channel is available
            if ( iCurChanID == INVALID_CHANNEL_ID )
            {
                // fire message for the state that no free channel is available
                emit ServerFull ( RecHostAddr );
            }
        }
    }
}
