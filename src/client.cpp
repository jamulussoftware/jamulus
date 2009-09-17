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

#include "client.h"


/* Implementation *************************************************************/
CClient::CClient ( const quint16 iPortNumber ) :
    Channel ( false ), /* we need a client channel -> "false" */
    Sound ( AudioCallback, this ),
    Socket ( &Channel, iPortNumber ),
    iAudioInFader ( AUD_FADER_IN_MIDDLE ),
    iReverbLevel ( 0 ),
    bReverbOnLeftChan ( false ),
    vstrIPAddress ( MAX_NUM_SERVER_ADDR_ITEMS, "" ), strName ( "" ),
    bOpenChatOnNewMessage ( true ),
    bDoAutoSockBufSize ( true ),
    iSndCrdPrefFrameSizeFactor ( FRAME_SIZE_FACTOR_DEFAULT ),
    iSndCrdFrameSizeFactor ( FRAME_SIZE_FACTOR_DEFAULT ),
    iCeltNumCodedBytes ( CELT_NUM_BYTES_NORMAL_QUALITY ),
    bCeltDoHighQuality ( false )
{
    // init audio endocder/decoder (mono)
    CeltMode = celt_mode_create (
        SYSTEM_SAMPLE_RATE, 1, SYSTEM_FRAME_SIZE_SAMPLES, NULL );

    CeltEncoder = celt_encoder_create ( CeltMode );
    CeltDecoder = celt_decoder_create ( CeltMode );

#ifdef USE_LOW_COMPLEXITY_CELT_ENC
    // set encoder low complexity
    celt_encoder_ctl(CeltEncoder,
        CELT_SET_COMPLEXITY_REQUEST, celt_int32_t ( 1 ) );
#endif


    // connections -------------------------------------------------------------
    // connection for protocol
    QObject::connect ( &Channel,
        SIGNAL ( MessReadyForSending ( CVector<uint8_t> ) ),
        this, SLOT ( OnSendProtMessage ( CVector<uint8_t> ) ) );

    QObject::connect ( &Channel, SIGNAL ( ReqJittBufSize() ),
        this, SLOT ( OnReqJittBufSize() ) );

    QObject::connect ( &Channel,
        SIGNAL ( ConClientListMesReceived ( CVector<CChannelShortInfo> ) ),
        SIGNAL ( ConClientListMesReceived ( CVector<CChannelShortInfo> ) ) );

    QObject::connect ( &Channel,
        SIGNAL ( Disconnected() ),
        SIGNAL ( Disconnected() ) );

    QObject::connect ( &Channel, SIGNAL ( NewConnection() ),
        this, SLOT ( OnNewConnection() ) );

    QObject::connect ( &Channel, SIGNAL ( ChatTextReceived ( QString ) ),
        this, SIGNAL ( ChatTextReceived ( QString ) ) );

    QObject::connect ( &Channel, SIGNAL ( PingReceived ( int ) ),
        this, SLOT ( OnReceivePingMessage ( int ) ) );

    QObject::connect ( &Sound, SIGNAL ( ReinitRequest() ),
        this, SLOT ( OnSndCrdReinitRequest() ) );
}

void CClient::OnSendProtMessage ( CVector<uint8_t> vecMessage )
{
    // the protocol queries me to call the function to send the message
    // send it through the network
    Socket.SendPacket ( vecMessage, Channel.GetAddress() );
}

void CClient::OnReqJittBufSize()
{
// TODO cant we implement this OnReqJjittBufSize inside the channel object?
    Channel.CreateJitBufMes ( Channel.GetSockBufNumFrames() );
}

void CClient::OnNewConnection()
{
    // a new connection was successfully initiated, send name and request
    // connected clients list
    Channel.SetRemoteName ( strName );

    // We have to send a connected clients list request since it can happen
    // that we just had connected to the server and then disconnected but
    // the server still thinks that we are connected (the server is still
    // waiting for the channel time-out). If we now connect again, we would
    // not get the list because the server does not know about a new connection.
    // Same problem is with the jitter buffer message.
    Channel.CreateReqConnClientsList();
    Channel.CreateJitBufMes ( Channel.GetSockBufNumFrames() );
}

void CClient::OnReceivePingMessage ( int iMs )
{
    // calculate difference between received time in ms and current time in ms,
    // take care of wrap arounds (if wrapping, do not use result)
    const int iCurDiff = PreciseTime.elapsed() - iMs;
    if ( iCurDiff >= 0 )
    {
        emit PingTimeReceived ( iCurDiff );
    }
}

