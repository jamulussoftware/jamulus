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

#if !defined ( TESTBENCH_HOIHJH8_3_43445KJIUHF1912__INCLUDED_ )
#define TESTBENCH_HOIHJH8_3_43445KJIUHF1912__INCLUDED_

#include <qobject.h>
#include <qtimer.h>
#include <qdatetime.h>
#include <qhostaddress.h>
#include "global.h"
#include "socket.h"
#include "protocol.h"
#include "util.h"


/* Classes ********************************************************************/
class CTestbench : public QObject
{
    Q_OBJECT

public:
    CTestbench ( QString sNewAddress, quint16 iNewPort ) :
        sAddress ( sNewAddress ), iPort ( iNewPort )
    {
        // bind socket (try 100 port numbers)
        quint16 iPortIncrement = 0;     // start value: port nubmer plus ten
        bool    bSuccess       = false; // initialization for while loop
        while ( !bSuccess && ( iPortIncrement <= 100 ) )
        {
            bSuccess = UdpSocket.bind (
                QHostAddress( QHostAddress::Any ),
                22222 + iPortIncrement );

            iPortIncrement++;
        }

        // connect protocol signal
        QObject::connect ( &Protocol, SIGNAL ( MessReadyForSending ( CVector<uint8_t> ) ),
            this, SLOT ( OnSendProtMessage ( CVector<uint8_t> ) ) );

        // connect and start the timer (testbench heartbeat)
        QObject::connect ( &Timer, SIGNAL ( timeout() ),
            this, SLOT ( OnTimer() ) );

        Timer.start ( 1 ); // 1 ms
    }

protected:
    int GenRandomIntInRange ( const int iStart, const int iEnd ) const
    {
        return static_cast<int> ( iStart +
            ( ( static_cast<double> ( iEnd - iStart + 1 ) * rand() ) / RAND_MAX ) );
    }

    QString GenRandomString() const
    {
        const int iLen = GenRandomIntInRange ( 0, 111 );
        QString   strReturn = "";
        for ( int i = 0; i < iLen; i++ )
        {
            strReturn += static_cast<char> ( GenRandomIntInRange ( 0, 255 ) );
        }
        return strReturn;
    }

