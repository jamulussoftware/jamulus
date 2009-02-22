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
                   const QString& strServerNameForHTMLStatusFile,
                   const bool bForceLowUploadRate ) :
    Socket ( &ChannelSet, this, iPortNumber ),
    ChannelSet ( bForceLowUploadRate )
{
    vecsSendData.Init ( MIN_BLOCK_SIZE_SAMPLES );

    // init moving average buffer for response time evaluation
    RespTimeMoAvBuf.Init ( LEN_MOV_AV_RESPONSE );

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

#ifdef _WIN32
    // event handling of custom events seems not to work under Windows in this
    // class, do not use automatic start/stop of server in Windows version
    Start();
#endif

    // enable logging (if requested)
    if ( !strLoggingFileName.isEmpty() )
    {
        Logging.Start ( strLoggingFileName );
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

// convert unsigned uint8_t in char, TODO convert all buffers in uint8_t
CVector<unsigned char> vecbyDataConv ( vecMessage.Size() );
for ( int i = 0; i < vecMessage.Size (); i++ ) {
    vecbyDataConv[i] = static_cast<unsigned char> ( vecMessage[i] );
}

    // the protocol queries me to call the function to send the message
    // send it through the network
    Socket.SendPacket ( vecbyDataConv, ChannelSet.GetAddress ( iChID ) );
}

void CServer::Start()
{
    if ( !IsRunning() )
    {
        // start main timer
        Timer.start ( MIN_BLOCK_DURATION_MS );

        // init time for response time evaluation
        TimeLastBlock = PreciseTime.elapsed();
        RespTimeMoAvBuf.Reset();
    }
}

void CServer::Stop()
{
    // stop main timer
    Timer.stop();

    // logging
    const QString strLogStr = CLogTimeDate::toString() + ": server stopped "
        "############################################";

    qDebug() << strLogStr; // on console
    Logging << strLogStr; // in log file
}

void CServer::OnNewChannel ( CHostAddress ChanAddr )
{
    // logging of new connected channel
    const QString strLogStr = CLogTimeDate::toString() + ": " +
        ChanAddr.InetAddr.toString() + " connected";

    qDebug() << strLogStr; // on console
    Logging << strLogStr; // in log file
}

void CServer::OnTimer()
{
    CVector<int>              vecChanID;
    CVector<CVector<double> > vecvecdData ( MIN_BLOCK_SIZE_SAMPLES );
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
#ifndef _WIN32
        // event handling of custom events seems not to work under Windows in this
        // class, do not use automatic start/stop of server in Windows version
        Stop();
#endif
    }

    // update response time measurement ----------------------------------------
    // add time difference
    const int CurTime = PreciseTime.elapsed();

    // we want to calculate the standard deviation (we assume that the mean
    // is correct at the block period time)
    const double dCurAddVal =
        ( (double) ( CurTime - TimeLastBlock ) - MIN_BLOCK_DURATION_MS );

    RespTimeMoAvBuf.Add ( dCurAddVal * dCurAddVal ); // add squared value

    // store old time value
    TimeLastBlock = CurTime;
}

CVector<short> CServer::ProcessData ( CVector<CVector<double> >& vecvecdData,
                                      CVector<double>&           vecdGains )
{
    CVector<short> vecsOutData ( MIN_BLOCK_SIZE_SAMPLES );

    const int iNumClients = vecvecdData.Size();

    // 3 dB offset to avoid overload if all clients are set to gain 1
    const double dNorm = (double) 2.0;

    // mix all audio data from all clients together
    for ( int i = 0; i < MIN_BLOCK_SIZE_SAMPLES; i++ )
    {
        double dMixedData = 0.0;

        for ( int j = 0; j < iNumClients; j++ )
        {
            dMixedData += vecvecdData[j][i] * vecdGains[j];
        }

        // normalization and truncating to short
        vecsOutData[i] = Double2Short ( dMixedData / dNorm );
    }

    return vecsOutData;
}

bool CServer::GetTimingStdDev ( double& dCurTiStdDev )
{
    dCurTiStdDev = 0.0; // init return value

    /* only return value if server is active and the actual measurement is
       updated */
    if ( IsRunning() )
    {
        /* we want to return the standard deviation, for that we need to calculate
           the sqaure root */
        dCurTiStdDev = sqrt ( RespTimeMoAvBuf.GetAverage() );

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
