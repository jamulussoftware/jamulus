/******************************************************************************\
 * Copyright (c) 2004-2025
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
#    include <winsock2.h>
#    include <ws2tcpip.h>
#else
#    include <arpa/inet.h>
#endif

/* Implementation *************************************************************/

// Connections -------------------------------------------------------------
// it is important to do the following connections in this class since we
// have a thread transition

// we have different connections for client and server, created after Init in corresponding constructor

CSocket::CSocket ( CChannel* pNewChannel, const quint16 iPortNumber, const quint16 iQosNumber, const QString& strServerBindIP, bool bEnableIPv6 ) :
    pChannel ( pNewChannel ),
    bIsClient ( true ),
    bJitterBufferOK ( true ),
    bEnableIPv6 ( bEnableIPv6 )
{
    Init ( iPortNumber, iQosNumber, strServerBindIP );
#if !defined( HEADLESS ) && !defined( JAMULUS_USE_JUCE_NET )
    QObject::connect ( this, &CSocket::ProtocolMessageReceived, pChannel, &CChannel::OnProtocolMessageReceived );
    QObject::connect ( this, &CSocket::ProtocolCLMessageReceived, pChannel, &CChannel::OnProtocolCLMessageReceived );
    QObject::connect ( this, static_cast<void ( CSocket::* )()> ( &CSocket::NewConnection ), pChannel, &CChannel::OnNewConnection );
#endif
}

CSocket::CSocket ( CServer* pNServP, const quint16 iPortNumber, const quint16 iQosNumber, const QString& strServerBindIP, bool bEnableIPv6 ) :
    pServer ( pNServP ),
    bIsClient ( false ),
    bJitterBufferOK ( true ),
    bEnableIPv6 ( bEnableIPv6 )
{
    Init ( iPortNumber, iQosNumber, strServerBindIP );
#if !defined( HEADLESS ) && !defined( JAMULUS_USE_JUCE_NET )
    QObject::connect ( this, &CSocket::ProtocolMessageReceived, pServer, &CServer::OnProtocolMessageReceived );
    QObject::connect ( this, &CSocket::ProtocolCLMessageReceived, pServer, &CServer::OnProtocolCLMessageReceived );
    QObject::connect ( this,
                       static_cast<void ( CSocket::* ) ( int, int, CHostAddress )> ( &CSocket::NewConnection ),
                       pServer,
                       &CServer::OnNewConnection );
    QObject::connect ( this, &CSocket::ServerFull, pServer, &CServer::OnServerFull );
#endif
}