bool CClient::SetServerAddr ( QString strNAddr )
{
    QHostAddress InetAddr;
    quint16      iNetPort = LLCON_DEFAULT_PORT_NUMBER;

    // parse input address for the type [IP address]:[port number]
    QString strPort = strNAddr.section ( ":", 1, 1 );
    if ( !strPort.isEmpty() )
    {
        // a colon is present in the address string, try to extract port number
        iNetPort = strPort.toInt();

        // extract address port before colon (should be actual internet address)
        strNAddr = strNAddr.section ( ":", 0, 0 );
    }

    // first try if this is an IP number an can directly applied to QHostAddress
    if ( !InetAddr.setAddress ( strNAddr ) )
    {
        // it was no vaild IP address, try to get host by name, assuming
        // that the string contains a valid host name string
        QHostInfo HostInfo = QHostInfo::fromName ( strNAddr );

        if ( HostInfo.error() == QHostInfo::NoError )
        {
            // apply IP address to QT object
             if ( !HostInfo.addresses().isEmpty() )
             {
                 // use the first IP address
                 InetAddr = HostInfo.addresses().first();
             }
        }
        else
        {
            return false; // invalid address
        }
    }

    // apply address (the server port is fixed and always the same)
    Channel.SetAddress ( CHostAddress ( InetAddr, iNetPort ) );

    return true;
}

void CClient::SetSndCrdPrefFrameSizeFactor ( const int iNewFactor )
{
    // first check new input parameter
    if ( ( iNewFactor == FRAME_SIZE_FACTOR_PREFERRED ) ||
         ( iNewFactor == FRAME_SIZE_FACTOR_DEFAULT ) ||
         ( iNewFactor == FRAME_SIZE_FACTOR_SAFE ) )
    {
        // init with new parameter, if client was running then first
        // stop it and restart again after new initialization
        const bool bWasRunning = Sound.IsRunning();
        if ( bWasRunning )
        {
            Sound.Stop();
        }

        // set new parameter
        iSndCrdPrefFrameSizeFactor = iNewFactor;

        // init with new block size index parameter
        const bool bInitWasOk = Init();

        if ( bWasRunning )
        {
            if ( bInitWasOk )
            {
                // init was ok, restart client
                Sound.Start();
            }
            else
            {
                // init was not successful, do not restart client and
                // inform main window of the stopped client
                Stop();
                emit Stopped();
            }
        }
    }
}

void CClient::SetCELTHighQuality ( const bool bNCeltHighQualityFlag )
{
    // init with new parameter, if client was running then first
    // stop it and restart again after new initialization
    const bool bWasRunning = Sound.IsRunning();
    if ( bWasRunning )
    {
        Sound.Stop();
    }

    // set new parameter
    bCeltDoHighQuality = bNCeltHighQualityFlag;

    // init with new block size index parameter
    Init();

    if ( bWasRunning )
    {
        Sound.Start();
    }
}

QString CClient::SetSndCrdDev ( const int iNewDev )
{
    // if client was running then first
    // stop it and restart again after new initialization
    const bool bWasRunning = Sound.IsRunning();
    if ( bWasRunning )
    {
        Sound.Stop();
    }

    const QString strReturn = Sound.SetDev ( iNewDev ).c_str();

    // init again because the sound card actual buffer size might
    // be changed on new device
    const bool bInitWasOk = Init();

    if ( bWasRunning )
    {
        if ( bInitWasOk )
        {
            // init was ok, restart client
            Sound.Start();
        }
        else
        {
            // init was not successful, do not restart client and
            // inform main window of the stopped client
            Stop();
            emit Stopped();
        }
    }

    return strReturn;
}

void CClient::OnSndCrdReinitRequest()
{
    // if client was running then first
    // stop it and restart again after new initialization
    const bool bWasRunning = Sound.IsRunning();
    if ( bWasRunning )
    {
        Sound.Stop();
    }

    // reinit the driver (we use the currently selected driver) and
    // init client object, too
    Sound.SetDev ( Sound.GetDev() );
    const bool bInitWasOk = Init();

    if ( bWasRunning )
    {
        if ( bInitWasOk )
        {
            // init was ok, restart client
            Sound.Start();
        }
        else
        {
            // init was not successful, do not restart client and
            // inform main window of the stopped client
            Stop();
            emit Stopped();
        }
    }
}

