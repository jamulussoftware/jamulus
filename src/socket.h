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
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
\******************************************************************************/

#pragma once

#include <QObject>
#include <QMessageBox>
#include <QThread>
#include <QMutex>
#include <vector>
#include "global.h"
#include "protocol.h"
#include "util.h"
#ifndef _WIN32
# include <netinet/in.h>
# include <sys/socket.h>
#endif


// The header files channel.h and server.h require to include this header file
// so we get a cyclic dependency. To solve this issue, a prototype of the
// channel class and server class is defined here.
class CServer;  // forward declaration of CServer
class CChannel; // forward declaration of CChannel


/* Definitions ****************************************************************/
// number of ports we try to bind until we give up
#define NUM_SOCKET_PORTS_TO_TRY         50


/* Classes ********************************************************************/
/* Base socket class -------------------------------------------------------- */
class CSocket : public QObject
{
    Q_OBJECT

public:
    CSocket ( CChannel*     pNewChannel,
              const quint16 iPortNumber )
        : pChannel ( pNewChannel ),
          bIsClient ( true ),
          bJitterBufferOK ( true ) { Init ( iPortNumber ); }

    CSocket ( CServer*      pNServP,
              const quint16 iPortNumber )
        : pServer ( pNServP ),
          bIsClient ( false ),
          bJitterBufferOK ( true ) { Init ( iPortNumber ); }

    virtual ~CSocket();

    void SendPacket ( const CVector<uint8_t>& vecbySendBuf,
                      const CHostAddress&     HostAddr );

    bool GetAndResetbJitterBufferOKFlag();
    void Close();

protected:
    void Init ( const quint16 iPortNumber );

#ifdef _WIN32
    SOCKET           UdpSocket;
#else
    int              UdpSocket;
#endif

    QMutex           Mutex;

    CVector<uint8_t> vecbyRecBuf;
    CHostAddress     RecHostAddr;
    QHostAddress     SenderAddress;
    quint16          SenderPort;

    CChannel*        pChannel; // for client
    CServer*         pServer;  // for server

    bool             bIsClient;

    bool             bJitterBufferOK;

public slots:
    void OnDataReceived();

signals:
    void NewConnection(); // for the client

    void NewConnection ( int          iChID,
                         CHostAddress RecHostAddr ); // for the server

    void ServerFull ( CHostAddress RecHostAddr );

    void InvalidPacketReceived ( CHostAddress RecHostAddr );

    void ProtcolMessageReceived ( int              iRecCounter,
                                  int              iRecID,
                                  CVector<uint8_t> vecbyMesBodyData,
                                  CHostAddress     HostAdr );

    void ProtcolCLMessageReceived ( int              iRecID,
                                    CVector<uint8_t> vecbyMesBodyData,
                                    CHostAddress     HostAdr );
};


/* Socket which runs in a separate high priority thread --------------------- */
// The receive socket should be put in a high priority thread to ensure the GUI
// does not effect the stability of the audio stream (e.g. if the GUI is on
// high load because of a table update, the incoming network packets must still
// be put in the jitter buffer with highest priority).
class CHighPrioSocket : public QObject
{
    Q_OBJECT

public:
    CHighPrioSocket ( CChannel*     pNewChannel,
                      const quint16 iPortNumber )
        : Socket ( pNewChannel, iPortNumber ) { Init(); }

    CHighPrioSocket ( CServer*      pNewServer,
                      const quint16 iPortNumber )
        : Socket ( pNewServer, iPortNumber ) { Init(); }

    virtual ~CHighPrioSocket()
    {
        NetworkWorkerThread.Stop();
    }

    void Start()
    {
        // starts the high priority socket receive thread (with using blocking
        // socket request call)
        NetworkWorkerThread.start ( QThread::TimeCriticalPriority );
    }

    void SendPacket ( const CVector<uint8_t>& vecbySendBuf,
                      const CHostAddress&     HostAddr )
    {
        Socket.SendPacket ( vecbySendBuf, HostAddr );
    }

    bool GetAndResetbJitterBufferOKFlag()
    {
        return Socket.GetAndResetbJitterBufferOKFlag();
    }

protected:
    class CSocketThread : public QThread
    {
    public:
        CSocketThread ( CSocket* pNewSocket = nullptr, QObject* parent = nullptr ) :
          QThread ( parent ), pSocket ( pNewSocket ), bRun ( true ) {}

        void Stop()
        {
            // disable run flag so that the thread loop can be exit
            bRun = false;

            // to leave blocking wait for receive
            pSocket->Close();

            // give thread some time to terminate
            wait ( 5000 );
        }

        void SetSocket ( CSocket* pNewSocket ) { pSocket = pNewSocket; }

    protected:
        void run() {
            // make sure the socket pointer is initialized (should be always the
            // case)
            if ( pSocket != nullptr )
            {
                while ( bRun )
                {
                    // this function is a blocking function (waiting for network
                    // packets to be received and processed)
                    pSocket->OnDataReceived();
                }
            }
        }

        CSocket* pSocket;
        bool     bRun;
    };

    void Init()
    {
        // Creation of the new socket thread which has to have the highest
        // possible thread priority to make sure the jitter buffer is reliably
        // filled with the network audio packets and does not get interrupted
        // by other GUI threads. The following code is based on:
        // http://qt-project.org/wiki/Threads_Events_QObjects
        Socket.moveToThread ( &NetworkWorkerThread );

        NetworkWorkerThread.SetSocket ( &Socket );

        // connect the "InvalidPacketReceived" signal
        QObject::connect ( &Socket,
            SIGNAL ( InvalidPacketReceived ( CHostAddress ) ),
            SIGNAL ( InvalidPacketReceived ( CHostAddress ) ) );
    }

    CSocketThread NetworkWorkerThread;
    CSocket       Socket;

signals:
    void InvalidPacketReceived ( CHostAddress RecHostAddr );
};
