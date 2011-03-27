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

#include "client.h"


/* Implementation *************************************************************/
CClient::CClient ( const quint16 iPortNumber ) :
    vstrIPAddress ( MAX_NUM_SERVER_ADDR_ITEMS, "" ),
    strName ( "" ),
    Channel ( false ), /* we need a client channel -> "false" */
    bDoAutoSockBufSize ( true ),
    iCeltNumCodedBytes ( CELT_NUM_BYTES_MONO_NORMAL_QUALITY ),
    bCeltDoHighQuality ( false ),
    bUseStereo ( false ),
    Socket ( &Channel, iPortNumber ),
    Sound ( AudioCallback, this ),
    iAudioInFader ( AUD_FADER_IN_MIDDLE ),
    bReverbOnLeftChan ( false ),
    iReverbLevel ( 0 ),
#if defined ( __APPLE__ ) || defined ( __MACOSX )
	// we assume on Mac always the preferred frame size works fine
    iSndCrdPrefFrameSizeFactor ( FRAME_SIZE_FACTOR_PREFERRED ),
    iSndCrdFrameSizeFactor ( FRAME_SIZE_FACTOR_PREFERRED ),
#else
	// if ASIO4All is used, it is safer to use the default value
	iSndCrdPrefFrameSizeFactor ( FRAME_SIZE_FACTOR_DEFAULT ),
	iSndCrdFrameSizeFactor ( FRAME_SIZE_FACTOR_DEFAULT ),
