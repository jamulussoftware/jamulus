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

#if !defined ( SOCKET_HOIHGE76GEKJH98_3_4344_BB23945IUHF1912__INCLUDED_ )
#define SOCKET_HOIHGE76GEKJH98_3_4344_BB23945IUHF1912__INCLUDED_

#include <QObject>
#include <QMessageBox>
#include <QUdpSocket>
#include <QSocketNotifier>
#include <QThread>
#include <QMutex>
#include <vector>
#include "global.h"
#include "protocol.h"
#include "util.h"
#ifdef ENABLE_RECEIVE_SOCKET_IN_SEPARATE_THREAD
# ifndef _WIN32
#  include <netinet/in.h>
#  include <sys/socket.h>
# endif
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
/* Base socket class ---------------------------------------------------------*/
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

#ifdef ENABLE_RECEIVE_SOCKET_IN_SEPARATE_THREAD
    void Close();

    void EmitDetectedCLMessage ( const CVector<uint8_t>& vecbyMesBodyData,
                                 const int               iRecID )
    {
        emit DetectedCLMessage ( vecbyMesBodyData, iRecID );
    }

    void EmitParseMessageBody ( const CVector<uint8_t>& vecbyMesBodyData,
                                const int               iRecCounter,
                                const int               iRecID )
    {
        emit ParseMessageBody ( vecbyMesBodyData, iRecCounter, iRecID );
    }
#endif

protected:
    void Init ( const quint16 iPortNumber = LLCON_DEFAULT_PORT_NUMBER );

#ifdef ENABLE_RECEIVE_SOCKET_IN_SEPARATE_THREAD
# ifdef _WIN32
    SOCKET           UdpSocket;
# else
    int              UdpSocket;
# endif
#endif
    QUdpSocket       SocketDevice;

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
#ifdef ENABLE_RECEIVE_SOCKET_IN_SEPARATE_THREAD
    void DetectedCLMessage ( CVector<uint8_t> vecbyMesBodyData,
                             int              iRecID );

    void ParseMessageBody ( CVector<uint8_t> vecbyMesBodyData,
                            int              iRecCounter,
                            int              iRecID );
#endif
    void InvalidPacketReceived ( CVector<uint8_t> vecbyRecBuf,
                                 int              iNumBytesRead,
                                 CHostAddress     RecHostAddr );
};


#ifdef ENABLE_RECEIVE_SOCKET_IN_SEPARATE_THREAD
/* Socket which runs in a separate high priority thread ----------------------*/
class CHighPrioSocket : public QObject
{
    Q_OBJECT

public:
    CHighPrioSocket ( CChannel*     pNewChannel,
                      const quint16 iPortNumber ) :
        Socket ( pNewChannel, iPortNumber )
    {
        // Creation of the new socket thread which has to have the highest
        // possible thread priority to make sure the jitter buffer is reliably
        // filled with the network audio packets and does not get interrupted
        // by other GUI threads. The following code is based on:
        // http://qt-project.org/wiki/Threads_Events_QObjects
        Socket.moveToThread ( &NetworkWorkerThread );

        NetworkWorkerThread.SetSocket ( &Socket );

        NetworkWorkerThread.start ( QThread::TimeCriticalPriority );

        // connect the "InvalidPacketReceived" signal
        QObject::connect ( &Socket,
            SIGNAL ( InvalidPacketReceived ( CVector<uint8_t>, int, CHostAddress ) ),
            SIGNAL ( InvalidPacketReceived ( CVector<uint8_t>, int, CHostAddress ) ) );
    }

    virtual ~CHighPrioSocket()
    {
        NetworkWorkerThread.exit();
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
        CSocketThread ( CSocket* pNewSocket = NULL, QObject* parent = 0 ) :
          QThread ( parent ), pSocket ( pNewSocket ), bRun ( true ) {}

        virtual ~CSocketThread()
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
            if ( pSocket != NULL )
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

    CSocketThread NetworkWorkerThread;
    CSocket       Socket;

signals:
    void InvalidPacketReceived ( CVector<uint8_t> vecbyRecBuf,
                                 int              iNumBytesRead,
                                 CHostAddress     RecHostAddr );
};
#endif

#endif /* !defined ( SOCKET_HOIHGE76GEKJH98_3_4344_BB23945IUHF1912__INCLUDED_ ) */
