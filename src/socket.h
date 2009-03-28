/******************************************************************************\
 * Copyright (c) 2004-2009
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
#include <vector>
#include "global.h"
#include "channel.h"
#include "util.h"


/* Definitions ****************************************************************/
// Maximum block size for network input buffer. Consider a maximum sample rate
// of 48 kHz and two audio channels and two bytes per sample.
#define MAX_SIZE_BYTES_NETW_BUF         ( MAX_MONO_AUD_BUFF_SIZE_AT_48KHZ * 4 )


/* Classes ********************************************************************/
class CSocket : public QObject
{
    Q_OBJECT

public:
    CSocket ( CChannel* pNewChannel, const quint16 iPortNumber ) :
        pChannel( pNewChannel ), bIsClient ( true ) { Init ( iPortNumber ); }
    CSocket ( CChannelSet* pNewChannelSet, QObject* pNServP, const quint16 iPortNumber ) :
        pChannelSet ( pNewChannelSet ), pServer ( pNServP ), bIsClient ( false )
        { Init ( iPortNumber ); }
    virtual ~CSocket() {}

    void SendPacket ( const CVector<uint8_t>& vecbySendBuf,
                      const CHostAddress& HostAddr );

protected:
    void Init ( const quint16 iPortNumber = LLCON_DEFAULT_PORT_NUMBER );

    QUdpSocket       SocketDevice;

    CVector<uint8_t> vecbyRecBuf;
    CHostAddress     RecHostAddr;

    CChannel*        pChannel;    // for client
    CChannelSet*     pChannelSet; // for server

    QObject*         pServer;
    bool             bIsClient;

public slots:
    void OnDataReceived();
};

#endif /* !defined ( SOCKET_HOIHGE76GEKJH98_3_4344_BB23945IUHF1912__INCLUDED_ ) */
