/******************************************************************************\
 * Copyright (c) 2004-2020
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
CClient::CClient ( const quint16  iPortNumber,
                   const QString& strConnOnStartupAddress,
                   const int      iCtrlMIDIChannel,
                   const bool     bNoAutoJackConnect ) :
    vstrIPAddress                    ( MAX_NUM_SERVER_ADDR_ITEMS, "" ),
    ChannelInfo                      (),
    vecStoredFaderTags               ( MAX_NUM_STORED_FADER_SETTINGS, "" ),
    vecStoredFaderLevels             ( MAX_NUM_STORED_FADER_SETTINGS, AUD_MIX_FADER_MAX ),
    vecStoredFaderIsSolo             ( MAX_NUM_STORED_FADER_SETTINGS, false ),
    iNewClientFaderLevel             ( 100 ),
    vecWindowPosMain                 (), // empty array
    vecWindowPosSettings             (), // empty array
    vecWindowPosChat                 (), // empty array
    vecWindowPosProfile              (), // empty array
    vecWindowPosConnect              (), // empty array
    bWindowWasShownSettings          ( false ),
    bWindowWasShownChat              ( false ),
    bWindowWasShownProfile           ( false ),
    bWindowWasShownConnect           ( false ),
    Channel                          ( false ), /* we need a client channel -> "false" */
    eAudioCompressionType            ( CT_OPUS ),
    iCeltNumCodedBytes               ( OPUS_NUM_BYTES_MONO_LOW_QUALITY ),
    eAudioQuality                    ( AQ_NORMAL ),
    eAudioChannelConf                ( CC_MONO ),
    bIsInitializationPhase           ( true ),
    Socket                           ( &Channel, iPortNumber ),
    Sound                            ( AudioCallback, this, iCtrlMIDIChannel, bNoAutoJackConnect ),
    iAudioInFader                    ( AUD_FADER_IN_MIDDLE ),
    bReverbOnLeftChan                ( false ),
    iReverbLevel                     ( 0 ),
    iSndCrdPrefFrameSizeFactor       ( FRAME_SIZE_FACTOR_PREFERRED ),
    iSndCrdFrameSizeFactor           ( FRAME_SIZE_FACTOR_PREFERRED ),
    bSndCrdConversionBufferRequired  ( false ),
    iSndCardMonoBlockSizeSamConvBuff ( 0 ),
    bFraSiFactPrefSupported          ( false ),
    bFraSiFactDefSupported           ( false ),
    bFraSiFactSafeSupported          ( false ),
    eGUIDesign                       ( GD_ORIGINAL ),
    bDisplayChannelLevels            ( false ),
    bJitterBufferOK                  ( true ),
    strCentralServerAddress          ( "" ),
    bUseDefaultCentralServerAddress  ( true ),
    iServerSockBufNumFrames          ( DEF_NET_BUF_SIZE_NUM_BL )
{
    int iOpusError;

    // init audio encoder/decoder (mono)
    OpusMode = opus_custom_mode_create ( SYSTEM_SAMPLE_RATE_HZ,
                                         SYSTEM_FRAME_SIZE_SAMPLES,
                                         &iOpusError );

    OpusEncoderMono = opus_custom_encoder_create ( OpusMode,
                                                   1,
                                                   &iOpusError );

    OpusDecoderMono = opus_custom_decoder_create ( OpusMode,
                                                   1,
                                                   &iOpusError );

    // we require a constant bit rate
    opus_custom_encoder_ctl ( OpusEncoderMono,
                              OPUS_SET_VBR ( 0 ) );

    // we want as low delay as possible
    opus_custom_encoder_ctl ( OpusEncoderMono,
                              OPUS_SET_APPLICATION ( OPUS_APPLICATION_RESTRICTED_LOWDELAY ) );

#ifdef USE_LOW_COMPLEXITY_CELT_ENC
    // set encoder low complexity
    opus_custom_encoder_ctl ( OpusEncoderMono,
                              OPUS_SET_COMPLEXITY ( 1 ) );
#endif

    // init audio encoder/decoder (stereo)
    OpusEncoderStereo = opus_custom_encoder_create ( OpusMode,
                                                     2,
                                                     &iOpusError );

    OpusDecoderStereo = opus_custom_decoder_create ( OpusMode,
                                                     2,
                                                     &iOpusError );

    // we require a constant bit rate
    opus_custom_encoder_ctl ( OpusEncoderStereo,
                              OPUS_SET_VBR ( 0 ) );

    // we want as low delay as possible
    opus_custom_encoder_ctl ( OpusEncoderStereo,
                              OPUS_SET_APPLICATION ( OPUS_APPLICATION_RESTRICTED_LOWDELAY ) );

#ifdef USE_LOW_COMPLEXITY_CELT_ENC
    // set encoder low complexity
    opus_custom_encoder_ctl ( OpusEncoderStereo,
                              OPUS_SET_COMPLEXITY ( 1 ) );
#endif


    // Connections -------------------------------------------------------------
    // connections for the protocol mechanism
    QObject::connect ( &Channel,
        SIGNAL ( MessReadyForSending ( CVector<uint8_t> ) ),
        this, SLOT ( OnSendProtMessage ( CVector<uint8_t> ) ) );

    QObject::connect ( &Channel,
        SIGNAL ( DetectedCLMessage ( CVector<uint8_t>, int, CHostAddress ) ),
        this, SLOT ( OnDetectedCLMessage ( CVector<uint8_t>, int, CHostAddress ) ) );

    QObject::connect ( &Channel, SIGNAL ( ReqJittBufSize() ),
        this, SLOT ( OnReqJittBufSize() ) );

    QObject::connect ( &Channel, SIGNAL ( JittBufSizeChanged ( int ) ),
        this, SLOT ( OnJittBufSizeChanged ( int ) ) );

    QObject::connect ( &Channel, SIGNAL ( ReqChanInfo() ),
        this, SLOT ( OnReqChanInfo() ) );

    QObject::connect ( &Channel,
        SIGNAL ( ConClientListMesReceived ( CVector<CChannelInfo> ) ),
        SIGNAL ( ConClientListMesReceived ( CVector<CChannelInfo> ) ) );

    QObject::connect ( &Channel,
        SIGNAL ( Disconnected() ),
        SIGNAL ( Disconnected() ) );

    QObject::connect ( &Channel, SIGNAL ( NewConnection() ),
        this, SLOT ( OnNewConnection() ) );

    QObject::connect ( &Channel,
        SIGNAL ( ChatTextReceived ( QString ) ),
        SIGNAL ( ChatTextReceived ( QString ) ) );

    QObject::connect( &Channel,
        SIGNAL ( LicenceRequired ( ELicenceType ) ),
        SIGNAL ( LicenceRequired ( ELicenceType ) ) );

    QObject::connect ( &ConnLessProtocol,
        SIGNAL ( CLMessReadyForSending ( CHostAddress, CVector<uint8_t> ) ),
        this, SLOT ( OnSendCLProtMessage ( CHostAddress, CVector<uint8_t> ) ) );

    QObject::connect ( &ConnLessProtocol,
        SIGNAL ( CLServerListReceived ( CHostAddress, CVector<CServerInfo> ) ),
        SIGNAL ( CLServerListReceived ( CHostAddress, CVector<CServerInfo> ) ) );

    QObject::connect ( &ConnLessProtocol,
        SIGNAL ( CLConnClientsListMesReceived ( CHostAddress, CVector<CChannelInfo> ) ),
        SIGNAL ( CLConnClientsListMesReceived ( CHostAddress, CVector<CChannelInfo> ) ) );

    QObject::connect ( &ConnLessProtocol,
        SIGNAL ( CLPingReceived ( CHostAddress, int ) ),
        this, SLOT ( OnCLPingReceived ( CHostAddress, int ) ) );

    QObject::connect ( &ConnLessProtocol,
        SIGNAL ( CLPingWithNumClientsReceived ( CHostAddress, int, int ) ),
        this, SLOT ( OnCLPingWithNumClientsReceived ( CHostAddress, int, int ) ) );

    QObject::connect ( &ConnLessProtocol,
        SIGNAL ( CLDisconnection ( CHostAddress ) ),
        this, SLOT ( OnCLDisconnection ( CHostAddress ) ) );

#ifdef ENABLE_CLIENT_VERSION_AND_OS_DEBUGGING
    QObject::connect ( &ConnLessProtocol,
        SIGNAL ( CLVersionAndOSReceived ( CHostAddress, COSUtil::EOpSystemType, QString ) ),
        SIGNAL ( CLVersionAndOSReceived ( CHostAddress, COSUtil::EOpSystemType, QString ) ) );
#endif

    // other
    QObject::connect ( &Sound, SIGNAL ( ReinitRequest ( int ) ),
        this, SLOT ( OnSndCrdReinitRequest ( int ) ) );

    QObject::connect ( &Sound,
        SIGNAL ( ControllerInFaderLevel ( int, int ) ),
        SIGNAL ( ControllerInFaderLevel ( int, int ) ) );

    QObject::connect ( &Socket, SIGNAL ( InvalidPacketReceived ( CHostAddress ) ),
        this, SLOT ( OnInvalidPacketReceived ( CHostAddress ) ) );


    // start the socket (it is important to start the socket after all
    // initializations and connections)
    Socket.Start();

    // do an immediate start if a server address is given
    if ( !strConnOnStartupAddress.isEmpty() )
    {
        SetServerAddr ( strConnOnStartupAddress );
        Start();
    }
}

