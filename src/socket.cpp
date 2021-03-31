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
#include <arpa/inet.h>
/* Implementation *************************************************************/
void CSocket::Init ( const quint16 iPortNumber )
{
#ifdef _WIN32
    // for the Windows socket usage we have to start it up first

// TODO check for error and exit application on error

    WSADATA wsa;
    WSAStartup ( MAKEWORD(1, 0), &wsa );
#endif

    // create the UDP socket
    UdpSocket = socket ( AF_INET6, SOCK_DGRAM, 0);
    int on = 1;
    int off = 0;
    setsockopt(UdpSocket, IPPROTO_IPV6, IPV6_PKTINFO,  &on, sizeof(on));
    // allow to receive ipv4 packet info (i.e. destination ip address through recvmsg
    setsockopt(UdpSocket, IPPROTO_IP, IP_PKTINFO, &on, sizeof(on));
    // allow to receive ipv6 packetinfo
    setsockopt(UdpSocket, IPPROTO_IPV6, IPV6_RECVPKTINFO, &on, sizeof(on));
    // allow to receive v6 and v4 requests
    setsockopt(UdpSocket, IPPROTO_IPV6, IPV6_V6ONLY, &off, sizeof(off));
    // allocate memory for network receive and send buffer in samples
    vecbyRecBuf.Init ( MAX_SIZE_BYTES_NETW_BUF );

    // preinitialize socket in address (only the port number is missing)
    sockaddr_in6 UdpSocketInAddr;
    UdpSocketInAddr.sin6_family      = AF_INET6;
    UdpSocketInAddr.sin6_addr       = in6addr_any;
    // initialize the listening socket
    bool bSuccess;

    if ( bIsClient )
    {
        if ( iPortNumber == 0 )
        {
            // if port number is 0, bind the client to a random available port
            UdpSocketInAddr.sin6_port = htons ( 0 );
            bSuccess = ( ::bind ( UdpSocket ,
                                (sockaddr*) &UdpSocketInAddr,
				sizeof ( sockaddr_in6 ) ) == 0);
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
                UdpSocketInAddr.sin6_port = htons ( startingPortNumber + iClientPortIncrement );
                bSuccess = ( ::bind ( UdpSocket ,
                                      (sockaddr*) &UdpSocketInAddr,
                                      sizeof ( sockaddr_in6 ) ) == 0 );

                iClientPortIncrement++;
            }
        }
    }
    else
    {
        // for the server, only try the given port number and do not try out
        // other port numbers to bind since it is important that the server
        // gets the desired port number
        UdpSocketInAddr.sin6_port = htons ( iPortNumber );
        bSuccess = ( ::bind ( UdpSocket ,
                              (sockaddr*) &UdpSocketInAddr,
                              sizeof ( sockaddr_in6 ) ) == 0 );
    }

    if ( !bSuccess )
    {
        // we cannot bind socket, throw error
        throw CGenErr ( "Cannot bind the socket (maybe "
            "the software is already running).", "Network Error" );
    }


    // Connections -------------------------------------------------------------
    // it is important to do the following connections in this class since we
    // have a thread transition

    // we have different connections for client and server
    if ( bIsClient )
    {
        // client connections:

        QObject::connect ( this, &CSocket::ProtcolMessageReceived,
            pChannel, &CChannel::OnProtcolMessageReceived );

        QObject::connect ( this, &CSocket::ProtcolCLMessageReceived,
            pChannel, &CChannel::OnProtcolCLMessageReceived );

        QObject::connect ( this, static_cast<void (CSocket::*)()> ( &CSocket::NewConnection ),
            pChannel, &CChannel::OnNewConnection );
    }
    else
    {
        // server connections:

        QObject::connect ( this, &CSocket::ProtcolMessageReceived,
            pServer, &CServer::OnProtcolMessageReceived );

        QObject::connect ( this, &CSocket::ProtcolCLMessageReceived,
            pServer, &CServer::OnProtcolCLMessageReceived );

        QObject::connect ( this, static_cast<void (CSocket::*) ( int, CHostAddress )> ( &CSocket::NewConnection ),
            pServer, &CServer::OnNewConnection );

        QObject::connect ( this, &CSocket::ServerFull,
            pServer, &CServer::OnServerFull );
    }
}