#endif
    bSndCrdConversionBufferRequired ( false ),
    iSndCardMonoBlockSizeSamConvBuff ( 0 ),
    bFraSiFactPrefSupported ( false ),
    bFraSiFactDefSupported ( false ),
    bFraSiFactSafeSupported ( false ),
    bOpenChatOnNewMessage ( true ),
    eGUIDesign ( GD_ORIGINAL )
{
    // init audio encoder/decoder (mono)
    CeltModeMono = celt_mode_create (
        SYSTEM_SAMPLE_RATE, 1, SYSTEM_FRAME_SIZE_SAMPLES, NULL );

    CeltEncoderMono = celt_encoder_create ( CeltModeMono );
    CeltDecoderMono = celt_decoder_create ( CeltModeMono );

#ifdef USE_LOW_COMPLEXITY_CELT_ENC
    // set encoder low complexity
    celt_encoder_ctl ( CeltEncoderMono,
        CELT_SET_COMPLEXITY_REQUEST, celt_int32_t ( 1 ) );
#endif

    // init audio encoder/decoder (stereo)
    CeltModeStereo = celt_mode_create (
        SYSTEM_SAMPLE_RATE, 2, SYSTEM_FRAME_SIZE_SAMPLES, NULL );

    CeltEncoderStereo = celt_encoder_create ( CeltModeStereo );
    CeltDecoderStereo = celt_decoder_create ( CeltModeStereo );

#ifdef USE_LOW_COMPLEXITY_CELT_ENC
    // set encoder low complexity
    celt_encoder_ctl ( CeltEncoderStereo,
        CELT_SET_COMPLEXITY_REQUEST, celt_int32_t ( 1 ) );
#endif


    // connections -------------------------------------------------------------
    // connection for protocol
    QObject::connect ( &Channel,
        SIGNAL ( MessReadyForSending ( CVector<uint8_t> ) ),
        this, SLOT ( OnSendProtMessage ( CVector<uint8_t> ) ) );

    QObject::connect ( &Channel, SIGNAL ( ReqJittBufSize() ),
        this, SLOT ( OnReqJittBufSize() ) );

    QObject::connect ( &Channel, SIGNAL ( ReqChanName() ),
        this, SLOT ( OnReqChanName() ) );

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
        Init();

        if ( bWasRunning )
        {
            // restart client
            Sound.Start();
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
    Init();

    if ( bWasRunning )
    {
        Sound.Start();
    }
}

void CClient::SetUseStereo ( const bool bNUseStereo )
{
    // init with new parameter, if client was running then first
    // stop it and restart again after new initialization
    const bool bWasRunning = Sound.IsRunning();
    if ( bWasRunning )
    {
        Sound.Stop();
    }

    // set new parameter
    bUseStereo = bNUseStereo;
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

    const QString strReturn = Sound.SetDev ( iNewDev );

    // init again because the sound card actual buffer size might
    // be changed on new device
    Init();

    if ( bWasRunning )
    {
        // restart client
        Sound.Start();
    }

    return strReturn;
}

void CClient::SetSndCrdLeftInputChannel ( const int iNewChan )
{
    // if client was running then first
    // stop it and restart again after new initialization
    const bool bWasRunning = Sound.IsRunning();
    if ( bWasRunning )
    {
        Sound.Stop();
    }

    Sound.SetLeftInputChannel ( iNewChan );
    Init();

    if ( bWasRunning )
    {
        // restart client
        Sound.Start();
    }
}

void CClient::SetSndCrdRightInputChannel ( const int iNewChan )
{
    // if client was running then first
    // stop it and restart again after new initialization
    const bool bWasRunning = Sound.IsRunning();
    if ( bWasRunning )
    {
        Sound.Stop();
    }

    Sound.SetRightInputChannel ( iNewChan );
    Init();

    if ( bWasRunning )
    {
        // restart client
        Sound.Start();
    }
}

void CClient::SetSndCrdLeftOutputChannel ( const int iNewChan )
{
    // if client was running then first
    // stop it and restart again after new initialization
    const bool bWasRunning = Sound.IsRunning();
    if ( bWasRunning )
    {
        Sound.Stop();
    }

    Sound.SetLeftOutputChannel ( iNewChan );
    Init();

    if ( bWasRunning )
    {
        // restart client
        Sound.Start();
    }
}

void CClient::SetSndCrdRightOutputChannel ( const int iNewChan )
{
    // if client was running then first
    // stop it and restart again after new initialization
    const bool bWasRunning = Sound.IsRunning();
    if ( bWasRunning )
    {
        Sound.Stop();
    }

    Sound.SetRightOutputChannel ( iNewChan );
    Init();

    if ( bWasRunning )
    {
        // restart client
        Sound.Start();
    }
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
    Init();

    if ( bWasRunning )
    {
        // restart client
        Sound.Start();
    }
}

void CClient::Start()
{
    // init object
    Init();

    // enable channel
    Channel.SetEnable ( true );

    // start audio interface
    Sound.Start();
}

void CClient::Stop()
{
    // stop audio interface
    Sound.Stop();

    // wait for approx. 300 ms to make sure no audio packet is still in the
    // network queue causing the channel to be reconnected right after having
    // received the disconnect message (seems not to gain much, disconnect is
    // still not working reliably)
    QTime dieTime = QTime::currentTime().addMSecs ( 300 );
    while ( QTime::currentTime() < dieTime )
    {
	    QCoreApplication::processEvents ( QEventLoop::AllEvents, 100 );
    }

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

void CClient::Init()
{
    // check if possible frame size factors are supported
    const int iFraSizePreffered =
        FRAME_SIZE_FACTOR_PREFERRED * SYSTEM_FRAME_SIZE_SAMPLES;

    bFraSiFactPrefSupported =
        ( Sound.Init ( iFraSizePreffered ) == iFraSizePreffered );

    const int iFraSizeDefault =
        FRAME_SIZE_FACTOR_DEFAULT * SYSTEM_FRAME_SIZE_SAMPLES;

    bFraSiFactDefSupported =
        ( Sound.Init ( iFraSizeDefault ) == iFraSizeDefault );

    const int iFraSizeSafe =
        FRAME_SIZE_FACTOR_SAFE * SYSTEM_FRAME_SIZE_SAMPLES;

    bFraSiFactSafeSupported =
        ( Sound.Init ( iFraSizeSafe ) == iFraSizeSafe );

    // translate block size index in actual block size
    const int iPrefMonoFrameSize =
        iSndCrdPrefFrameSizeFactor * SYSTEM_FRAME_SIZE_SAMPLES;

    // get actual sound card buffer size using preferred size
    iMonoBlockSizeSam = Sound.Init ( iPrefMonoFrameSize );

    // Calculate the current sound card frame size factor. In case
    // the current mono block size is not a multiple of the system
    // frame size, we have to use a sound card conversion buffer.
    if ( ( iMonoBlockSizeSam == ( SYSTEM_FRAME_SIZE_SAMPLES * FRAME_SIZE_FACTOR_PREFERRED ) ) ||
         ( iMonoBlockSizeSam == ( SYSTEM_FRAME_SIZE_SAMPLES * FRAME_SIZE_FACTOR_DEFAULT ) ) ||
         ( iMonoBlockSizeSam == ( SYSTEM_FRAME_SIZE_SAMPLES * FRAME_SIZE_FACTOR_SAFE ) ) )
    {
        // regular case: one of our predefined buffer sizes is available
        iSndCrdFrameSizeFactor = iMonoBlockSizeSam / SYSTEM_FRAME_SIZE_SAMPLES;

        // no sound card conversion buffer required
        bSndCrdConversionBufferRequired  = false;
    }
    else
    {
        // An unsupported sound card buffer size is currently used -> we have
        // to use a conversion buffer. Per definition we use the smallest buffer
        // size as llcon frame size

        // store actual sound card buffer size (stereo)
        iSndCardMonoBlockSizeSamConvBuff             = iMonoBlockSizeSam;
        const int iSndCardStereoBlockSizeSamConvBuff = 2 * iMonoBlockSizeSam;

        // overwrite block size by smallest supported llcon buffer size
        iSndCrdFrameSizeFactor = FRAME_SIZE_FACTOR_PREFERRED;
        iMonoBlockSizeSam =
            SYSTEM_FRAME_SIZE_SAMPLES * FRAME_SIZE_FACTOR_PREFERRED;

        iStereoBlockSizeSam = 2 * iMonoBlockSizeSam;

        // inits for conversion buffer (the size of the conversion buffer must
        // be the sum of input/output sizes which is the worst case fill level)
        const int iConBufSize =
            iStereoBlockSizeSam + iSndCardStereoBlockSizeSamConvBuff;

        SndCrdConversionBufferIn.Init  ( iConBufSize );
        SndCrdConversionBufferOut.Init ( iConBufSize );
        vecDataConvBuf.Init            ( iStereoBlockSizeSam );

        // the output conversion buffer must be filled with the inner
        // block size for initialization (this is the latency which is
        // introduced by the conversion buffer) to avoid buffer underruns
        const CVector<int16_t> vZeros ( iStereoBlockSizeSam, 0 );
        SndCrdConversionBufferOut.Put ( vZeros, vZeros.Size() );

        bSndCrdConversionBufferRequired = true;
    }

    // calculate stereo (two channels) buffer size
    iStereoBlockSizeSam = 2 * iMonoBlockSizeSam;

    vecsAudioSndCrdMono.Init ( iMonoBlockSizeSam );
    vecdAudioStereo.Init     ( iStereoBlockSizeSam );

    // init response time evaluation
    CycleTimeVariance.Init ( iMonoBlockSizeSam,
        SYSTEM_SAMPLE_RATE, TIME_MOV_AV_RESPONSE );

    CycleTimeVariance.Reset();

    // init reverberation
    AudioReverbL.Init ( SYSTEM_SAMPLE_RATE );
    AudioReverbR.Init ( SYSTEM_SAMPLE_RATE );

    // inits for CELT coding
    if ( bCeltDoHighQuality )
    {
        if ( bUseStereo )
        {
            iCeltNumCodedBytes = CELT_NUM_BYTES_STEREO_HIGH_QUALITY;
        }
        else
        {
            iCeltNumCodedBytes = CELT_NUM_BYTES_MONO_HIGH_QUALITY;
        }
    }
    else
    {
        if ( bUseStereo )
        {
            iCeltNumCodedBytes = CELT_NUM_BYTES_STEREO_NORMAL_QUALITY;
        }
        else
        {
            iCeltNumCodedBytes = CELT_NUM_BYTES_MONO_NORMAL_QUALITY;
        }
    }
    vecCeltData.Init ( iCeltNumCodedBytes );

    // inits for network and channel
    vecbyNetwData.Init ( iCeltNumCodedBytes );
    if ( bUseStereo )
    {
        vecsNetwork.Init ( iStereoBlockSizeSam );

        // set the channel network properties
        Channel.SetAudioStreamProperties ( iCeltNumCodedBytes,
                                           iSndCrdFrameSizeFactor,
                                           2 );
    }
    else
    {
        vecsNetwork.Init ( iMonoBlockSizeSam );

        // set the channel network properties
        Channel.SetAudioStreamProperties ( iCeltNumCodedBytes,
                                           iSndCrdFrameSizeFactor,
                                           1 );
    }
}

void CClient::AudioCallback ( CVector<int16_t>& psData, void* arg )
{
    // get the pointer to the object
    CClient* pMyClientObj = reinterpret_cast<CClient*> ( arg );

    // process audio data
    pMyClientObj->ProcessSndCrdAudioData ( psData );
}

void CClient::ProcessSndCrdAudioData ( CVector<int16_t>& vecsStereoSndCrd )
{
    // check if a conversion buffer is required or not
    if ( bSndCrdConversionBufferRequired )
    {
        // add new sound card block in conversion buffer
        SndCrdConversionBufferIn.Put ( vecsStereoSndCrd, vecsStereoSndCrd.Size() );

        // process all available blocks of data
        while ( SndCrdConversionBufferIn.GetAvailData() >= iStereoBlockSizeSam )
        {
            // get one block of data for processing
            SndCrdConversionBufferIn.Get ( vecDataConvBuf );

            // process audio data
            ProcessAudioDataIntern ( vecDataConvBuf );

            SndCrdConversionBufferOut.Put ( vecDataConvBuf, vecDataConvBuf.Size() );
        }

        // get processed sound card block out of the conversion buffer
        SndCrdConversionBufferOut.Get ( vecsStereoSndCrd );
    }
    else
    {
        // regular case: no conversion buffer required
        // process audio data
        ProcessAudioDataIntern ( vecsStereoSndCrd );
    }
}

void CClient::ProcessAudioDataIntern ( CVector<int16_t>& vecsStereoSndCrd )
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

        if ( bUseStereo )
        {
            // for stereo always apply reverberation effect on both channels
            for ( i = 0; i < iStereoBlockSizeSam; i += 2 )
            {
                // left channel
                vecdAudioStereo[i] +=
                    dRevLev * AudioReverbL.ProcessSample ( vecdAudioStereo[i] );

                // right channel
                vecdAudioStereo[i + 1] +=
                    dRevLev * AudioReverbR.ProcessSample ( vecdAudioStereo[i + 1] );
            }
        }
        else
        {
            if ( bReverbOnLeftChan )
            {
                for ( i = 0; i < iStereoBlockSizeSam; i += 2 )
                {
                    // left channel
                    vecdAudioStereo[i] +=
                        dRevLev * AudioReverbL.ProcessSample ( vecdAudioStereo[i] );
                }
            }
            else
            {
                for ( i = 1; i < iStereoBlockSizeSam; i += 2 )
                {
                    // right channel
                    vecdAudioStereo[i] +=
                        dRevLev * AudioReverbR.ProcessSample ( vecdAudioStereo[i] );
                }
            }
        }
    }

    // mix both signals depending on the fading setting, convert
    // from double to short
    if ( iAudioInFader == AUD_FADER_IN_MIDDLE )
    {
        if ( bUseStereo )
        {
            // perform type conversion
            for ( i = 0; i < iStereoBlockSizeSam; i++ )
            {
                vecsNetwork[i] = Double2Short ( vecdAudioStereo[i] );
            }
        }
        else
        {
            // mix channels together
            for ( i = 0, j = 0; i < iMonoBlockSizeSam; i++, j += 2 )
            {
                vecsNetwork[i] =
                    Double2Short ( ( vecdAudioStereo[j] +
                    vecdAudioStereo[j + 1] ) / 2 );
            }
        }
    }
    else
    {
        if ( bUseStereo )
        {
            // stereo
            const double dAttFactStereo = static_cast<double> (
                AUD_FADER_IN_MIDDLE - abs ( AUD_FADER_IN_MIDDLE - iAudioInFader ) ) /
                AUD_FADER_IN_MIDDLE;

            if ( iAudioInFader > AUD_FADER_IN_MIDDLE )
            {
                for ( i = 0, j = 0; i < iMonoBlockSizeSam; i++, j += 2 )
                {
                    // attenuation on right channel
                    vecsNetwork[j] = Double2Short (
                        vecdAudioStereo[j] );

                    vecsNetwork[j + 1] = Double2Short (
                        dAttFactStereo * vecdAudioStereo[j + 1] );
                }
            }
            else
            {
                for ( i = 0, j = 0; i < iMonoBlockSizeSam; i++, j += 2 )
                {
                    // attenuation on left channel
                    vecsNetwork[j] = Double2Short (
                        dAttFactStereo * vecdAudioStereo[j] );

                    vecsNetwork[j + 1] = Double2Short (
                        vecdAudioStereo[j + 1] );
                }
            }
        }
        else
        {
            // mono
            // make sure that in the middle position the two channels are
            // amplified by 1/2, if the pan is set to one channel, this
            // channel should have an amplification of 1 
            const double dAttFactMono = static_cast<double> (
                AUD_FADER_IN_MIDDLE - abs ( AUD_FADER_IN_MIDDLE - iAudioInFader ) ) /
                AUD_FADER_IN_MIDDLE / 2;

            const double dAmplFactMono = 0.5 + static_cast<double> (
                abs ( AUD_FADER_IN_MIDDLE - iAudioInFader ) ) /
                AUD_FADER_IN_MIDDLE / 2;

            if ( iAudioInFader > AUD_FADER_IN_MIDDLE )
            {
                for ( i = 0, j = 0; i < iMonoBlockSizeSam; i++, j += 2 )
                {
                    // attenuation on right channel
                    vecsNetwork[i] = Double2Short (
                        dAmplFactMono * vecdAudioStereo[j] +
                        dAttFactMono * vecdAudioStereo[j + 1] );
                }
            }
            else
            {
                for ( i = 0, j = 0; i < iMonoBlockSizeSam; i++, j += 2 )
                {
                    // attenuation on left channel
                    vecsNetwork[i] = Double2Short (
                        dAmplFactMono * vecdAudioStereo[j + 1] +
                        dAttFactMono * vecdAudioStereo[j] );
                }
            }
        }
    }

    for ( i = 0; i < iSndCrdFrameSizeFactor; i++ )
    {
        if ( bUseStereo )
        {
            // encode current audio frame with CELT encoder
            celt_encode ( CeltEncoderStereo,
                          &vecsNetwork[i * 2 * SYSTEM_FRAME_SIZE_SAMPLES],
                          NULL,
                          &vecCeltData[0],
                          iCeltNumCodedBytes );
        }
        else
        {
            // encode current audio frame with CELT encoder
            celt_encode ( CeltEncoderMono,
                          &vecsNetwork[i * SYSTEM_FRAME_SIZE_SAMPLES],
                          NULL,
                          &vecCeltData[0],
                          iCeltNumCodedBytes );
        }

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
            if ( bUseStereo )
            {
                celt_decode ( CeltDecoderStereo,
                              &vecbyNetwData[0],
                              iCeltNumCodedBytes,
                              &vecsStereoSndCrd[i * 2 * SYSTEM_FRAME_SIZE_SAMPLES] );
            }
            else
            {
                celt_decode ( CeltDecoderMono,
                              &vecbyNetwData[0],
                              iCeltNumCodedBytes,
                              &vecsAudioSndCrdMono[i * SYSTEM_FRAME_SIZE_SAMPLES] );
            }
        }
        else
        {
            // lost packet
            if ( bUseStereo )
            {
                celt_decode ( CeltDecoderStereo,
                              NULL,
                              0,
                              &vecsStereoSndCrd[i * 2 * SYSTEM_FRAME_SIZE_SAMPLES] );
            }
            else
            {
                celt_decode ( CeltDecoderMono,
                              NULL,
                              0,
                              &vecsAudioSndCrdMono[i * SYSTEM_FRAME_SIZE_SAMPLES] );
            }
        }
    }

    // check if channel is connected
    if ( Channel.IsConnected() )
    {
        if ( !bUseStereo )
        {
            // copy mono data in stereo sound card buffer
            for ( i = 0, j = 0; i < iMonoBlockSizeSam; i++, j += 2 )
            {
                vecsStereoSndCrd[j] = vecsStereoSndCrd[j + 1] =
                    vecsAudioSndCrdMono[i];
            }
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
            ( GetSndCrdActualMonoBlSize() +
              GetSndCrdConvBufAdditionalDelayMonoBlSize() ) *
            1000 / SYSTEM_SAMPLE_RATE;

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

int CClient::EstimatedOverallDelay ( const int iPingTimeMs )
{
/*
    For estimating the overall delay, use the following assumptions:
    - the mean delay of a cyclic buffer is half the buffer size (since
      for the average it is assumed that the buffer is half filled)
    - consider the jitter buffer on the server side, too
*/
    // 2 times buffers at client and server divided by 2 (half the buffer
    // for the delay) is simply the total socket buffer size
    const double dTotalJitterBufferDelayMs =
        SYSTEM_BLOCK_DURATION_MS_FLOAT * GetSockBufNumFrames();

    // we assume that we have two period sizes for the input and one for the
    // output, therefore we have "3 *" instead of "2 *" (for input and output)
    // the actual sound card buffer size, also consider delay introduced by
    // sound card conversion buffer by using
    // "GetSndCrdConvBufAdditionalDelayMonoBlSize"
    const double dTotalSoundCardDelayMs =
        ( 3 * GetSndCrdActualMonoBlSize() +
        GetSndCrdConvBufAdditionalDelayMonoBlSize() ) *
        1000 / SYSTEM_SAMPLE_RATE;

    // network packets are of the same size as the audio packets per definition
    // if no sound card conversion buffer is used
    const double dDelayToFillNetworkPacketsMs =
        GetSystemMonoBlSize() * 1000 / SYSTEM_SAMPLE_RATE;

    // CELT additional delay at small frame sizes is half a frame size
    const double dAdditionalAudioCodecDelayMs =
        SYSTEM_BLOCK_DURATION_MS_FLOAT / 2;

    const double dTotalBufferDelayMs =
        dDelayToFillNetworkPacketsMs +
        dTotalJitterBufferDelayMs +
        dTotalSoundCardDelayMs +
        dAdditionalAudioCodecDelayMs;

    return LlconMath::round ( dTotalBufferDelayMs + iPingTimeMs );
}