void CSocket::Init ( const quint16 iNewPortNumber, const quint16 iNewQosNumber, const QString& strNewServerBindIP )
{
    uSockAddr UdpSocketAddr;

    int       UdpSocketAddrLen;
    uint16_t* UdpPort;

    // first store parameters, in case reinit is required (mostly for iOS)
    iPortNumber     = iNewPortNumber;
    iQosNumber      = iNewQosNumber;
    strServerBindIP = strNewServerBindIP;

#ifdef _WIN32
    // for the Windows socket usage we have to start it up first

    //### TODO: BEGIN ###//
    // check for error and exit application on error
    //### TODO: END ###//

    WSADATA wsa;
    WSAStartup ( MAKEWORD ( 1, 0 ), &wsa );
#endif

    memset ( &UdpSocketAddr, 0, sizeof ( UdpSocketAddr ) );

    if ( bEnableIPv6 )
    {
        // try to create a IPv6 UDP socket
        UdpSocket = socket ( AF_INET6, SOCK_DGRAM, 0 );
        if ( UdpSocket == -1 )
        {
            // IPv6 requested but not available, throw error
            throw CGenErr ( "IPv6 requested but not available on this system.", "Network Error" );
        }

        // The IPV6_V6ONLY socket option must be false in order for the socket to listen on both protocols.
        // On Linux it's false by default on most (all?) distros, but on Windows it is true by default
        const uint8_t no = 0;
        setsockopt ( UdpSocket, IPPROTO_IPV6, IPV6_V6ONLY, (const char*) &no, sizeof ( no ) );

        // set the QoS
        const char tos = (char) iQosNumber; // Quality of Service
        setsockopt ( UdpSocket, IPPROTO_IPV6, IPV6_TCLASS, &tos, sizeof ( tos ) );

        UdpSocketAddr.sa6.sin6_family = AF_INET6;
        UdpSocketAddr.sa6.sin6_addr   = in6addr_any;
        UdpSocketAddrLen              = sizeof ( UdpSocketAddr.sa6 );

        UdpPort = &UdpSocketAddr.sa6.sin6_port; // where to put the port number

        // FIXME: If binding a dual-protocol interface to a specific address, does it cease to be dual-protocol?

        // TODO - ALLOW IPV6 ADDRESS
        // if ( !strServerBindIP.isEmpty() )
        //{
        //    UdpSocketInAddr.sin_addr.s_addr = htonl ( QHostAddress ( strServerBindIP ).toIPv4Address() );
        //}
        // END TODO - ALLOW IPV6 ADDRESS
    }
    else
    {
        // create the UDP socket for IPv4
        UdpSocket = socket ( AF_INET, SOCK_DGRAM, 0 );
        if ( UdpSocket == -1 )
        {
            // IPv4 requested but not available, throw error (should never happen, but check anyway)
            throw CGenErr ( "IPv4 requested but not available on this system.", "Network Error" );
        }

        // set the QoS
        const char tos = (char) iQosNumber; // Quality of Service
        setsockopt ( UdpSocket, IPPROTO_IP, IP_TOS, &tos, sizeof ( tos ) );

        // preinitialize socket in address (only the port number is missing)
        UdpSocketAddr.sa4.sin_family      = AF_INET;
        UdpSocketAddr.sa4.sin_addr.s_addr = INADDR_ANY;
        UdpSocketAddrLen                  = sizeof ( UdpSocketAddr.sa4 );

        UdpPort = &UdpSocketAddr.sa4.sin_port; // where to put the port number

        if ( !strServerBindIP.isEmpty() )
        {
#if defined( HEADLESS ) || defined( JAMULUS_USE_JUCE_NET )
            QByteArray addrBytes = strServerBindIP.toLocal8Bit();
            struct in_addr v4addr;
            if ( inet_pton ( AF_INET, addrBytes.constData(), &v4addr ) == 1 )
            {
                UdpSocketAddr.sa4.sin_addr.s_addr = v4addr.s_addr;
            }
#else
            UdpSocketAddr.sa4.sin_addr.s_addr = htonl ( QHostAddress ( strServerBindIP ).toIPv4Address() );
#endif
        }
    }

#ifdef Q_OS_IOS
    // ignore the broken pipe signal to avoid crash (iOS)
    int valueone = 1;
    setsockopt ( UdpSocket, SOL_SOCKET, SO_NOSIGPIPE, &valueone, sizeof ( valueone ) );
#endif

    // allocate memory for network receive and send buffer in samples
    vecbyRecBuf.Init ( MAX_SIZE_BYTES_NETW_BUF );

    // initialize the listening socket
    bool bSuccess;

    if ( bIsClient )
    {
        if ( iPortNumber == 0 )
        {
            // if port number is 0, bind the client to a random available port
            *UdpPort = htons ( 0 );

            bSuccess = ( ::bind ( UdpSocket, &UdpSocketAddr.sa, UdpSocketAddrLen ) == 0 );
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
                *UdpPort = htons ( startingPortNumber + iClientPortIncrement );

                bSuccess = ( ::bind ( UdpSocket, &UdpSocketAddr.sa, UdpSocketAddrLen ) == 0 );

                iClientPortIncrement++;
            }
        }
    }
    else
    {
        // for the server, only try the given port number and do not try out
        // other port numbers to bind since it is important that the server
        // gets the desired port number
        *UdpPort = htons ( iPortNumber );

        bSuccess = ( ::bind ( UdpSocket, &UdpSocketAddr.sa, UdpSocketAddrLen ) == 0 );
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
    int status = 0;

    uSockAddr UdpSocketAddr;

    memset ( &UdpSocketAddr, 0, sizeof ( UdpSocketAddr ) );

#if defined( HEADLESS )
    std::lock_guard<std::mutex> locker ( Mutex );
#elif defined( JAMULUS_USE_JUCE_NET )
    juce::ScopedLock locker ( Mutex );
#else
    QMutexLocker locker ( &Mutex );
#endif

    const int iVecSizeOut = vecbySendBuf.Size();

    if ( iVecSizeOut > 0 )
    {
        for ( int tries = 0; tries < 2; tries++ )
        {
#if defined( HEADLESS ) || defined( JAMULUS_USE_JUCE_NET )
            std::string addrStr  = HostAddr.address;
            const char* addrBytes = addrStr.c_str();

            if ( bEnableIPv6 )
            {
                UdpSocketAddr.sa6.sin6_family = AF_INET6;
                UdpSocketAddr.sa6.sin6_port   = htons ( HostAddr.iPort );

                if ( inet_pton ( AF_INET6, addrBytes, &UdpSocketAddr.sa6.sin6_addr ) <= 0 )
                {
                    struct in_addr v4addr;
                    if ( inet_pton ( AF_INET, addrBytes, &v4addr ) <= 0 )
                    {
                        return;
                    }

                    uint32_t* addr = (uint32_t*) &UdpSocketAddr.sa6.sin6_addr;
                    addr[0]        = 0;
                    addr[1]        = 0;
                    addr[2]        = htonl ( 0xFFFF );
                    addr[3]        = v4addr.s_addr;
                }

                status = sendto ( UdpSocket,
                                  (const char*) &( (CVector<uint8_t>) vecbySendBuf )[0],
                                  iVecSizeOut,
                                  0,
                                  &UdpSocketAddr.sa,
                                  sizeof ( UdpSocketAddr.sa6 ) );
            }
            else
            {
                UdpSocketAddr.sa4.sin_family = AF_INET;
                UdpSocketAddr.sa4.sin_port   = htons ( HostAddr.iPort );

                struct in_addr v4addr;
                if ( inet_pton ( AF_INET, addrBytes, &v4addr ) <= 0 )
                {
                    return;
                }

                UdpSocketAddr.sa4.sin_addr.s_addr = v4addr.s_addr;

                status = sendto ( UdpSocket,
                                  (const char*) &( (CVector<uint8_t>) vecbySendBuf )[0],
                                  iVecSizeOut,
                                  0,
                                  &UdpSocketAddr.sa,
                                  sizeof ( UdpSocketAddr.sa4 ) );
            }
#else
            if ( HostAddr.InetAddr.protocol() == QAbstractSocket::IPv4Protocol )
            {
                if ( bEnableIPv6 )
                {
                    UdpSocketAddr.sa6.sin6_family = AF_INET6;
                    UdpSocketAddr.sa6.sin6_port   = htons ( HostAddr.iPort );

                    uint32_t* addr = (uint32_t*) &UdpSocketAddr.sa6.sin6_addr;

                    addr[0] = 0;
                    addr[1] = 0;
                    addr[2] = htonl ( 0xFFFF );
                    addr[3] = htonl ( HostAddr.InetAddr.toIPv4Address() );

                    status = sendto ( UdpSocket,
                                      (const char*) &( (CVector<uint8_t>) vecbySendBuf )[0],
                                      iVecSizeOut,
                                      0,
                                      &UdpSocketAddr.sa,
                                      sizeof ( UdpSocketAddr.sa6 ) );
                }
                else
                {
                    UdpSocketAddr.sa4.sin_family      = AF_INET;
                    UdpSocketAddr.sa4.sin_port        = htons ( HostAddr.iPort );
                    UdpSocketAddr.sa4.sin_addr.s_addr = htonl ( HostAddr.InetAddr.toIPv4Address() );

                    status = sendto ( UdpSocket,
                                      (const char*) &( (CVector<uint8_t>) vecbySendBuf )[0],
                                      iVecSizeOut,
                                      0,
                                      &UdpSocketAddr.sa,
                                      sizeof ( UdpSocketAddr.sa4 ) );
                }
            }
            else if ( bEnableIPv6 )
            {
                UdpSocketAddr.sa6.sin6_family = AF_INET6;
                UdpSocketAddr.sa6.sin6_port   = htons ( HostAddr.iPort );
                inet_pton ( AF_INET6, HostAddr.InetAddr.toString().toLocal8Bit().constData(), &UdpSocketAddr.sa6.sin6_addr );

                status = sendto ( UdpSocket,
                                  (const char*) &( (CVector<uint8_t>) vecbySendBuf )[0],
                                  iVecSizeOut,
                                  0,
                                  &UdpSocketAddr.sa,
                                  sizeof ( UdpSocketAddr.sa6 ) );
            }
#endif

            if ( status >= 0 )
            {
                break;
            }

#ifdef Q_OS_IOS
            Init ( iPortNumber, iQosNumber, strServerBindIP );
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

    // read block from network interface and query address of sender
    uSockAddr UdpSocketAddr;
#ifdef _WIN32
    int SenderAddrSize = sizeof ( UdpSocketAddr );
#else
    socklen_t SenderAddrSize = sizeof ( UdpSocketAddr );
#endif

    const long iNumBytesRead = recvfrom ( UdpSocket, (char*) &vecbyRecBuf[0], MAX_SIZE_BYTES_NETW_BUF, 0, &UdpSocketAddr.sa, &SenderAddrSize );

    // check if an error occurred or no data could be read
    if ( iNumBytesRead <= 0 )
    {
        return;
    }

    // convert address of client
#if defined( HEADLESS ) || defined( JAMULUS_USE_JUCE_NET )
    char addrBuf[INET6_ADDRSTRLEN] = {};

    if ( UdpSocketAddr.sa.sa_family == AF_INET6 )
    {
        if ( IN6_IS_ADDR_V4MAPPED ( &( UdpSocketAddr.sa6.sin6_addr ) ) )
        {
            struct in_addr v4addr;
            std::memcpy ( &v4addr, &UdpSocketAddr.sa6.sin6_addr.s6_addr[12], sizeof ( v4addr ) );
            inet_ntop ( AF_INET, &v4addr, addrBuf, sizeof ( addrBuf ) );
        }
        else
        {
            inet_ntop ( AF_INET6, &UdpSocketAddr.sa6.sin6_addr, addrBuf, sizeof ( addrBuf ) );
        }

        RecHostAddr.address = std::string ( addrBuf );
        RecHostAddr.iPort   = ntohs ( UdpSocketAddr.sa6.sin6_port );
    }
    else
    {
        struct in_addr v4addr;
        v4addr.s_addr = UdpSocketAddr.sa4.sin_addr.s_addr;
        inet_ntop ( AF_INET, &v4addr, addrBuf, sizeof ( addrBuf ) );

        RecHostAddr.address = std::string ( addrBuf );
        RecHostAddr.iPort   = ntohs ( UdpSocketAddr.sa4.sin_port );
    }
#else
    if ( UdpSocketAddr.sa.sa_family == AF_INET6 )
    {
        if ( IN6_IS_ADDR_V4MAPPED ( &( UdpSocketAddr.sa6.sin6_addr ) ) )
        {
            const uint32_t addr = ( (const uint32_t*) ( &( UdpSocketAddr.sa6.sin6_addr ) ) )[3];
            RecHostAddr.InetAddr.setAddress ( ntohl ( addr ) );
        }
        else
        {
            RecHostAddr.InetAddr.setAddress ( UdpSocketAddr.sa6.sin6_addr.s6_addr );
        }
        RecHostAddr.iPort = ntohs ( UdpSocketAddr.sa6.sin6_port );
    }
    else
    {
        RecHostAddr.InetAddr.setAddress ( ntohl ( UdpSocketAddr.sa4.sin_addr.s_addr ) );
        RecHostAddr.iPort = ntohs ( UdpSocketAddr.sa4.sin_port );
    }
#endif

    // check if this is a protocol message
    int              iRecCounter;
    int              iRecID;
    CVector<uint8_t> vecbyMesBodyData;

    if ( !CProtocol::ParseMessageFrame ( vecbyRecBuf, iNumBytesRead, vecbyMesBodyData, iRecCounter, iRecID ) )
    {
        // this is a protocol message, check the type of the message
        if ( CProtocol::IsConnectionLessMessageID ( iRecID ) )
        {
            // In headless builds, call directly to avoid Qt signals
#if defined( HEADLESS ) || defined( JAMULUS_USE_JUCE_NET )
            if ( bIsClient && pChannel )
                pChannel->OnProtocolCLMessageReceived ( iRecID, vecbyMesBodyData, RecHostAddr );
            else if ( pServer )
                pServer->OnProtocolCLMessageReceived ( iRecID, vecbyMesBodyData, RecHostAddr );
#else
            emit ProtocolCLMessageReceived ( iRecID, vecbyMesBodyData, RecHostAddr );
#endif
        }
        else
        {
            // In headless builds, call directly to avoid Qt signals
#if defined( HEADLESS ) || defined( JAMULUS_USE_JUCE_NET )
            if ( bIsClient && pChannel )
                pChannel->OnProtocolMessageReceived ( iRecCounter, iRecID, vecbyMesBodyData, RecHostAddr );
            else if ( pServer )
                pServer->OnProtocolMessageReceived ( iRecCounter, iRecID, vecbyMesBodyData, RecHostAddr );
#else
            emit ProtocolMessageReceived ( iRecCounter, iRecID, vecbyMesBodyData, RecHostAddr );
#endif
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
                // inform other objects about new connection
#if defined( HEADLESS ) || defined( JAMULUS_USE_JUCE_NET )
                if ( pChannel )
                    pChannel->OnNewConnection();
#else
                emit NewConnection();
#endif
                break;

            case PS_AUDIO_INVALID:
#if !defined( HEADLESS ) && !defined( JAMULUS_USE_JUCE_NET )
                emit InvalidPacketReceived ( RecHostAddr );
#endif
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
                // we have a new connection
#if defined( HEADLESS ) || defined( JAMULUS_USE_JUCE_NET )
                pServer->OnNewConnection ( iCurChanID, pServer->GetNumberOfConnectedClients(), RecHostAddr );
#else
                emit NewConnection ( iCurChanID, pServer->GetNumberOfConnectedClients(), RecHostAddr );
#endif

                // this was an audio packet, start server if it is in sleep mode
                if ( !pServer->IsRunning() )
                {
#if !defined( HEADLESS ) && !defined( JAMULUS_USE_JUCE_NET )
                    QCoreApplication::postEvent ( pServer, new CCustomEvent ( MS_PACKET_RECEIVED, 0, 0 ) );
#endif
                }
            }

            // check if no channel is available
            if ( iCurChanID == INVALID_CHANNEL_ID )
            {
                // notify about no free channel
#if defined( HEADLESS ) || defined( JAMULUS_USE_JUCE_NET )
                pServer->OnServerFull ( RecHostAddr );
#else
                emit ServerFull ( RecHostAddr );
#endif
            }
        }
    }
}
