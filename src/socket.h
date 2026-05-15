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

#pragma once

#include <QObject>
#include <QThread>
#include <QMutex>
#include <cstring>
#include <vector>
#include "global.h"
#include "net_abstraction.h"
#include "protocol.h"
#include "util.h"
#ifndef _WIN32
#    include <netinet/in.h>
#    include <sys/socket.h>
#endif

class CServer;
class CChannel;

#define NUM_SOCKET_PORTS_TO_TRY 100

class CSocket : public QObject
{
    Q_OBJECT

public:
    CSocket ( CChannel* pNewChannel, const quint16 iPortNumber, const quint16 iQosNumber, const QString& strServerBindIP, bool bEnableIPv6 );
    CSocket ( CServer* pNServP, const quint16 iPortNumber, const quint16 iQosNumber, const QString& strServerBindIP, bool bEnableIPv6 );

    virtual ~CSocket();

    void SendPacket ( const CVector<uint8_t>& vecbySendBuf, const CHostAddress& HostAddr );

    bool GetAndResetbJitterBufferOKFlag();
    void Close();

protected:
    void    Init ( const quint16 iPortNumber, const quint16 iQosNumber, const QString& strServerBindIP );
    quint16 iPortNumber;
    quint16 iQosNumber;
    QString strServerBindIP;

#ifdef _WIN32
    SOCKET UdpSocket;
#else
    int UdpSocket;
#endif

    QMutex Mutex;

    CVector<uint8_t> vecbyRecBuf;

    CChannel* pChannel;
    CServer*  pServer;

    bool bIsClient;

    bool bJitterBufferOK;

    bool bEnableIPv6;

public:
    void OnDataReceived();

signals:
    void NewConnection();

    void NewConnection ( int iChID, int iTotChans,
                         CHostAddress RecHostAddr );

    void ServerFull ( CHostAddress RecHostAddr );

    void InvalidPacketReceived ( CHostAddress RecHostAddr );

    void ProtocolMessageReceived ( int iRecCounter, int iRecID, CVector<uint8_t> vecbyMesBodyData, CHostAddress HostAdr );

    void ProtocolCLMessageReceived ( int iRecID, CVector<uint8_t> vecbyMesBodyData, CHostAddress HostAdr );
};

class CHighPrioSocket : public QObject
{
    Q_OBJECT

public:
    CHighPrioSocket ( CChannel* pNewChannel, const quint16 iPortNumber, const quint16 iQosNumber, const QString& strServerBindIP, bool bEnableIPv6 ) :
        Socket ( pNewChannel, iPortNumber, iQosNumber, strServerBindIP, bEnableIPv6 )
    {
        Init();
    }

    CHighPrioSocket ( CChannel* pNewChannel, const quint16 iPortNumber, const quint16 iQosNumber, bool bEnableIPv6 ) :
        Socket ( pNewChannel, iPortNumber, iQosNumber, "", bEnableIPv6 )
    {
        Init();
    }

    CHighPrioSocket ( CServer* pNewServer, const quint16 iPortNumber, const quint16 iQosNumber, const QString& strServerBindIP, bool bEnableIPv6 ) :
        Socket ( pNewServer, iPortNumber, iQosNumber, strServerBindIP, bEnableIPv6 )
    {
        Init();
    }

    virtual ~CHighPrioSocket() { NetworkWorkerThread.Stop(); }

    void Start()
    {
        NetworkWorkerThread.start ( QThread::TimeCriticalPriority );
    }

    void SendPacket ( const CVector<uint8_t>& vecbySendBuf, const CHostAddress& HostAddr ) { Socket.SendPacket ( vecbySendBuf, HostAddr ); }

    bool GetAndResetbJitterBufferOKFlag() { return Socket.GetAndResetbJitterBufferOKFlag(); }

protected:
    class CSocketThread : public QThread
    {
    public:
        CSocketThread ( CSocket* pNewSocket = nullptr, QObject* parent = nullptr ) : QThread ( parent ), pSocket ( pNewSocket ), bRun ( true )
        {
            setObjectName ( "CSocketThread" );
        }

        void Stop()
        {
            bRun = false;
            pSocket->Close();
            wait ( 5000 );
        }

        void SetSocket ( CSocket* pNewSocket ) { pSocket = pNewSocket; }

    protected:
        void run()
        {
            if ( pSocket != nullptr )
            {
                while ( bRun )
                {
                    pSocket->OnDataReceived();
                }
            }
        }

        CSocket* pSocket;
        bool     bRun;
    };

    void Init()
    {
        Socket.moveToThread ( &NetworkWorkerThread );

        NetworkWorkerThread.SetSocket ( &Socket );

        QObject::connect ( &Socket, &CSocket::InvalidPacketReceived, this, &CHighPrioSocket::InvalidPacketReceived );
    }

    CSocketThread NetworkWorkerThread;
    CSocket       Socket;

signals:
    void InvalidPacketReceived ( CHostAddress RecHostAddr );
};

class SocketNetworkAdapter : public INetworkSocket
{
public:
    explicit SocketNetworkAdapter ( CHighPrioSocket* s ) : socket ( s ) {}

    bool bind ( const NetEndpoint& ) override { return true; }

    bool joinMulticast ( const NetEndpoint& ) override { return false; }

    bool sendTo ( const NetEndpoint& remote, const void* data, std::size_t size ) override
    {
        if ( socket == nullptr || data == nullptr || size == 0 )
            return false;

        CVector<uint8_t> buf;
        buf.Init ( static_cast<int> ( size ) );
        std::memcpy ( &buf[0], data, size );

#if defined( JAMULUS_USE_JUCE_NET )
        CHostAddress hostAddr ( remote.address, static_cast<uint16_t> ( remote.port ) );
#else
        QString      addrStr = QString::fromStdString ( remote.address );
        CHostAddress hostAddr ( QHostAddress ( addrStr ), static_cast<quint16> ( remote.port ) );
#endif

        socket->SendPacket ( buf, hostAddr );
        return true;
    }

    int recvFrom ( NetEndpoint&, void*, std::size_t ) override { return 0; }

private:
    CHighPrioSocket* socket;
};

typedef union
{
    struct sockaddr     sa;
    struct sockaddr_in  sa4;
    struct sockaddr_in6 sa6;
} uSockAddr;