void CClient::Start()
{
    // init object
    if ( !Init() )
    {
        const QString strError = "The current sound card frame size of <b>" +
            QString().setNum ( iMonoBlockSizeSam ) + " samples</b> is not supported "
            "by this software. Please open your "
#ifdef _WIN32
            "ASIO "
#else
            "JACK "
#endif
            "configuration panel and use one of the following frame sizes: <b>" +
            QString().setNum ( SYSTEM_FRAME_SIZE_SAMPLES * FRAME_SIZE_FACTOR_PREFERRED ) + ", " +
            QString().setNum ( SYSTEM_FRAME_SIZE_SAMPLES * FRAME_SIZE_FACTOR_DEFAULT ) + ", or " +
            QString().setNum ( SYSTEM_FRAME_SIZE_SAMPLES * FRAME_SIZE_FACTOR_SAFE ) +
            " samples</b>."
#ifdef _WIN32
            "<br><br>To open the ASIO configuration panel, you can open the Settings "
            "Dialog in the llcon software (using the menu) and click on the \"ASIO "
            "Setup\" button."
#endif
            ;

        throw CGenErr ( strError );
    }

    // enable channel
    Channel.SetEnable ( true );

    // start audio interface
    Sound.Start();
}

void CClient::Stop()
{
    // stop audio interface
    Sound.Stop();

/*
// TEST (seems not to work, disconnect is still not working reliably)
QTime dieTime = QTime::currentTime().addMSecs(500);
while( QTime::currentTime() < dieTime ) {
	QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
}
*/

    // Send disconnect message to server (Since we disable our protocol
    // receive mechanism with the next command, we do not evaluate any
    // respond from the server, therefore we just hope that the message
    // gets its way to the server, if not, the old behaviour time-out
    // disconnects the connection anyway. Send the message three times
    // to increase the probability that at least one message makes it
    // through).
    Channel.CreateAndImmSendDisconnectionMes();
    Channel.CreateAndImmSendDisconnectionMes();
    Channel.CreateAndImmSendDisconnectionMes();

    // disable channel
    Channel.SetEnable ( false );

    // reset current signal level and LEDs
    SignalLevelMeter.Reset();
    PostWinMessage ( MS_RESET_ALL, 0 );
}

void CClient::AudioCallback ( CVector<int16_t>& psData, void* arg )
{
    // get the pointer to the object
    CClient* pMyClientObj = reinterpret_cast<CClient*> ( arg ); 

    // process audio data
    pMyClientObj->ProcessAudioData ( psData );
}

bool CClient::Init()
{
    // translate block size index in actual block size
    const int iPrefMonoFrameSize =
        iSndCrdPrefFrameSizeFactor * SYSTEM_FRAME_SIZE_SAMPLES;

    // get actual sound card buffer size using preferred size
    iMonoBlockSizeSam   = Sound.Init ( iPrefMonoFrameSize );
    iStereoBlockSizeSam = 2 * iMonoBlockSizeSam;


// TEST (we assume here that "iMonoBlockSizeSam" is divisible by
// "SYSTEM_FRAME_SIZE_SAMPLES")
// calculate actual frame size factor
iSndCrdFrameSizeFactor = max ( 1, iMonoBlockSizeSam / SYSTEM_FRAME_SIZE_SAMPLES );


    vecsAudioSndCrdMono.Init   ( iMonoBlockSizeSam );
    vecsAudioSndCrdStereo.Init ( iStereoBlockSizeSam );
    vecdAudioStereo.Init       ( iStereoBlockSizeSam );

    // init response time evaluation
    CycleTimeVariance.Init ( iMonoBlockSizeSam,
        SYSTEM_SAMPLE_RATE, TIME_MOV_AV_RESPONSE );

    CycleTimeVariance.Reset();

    // init reverberation
    AudioReverb.Init ( SYSTEM_SAMPLE_RATE );

    // inits for CELT coding
    if ( bCeltDoHighQuality )
    {
        iCeltNumCodedBytes = CELT_NUM_BYTES_HIGH_QUALITY;
    }
    else
    {
        iCeltNumCodedBytes = CELT_NUM_BYTES_NORMAL_QUALITY;
    }
    vecCeltData.Init ( iCeltNumCodedBytes );

    // init network buffers
    vecsNetwork.Init   ( iMonoBlockSizeSam );
    vecbyNetwData.Init ( iCeltNumCodedBytes );

    // set the channel network properties
    Channel.SetNetwFrameSizeAndFact ( iCeltNumCodedBytes,
                                      iSndCrdFrameSizeFactor );

    // check sound card buffer sizes, if not supported, return error flag
    if ( ( iMonoBlockSizeSam != ( SYSTEM_FRAME_SIZE_SAMPLES * FRAME_SIZE_FACTOR_PREFERRED ) ) &&
         ( iMonoBlockSizeSam != ( SYSTEM_FRAME_SIZE_SAMPLES * FRAME_SIZE_FACTOR_DEFAULT ) ) &&
         ( iMonoBlockSizeSam != ( SYSTEM_FRAME_SIZE_SAMPLES * FRAME_SIZE_FACTOR_SAFE ) ) )
    {
        return false; // init was not successful
    }
    else
    {
        return true; // ok
    }
}

