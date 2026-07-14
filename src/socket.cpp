/******************************************************************************\
 * Copyright (c) 2004-2026
 *
 * Author(s):
 *  Volker Fischer
 *
 * As of Jamulus 3.12.1dev (commit eb172d47): All new source code contributions must be licensed
 * under AGPL 3.0 or any later version.
 *
 * Existing code: Code contributed before 3.12.1dev (commit eb172d47) was licensed under GPL 2.0+.
 * This code will be licensed under GPL 3.0 (or any later version) from
 * 3.12.1dev (commit eb172d47).  When distributed as part of Jamulus, the AGPL 3.0 terms govern
 * the combined work, including network use provisions.
 *
 ******************************************************************************
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 * ---------------------------------------------------------------------------
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
\******************************************************************************/

#include "socket.h"
#include "server.h"

#ifdef _WIN32
// Winsock versions of pollfd and poll are named differently, but work the same
#    define pollfd WSAPOLLFD
#    define poll   WSAPoll
typedef int socklen_t;
#else
// include Unix-style support for non-blocking and poll()
#    include <fcntl.h>
#    include <poll.h>
#endif

/* Implementation *************************************************************/

// Connections -------------------------------------------------------------
// it is important to do the following connections in this class since we
// have a thread transition

// we have different connections for client and server, created after Init in corresponding constructor

CSocket::CSocket ( CChannel*      pNewChannel,
                   const quint16  iPortNumber,
                   const quint16  iQosNumber,
                   const QString& strServerBindIP4,
                   const QString& strServerBindIP6,
                   const bool     bDisableIPv6,
                   bool&          bIPv6Available ) :
    pChannel ( pNewChannel ),
    bIsClient ( true ),
    bJitterBufferOK ( true ),
    bIPv6Available ( bIPv6Available )
{
#ifdef _WIN32
    // for the Windows socket usage we have to start it up first

    //### TODO: BEGIN ###//
    // check for error and exit application on error
    //### TODO: END ###//

    WSADATA wsa;
    WSAStartup ( MAKEWORD ( 1, 0 ), &wsa );
#endif

    UdpSocket4 = INVALID_SOCKET;
    UdpSocket6 = INVALID_SOCKET;

    Init ( iPortNumber, iQosNumber, strServerBindIP4, strServerBindIP6, bDisableIPv6 );

    // client connections:
    QObject::connect ( this, &CSocket::ProtocolMessageReceived, pChannel, &CChannel::OnProtocolMessageReceived );

    QObject::connect ( this, &CSocket::ProtocolCLMessageReceived, pChannel, &CChannel::OnProtocolCLMessageReceived );

    QObject::connect ( this, static_cast<void ( CSocket::* )()> ( &CSocket::NewConnection ), pChannel, &CChannel::OnNewConnection );
}

CSocket::CSocket ( CServer*       pNServP,
                   const quint16  iPortNumber,
                   const quint16  iQosNumber,
                   const QString& strServerBindIP4,
                   const QString& strServerBindIP6,
                   const bool     bDisableIPv6,
                   bool&          bIPv6Available ) :
    pServer ( pNServP ),
    bIsClient ( false ),
    bJitterBufferOK ( true ),
    bIPv6Available ( bIPv6Available )
{
#ifdef _WIN32
    // for the Windows socket usage we have to start it up first

    //### TODO: BEGIN ###//
    // check for error and exit application on error
    //### TODO: END ###//

    WSADATA wsa;
    WSAStartup ( MAKEWORD ( 1, 0 ), &wsa );
#endif

    UdpSocket4 = INVALID_SOCKET;
    UdpSocket6 = INVALID_SOCKET;

    Init ( iPortNumber, iQosNumber, strServerBindIP4, strServerBindIP6, bDisableIPv6 );

    // server connections:
    QObject::connect ( this, &CSocket::ProtocolMessageReceived, pServer, &CServer::OnProtocolMessageReceived );

    QObject::connect ( this, &CSocket::ProtocolCLMessageReceived, pServer, &CServer::OnProtocolCLMessageReceived );

    QObject::connect ( this,
                       static_cast<void ( CSocket::* ) ( int, int, CHostAddress )> ( &CSocket::NewConnection ),
                       pServer,
                       &CServer::OnNewConnection );

    QObject::connect ( this, &CSocket::ServerFull, pServer, &CServer::OnServerFull );
}

