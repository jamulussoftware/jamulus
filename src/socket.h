/******************************************************************************\
 * Copyright (c) 2004-2013
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
#include "channel.h"
#include "protocol.h"
#include "util.h"

// The header file server.h requires to include this header file so we get a
// cyclic dependency. To solve this issue, a prototype of the server class is
// defined here.
class CServer; // forward declaration of CServer


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
        : pChannel( pNewChannel ), bIsClient ( true ) { Init ( iPortNumber ); }

    CSocket ( CServer*      pNServP,
              const quint16 iPortNumber )
        : pServer ( pNServP ), bIsClient ( false ) { Init ( iPortNumber ); }

    void SendPacket ( const CVector<uint8_t>& vecbySendBuf,
                      const CHostAddress&     HostAddr );

protected:
    void Init ( const quint16 iPortNumber = LLCON_DEFAULT_PORT_NUMBER );

    QUdpSocket       SocketDevice;
    QMutex           Mutex;

    CVector<uint8_t> vecbyRecBuf;
    CHostAddress     RecHostAddr;

    CChannel*        pChannel; // for client
    CServer*         pServer;  // for server

    bool             bIsClient;

public slots:
    void OnDataReceived();

signals:
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
                      const quint16 iPortNumber )
    {
        // we have to register some classes to the Qt signal/slot mechanism
        // since we have thread crossings with the threaded code
        qRegisterMetaType<CVector<uint8_t> > ( "CVector<uint8_t>" );
        qRegisterMetaType<CHostAddress> ( "CHostAddress" );

        // Creation of the new socket thread which has to have the highest
        // possible thread priority to make sure the jitter buffer is reliably
        // filled with the network audio packets and does not get interrupted
        // by other GUI threads. The following code is based on:
        // http://qt-project.org/wiki/Threads_Events_QObjects
        pSocket = new CSocket ( pNewChannel, iPortNumber );
        pSocket->moveToThread ( &NetworkWorkerThread );
        NetworkWorkerThread.start ( QThread::TimeCriticalPriority );

        // connect the "InvalidPacketReceived" signal
        QObject::connect ( pSocket,
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
        pSocket->SendPacket ( vecbySendBuf, HostAddr );
    }

protected:
    QThread  NetworkWorkerThread;
    CSocket* pSocket;

signals:
    void InvalidPacketReceived ( CVector<uint8_t> vecbyRecBuf,
                                 int              iNumBytesRead,
                                 CHostAddress     RecHostAddr );
};
#endif

#endif /* !defined ( SOCKET_HOIHGE76GEKJH98_3_4344_BB23945IUHF1912__INCLUDED_ ) */
