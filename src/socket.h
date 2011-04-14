/******************************************************************************\
 * Copyright (c) 2004-2011
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

#include <qobject.h>
#include <qmessagebox.h>
#include <qudpsocket.h>
#include <qsocketnotifier.h>
#include <qmutex.h>
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
class CSocket : public QObject
{
    Q_OBJECT

public:
    CSocket ( CChannel*     pNewChannel,
              CProtocol*    pNewCLProtocol,
              const quint16 iPortNumber )
        : pChannel( pNewChannel ), pConnLessProtocol ( pNewCLProtocol ),
          bIsClient ( true ) { Init ( iPortNumber ); }

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

    CChannel*        pChannel;          // for client
    CProtocol*       pConnLessProtocol; // for client
    CServer*         pServer;           // for server

    bool             bIsClient;

public slots:
    void OnDataReceived();
};

#endif /* !defined ( SOCKET_HOIHGE76GEKJH98_3_4344_BB23945IUHF1912__INCLUDED_ ) */