void CClient::ProcessAudioData ( CVector<int16_t>& vecsStereoSndCrd )
{
    int i, j;

    // Transmit signal ---------------------------------------------------------
    // update stereo signal level meter
    SignalLevelMeter.Update ( vecsStereoSndCrd );

    // convert data from short to double
    for ( i = 0; i < iStereoBlockSizeSam; i++ )
    {
        vecdAudioStereo[i] = static_cast<double> ( vecsStereoSndCrd[i] );
    }

    // add reverberation effect if activated
    if ( iReverbLevel != 0 )
    {
        // calculate attenuation amplification factor
        const double dRevLev =
            static_cast<double> ( iReverbLevel ) / AUD_REVERB_MAX / 2;

        if ( bReverbOnLeftChan )
        {
            for ( i = 0; i < iStereoBlockSizeSam; i += 2 )
            {
                // left channel
                vecdAudioStereo[i] +=
                    dRevLev * AudioReverb.ProcessSample ( vecdAudioStereo[i] );
            }
        }
        else
        {
            for ( i = 1; i < iStereoBlockSizeSam; i += 2 )
            {
                // right channel
                vecdAudioStereo[i] +=
                    dRevLev * AudioReverb.ProcessSample ( vecdAudioStereo[i] );
            }
        }
    }

    // mix both signals depending on the fading setting, convert
    // from double to short
    if ( iAudioInFader == AUD_FADER_IN_MIDDLE )
    {
        // just mix channels together
        for ( i = 0, j = 0; i < iMonoBlockSizeSam; i++, j += 2 )
        {
            vecsNetwork[i] =
                Double2Short ( ( vecdAudioStereo[j] +
                vecdAudioStereo[j + 1] ) / 2 );
        }
    }
    else
    {
        const double dAttFact =
            static_cast<double> ( AUD_FADER_IN_MIDDLE - abs ( AUD_FADER_IN_MIDDLE - iAudioInFader ) ) /
            AUD_FADER_IN_MIDDLE;

        if ( iAudioInFader > AUD_FADER_IN_MIDDLE )
        {
            for ( i = 0, j = 0; i < iMonoBlockSizeSam; i++, j += 2 )
            {
                // attenuation on right channel
                vecsNetwork[i] =
                    Double2Short ( ( vecdAudioStereo[j] +
                    dAttFact * vecdAudioStereo[j + 1] ) / 2 );
            }
        }
        else
        {
            for ( i = 0, j = 0; i < iMonoBlockSizeSam; i++, j += 2 )
            {
                // attenuation on left channel
                vecsNetwork[i] =
                    Double2Short ( ( vecdAudioStereo[j + 1] +
                    dAttFact * vecdAudioStereo[j] ) / 2 );
            }
        }
    }

    for ( i = 0; i < iSndCrdFrameSizeFactor; i++ )
    {
        // encode current audio frame with CELT encoder
        celt_encode ( CeltEncoder,
                      &vecsNetwork[i * SYSTEM_FRAME_SIZE_SAMPLES],
                      NULL,
                      &vecCeltData[0],
                      iCeltNumCodedBytes );

        // send coded audio through the network
        Socket.SendPacket ( Channel.PrepSendPacket ( vecCeltData ),
            Channel.GetAddress() );
    }


    // Receive signal ----------------------------------------------------------
    for ( i = 0; i < iSndCrdFrameSizeFactor; i++ )
    {
        // receive a new block
        const bool bReceiveDataOk =
            ( Channel.GetData ( vecbyNetwData ) == GS_BUFFER_OK );

        if ( bReceiveDataOk )
        {
            PostWinMessage ( MS_JIT_BUF_GET, MUL_COL_LED_GREEN );
        }
        else
        {
            PostWinMessage ( MS_JIT_BUF_GET, MUL_COL_LED_RED );
        }

        // CELT decoding
        if ( bReceiveDataOk )
        {
            celt_decode ( CeltDecoder,
                          &vecbyNetwData[0],
                          iCeltNumCodedBytes,
                          &vecsAudioSndCrdMono[i * SYSTEM_FRAME_SIZE_SAMPLES] );
        }
        else
        {
            // lost packet
            celt_decode ( CeltDecoder,
                          NULL,
                          0,
                          &vecsAudioSndCrdMono[i * SYSTEM_FRAME_SIZE_SAMPLES] );
        }
    }

    // check if channel is connected
    if ( Channel.IsConnected() )
    {
        // copy mono data in stereo sound card buffer
        for ( i = 0, j = 0; i < iMonoBlockSizeSam; i++, j += 2 )
        {
            vecsStereoSndCrd[j] = vecsStereoSndCrd[j + 1] =
                vecsAudioSndCrdMono[i];
        }
    }
    else
    {
        // if not connected, clear data
        vecsStereoSndCrd.Reset ( 0 );
    }

    // update response time measurement and socket buffer size
    CycleTimeVariance.Update();
    UpdateSocketBufferSize();
}