void CClient::OnSendProtMessage ( CVector<uint8_t> vecMessage )
{
    // the protocol queries me to call the function to send the message
    // send it through the network
    Socket.SendPacket ( vecMessage, Channel.GetAddress() );
}

void CClient::OnSendCLProtMessage ( CHostAddress     InetAddr,
                                    CVector<uint8_t> vecMessage )
{
    // the protocol queries me to call the function to send the message
    // send it through the network
    Socket.SendPacket ( vecMessage, InetAddr );
}

void CClient::OnInvalidPacketReceived ( CHostAddress RecHostAddr )
{
    // message coult not be parsed, check if the packet comes
    // from the server we just connected -> if yes, send
    // disconnect message since the server may not know that we
    // are not connected anymore
    if ( Channel.GetAddress() == RecHostAddr )
    {
        ConnLessProtocol.CreateCLDisconnection ( RecHostAddr );
    }
}

void CClient::OnDetectedCLMessage ( CVector<uint8_t> vecbyMesBodyData,
                                    int              iRecID,
                                    CHostAddress     RecHostAddr )
{
    // connection less messages are always processed
    ConnLessProtocol.ParseConnectionLessMessageBody ( vecbyMesBodyData,
                                                      iRecID,
                                                      RecHostAddr );
}

void CClient::OnJittBufSizeChanged ( int iNewJitBufSize )
{
    // we received a jitter buffer size changed message from the server,
    // only apply this value if auto jitter buffer size is enabled
    if ( GetDoAutoSockBufSize() )
    {
        // Note: Do not use the "SetServerSockBufNumFrames" function for setting
        // the new server jitter buffer size since then a message would be sent
        // to the server which is incorrect.
        iServerSockBufNumFrames = iNewJitBufSize;
    }
}

