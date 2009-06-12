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

#include "server.h"


/* Implementation *************************************************************/
CServer::CServer ( const QString& strLoggingFileName,
                   const quint16 iPortNumber,
                   const QString& strHTMLStatusFileName,
                   const QString& strHistoryFileName,
                   const QString& strServerNameForHTMLStatusFile,
                   const int iNewUploadRateLimitKbps ) :
    Socket ( &ChannelSet, this, iPortNumber ),
    ChannelSet ( iNewUploadRateLimitKbps )
{
    vecsSendData.Init ( MIN_SERVER_BLOCK_SIZE_SAMPLES );

    // init moving average buffer for response time evaluation
    CycleTimeVariance.Init (
        MIN_SERVER_BLOCK_SIZE_SAMPLES, TIME_MOV_AV_RESPONSE );

    // connect timer timeout signal
    QObject::connect ( &Timer, SIGNAL ( timeout() ),
        this, SLOT ( OnTimer() ) );

    // connection for protocol
    QObject::connect ( &ChannelSet,
        SIGNAL ( MessReadyForSending ( int, CVector<uint8_t> ) ),
        this, SLOT ( OnSendProtMessage ( int, CVector<uint8_t> ) ) );

    // connection for logging
    QObject::connect ( &ChannelSet,
        SIGNAL ( ChannelConnected ( CHostAddress ) ),
        this, SLOT ( OnNewChannel ( CHostAddress ) ) );

    // enable logging (if requested)
    if ( !strLoggingFileName.isEmpty() )
    {
        Logging.Start ( strLoggingFileName );
    }

    // enable history graph (if requested)
    if ( !strHistoryFileName.isEmpty() )
    {
        Logging.EnableHistory ( strHistoryFileName );
    }

    // HTML status file writing
    if ( !strHTMLStatusFileName.isEmpty() )
    {
        QString strCurServerNameForHTMLStatusFile = strServerNameForHTMLStatusFile;

        // if server name is empty, substitude a default name
        if ( strCurServerNameForHTMLStatusFile.isEmpty() )
        {
            strCurServerNameForHTMLStatusFile = "[server address]";
        }

        // (the static cast to integer of the port number is required so that it
        // works correctly under Linux)
        ChannelSet.StartStatusHTMLFileWriting ( strHTMLStatusFileName,
            strCurServerNameForHTMLStatusFile + ":" +
            QString().number( static_cast<int> ( iPortNumber ) ) );
    }
}

void CServer::OnSendProtMessage ( int iChID, CVector<uint8_t> vecMessage )
{
    // the protocol queries me to call the function to send the message
    // send it through the network
    Socket.SendPacket ( vecMessage, ChannelSet.GetAddress ( iChID ) );
}

void CServer::Start()
{
    if ( !IsRunning() )
    {
        // start main timer
        Timer.start ( MIN_SERVER_BLOCK_DURATION_MS );

        // init time for response time evaluation
        CycleTimeVariance.Reset();
    }
}

void CServer::Stop()
{
    // stop main timer
    Timer.stop();

    // logging
    Logging.AddServerStopped();
}

void CServer::OnNewChannel ( CHostAddress ChanAddr )
{
    // logging of new connected channel
    Logging.AddNewConnection ( ChanAddr.InetAddr );
}

void CServer::OnTimer()
{
    CVector<int>              vecChanID;
    CVector<CVector<double> > vecvecdData ( MIN_SERVER_BLOCK_SIZE_SAMPLES );
    CVector<CVector<double> > vecvecdGains;

    // get data from all connected clients
    ChannelSet.GetBlockAllConC ( vecChanID, vecvecdData, vecvecdGains );
    const int iNumClients = vecChanID.Size();

    // Check if at least one client is connected. If not, stop server until
    // one client is connected
    if ( iNumClients != 0 )
    {
        for ( int i = 0; i < iNumClients; i++ )
        {
            // generate a sparate mix for each channel
            // actual processing of audio data -> mix
            vecsSendData = ProcessData ( vecvecdData, vecvecdGains[i] );

            // send separate mix to current clients
            Socket.SendPacket (
                ChannelSet.PrepSendPacket ( vecChanID[i], vecsSendData ),
                ChannelSet.GetAddress ( vecChanID[i] ) );
        }
    }
    else
    {
        // Disable server if no clients are connected. In this case the server
        // does not consume any significant CPU when no client is connected.
        Stop();
    }

    // update response time measurement
    CycleTimeVariance.Update();
}

CVector<short> CServer::ProcessData ( CVector<CVector<double> >& vecvecdData,
                                      CVector<double>&           vecdGains )
{
    int i;

    // init return vector with zeros since we mix all channels on that vector
    CVector<short> vecsOutData ( MIN_SERVER_BLOCK_SIZE_SAMPLES, 0 );

    const int iNumClients = vecvecdData.Size();

    // mix all audio data from all clients together
    for ( int j = 0; j < iNumClients; j++ )
    {
        // if channel gain is 1, avoid multiplication for speed optimization
        if ( vecdGains[j] == static_cast<double> ( 1.0 ) )
        {
            for ( int i = 0; i < MIN_SERVER_BLOCK_SIZE_SAMPLES; i++ )
            {
                vecsOutData[i] =
                    Double2Short ( vecsOutData[i] + vecvecdData[j][i] );
            }
        }
        else
        {
            for ( int i = 0; i < MIN_SERVER_BLOCK_SIZE_SAMPLES; i++ )
            {
                vecsOutData[i] =
                    Double2Short ( vecsOutData[i] +
                    vecvecdData[j][i] * vecdGains[j] );
            }
        }
    }

    return vecsOutData;
}

bool CServer::GetTimingStdDev ( double& dCurTiStdDev )
{
    dCurTiStdDev = 0.0; // init return value

    // only return value if server is active and the actual measurement is
    // updated
    if ( IsRunning() )
    {
        // return the standard deviation
        dCurTiStdDev = CycleTimeVariance.GetStdDev();

        return true;
    }
    else
    {
        return false;
    }
}

void CServer::customEvent ( QEvent* Event )
{
    if ( Event->type() == QEvent::User + 11 )
    {
        const int iMessType = ( ( CLlconEvent* ) Event )->iMessType;

        switch ( iMessType )
        {
        case MS_PACKET_RECEIVED:
            // wake up the server if a packet was received
            // if the server is still running, the call to Start() will have
            // no effect
            Start();
            break;
        }
    }
}