void CClient::UpdateSocketBufferSize()
{
    // just update the socket buffer size if auto setting is enabled, otherwise
    // do nothing
    if ( bDoAutoSockBufSize )
    {
        // We use the time response measurement for the automatic setting.
        // Assumptions:
        // - the audio interface/network jitter is assumed to be Gaussian
        // - the buffer size is set to 3.3 times the standard deviation of
        //   the jitter (~98% of the jitter should be fit in the
        //   buffer)
        // - introduce a hysteresis to avoid switching the buffer sizes all the
        //   time in case the time response measurement is close to a bound
        // - only use time response measurement results if averaging buffer is
        //   completely filled
        const double dHysteresis = 0.3;

        // calculate current buffer setting
        const double dAudioBufferDurationMs =
            iMonoBlockSizeSam * 1000 / SYSTEM_SAMPLE_RATE;

        // jitter introduced in the server by the timer implementation
        const double dServerJitterMs = 0.666666; // ms

        // accumulate the standard deviations of input network stream and
        // internal timer,
        // add 0.5 to "round up" -> ceil,
        // divide by MIN_SERVER_BLOCK_DURATION_MS because this is the size of
        // one block in the jitter buffer
        const double dEstCurBufSet = ( dAudioBufferDurationMs + dServerJitterMs +
            3.3 * ( Channel.GetTimingStdDev() + CycleTimeVariance.GetStdDev() ) ) /
            SYSTEM_BLOCK_DURATION_MS_FLOAT + 0.5;

        // upper/lower hysteresis decision
        const int iUpperHystDec = LlconMath().round ( dEstCurBufSet - dHysteresis );
        const int iLowerHystDec = LlconMath().round ( dEstCurBufSet + dHysteresis );

        // if both decisions are equal than use the result
        if ( iUpperHystDec == iLowerHystDec )
        {
            // set the socket buffer via the main window thread since somehow
            // it gives a protocol deadlock if we call the SetSocketBufSize()
            // function directly
            PostWinMessage ( MS_SET_JIT_BUF_SIZE, iUpperHystDec );
        }
        else
        {
            // we are in the middle of the decision region, use
            // previous setting for determing the new decision
            if ( !( ( GetSockBufNumFrames() == iUpperHystDec ) ||
                    ( GetSockBufNumFrames() == iLowerHystDec ) ) )
            {
                // The old result is not near the new decision,
                // use per definition the upper decision.
                // Set the socket buffer via the main window thread since somehow
                // it gives a protocol deadlock if we call the SetSocketBufSize()
                // function directly.
                PostWinMessage ( MS_SET_JIT_BUF_SIZE, iUpperHystDec );
            }
        }
    }
}