    QString    sAddress;
    quint16    iPort;        
    QTimer     Timer;
    CProtocol  Protocol;
    QUdpSocket UdpSocket;

public slots:
    void OnTimer()
    {
        CVector<CChannelShortInfo> vecChanInfo ( 1 );
        CNetworkTransportProps     NetTrProps;
        CServerCoreInfo            ServerInfo;
        CVector<CServerInfo>       vecServerInfo ( 1 );
        CHostAddress               CurHostAddress ( QHostAddress ( sAddress ), iPort );

        // generate random protocol message
        switch ( GenRandomIntInRange ( 0, 22 ) )
        {
        case 0:
            Protocol.CreateJitBufMes ( GenRandomIntInRange ( 0, 10 ) );
            break;

        case 1:
            Protocol.CreateReqJitBufMes();
            break;

        case 2:
            Protocol.CreateChanGainMes ( GenRandomIntInRange ( 0, 20 ),
                GenRandomIntInRange ( -100, 100 ) );
            break;

        case 3:
            vecChanInfo[0].iChanID = GenRandomIntInRange ( -2, 20 );
            vecChanInfo[0].iIpAddr = GenRandomIntInRange ( 0, 100000 );
            vecChanInfo[0].strName = GenRandomString();

            Protocol.CreateConClientListMes ( vecChanInfo );
            break;

        case 4:
            Protocol.CreateReqConnClientsList();
            break;

        case 5:
            Protocol.CreateChanNameMes ( GenRandomString() );
            break;

        case 6:
            Protocol.CreateReqChanNameMes();
            break;

        case 7:
            Protocol.CreateChatTextMes ( GenRandomString() );
            break;

        case 8:
            Protocol.CreatePingMes ( GenRandomIntInRange ( 0, 100000 ) );
            break;

        case 9:
            NetTrProps.eAudioCodingType =
                static_cast<EAudComprType> ( GenRandomIntInRange ( 0, 2 ) );

            NetTrProps.iAudioCodingArg        = GenRandomIntInRange ( -100, 100 );
            NetTrProps.iBaseNetworkPacketSize = GenRandomIntInRange ( -2, 1000 );
            NetTrProps.iBlockSizeFact         = GenRandomIntInRange ( -2, 100 );
            NetTrProps.iNumAudioChannels      = GenRandomIntInRange ( -2, 10 );
            NetTrProps.iSampleRate            = GenRandomIntInRange ( -2, 10000 );
            NetTrProps.iVersion               = GenRandomIntInRange ( -2, 10000 );

            Protocol.CreateNetwTranspPropsMes ( NetTrProps );
            break;

        case 10:
            Protocol.CreateReqNetwTranspPropsMes();
            break;

        case 11:
            Protocol.CreateCLPingMes ( CurHostAddress,
                                       GenRandomIntInRange ( -2, 1000 ) );
            break;

        case 12:
            Protocol.CreateCLPingWithNumClientsMes ( CurHostAddress,
                                                     GenRandomIntInRange ( -2, 1000 ),
                                                     GenRandomIntInRange ( -2, 1000 ) );
            break;

        case 13:
            Protocol.CreateCLServerFullMes ( CurHostAddress );
            break;

        case 14:
            ServerInfo.bPermanentOnline =
                static_cast<bool> ( GenRandomIntInRange ( 0, 1 ) );

            ServerInfo.eCountry =
                static_cast<QLocale::Country> ( GenRandomIntInRange ( 0, 100 ) );

            ServerInfo.iLocalPortNumber = GenRandomIntInRange ( -2, 10000 );
            ServerInfo.iMaxNumClients   = GenRandomIntInRange ( -2, 10000 );
            ServerInfo.strCity          = GenRandomString();
            ServerInfo.strName          = GenRandomString();
            ServerInfo.strTopic         = GenRandomString();

            Protocol.CreateCLRegisterServerMes ( CurHostAddress,
                                                 ServerInfo );
            break;

        case 15:
            Protocol.CreateCLUnregisterServerMes ( CurHostAddress );
            break;

        case 16:
            vecServerInfo[0].bPermanentOnline =
                static_cast<bool> ( GenRandomIntInRange ( 0, 1 ) );

            vecServerInfo[0].eCountry =
                static_cast<QLocale::Country> ( GenRandomIntInRange ( 0, 100 ) );

            vecServerInfo[0].HostAddr         = CurHostAddress;
            vecServerInfo[0].iLocalPortNumber = GenRandomIntInRange ( -2, 10000 );
            vecServerInfo[0].iMaxNumClients   = GenRandomIntInRange ( -2, 10000 );
            vecServerInfo[0].strCity          = GenRandomString();
            vecServerInfo[0].strName          = GenRandomString();
            vecServerInfo[0].strTopic         = GenRandomString();

            Protocol.CreateCLServerListMes ( CurHostAddress,
                                             vecServerInfo );
            break;

        case 17:
            Protocol.CreateCLReqServerListMes ( CurHostAddress );
            break;

        case 18:
            Protocol.CreateCLSendEmptyMesMes ( CurHostAddress,
                                               CurHostAddress );
            break;

        case 19:
            Protocol.CreateCLEmptyMes ( CurHostAddress );
            break;

        case 20:
            Protocol.CreateAndImmSendAcknMess ( GenRandomIntInRange ( -10, 100 ),
                GenRandomIntInRange ( -100, 100 ) );
            break;

        case 21:
            Protocol.CreateAndImmSendDisconnectionMes();
            break;

        case 22:
            // arbitrary "audio" packet (with random sizes)
            CVector<uint8_t> vecMessage ( GenRandomIntInRange ( 1, 1000 ) );
            OnSendProtMessage ( vecMessage );
            break;
        }
    }

    void OnSendProtMessage ( CVector<uint8_t> vecMessage )
    {
        UdpSocket.writeDatagram (
            (const char*) &( (CVector<uint8_t>) vecMessage )[0],
            vecMessage.Size(), QHostAddress ( sAddress ), iPort );

        // reset protocol so that we do not have to wait for an acknowledge to
        // send the next message
        Protocol.Reset();
    }
};

#endif /* !defined ( TESTBENCH_HOIHJH8_3_43445KJIUHF1912__INCLUDED_ ) */