void CClient::OnNewConnection()
{
    // a new connection was successfully initiated, send infos and request
    // connected clients list
    Channel.SetRemoteInfo ( ChannelInfo );

    // We have to send a connected clients list request since it can happen
    // that we just had connected to the server and then disconnected but
    // the server still thinks that we are connected (the server is still
    // waiting for the channel time-out). If we now connect again, we would
    // not get the list because the server does not know about a new connection.
    // Same problem is with the jitter buffer message.
    Channel.CreateReqConnClientsList();
    CreateServerJitterBufferMessage();
}

void CClient::CreateServerJitterBufferMessage()
{
    // per definition in the client: if auto jitter buffer is enabled, both,
    // the client and server shall use an auto jitter buffer
    if ( GetDoAutoSockBufSize() )
    {
        // in case auto jitter buffer size is enabled, we have to transmit a
        // special value
        Channel.CreateJitBufMes ( AUTO_NET_BUF_SIZE_FOR_PROTOCOL );
    }
    else
    {
        Channel.CreateJitBufMes ( GetServerSockBufNumFrames() );
    }
}

void CClient::OnCLPingReceived ( CHostAddress InetAddr,
                                 int          iMs )
{
    // make sure we are running and the server address is correct
    if ( IsRunning() && ( InetAddr == Channel.GetAddress() ) )
    {
        // take care of wrap arounds (if wrapping, do not use result)
        const int iCurDiff = EvaluatePingMessage ( iMs );
        if ( iCurDiff >= 0 )
        {
            emit PingTimeReceived ( iCurDiff );
        }
    }
}

void CClient::OnCLPingWithNumClientsReceived ( CHostAddress InetAddr,
                                               int          iMs,
                                               int          iNumClients )
{
    // take care of wrap arounds (if wrapping, do not use result)
    const int iCurDiff = EvaluatePingMessage ( iMs );
    if ( iCurDiff >= 0 )
    {
        emit CLPingTimeWithNumClientsReceived ( InetAddr,
                                                iCurDiff,
                                                iNumClients );
    }
}