void CSocket::Init ( const quint16  iNewPortNumber,
                     const quint16  iNewQosNumber,
                     const QString& strNewServerBindIP,
                     const QString& strNewServerBindIP6,
                     const bool     bDisableIPv6 )
{
    const bool bDisableIPv4 = false; // for future use

    // first, close sockets if they are open - in case of re-init
    if ( UdpSocket4 != INVALID_SOCKET )
    {
#ifdef _WIN32
        closesocket ( UdpSocket4 );
#else
        close ( UdpSocket4 );
#endif
        UdpSocket4 = INVALID_SOCKET;
    }

    if ( UdpSocket6 != INVALID_SOCKET )
    {
#ifdef _WIN32
        closesocket ( UdpSocket6 );
#else
        close ( UdpSocket6 );
#endif
        UdpSocket6 = INVALID_SOCKET;
    }

    struct sockaddr_in sa4;
    socklen_t          sa4len = sizeof ( sa4 );
    memset ( &sa4, 0, sa4len );

    struct sockaddr_in6 sa6;
    socklen_t           sa6len = sizeof ( sa6 );
    memset ( &sa6, 0, sa6len );

    // first store parameters, in case reinit is required (mostly for iOS)
    iPortNumber      = iNewPortNumber;
    iQosNumber       = iNewQosNumber;
    strServerBindIP4 = strNewServerBindIP;
    strServerBindIP6 = strNewServerBindIP6;

    if ( !bDisableIPv4 )
    {
        // create the UDP socket for IPv4
        UdpSocket4 = socket ( AF_INET, SOCK_DGRAM, 0 );
        if ( UdpSocket4 == INVALID_SOCKET )
        {
            // IPv4 requested but not available, throw error (should never happen, but check anyway)
            throw CGenErr ( "IPv4 requested but not available on this system.", "Network Error" );
        }

#if !defined( Q_OS_WIN )
        // set the QoS
        const int tos = (int) iQosNumber; // Quality of Service
        if ( setsockopt ( UdpSocket4, IPPROTO_IP, IP_TOS, (const char*) &tos, sizeof ( tos ) ) == -1 )
        {
            throw CGenErr ( "request to set ToS for IPv4 failed", "Network Error" );
        }
#endif

        // preinitialize socket in address (only the port number is missing)
#ifdef Q_OS_BSD4
        sa4.sin_len = sa4len;
#endif
        sa4.sin_family      = AF_INET;
        sa4.sin_addr.s_addr = INADDR_ANY;

        if ( !strServerBindIP4.isEmpty() )
        {
            QHostAddress qtAddr ( strServerBindIP4 );
            if ( qtAddr.protocol() == QAbstractSocket::IPv4Protocol )
            {
                sa4.sin_addr.s_addr = htonl ( QHostAddress ( strServerBindIP4 ).toIPv4Address() );
            }
            else
            {
                throw CGenErr ( "Invalid IPv4 address given for bind", "Network Error" );
            }
        }

        // set socket to non-blocking
#ifdef _WIN32
        unsigned long mode = 1;
        ioctlsocket ( UdpSocket4, FIONBIO, &mode ); // TODO check for error
#else
        int flags = fcntl ( UdpSocket4, F_GETFL, 0 );
        if ( flags != -1 )
        {
            fcntl ( UdpSocket4, F_SETFL, flags | O_NONBLOCK ); // TODO check for error
        }
#endif

#ifdef Q_OS_IOS
        // ignore the broken pipe signal to avoid crash (iOS)
        int valueone = 1;
        setsockopt ( UdpSocket4, SOL_SOCKET, SO_NOSIGPIPE, &valueone, sizeof ( valueone ) );
#endif

        qInfo() << "IPv4 socket created";
    }

    if ( !bDisableIPv6 )
    {
        // try to create a IPv6 UDP socket
        UdpSocket6 = socket ( AF_INET6, SOCK_DGRAM, 0 );
        if ( UdpSocket6 != INVALID_SOCKET )
        {
            // The IPV6_V6ONLY socket option must be true in order for the socket to listen on just IPv6 protocol.
            // On Linux it's false by default on most (all?) distros, but on Windows it is true by default
            const int yes = 1;
            if ( setsockopt ( UdpSocket6, IPPROTO_IPV6, IPV6_V6ONLY, (const char*) &yes, sizeof ( yes ) ) == -1 )
            {
                throw CGenErr ( "request to set IPv6-only failed", "Network Error" );
            }

            // set the QoS
            const int tos = (int) iQosNumber; // Quality of Service
#if !defined( Q_OS_WIN )
            if ( setsockopt ( UdpSocket6, IPPROTO_IPV6, IPV6_TCLASS, (const char*) &tos, sizeof ( tos ) ) == -1 )
            {
                throw CGenErr ( "request to set ToS for IPv6 failed", "Network Error" );
            }
#endif

#ifdef Q_OS_BSD4
            sa6.sin6_len = sa6len;
#endif
            sa6.sin6_family = AF_INET6;
            sa6.sin6_addr   = in6addr_any;

            if ( !strServerBindIP6.isEmpty() )
            {
                QHostAddress qtAddr ( strServerBindIP6 );
                if ( qtAddr.protocol() == QAbstractSocket::IPv6Protocol )
                {
                    Q_IPV6ADDR ip6 = qtAddr.toIPv6Address();
                    memcpy ( &sa6.sin6_addr.s6_addr, ip6.c, sizeof ( sa6.sin6_addr.s6_addr ) );
                    sa6.sin6_scope_id = qtAddr.scopeId().toUInt();
                }
                else
                {
                    throw CGenErr ( "Invalid IPv6 address given for bind", "Network Error" );
                }
            }

            bIPv6Available = true; // this is a reference to CClient::bIPv6Available or CServer::bIPv6Available

            // set socket to non-blocking
#ifdef _WIN32
            unsigned long mode = 1;
            ioctlsocket ( UdpSocket6, FIONBIO, &mode ); // TODO check for error
#else
            int flags = fcntl ( UdpSocket6, F_GETFL, 0 );
            if ( flags != -1 )
            {
                fcntl ( UdpSocket6, F_SETFL, flags | O_NONBLOCK ); // TODO check for error
            }
#endif

#ifdef Q_OS_IOS
            // ignore the broken pipe signal to avoid crash (iOS)
            int valueone = 1;
            setsockopt ( UdpSocket6, SOL_SOCKET, SO_NOSIGPIPE, &valueone, sizeof ( valueone ) );
#endif

            qInfo() << "IPv6 socket created";
        }
    }

    // allocate memory for network receive and send buffer in samples
    vecbyRecBuf.Init ( MAX_SIZE_BYTES_NETW_BUF );

    // initialize the listening socket
    bool bSuccess;

    if ( bIsClient )
    {
        if ( iPortNumber == 0 )
        {
            // if port number is 0, bind the client to a random available port
            sa4.sin_port = sa6.sin6_port = htons ( 0 );

            bSuccess = ( ::bind ( UdpSocket4, (struct sockaddr*) &sa4, sa4len ) == 0 );

            if ( UdpSocket6 != INVALID_SOCKET )
            {
                bSuccess = bSuccess && ( ::bind ( UdpSocket6, (struct sockaddr*) &sa6, sa6len ) == 0 );
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
                sa4.sin_port = sa6.sin6_port = htons ( startingPortNumber + iClientPortIncrement );

                bSuccess = ( ::bind ( UdpSocket4, (struct sockaddr*) &sa4, sa4len ) == 0 );

                if ( UdpSocket6 != INVALID_SOCKET )
                {
                    bSuccess = bSuccess && ( ::bind ( UdpSocket6, (struct sockaddr*) &sa6, sa6len ) == 0 );
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

        sa4.sin_port = sa6.sin6_port = htons ( iPortNumber );

        bSuccess = ( ::bind ( UdpSocket4, (struct sockaddr*) &sa4, sa4len ) == 0 );

        if ( UdpSocket6 != INVALID_SOCKET )
        {
            bSuccess = bSuccess && ( ::bind ( UdpSocket6, (struct sockaddr*) &sa6, sa6len ) == 0 );
        }
    }

    if ( !bSuccess )
    {
        // we cannot bind socket, throw error
        throw CGenErr ( "Cannot bind the socket (maybe "
                        "the software is already running).",
                        "Network Error" );
    }
}

void CSocket::Close()
{
    if ( UdpSocket4 != INVALID_SOCKET )
    {
#ifdef _WIN32
        // closesocket will cause recvfrom to return with an error because the
        // socket is closed -> then the thread can safely be shut down
        closesocket ( UdpSocket4 );
#elif defined( __APPLE__ ) || defined( __MACOSX )
        // on Mac the general close has the same effect as closesocket on Windows
        close ( UdpSocket4 );
#else
        // on Linux the shutdown call cancels the recvfrom
        shutdown ( UdpSocket4, SHUT_RDWR );
#endif
        UdpSocket4 = INVALID_SOCKET;
    }

    if ( UdpSocket6 != INVALID_SOCKET )
    {
#ifdef _WIN32
        // closesocket will cause recvfrom to return with an error because the
        // socket is closed -> then the thread can safely be shut down
        closesocket ( UdpSocket6 );
#elif defined( __APPLE__ ) || defined( __MACOSX )
        // on Mac the general close has the same effect as closesocket on Windows
        close ( UdpSocket6 );
#else
        // on Linux the shutdown call cancels the recvfrom
        shutdown ( UdpSocket6, SHUT_RDWR );
#endif
        UdpSocket6 = INVALID_SOCKET;
    }
}

CSocket::~CSocket()
{
    // cleanup the socket (on Windows the WSA cleanup must also be called)
#ifdef _WIN32
    if ( UdpSocket4 != INVALID_SOCKET )
        closesocket ( UdpSocket4 );
    if ( UdpSocket6 != INVALID_SOCKET )
        closesocket ( UdpSocket6 );
    WSACleanup();
#else
    if ( UdpSocket4 != INVALID_SOCKET )
        close ( UdpSocket4 );
    if ( UdpSocket6 != INVALID_SOCKET )
        close ( UdpSocket6 );
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

        for ( int tries = 0; tries < 2; tries++ ) // retry loop in case send fails on iOS
        {
            int status = -1;

            if ( HostAddr.InetAddr.protocol() == QAbstractSocket::IPv4Protocol )
            {
                if ( UdpSocket4 != INVALID_SOCKET )
                {
                    struct sockaddr_in sa4;
                    memset ( &sa4, 0, sizeof ( sa4 ) );
#ifdef Q_OS_BSD4
                    sa4.sin_len = sizeof ( sa4 );
#endif
                    sa4.sin_family      = AF_INET;
                    sa4.sin_port        = htons ( HostAddr.iPort );
                    sa4.sin_addr.s_addr = htonl ( HostAddr.InetAddr.toIPv4Address() );

                    status = sendto ( UdpSocket4,
                                      (const char*) &( (CVector<uint8_t>) vecbySendBuf )[0],
                                      iVecSizeOut,
                                      0,
                                      (struct sockaddr*) &sa4,
                                      sizeof ( sa4 ) );
                }
            }
            else if ( HostAddr.InetAddr.protocol() == QAbstractSocket::IPv6Protocol )
            {
                if ( UdpSocket6 != INVALID_SOCKET )
                {
                    struct sockaddr_in6 sa6;
                    memset ( &sa6, 0, sizeof ( sa6 ) );
#ifdef Q_OS_BSD4
                    sa6.sin6_len = sizeof ( sa6 );
#endif
                    sa6.sin6_family = AF_INET6;
                    sa6.sin6_port   = htons ( HostAddr.iPort );

                    Q_IPV6ADDR ip6 = HostAddr.InetAddr.toIPv6Address();
                    memcpy ( &sa6.sin6_addr.s6_addr, ip6.c, sizeof ( sa6.sin6_addr.s6_addr ) );
                    sa6.sin6_scope_id = HostAddr.InetAddr.scopeId().toUInt();

                    status = sendto ( UdpSocket6,
                                      (const char*) &( (CVector<uint8_t>) vecbySendBuf )[0],
                                      iVecSizeOut,
                                      0,
                                      (struct sockaddr*) &sa6,
                                      sizeof ( sa6 ) );
                }
            }

            if ( status >= 0 )
            {
                break; // do not retry if success
            }

#ifdef Q_OS_IOS
            // qDebug("Socket send exception - mostly happens in iOS when returning from idle");
            Init ( iPortNumber, iQosNumber, strServerBindIP4, strServerBindIP6, !bIPv6Available ); // reinit

            // loop back to retry
#endif
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

    CHostAddress RecHostAddr;

    // first, poll to wait until data is available
    int    numfds = 0;
    pollfd fds[2];
    if ( UdpSocket4 != INVALID_SOCKET )
    {
        fds[numfds].fd      = UdpSocket4;
        fds[numfds].events  = POLLIN; // wait for ready to read
        fds[numfds].revents = 0;      // clear returned events
        numfds++;
    }
    if ( UdpSocket6 != INVALID_SOCKET )
    {
        fds[numfds].fd      = UdpSocket6;
        fds[numfds].events  = POLLIN; // wait for ready to read
        fds[numfds].revents = 0;      // clear returned events
        numfds++;
    }

    // wait for data ready with 100ms timeout
    int pollResult = poll ( fds, numfds, 100 );
    if ( pollResult < 0 )
    {
        qDebug() << "Error returned from poll()";
        return;
    }

    if ( pollResult == 0 )
    {
        // timeout with no data
        return;
    }

    int retfds = 0;
    // IPv4 socket
    if ( retfds < numfds && fds[retfds].fd == UdpSocket4 )
    {
        if ( fds[retfds].revents & POLLIN )
        {
            // read block from network interface and query address of sender
            sockaddr_in sa4;
            socklen_t   sa4len = sizeof ( sa4 );

            while ( true )
            {
                const long iNumBytesRead =
                    recvfrom ( UdpSocket4, (char*) &vecbyRecBuf[0], MAX_SIZE_BYTES_NETW_BUF, 0, (struct sockaddr*) &sa4, &sa4len );

                // check if an error occurred or no data could be read
                if ( iNumBytesRead < 0 )
                {
#ifdef _WIN32
                    int err = WSAGetLastError();
                    if ( err == WSAEWOULDBLOCK )
                    {
                        break;
                    }
#else
                    if ( errno == EAGAIN || errno == EWOULDBLOCK )
                    {
                        break;
                    }
#endif
                    qDebug() << "Unexpected error returned by recvfrom()";
                    break;
                }

                if ( iNumBytesRead == 0 )
                {
                    qDebug() << "Zero bytes returned by recvfrom()";
                    break;
                }

                // convert address of client
                RecHostAddr.InetAddr.setAddress ( ntohl ( sa4.sin_addr.s_addr ) );
                RecHostAddr.iPort = ntohs ( sa4.sin_port );

                ProcessPacket ( RecHostAddr, iNumBytesRead );
            }
        }
        retfds++;
    }

    // IPv6 socket
    if ( retfds < numfds && fds[retfds].fd == UdpSocket6 )
    {
        if ( fds[retfds].revents & POLLIN )
        {
            // read block from network interface and query address of sender
            sockaddr_in6 sa6;
            socklen_t    sa6len = sizeof ( sa6 );

            while ( true )
            {
                const long iNumBytesRead =
                    recvfrom ( UdpSocket6, (char*) &vecbyRecBuf[0], MAX_SIZE_BYTES_NETW_BUF, 0, (struct sockaddr*) &sa6, &sa6len );

                // check if an error occurred or no data could be read
                if ( iNumBytesRead < 0 )
                {
#ifdef _WIN32
                    int err = WSAGetLastError();
                    if ( err == WSAEWOULDBLOCK )
                    {
                        break;
                    }
#else
                    if ( errno == EAGAIN || errno == EWOULDBLOCK )
                    {
                        break;
                    }
#endif
                    qDebug() << "Unexpected error returned by recvfrom()";
                    break;
                }

                if ( iNumBytesRead == 0 )
                {
                    qDebug() << "Zero bytes returned by recvfrom()";
                    break;
                }

                // convert address of client
                RecHostAddr.InetAddr.setAddress ( sa6.sin6_addr.s6_addr );
                RecHostAddr.iPort = ntohs ( sa6.sin6_port );

                ProcessPacket ( RecHostAddr, iNumBytesRead );
            }
        }
        retfds++;
    }
}

void CSocket::ProcessPacket ( const CHostAddress& RecHostAddr, const int iNumBytesRead )
{
    // check if this is a protocol message
    int              iRecCounter;
    int              iRecID;
    CVector<uint8_t> vecbyMesBodyData;

    if ( !CProtocol::ParseMessageFrame ( vecbyRecBuf, iNumBytesRead, vecbyMesBodyData, iRecCounter, iRecID ) )
    {
        // this is a protocol message, check the type of the message
        if ( CProtocol::IsConnectionLessMessageID ( iRecID ) )
        {
            //### TODO: BEGIN ###//
            // a copy of the vector is used -> avoid malloc in real-time routine
            emit ProtocolCLMessageReceived ( iRecID, vecbyMesBodyData, RecHostAddr );
            //### TODO: END ###//
        }
        else
        {
            //### TODO: BEGIN ###//
            // a copy of the vector is used -> avoid malloc in real-time routine
            emit ProtocolMessageReceived ( iRecCounter, iRecID, vecbyMesBodyData, RecHostAddr );
            //### TODO: END ###//
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
                emit NewConnection ( iCurChanID, pServer->GetNumberOfConnectedClients(), RecHostAddr );

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