void CSocket::Close()
{
#ifdef _WIN32
    // closesocket will cause recvfrom to return with an error because the
    // socket is closed -> then the thread can safely be shut down
    closesocket ( UdpSocket );
#elif defined ( __APPLE__ ) || defined ( __MACOSX )
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

void CSocket::SendPacket ( const CVector<uint8_t>& vecbySendBuf,
                           const CHostAddress&     HostAddr )
{
    QMutexLocker locker ( &Mutex );

    const int iVecSizeOut = vecbySendBuf.Size();

    if ( iVecSizeOut > 0 )
    {
        // send packet through network (we have to convert the constant unsigned
        // char vector in "const char*", for this we first convert the const
        // uint8_t vector in a read/write uint8_t vector and then do the cast to
        // const char*)
        sockaddr_in6 UdpSocketOutAddr;
        UdpSocketOutAddr.sin6_family      = AF_INET6;
        UdpSocketOutAddr.sin6_port        = htons ( HostAddr.iPort );

//	memcpy(&UdpSocketOutAddr.sin6_addr.s6_addr,(void *)&HostAddr.InetAddr.toIPv6Address(),16);
  	    Q_IPV6ADDR ipv6addr_h = HostAddr.InetAddr.toIPv6Address();
	    memcpy (&UdpSocketOutAddr.sin6_addr.s6_addr,&ipv6addr_h,16);
        msghdr m;
        struct iovec iov[1];
        int rv;
        char pktinfo[100];
        m.msg_name = (sockaddr*) &UdpSocketOutAddr;
        m.msg_namelen = sizeof ( sockaddr_in6 );
          
        iov[0].iov_base = (void *) &( vecbySendBuf )[0];
        iov[0].iov_len = iVecSizeOut;
        m.msg_iov = iov;
        m.msg_iovlen = 1;
        //m.msg_control = pktinfo;
        m.msg_control = NULL;
        m.msg_controllen = 0;
        bool isv4;
        quint32 ipaddr4_l = HostAddr.LocalAddr.toIPv4Address(&isv4);
        if (isv4)
        {
            if (ipaddr4_l != 0)
            {
               struct in_addr ia;
               struct in_addr da;
               memset(pktinfo,0,sizeof(pktinfo));
               struct in_pktinfo *pktinfo2;
               m.msg_control = pktinfo;
               struct cmsghdr *cmsg = (struct cmsghdr *) pktinfo;
               cmsg->cmsg_level = IPPROTO_IP;
               cmsg->cmsg_type = IP_PKTINFO;
               cmsg->cmsg_len = CMSG_LEN(sizeof(struct in_pktinfo));
               
               pktinfo2 = (struct in_pktinfo*) CMSG_DATA(cmsg);
               pktinfo2->ipi_ifindex = 0;
               ia.s_addr = ipaddr4_l;
               da.s_addr = 0;
               pktinfo2->ipi_spec_dst = ia;
               pktinfo2->ipi_addr = da;
               m.msg_controllen += CMSG_SPACE(sizeof(struct in_pktinfo));
               
            } else 
            { 
                ;
            }
        }
        else
        {
           memset(pktinfo,0,sizeof(pktinfo));
           m.msg_control = pktinfo;
           struct cmsghdr *cmsg = (struct cmsghdr *)pktinfo;
           cmsg->cmsg_level = IPPROTO_IPV6;
           cmsg->cmsg_type = IPV6_PKTINFO;
           cmsg->cmsg_len = CMSG_LEN(16);
           memcpy(CMSG_DATA(cmsg),&HostAddr.LocalAddr,16);
           m.msg_controllen += CMSG_SPACE(16);          
        }
        rv=sendmsg(UdpSocket,&m,0);
        if (rv<0) fprintf(stderr, "%s\n", strerror(errno));
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
	sockaddr_in6 SenderAddr;
    in6_addr LocalInterfaceAddr;
#ifdef _WIN32
    int SenderAddrSize = sizeof ( sockaddr_in );
#else
    socklen_t SenderAddrSize = sizeof ( sockaddr_in6 );
#endif
    //const long iNumBytesRead = recvfrom ( UdpSocket,
    //                                      (char*) &vecbyRecBuf[0],
    //                                      MAX_SIZE_BYTES_NETW_BUF,
    //                        f              0,
    //                                      (sockaddr*) &SenderAddr,
    //                                      &SenderAddrSize );
           struct msghdr msgh;
           struct iovec iov[1];
        
           struct cmsghdr *cmsg;
           // Prepare message header structure
           memset( &msgh, 0, sizeof(msgh));
           msgh.msg_name = &SenderAddr;
           msgh.msg_namelen = SenderAddrSize;
           msgh.msg_iovlen = 1;
           msgh.msg_iov = iov;
           iov[0].iov_base = (char *) &vecbyRecBuf[0];
           iov[0].iov_len = MAX_SIZE_BYTES_NETW_BUF;
           char controlbuffer[4096];
           memset( controlbuffer, 0, sizeof(controlbuffer));
           msgh.msg_control = controlbuffer;
           msgh.msg_controllen = sizeof(controlbuffer);
           const long iNumBytesRead  = ::recvmsg(UdpSocket, &msgh, 0);
           
           /* Receive auxiliary data in msgh */
           bool la_found = false;
           for (cmsg = CMSG_FIRSTHDR(&msgh); cmsg != NULL;
                   cmsg = CMSG_NXTHDR(&msgh, cmsg)) {
               //printf("d=%d ",cmsg->cmsg_level);
               if (cmsg->cmsg_level == IPPROTO_IPV6
                       && cmsg->cmsg_type == IPV6_PKTINFO) {
                   memcpy(&LocalInterfaceAddr, CMSG_DATA(cmsg), sizeof(LocalInterfaceAddr));
                   char lesbareadresse[40];
                   inet_ntop(AF_INET6,&LocalInterfaceAddr,lesbareadresse,sizeof(lesbareadresse));
                   la_found = true;
                   //fprintf(stderr,"inet6addresse = %s",lesbareadresse);
                   break;
               }                  
           }


    // check if an error occurred or no data could be read
    if ( iNumBytesRead <= 0 )
    {
        return;
    }

    // convert address of client
    Q_IPV6ADDR saddr6_tmp;
    Q_IPV6ADDR iaddr6_tmp;
    bool isv4 = false;
    quint32 i4a;
    memcpy(&saddr6_tmp,&SenderAddr.sin6_addr,16);
    memcpy(&iaddr6_tmp,&LocalInterfaceAddr,16);
    // Behandle ipv6-mapped ipv4-Adressen
    RecHostAddr.InetAddr.setAddress(saddr6_tmp);
    i4a = RecHostAddr.InetAddr.toIPv4Address(&isv4);
    if (isv4)
    {
	RecHostAddr.InetAddr.setAddress(i4a);
    }
    isv4 = false;
    if (la_found) RecHostAddr.LocalAddr.setAddress(iaddr6_tmp);
    RecHostAddr.iPort = ntohs ( SenderAddr.sin6_port );


    // check if this is a protocol message
    int              iRecCounter;
    int              iRecID;
    CVector<uint8_t> vecbyMesBodyData;

    if ( !CProtocol::ParseMessageFrame ( vecbyRecBuf,
                                         iNumBytesRead,
                                         vecbyMesBodyData,
                                         iRecCounter,
                                         iRecID ) )
    {
        // this is a protocol message, check the type of the message
        if ( CProtocol::IsConnectionLessMessageID ( iRecID ) )
        {

// TODO a copy of the vector is used -> avoid malloc in real-time routine

            emit ProtcolCLMessageReceived ( iRecID, vecbyMesBodyData, RecHostAddr );
        }
        else
        {

// TODO a copy of the vector is used -> avoid malloc in real-time routine

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
                    QCoreApplication::postEvent ( pServer,
                        new CCustomEvent ( MS_PACKET_RECEIVED, 0, 0 ) );
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