int CClient::PreparePingMessage()
{
    // transmit the current precise time (in ms)
    return PreciseTime.elapsed();
}

int CClient::EvaluatePingMessage ( const int iMs )
{
    // calculate difference between received time in ms and current time in ms
    return PreciseTime.elapsed() - iMs;
}

void CClient::SetDoAutoSockBufSize ( const bool bValue )
{
    // first, set new value in the channel object
    Channel.SetDoAutoSockBufSize ( bValue );

    // inform the server about the change
    CreateServerJitterBufferMessage();
}

bool CClient::SetServerAddr ( QString strNAddr )
{
    CHostAddress HostAddress;
    if ( NetworkUtil().ParseNetworkAddress ( strNAddr,
                                             HostAddress ) )
    {
        // apply address to the channel
        Channel.SetAddress ( HostAddress );

        return true;
    }
    else
    {
        return false; // invalid address
    }
}

bool CClient::GetAndResetbJitterBufferOKFlag()
{
    // get the socket buffer put status flag and reset it
    const bool bSocketJitBufOKFlag = Socket.GetAndResetbJitterBufferOKFlag();

    if ( !bJitterBufferOK )
    {
        // our jitter buffer get status is not OK so the overall status of the
        // jitter buffer is also not OK (we do not have to consider the status
        // of the socket buffer put status flag)

        // reset flag before returning the function
        bJitterBufferOK = true;
        return false;
    }

    // the jitter buffer get (our own status flag) is OK, the final status
    // now depends on the jitter buffer put status flag from the socket
    // since per definition the jitter buffer status is OK if both the
    // put and get status are OK
    return bSocketJitBufOKFlag;
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

void CClient::SetAudioQuality ( const EAudioQuality eNAudioQuality )
{
    // init with new parameter, if client was running then first
    // stop it and restart again after new initialization
    const bool bWasRunning = Sound.IsRunning();
    if ( bWasRunning )
    {
        Sound.Stop();
    }

    // set new parameter
    eAudioQuality = eNAudioQuality;
    Init();

    if ( bWasRunning )
    {
        Sound.Start();
    }
}

void CClient::SetAudioChannels ( const EAudChanConf eNAudChanConf )
{
    // init with new parameter, if client was running then first
    // stop it and restart again after new initialization
    const bool bWasRunning = Sound.IsRunning();
    if ( bWasRunning )
    {
        Sound.Stop();
    }

    // set new parameter
    eAudioChannelConf = eNAudChanConf;
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

void CClient::OnSndCrdReinitRequest ( int iSndCrdResetType )
{
    // in older QT versions, enums cannot easily be used in signals without
    // registering them -> workaroud: we use the int type and cast to the enum
    const ESndCrdResetType eSndCrdResetType =
        static_cast<ESndCrdResetType> ( iSndCrdResetType );

    // if client was running then first
    // stop it and restart again after new initialization
    const bool bWasRunning = Sound.IsRunning();
    if ( bWasRunning )
    {
        Sound.Stop();
    }

    // perform reinit request as indicated by the request type parameter
    if ( eSndCrdResetType != RS_ONLY_RESTART )
    {
        if ( eSndCrdResetType != RS_ONLY_RESTART_AND_INIT )
        {
            // reinit the driver if requested
            // (we use the currently selected driver)
            Sound.SetDev ( Sound.GetDev() );
        }

        // init client object (must always be performed if the driver
        // was changed)
        Init();
    }

    if ( bWasRunning )
    {
        // restart client
        Sound.Start();
    }
}

void CClient::Start()
{
    // always use the OPUS codec
#if ( SYSTEM_FRAME_SIZE_SAMPLES == 64 )
    eAudioCompressionType = CT_OPUS64;
#else
    eAudioCompressionType = CT_OPUS;
#endif

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

    // disable channel
    Channel.SetEnable ( false );

    // wait for approx. 100 ms to make sure no audio packet is still in the
    // network queue causing the channel to be reconnected right after having
    // received the disconnect message (seems not to gain much, disconnect is
    // still not working reliably)
    QTime DieTime = QTime::currentTime().addMSecs ( 100 );
    while ( QTime::currentTime() < DieTime )
    {
        // exclude user input events because if we use AllEvents, it happens
        // that if the user initiates a connection and disconnection quickly
        // (e.g. quickly pressing enter five times), the software can get into
        // an unknown state
        QCoreApplication::processEvents (
            QEventLoop::ExcludeUserInputEvents, 100 );
    }

    // Send disconnect message to server (Since we disable our protocol
    // receive mechanism with the next command, we do not evaluate any
    // respond from the server, therefore we just hope that the message
    // gets its way to the server, if not, the old behaviour time-out
    // disconnects the connection anyway).
    ConnLessProtocol.CreateCLDisconnection ( Channel.GetAddress() );

    // reset current signal level and LEDs
    bJitterBufferOK = true;
    SignalLevelMeter.Reset();
}

void CClient::Init()
{
    // check if possible frame size factors are supported
    const int iFraSizePreffered = FRAME_SIZE_FACTOR_PREFERRED * SYSTEM_FRAME_SIZE_SAMPLES;
    const int iFraSizeDefault   = FRAME_SIZE_FACTOR_DEFAULT * SYSTEM_FRAME_SIZE_SAMPLES;
    const int iFraSizeSafe      = FRAME_SIZE_FACTOR_SAFE * SYSTEM_FRAME_SIZE_SAMPLES;

    bFraSiFactPrefSupported = ( Sound.Init ( iFraSizePreffered ) == iFraSizePreffered );
    bFraSiFactDefSupported  = ( Sound.Init ( iFraSizeDefault )   == iFraSizeDefault );
    bFraSiFactSafeSupported = ( Sound.Init ( iFraSizeSafe )      == iFraSizeSafe );

    // translate block size index in actual block size
    const int iPrefMonoFrameSize = iSndCrdPrefFrameSizeFactor * SYSTEM_FRAME_SIZE_SAMPLES;

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
        bSndCrdConversionBufferRequired = false;
    }
    else
    {
        // An unsupported sound card buffer size is currently used -> we have
        // to use a conversion buffer. Per definition we use the smallest buffer
        // size as the current frame size

        // store actual sound card buffer size (stereo)
        iSndCardMonoBlockSizeSamConvBuff             = iMonoBlockSizeSam;
        const int iSndCardStereoBlockSizeSamConvBuff = 2 * iMonoBlockSizeSam;

        // overwrite block size by smallest supported buffer size
        iSndCrdFrameSizeFactor = FRAME_SIZE_FACTOR_PREFERRED;
        iMonoBlockSizeSam      = SYSTEM_FRAME_SIZE_SAMPLES * FRAME_SIZE_FACTOR_PREFERRED;

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

    // init reverberation
    AudioReverbL.Init ( SYSTEM_SAMPLE_RATE_HZ );
    AudioReverbR.Init ( SYSTEM_SAMPLE_RATE_HZ );

    // inits for audio coding
    if ( eAudioChannelConf == CC_MONO )
    {
        switch ( eAudioQuality )
        {
        case AQ_LOW:
            iCeltNumCodedBytes = OPUS_NUM_BYTES_MONO_LOW_QUALITY;
            break;

        case AQ_NORMAL:
            iCeltNumCodedBytes = OPUS_NUM_BYTES_MONO_NORMAL_QUALITY;
            break;

        case AQ_HIGH:
            iCeltNumCodedBytes = OPUS_NUM_BYTES_MONO_HIGH_QUALITY;
            break;
        }
    }
    else
    {
        switch ( eAudioQuality )
        {
        case AQ_LOW:
            iCeltNumCodedBytes = OPUS_NUM_BYTES_STEREO_LOW_QUALITY;
            break;

        case AQ_NORMAL:
            iCeltNumCodedBytes = OPUS_NUM_BYTES_STEREO_NORMAL_QUALITY;
            break;

        case AQ_HIGH:
            iCeltNumCodedBytes = OPUS_NUM_BYTES_STEREO_HIGH_QUALITY;
            break;
        }
    }

    vecCeltData.Init ( iCeltNumCodedBytes );

    if ( eAudioChannelConf == CC_MONO )
    {
        opus_custom_encoder_ctl ( OpusEncoderMono,
                                  OPUS_SET_BITRATE (
                                      CalcBitRateBitsPerSecFromCodedBytes (
                                          iCeltNumCodedBytes ) ) );
    }
    else
    {
        opus_custom_encoder_ctl ( OpusEncoderStereo,
                                  OPUS_SET_BITRATE (
                                      CalcBitRateBitsPerSecFromCodedBytes (
                                          iCeltNumCodedBytes ) ) );
    }

    // inits for network and channel
    vecbyNetwData.Init ( iCeltNumCodedBytes );

    if ( eAudioChannelConf == CC_MONO )
    {
        // set the channel network properties
        Channel.SetAudioStreamProperties ( eAudioCompressionType,
                                           iCeltNumCodedBytes,
                                           iSndCrdFrameSizeFactor,
                                           1 );
    }
    else
    {
        // set the channel network properties
        Channel.SetAudioStreamProperties ( eAudioCompressionType,
                                           iCeltNumCodedBytes,
                                           iSndCrdFrameSizeFactor,
                                           2 );
    }

    // reset initialization phase flag
    bIsInitializationPhase = true;
}

void CClient::AudioCallback ( CVector<int16_t>& psData, void* arg )
{
    // get the pointer to the object
    CClient* pMyClientObj = static_cast<CClient*> ( arg );

    // process audio data
    pMyClientObj->ProcessSndCrdAudioData ( psData );
}

void CClient::ProcessSndCrdAudioData ( CVector<int16_t>& vecsStereoSndCrd )
{

/*
// TEST do a soundcard jitter measurement
static CTimingMeas JitterMeas ( 1000, "test2.dat" );
JitterMeas.Measure();
*/

    // check if a conversion buffer is required or not
    if ( bSndCrdConversionBufferRequired )
    {
        // add new sound card block in conversion buffer
        SndCrdConversionBufferIn.Put ( vecsStereoSndCrd, vecsStereoSndCrd.Size() );

        // process all available blocks of data
        while ( SndCrdConversionBufferIn.GetAvailData() >= iStereoBlockSizeSam )
        {
            // get one block of data for processing
            SndCrdConversionBufferIn.Get ( vecDataConvBuf, iStereoBlockSizeSam );

            // process audio data
            ProcessAudioDataIntern ( vecDataConvBuf );

            SndCrdConversionBufferOut.Put ( vecDataConvBuf, iStereoBlockSizeSam );
        }

        // get processed sound card block out of the conversion buffer
        SndCrdConversionBufferOut.Get ( vecsStereoSndCrd, vecsStereoSndCrd.Size() );
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

    // add reverberation effect if activated
    if ( iReverbLevel != 0 )
    {
        // calculate attenuation amplification factor
        const double dRevLev =
            static_cast<double> ( iReverbLevel ) / AUD_REVERB_MAX / 2;

        if ( eAudioChannelConf == CC_STEREO )
        {
            // for stereo always apply reverberation effect on both channels
            for ( i = 0; i < iStereoBlockSizeSam; i += 2 )
            {
                // both channels (stereo)
                AudioReverbL.ProcessSample ( vecsStereoSndCrd[i], vecsStereoSndCrd[i + 1], dRevLev );
            }
        }
        else
        {
            // mono and mono-in/stereo out mode
            if ( bReverbOnLeftChan )
            {
                for ( i = 0; i < iStereoBlockSizeSam; i += 2 )
                {
                    // left channel
                    int16_t sRightDummy = 0; // has to be 0 for mono reverb
                    AudioReverbL.ProcessSample ( vecsStereoSndCrd[i], sRightDummy, dRevLev );
                }
            }
            else
            {
                for ( i = 1; i < iStereoBlockSizeSam; i += 2 )
                {
                    // right channel
                    int16_t sRightDummy = 0; // has to be 0 for mono reverb
                    AudioReverbR.ProcessSample ( vecsStereoSndCrd[i], sRightDummy, dRevLev );
                }
            }
        }
    }

    // mix both signals depending on the fading setting, convert
    // from double to short
    if ( iAudioInFader == AUD_FADER_IN_MIDDLE )
    {
        // no action require if fader is in the middle and stereo is used
        if ( eAudioChannelConf != CC_STEREO )
        {
            // mix channels together (store result in first half of the vector)
            for ( i = 0, j = 0; i < iMonoBlockSizeSam; i++, j += 2 )
            {
                // for the sum make sure we have more bits available (cast to
                // int32), after the normalization by 2, the result will fit
                // into the old size so that cast to int16 is safe
                vecsStereoSndCrd[i] = static_cast<int16_t> (
                    ( static_cast<int32_t> ( vecsStereoSndCrd[j] ) +
                      vecsStereoSndCrd[j + 1] ) / 2 );
            }
        }
    }
    else
    {
        if ( eAudioChannelConf == CC_STEREO )
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
                    vecsStereoSndCrd[j + 1] = Double2Short (
                        dAttFactStereo * vecsStereoSndCrd[j + 1] );
                }
            }
            else
            {
                for ( i = 0, j = 0; i < iMonoBlockSizeSam; i++, j += 2 )
                {
                    // attenuation on left channel
                    vecsStereoSndCrd[j] = Double2Short (
                        dAttFactStereo * vecsStereoSndCrd[j] );
                }
            }
        }
        else
        {
            // mono and mono-in/stereo out mode
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
                    // attenuation on right channel (store result in first half
                    // of the vector)
                    vecsStereoSndCrd[i] = Double2Short (
                        dAmplFactMono * vecsStereoSndCrd[j] +
                        dAttFactMono * vecsStereoSndCrd[j + 1] );
                }
            }
            else
            {
                for ( i = 0, j = 0; i < iMonoBlockSizeSam; i++, j += 2 )
                {
                    // attenuation on left channel (store result in first half
                    // of the vector)
                    vecsStereoSndCrd[i] = Double2Short (
                        dAmplFactMono * vecsStereoSndCrd[j + 1] +
                        dAttFactMono * vecsStereoSndCrd[j] );
                }
            }
        }
    }

    // Support for mono-in/stereo-out mode: Per definition this mode works in
    // full stereo mode at the transmission level. The only thing which is done
    // is to mix both sound card inputs together and then put this signal on
    // both stereo channels to be transmitted to the server.
    if ( eAudioChannelConf == CC_MONO_IN_STEREO_OUT )
    {
        // copy mono data in stereo sound card buffer (note that since the input
        // and output is the same buffer, we have to start from the end not to
        // overwrite input values)
        for ( i = iMonoBlockSizeSam - 1, j = iStereoBlockSizeSam - 2; i >= 0; i--, j -= 2 )
        {
            vecsStereoSndCrd[j] = vecsStereoSndCrd[j + 1] =
                vecsStereoSndCrd[i];
        }
    }

    for ( i = 0; i < iSndCrdFrameSizeFactor; i++ )
    {
        // encode current audio frame
        if ( ( eAudioCompressionType == CT_OPUS ) ||
             ( eAudioCompressionType == CT_OPUS64 ) )
        {
            if ( eAudioChannelConf == CC_MONO )
            {
                opus_custom_encode ( OpusEncoderMono,
                                     &vecsStereoSndCrd[i * SYSTEM_FRAME_SIZE_SAMPLES],
                                     SYSTEM_FRAME_SIZE_SAMPLES,
                                     &vecCeltData[0],
                                     iCeltNumCodedBytes );
            }
            else
            {
                opus_custom_encode ( OpusEncoderStereo,
                                     &vecsStereoSndCrd[i * 2 * SYSTEM_FRAME_SIZE_SAMPLES],
                                     SYSTEM_FRAME_SIZE_SAMPLES,
                                     &vecCeltData[0],
                                     iCeltNumCodedBytes );
            }
        }

        // send coded audio through the network
        Channel.PrepAndSendPacket ( &Socket,
                                    vecCeltData,
                                    iCeltNumCodedBytes );
    }


    // Receive signal ----------------------------------------------------------
    for ( i = 0; i < iSndCrdFrameSizeFactor; i++ )
    {
        // receive a new block
        const bool bReceiveDataOk =
            ( Channel.GetData ( vecbyNetwData, iCeltNumCodedBytes ) == GS_BUFFER_OK );

        // invalidate the buffer OK status flag if necessary
        if ( !bReceiveDataOk )
        {
            bJitterBufferOK = false;
        }

        // CELT decoding
        if ( bReceiveDataOk )
        {
            // on any valid received packet, we clear the initialization phase
            // flag
            bIsInitializationPhase = false;

            if ( ( eAudioCompressionType == CT_OPUS ) ||
                 ( eAudioCompressionType == CT_OPUS64 ) )
            {
                if ( eAudioChannelConf == CC_MONO )
                {
                    opus_custom_decode ( OpusDecoderMono,
                                         &vecbyNetwData[0],
                                         iCeltNumCodedBytes,
                                         &vecsAudioSndCrdMono[i * SYSTEM_FRAME_SIZE_SAMPLES],
                                         SYSTEM_FRAME_SIZE_SAMPLES );
                }
                else
                {
                    opus_custom_decode ( OpusDecoderStereo,
                                         &vecbyNetwData[0],
                                         iCeltNumCodedBytes,
                                         &vecsStereoSndCrd[i * 2 * SYSTEM_FRAME_SIZE_SAMPLES],
                                         SYSTEM_FRAME_SIZE_SAMPLES );
                }
            }
        }
        else
        {
            // lost packet
            if ( ( eAudioCompressionType == CT_OPUS ) ||
                 ( eAudioCompressionType == CT_OPUS64 ) )
            {
                if ( eAudioChannelConf == CC_MONO )
                {
                    opus_custom_decode ( OpusDecoderMono,
                                         nullptr,
                                         iCeltNumCodedBytes,
                                         &vecsAudioSndCrdMono[i * SYSTEM_FRAME_SIZE_SAMPLES],
                                         SYSTEM_FRAME_SIZE_SAMPLES );
                }
                else
                {
                    opus_custom_decode ( OpusDecoderStereo,
                                         nullptr,
                                         iCeltNumCodedBytes,
                                         &vecsStereoSndCrd[i * 2 * SYSTEM_FRAME_SIZE_SAMPLES],
                                         SYSTEM_FRAME_SIZE_SAMPLES );
                }
            }
        }
    }


