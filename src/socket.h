/******************************************************************************\
 * Copyright (c) 2004-2006
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

#if !defined(SOCKET_HOIHGE76GEKJH98_3_4344_BB23945IUHF1912__INCLUDED_)
#define SOCKET_HOIHGE76GEKJH98_3_4344_BB23945IUHF1912__INCLUDED_

#include <vector>
#include <qobject.h>
#include <qmessagebox.h>
#include <qsocket.h>
#include <qsocketdevice.h>
#include <qsocketnotifier.h>
#include "global.h"
#include "channel.h"
#include "util.h"


/* Definitions ****************************************************************/
/* maximum block size for network input buffer. Consider two bytes per sample */
#define MAX_SIZE_BYTES_NETW_BUF         ( MAX_NET_BLOCK_SIZE_FACTOR * MIN_BLOCK_SIZE_SAMPLES * 2 )


/* Classes ********************************************************************/
class CSocket : public QObject
{
    Q_OBJECT

public:
    CSocket::CSocket(CChannel* pNewChannel) : pChannel(pNewChannel),
        SocketDevice(QSocketDevice::Datagram /* UDP */), bIsClient(true)
        {Init();}
    CSocket::CSocket(CChannelSet* pNewChannelSet, QObject* pNServP) :
        pChannelSet(pNewChannelSet), pServer ( pNServP ),
        SocketDevice(QSocketDevice::Datagram /* UDP */), bIsClient(false)
        {Init();}
    virtual ~CSocket() {}

    void SendPacket ( const CVector<unsigned char>& vecbySendBuf,
                      const CHostAddress& HostAddr );

protected:
    void Init();

    QSocketDevice           SocketDevice;

    CVector<unsigned char>  vecbyRecBuf;
    CHostAddress            RecHostAddr;

    CChannel*               pChannel; /* for client */
    CChannelSet*            pChannelSet; /* for server */

    QObject*                pServer;
    bool                    bIsClient;

public slots:
    void OnDataReceived();
};


#endif /* !defined(SOCKET_HOIHGE76GEKJH98_3_4344_BB23945IUHF1912__INCLUDED_) */