/*
// TEST
// fid=fopen('v.dat','r');x=fread(fid,'int16');fclose(fid);
static FILE* pFileDelay = fopen("c:\\temp\\test2.dat", "wb");
short sData[2];
for (i = 0; i < iMonoBlockSizeSam; i++)
{
    sData[0] = (short) vecsAudioSndCrdMono[i];
    fwrite(&sData, size_t(2), size_t(1), pFileDelay);
}
fflush(pFileDelay);
*/


    // check if channel is connected and if we do not have the initialization
    // phase
    if ( Channel.IsConnected() && ( !bIsInitializationPhase ) )
    {
        if ( eAudioChannelConf == CC_MONO )
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

    // update socket buffer size
    Channel.UpdateSocketBufferSize();
}

int CClient::EstimatedOverallDelay ( const int iPingTimeMs )
{
    // If the jitter buffers are set effectively, i.e. they are exactly the
    // size of the network jitter, then the delay of the buffer is the buffer
    // length. Since that is usually not the case but the buffers are usually
    // a bit larger than necessary, we introduce some factor for compensation.
    // Consider the jitter buffer on the client and on the server side, too.
    const double dTotalJitterBufferDelayMs = SYSTEM_BLOCK_DURATION_MS_FLOAT *
        static_cast<double> ( GetSockBufNumFrames() +
                              GetServerSockBufNumFrames() ) * 0.7;

    // consider delay introduced by the sound card conversion buffer by using
    // "GetSndCrdConvBufAdditionalDelayMonoBlSize()"
    double dTotalSoundCardDelayMs = GetSndCrdConvBufAdditionalDelayMonoBlSize() *
        1000 / SYSTEM_SAMPLE_RATE_HZ;

    // try to get the actual input/output sound card delay from the audio
    // interface, per definition it is not available if a 0 is returned
    const double dSoundCardInputOutputLatencyMs = Sound.GetInOutLatencyMs();

    if ( dSoundCardInputOutputLatencyMs == 0.0 )
    {
        // use an alternative aproach for estimating the sound card delay:
        //
        // we assume that we have two period sizes for the input and one for the
        // output, therefore we have "3 *" instead of "2 *" (for input and output)
        // the actual sound card buffer size
        // "GetSndCrdConvBufAdditionalDelayMonoBlSize"
        dTotalSoundCardDelayMs +=
            ( 3 * GetSndCrdActualMonoBlSize() ) *
            1000 / SYSTEM_SAMPLE_RATE_HZ;
    }
    else
    {
        // add the actual sound card latency in ms
        dTotalSoundCardDelayMs += dSoundCardInputOutputLatencyMs;
    }

    // network packets are of the same size as the audio packets per definition
    // if no sound card conversion buffer is used
    const double dDelayToFillNetworkPacketsMs =
        GetSystemMonoBlSize() * 1000 / SYSTEM_SAMPLE_RATE_HZ;

    // CELT additional delay at small frame sizes is half a frame size
    const double dAdditionalAudioCodecDelayMs =
        SYSTEM_BLOCK_DURATION_MS_FLOAT / 2;

    const double dTotalBufferDelayMs =
        dDelayToFillNetworkPacketsMs +
        dTotalJitterBufferDelayMs +
        dTotalSoundCardDelayMs +
        dAdditionalAudioCodecDelayMs;

    return MathUtils::round ( dTotalBufferDelayMs + iPingTimeMs );
}
