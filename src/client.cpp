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
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 *
\******************************************************************************/

#include "client.h"


/* Implementation *************************************************************/
CClient::CClient ( const quint16  iPortNumber,
                   const quint16  iQosNumber,
                   const QString& strConnOnStartupAddress,
                   const QString& strMIDISetup,
                   const bool     bNoAutoJackConnect,
                   const QString& strNClientName,
                   const bool     bNMuteMeInPersonalMix ) :
    ChannelInfo                      ( ),
    strClientName                    ( strNClientName ),
    Channel                          ( false ), /* we need a client channel -> "false" */
    CurOpusEncoder                   ( nullptr ),
    CurOpusDecoder                   ( nullptr ),
    eAudioCompressionType            ( CT_OPUS ),
    iCeltNumCodedBytes               ( OPUS_NUM_BYTES_MONO_LOW_QUALITY ),
    iOPUSFrameSizeSamples            ( DOUBLE_SYSTEM_FRAME_SIZE_SAMPLES ),
    eAudioQuality                    ( AQ_NORMAL ),
    eAudioChannelConf                ( CC_MONO ),
    iNumAudioChannels                ( 1 ),
    bIsInitializationPhase           ( true ),
    bMuteOutStream                   ( false ),
    fMuteOutStreamGain               ( 1.0f ),
    Socket                           ( &Channel, iPortNumber, iQosNumber ),
    Sound                            ( AudioCallback, this, strMIDISetup, bNoAutoJackConnect, strNClientName ),
    iAudioInFader                    ( AUD_FADER_IN_MIDDLE ),
    bReverbOnLeftChan                ( false ),
    iReverbLevel                     ( 0 ),
    iInputBoost                      ( 1 ),
    iSndCrdPrefFrameSizeFactor       ( FRAME_SIZE_FACTOR_DEFAULT ),
    iSndCrdFrameSizeFactor           ( FRAME_SIZE_FACTOR_DEFAULT ),
    bSndCrdConversionBufferRequired  ( false ),
    iSndCardMonoBlockSizeSamConvBuff ( 0 ),
    bFraSiFactPrefSupported          ( false ),
    bFraSiFactDefSupported           ( false ),
    bFraSiFactSafeSupported          ( false ),
    eGUIDesign                       ( GD_ORIGINAL ),
    bEnableOPUS64                    ( false ),
    bJitterBufferOK                  ( true ),
    bNuteMeInPersonalMix             ( bNMuteMeInPersonalMix ),
    iServerSockBufNumFrames          ( DEF_NET_BUF_SIZE_NUM_BL ),
    pSignalHandler                   ( CSignalHandler::getSingletonP() )
{
    int iOpusError;

    OpusMode = opus_custom_mode_create ( SYSTEM_SAMPLE_RATE_HZ,
                                         DOUBLE_SYSTEM_FRAME_SIZE_SAMPLES,
                                         &iOpusError );

    Opus64Mode = opus_custom_mode_create ( SYSTEM_SAMPLE_RATE_HZ,
                                           SYSTEM_FRAME_SIZE_SAMPLES,
                                           &iOpusError );

    // init audio encoders and decoders
    OpusEncoderMono     = opus_custom_encoder_create ( OpusMode,   1, &iOpusError ); // mono encoder legacy
    OpusDecoderMono     = opus_custom_decoder_create ( OpusMode,   1, &iOpusError ); // mono decoder legacy
    OpusEncoderStereo   = opus_custom_encoder_create ( OpusMode,   2, &iOpusError ); // stereo encoder legacy
    OpusDecoderStereo   = opus_custom_decoder_create ( OpusMode,   2, &iOpusError ); // stereo decoder legacy
    Opus64EncoderMono   = opus_custom_encoder_create ( Opus64Mode, 1, &iOpusError ); // mono encoder OPUS64
    Opus64DecoderMono   = opus_custom_decoder_create ( Opus64Mode, 1, &iOpusError ); // mono decoder OPUS64
    Opus64EncoderStereo = opus_custom_encoder_create ( Opus64Mode, 2, &iOpusError ); // stereo encoder OPUS64
    Opus64DecoderStereo = opus_custom_decoder_create ( Opus64Mode, 2, &iOpusError ); // stereo decoder OPUS64

    // we require a constant bit rate
    opus_custom_encoder_ctl ( OpusEncoderMono,     OPUS_SET_VBR ( 0 ) );
    opus_custom_encoder_ctl ( OpusEncoderStereo,   OPUS_SET_VBR ( 0 ) );
    opus_custom_encoder_ctl ( Opus64EncoderMono,   OPUS_SET_VBR ( 0 ) );
    opus_custom_encoder_ctl ( Opus64EncoderStereo, OPUS_SET_VBR ( 0 ) );

    // for 64 samples frame size we have to adjust the PLC behavior to avoid loud artifacts
    opus_custom_encoder_ctl ( Opus64EncoderMono,   OPUS_SET_PACKET_LOSS_PERC ( 35 ) );
    opus_custom_encoder_ctl ( Opus64EncoderStereo, OPUS_SET_PACKET_LOSS_PERC ( 35 ) );

    // we want as low delay as possible
    opus_custom_encoder_ctl ( OpusEncoderMono,     OPUS_SET_APPLICATION ( OPUS_APPLICATION_RESTRICTED_LOWDELAY ) );
    opus_custom_encoder_ctl ( OpusEncoderStereo,   OPUS_SET_APPLICATION ( OPUS_APPLICATION_RESTRICTED_LOWDELAY ) );
    opus_custom_encoder_ctl ( Opus64EncoderMono,   OPUS_SET_APPLICATION ( OPUS_APPLICATION_RESTRICTED_LOWDELAY ) );
    opus_custom_encoder_ctl ( Opus64EncoderStereo, OPUS_SET_APPLICATION ( OPUS_APPLICATION_RESTRICTED_LOWDELAY ) );

    // set encoder low complexity for legacy 128 samples frame size
    opus_custom_encoder_ctl ( OpusEncoderMono,   OPUS_SET_COMPLEXITY ( 1 ) );
    opus_custom_encoder_ctl ( OpusEncoderStereo, OPUS_SET_COMPLEXITY ( 1 ) );


    // Connections -------------------------------------------------------------
    // connections for the protocol mechanism
    QObject::connect ( &Channel, &CChannel::MessReadyForSending,
        this, &CClient::OnSendProtMessage );

    QObject::connect ( &Channel, &CChannel::DetectedCLMessage,
        this, &CClient::OnDetectedCLMessage );

    QObject::connect ( &Channel, &CChannel::ReqJittBufSize,
        this, &CClient::OnReqJittBufSize );

    QObject::connect ( &Channel, &CChannel::JittBufSizeChanged,
        this, &CClient::OnJittBufSizeChanged );

    QObject::connect ( &Channel, &CChannel::ReqChanInfo,
        this, &CClient::OnReqChanInfo );

    QObject::connect ( &Channel, &CChannel::ConClientListMesReceived,
        this, &CClient::ConClientListMesReceived );

    QObject::connect ( &Channel, &CChannel::Disconnected,
        this, &CClient::Disconnected );

    QObject::connect ( &Channel, &CChannel::NewConnection,
        this, &CClient::OnNewConnection );

    QObject::connect ( &Channel, &CChannel::ChatTextReceived,
        this, &CClient::ChatTextReceived );

    QObject::connect ( &Channel, &CChannel::ClientIDReceived,
        this, &CClient::OnClientIDReceived );

    QObject::connect ( &Channel, &CChannel::MuteStateHasChangedReceived,
        this, &CClient::MuteStateHasChangedReceived );

    QObject::connect ( &Channel, &CChannel::LicenceRequired,
        this, &CClient::LicenceRequired );

    QObject::connect ( &Channel, &CChannel::VersionAndOSReceived,
        this, &CClient::VersionAndOSReceived );

    QObject::connect ( &Channel, &CChannel::RecorderStateReceived,
        this, &CClient::RecorderStateReceived );

    QObject::connect ( &ConnLessProtocol, &CProtocol::CLMessReadyForSending,
        this, &CClient::OnSendCLProtMessage );

    QObject::connect ( &ConnLessProtocol, &CProtocol::CLServerListReceived,
        this, &CClient::CLServerListReceived );

    QObject::connect ( &ConnLessProtocol, &CProtocol::CLRedServerListReceived,
        this, &CClient::CLRedServerListReceived );

    QObject::connect ( &ConnLessProtocol, &CProtocol::CLConnClientsListMesReceived,
        this, &CClient::CLConnClientsListMesReceived );

    QObject::connect ( &ConnLessProtocol, &CProtocol::CLPingReceived,
        this, &CClient::OnCLPingReceived );

    QObject::connect ( &ConnLessProtocol, &CProtocol::CLPingWithNumClientsReceived,
        this, &CClient::OnCLPingWithNumClientsReceived );

    QObject::connect ( &ConnLessProtocol, &CProtocol::CLDisconnection ,
        this, &CClient::OnCLDisconnection );

    QObject::connect ( &ConnLessProtocol, &CProtocol::CLVersionAndOSReceived,
        this, &CClient::CLVersionAndOSReceived );

    QObject::connect ( &ConnLessProtocol, &CProtocol::CLChannelLevelListReceived,
        this, &CClient::CLChannelLevelListReceived );

    // other
    QObject::connect ( &Sound, &CSound::ReinitRequest,
        this, &CClient::OnSndCrdReinitRequest );

    QObject::connect ( &Sound, &CSound::ControllerInFaderLevel,
        this, &CClient::OnControllerInFaderLevel );

    QObject::connect ( &Sound, &CSound::ControllerInPanValue,
        this, &CClient::OnControllerInPanValue );

    QObject::connect ( &Sound, &CSound::ControllerInFaderIsSolo,
        this, &CClient::OnControllerInFaderIsSolo );

    QObject::connect ( &Sound, &CSound::ControllerInFaderIsMute,
        this, &CClient::OnControllerInFaderIsMute );

    QObject::connect ( &Socket, &CHighPrioSocket::InvalidPacketReceived,
        this, &CClient::OnInvalidPacketReceived );

    QObject::connect ( pSignalHandler, &CSignalHandler::HandledSignal,
        this, &CClient::OnHandledSignal );

    // start timer so that elapsed time works
    PreciseTime.start();

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

CClient::~CClient()
{
    // if we were running, stop sound device
    if ( Sound.IsRunning() )
    {
        Sound.Stop();
    }

    // free audio encoders and decoders
    opus_custom_encoder_destroy ( OpusEncoderMono );
    opus_custom_decoder_destroy ( OpusDecoderMono );
    opus_custom_encoder_destroy ( OpusEncoderStereo );
    opus_custom_decoder_destroy ( OpusDecoderStereo );
    opus_custom_encoder_destroy ( Opus64EncoderMono );
    opus_custom_decoder_destroy ( Opus64DecoderMono );
    opus_custom_encoder_destroy ( Opus64EncoderStereo );
    opus_custom_decoder_destroy ( Opus64DecoderStereo );

    // free audio modes
    opus_custom_mode_destroy ( OpusMode );
    opus_custom_mode_destroy ( Opus64Mode );
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
    // message could not be parsed, check if the packet comes
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

// TODO needed for compatibility to old servers >= 3.4.6 and <= 3.5.12
Channel.CreateReqChannelLevelListMes();
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

void CClient::SetRemoteChanGain ( const int   iId,
                                  const float fGain,
                                  const bool  bIsMyOwnFader )
{
    // if this gain is for my own channel, apply the value for the Mute Myself function
    if ( bIsMyOwnFader )
    {
        fMuteOutStreamGain = fGain;
    }

    Channel.SetRemoteChanGain ( iId, fGain );
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

void CClient::SetEnableOPUS64 ( const bool eNEnableOPUS64 )
{
    // init with new parameter, if client was running then first
    // stop it and restart again after new initialization
    const bool bWasRunning = Sound.IsRunning();
    if ( bWasRunning )
    {
        Sound.Stop();
    }

    // set new parameter
    bEnableOPUS64 = eNEnableOPUS64;
    Init();

    if ( bWasRunning )
    {
        Sound.Start();
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

QString CClient::SetSndCrdDev ( const QString strNewDev )
{
    // if client was running then first
    // stop it and restart again after new initialization
    const bool bWasRunning = Sound.IsRunning();
    if ( bWasRunning )
    {
        Sound.Stop();
    }

    const QString strError = Sound.SetDev ( strNewDev );

    // init again because the sound card actual buffer size might
    // be changed on new device
    Init();

    if ( bWasRunning )
    {
        // restart client
        Sound.Start();
    }

    // in case of an error inform the GUI about it
    if ( !strError.isEmpty() )
    {
        emit SoundDeviceChanged ( strError );
    }

    return strError;
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
    QString strError = "";

    // audio device notifications can come at any time and they are in a
    // different thread, therefore we need a mutex here
    MutexDriverReinit.lock();
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
                strError = Sound.SetDev ( Sound.GetDev() );
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
    MutexDriverReinit.unlock();

    // inform GUI about the sound card device change
    emit SoundDeviceChanged ( strError );
}

void CClient::OnHandledSignal ( int sigNum )
{
#ifdef _WIN32
    // Windows does not actually get OnHandledSignal triggered
    QCoreApplication::instance()->exit();
    Q_UNUSED ( sigNum )
#else
    switch ( sigNum )
    {
    case SIGINT:
    case SIGTERM:
        // if connected, terminate connection (needed for headless mode)
        if ( IsRunning() )
        {
            Stop();
        }

        // this should trigger OnAboutToQuit
        QCoreApplication::instance()->exit();
        break;

    default:
        break;
    }
#endif
}

void CClient::OnControllerInFaderLevel ( int iChannelIdx,
                                         int iValue )
{
    // in case of a headless client the faders cannot be moved so we need
    // to send the controller information directly to the server
#ifdef HEADLESS
    // only apply new fader level if channel index is valid
    if ( ( iChannelIdx >= 0 ) && ( iChannelIdx < MAX_NUM_CHANNELS ) )
    {
        SetRemoteChanGain ( iChannelIdx, MathUtils::CalcFaderGain ( iValue ), false );
    }
#endif

    emit ControllerInFaderLevel ( iChannelIdx, iValue );
}

void CClient::OnControllerInPanValue ( int iChannelIdx,
                                       int iValue )
{
    // in case of a headless client the panners cannot be moved so we need
    // to send the controller information directly to the server
#ifdef HEADLESS
    // channel index is valid
    SetRemoteChanPan ( iChannelIdx, static_cast<float>( iValue ) / AUD_MIX_PAN_MAX);
#endif

    emit ControllerInPanValue ( iChannelIdx, iValue );
}

void CClient::OnControllerInFaderIsSolo ( int iChannelIdx,
                                          bool bIsSolo )
{
    // in case of a headless client the buttons are not displayed so we need
    // to send the controller information directly to the server
#ifdef HEADLESS
    // FIXME: no idea what to do here.
#endif

    emit ControllerInFaderIsSolo ( iChannelIdx, bIsSolo );
}

void CClient::OnControllerInFaderIsMute ( int iChannelIdx,
                                          bool bIsMute )
{
    // in case of a headless client the buttons are not displayed so we need
    // to send the controller information directly to the server
#ifdef HEADLESS
    // FIXME: no idea what to do here.
#endif

    emit ControllerInFaderIsMute ( iChannelIdx, bIsMute );
}

void CClient::OnClientIDReceived ( int iChanID )
{
    // for headless mode we support to mute our own signal in the personal mix
    // (note that the check for headless is done in the main.cpp and must not
    // be checked here)
    if ( bNuteMeInPersonalMix )
    {
        SetRemoteChanGain ( iChanID, 0, false );
    }

    emit ClientIDReceived ( iChanID );
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
    const int iFraSizePreffered = SYSTEM_FRAME_SIZE_SAMPLES * FRAME_SIZE_FACTOR_PREFERRED;
    const int iFraSizeDefault   = SYSTEM_FRAME_SIZE_SAMPLES * FRAME_SIZE_FACTOR_DEFAULT;
    const int iFraSizeSafe      = SYSTEM_FRAME_SIZE_SAMPLES * FRAME_SIZE_FACTOR_SAFE;

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
    if ( ( ( iMonoBlockSizeSam == ( SYSTEM_FRAME_SIZE_SAMPLES * FRAME_SIZE_FACTOR_PREFERRED ) ) && bEnableOPUS64 ) ||
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
        // size as the current frame size.

        // store actual sound card buffer size (stereo)
        bSndCrdConversionBufferRequired  = true;
        iSndCardMonoBlockSizeSamConvBuff = iMonoBlockSizeSam;

        // overwrite block size factor by using one frame
        iSndCrdFrameSizeFactor = 1;
    }

    // select the OPUS frame size mode depending on current mono block size samples
    if ( bSndCrdConversionBufferRequired )
    {
        if ( ( iSndCardMonoBlockSizeSamConvBuff < DOUBLE_SYSTEM_FRAME_SIZE_SAMPLES ) && bEnableOPUS64 )
        {
            iMonoBlockSizeSam     = SYSTEM_FRAME_SIZE_SAMPLES;
            eAudioCompressionType = CT_OPUS64;
        }
        else
        {
            iMonoBlockSizeSam     = DOUBLE_SYSTEM_FRAME_SIZE_SAMPLES;
            eAudioCompressionType = CT_OPUS;
        }
    }
    else
    {
        if ( iMonoBlockSizeSam < DOUBLE_SYSTEM_FRAME_SIZE_SAMPLES  )
        {
            eAudioCompressionType = CT_OPUS64;
        }
        else
        {
            // since we use double size frame size for OPUS, we have to adjust the frame size factor
            iSndCrdFrameSizeFactor /= 2;
            eAudioCompressionType   = CT_OPUS;

        }
    }

    // inits for audio coding
    if ( eAudioCompressionType == CT_OPUS )
    {
        iOPUSFrameSizeSamples = DOUBLE_SYSTEM_FRAME_SIZE_SAMPLES;

        if ( eAudioChannelConf == CC_MONO )
        {
            CurOpusEncoder    = OpusEncoderMono;
            CurOpusDecoder    = OpusDecoderMono;
            iNumAudioChannels = 1;

            switch ( eAudioQuality )
            {
            case AQ_LOW:    iCeltNumCodedBytes = OPUS_NUM_BYTES_MONO_LOW_QUALITY_DBLE_FRAMESIZE;    break;
            case AQ_NORMAL: iCeltNumCodedBytes = OPUS_NUM_BYTES_MONO_NORMAL_QUALITY_DBLE_FRAMESIZE; break;
            case AQ_HIGH:   iCeltNumCodedBytes = OPUS_NUM_BYTES_MONO_HIGH_QUALITY_DBLE_FRAMESIZE;   break;
            }
        }
        else
        {
            CurOpusEncoder    = OpusEncoderStereo;
            CurOpusDecoder    = OpusDecoderStereo;
            iNumAudioChannels = 2;

            switch ( eAudioQuality )
            {
            case AQ_LOW:    iCeltNumCodedBytes = OPUS_NUM_BYTES_STEREO_LOW_QUALITY_DBLE_FRAMESIZE;    break;
            case AQ_NORMAL: iCeltNumCodedBytes = OPUS_NUM_BYTES_STEREO_NORMAL_QUALITY_DBLE_FRAMESIZE; break;
            case AQ_HIGH:   iCeltNumCodedBytes = OPUS_NUM_BYTES_STEREO_HIGH_QUALITY_DBLE_FRAMESIZE;   break;
            }
        }
    }
    else /* CT_OPUS64 */
    {
        iOPUSFrameSizeSamples = SYSTEM_FRAME_SIZE_SAMPLES;

        if ( eAudioChannelConf == CC_MONO )
        {
            CurOpusEncoder    = Opus64EncoderMono;
            CurOpusDecoder    = Opus64DecoderMono;
            iNumAudioChannels = 1;

            switch ( eAudioQuality )
            {
            case AQ_LOW:    iCeltNumCodedBytes = OPUS_NUM_BYTES_MONO_LOW_QUALITY;    break;
            case AQ_NORMAL: iCeltNumCodedBytes = OPUS_NUM_BYTES_MONO_NORMAL_QUALITY; break;
            case AQ_HIGH:   iCeltNumCodedBytes = OPUS_NUM_BYTES_MONO_HIGH_QUALITY;   break;
            }
        }
        else
        {
            CurOpusEncoder    = Opus64EncoderStereo;
            CurOpusDecoder    = Opus64DecoderStereo;
            iNumAudioChannels = 2;

            switch ( eAudioQuality )
            {
            case AQ_LOW:    iCeltNumCodedBytes = OPUS_NUM_BYTES_STEREO_LOW_QUALITY;    break;
            case AQ_NORMAL: iCeltNumCodedBytes = OPUS_NUM_BYTES_STEREO_NORMAL_QUALITY; break;
            case AQ_HIGH:   iCeltNumCodedBytes = OPUS_NUM_BYTES_STEREO_HIGH_QUALITY;   break;
            }
        }
    }

    // calculate stereo (two channels) buffer size
    iStereoBlockSizeSam = 2 * iMonoBlockSizeSam;

    vecCeltData.Init ( iCeltNumCodedBytes );
    vecZeros.Init ( iStereoBlockSizeSam, 0 );
    vecsStereoSndCrdMuteStream.Init ( iStereoBlockSizeSam );

    fMuteOutStreamGain = 1.0f;

    opus_custom_encoder_ctl ( CurOpusEncoder,
                              OPUS_SET_BITRATE (
                                  CalcBitRateBitsPerSecFromCodedBytes (
                                      iCeltNumCodedBytes, iOPUSFrameSizeSamples ) ) );

    // inits for network and channel
    vecbyNetwData.Init ( iCeltNumCodedBytes );

    // set the channel network properties
    Channel.SetAudioStreamProperties ( eAudioCompressionType,
                                       iCeltNumCodedBytes,
                                       iSndCrdFrameSizeFactor,
                                       iNumAudioChannels );

    // init reverberation
    AudioReverb.Init ( eAudioChannelConf,
                       iStereoBlockSizeSam,
                       SYSTEM_SAMPLE_RATE_HZ );

    // init the sound card conversion buffers
    if ( bSndCrdConversionBufferRequired )
    {
        // inits for conversion buffer (the size of the conversion buffer must
        // be the sum of input/output sizes which is the worst case fill level)
        const int iSndCardStereoBlockSizeSamConvBuff = 2 * iSndCardMonoBlockSizeSamConvBuff;
        const int iConBufSize                        = iStereoBlockSizeSam + iSndCardStereoBlockSizeSamConvBuff;

        SndCrdConversionBufferIn.Init  ( iConBufSize );
        SndCrdConversionBufferOut.Init ( iConBufSize );
        vecDataConvBuf.Init            ( iStereoBlockSizeSam );

        // the output conversion buffer must be filled with the inner
        // block size for initialization (this is the latency which is
        // introduced by the conversion buffer) to avoid buffer underruns
        SndCrdConversionBufferOut.Put ( vecZeros, iStereoBlockSizeSam );
    }

    // reset initialization phase flag and mute flag
    bIsInitializationPhase = true;
}

void CClient::AudioCallback ( CVector<int16_t>& psData, void* arg )
{
    // get the pointer to the object
    CClient* pMyClientObj = static_cast<CClient*> ( arg );

    // process audio data
    pMyClientObj->ProcessSndCrdAudioData ( psData );

/*
// TEST do a soundcard jitter measurement
static CTimingMeas JitterMeas ( 1000, "test2.dat" );
JitterMeas.Measure();
*/
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
    int            i, j, iUnused;
    unsigned char* pCurCodedData;


    // Transmit signal ---------------------------------------------------------

    if ( iInputBoost != 1 ) {
        // apply a general gain boost to all audio input:
        for ( i = 0, j = 0; i < iMonoBlockSizeSam; i++, j += 2 )
        {
            vecsStereoSndCrd[j + 1] = static_cast<int16_t> ( iInputBoost * vecsStereoSndCrd[j + 1] );
            vecsStereoSndCrd[j]     = static_cast<int16_t> ( iInputBoost * vecsStereoSndCrd[j] );
        }
    }

    // update stereo signal level meter (not needed in headless mode)
#ifndef HEADLESS
    SignalLevelMeter.Update ( vecsStereoSndCrd,
                              iMonoBlockSizeSam,
                              true );
#endif

    // add reverberation effect if activated
    if ( iReverbLevel != 0 )
    {
        AudioReverb.Process ( vecsStereoSndCrd,
                              bReverbOnLeftChan,
                              static_cast<float> ( iReverbLevel ) / AUD_REVERB_MAX / 4 );
    }

    // apply pan (audio fader) and mix mono signals
    if ( !( ( iAudioInFader == AUD_FADER_IN_MIDDLE ) && ( eAudioChannelConf == CC_STEREO ) ) )
    {
        // calculate pan gain in the range 0 to 1, where 0.5 is the middle position
        const float fPan = static_cast<float> ( iAudioInFader ) / AUD_FADER_IN_MAX;

        if ( eAudioChannelConf == CC_STEREO )
        {
            // for stereo only apply pan attenuation on one channel (same as pan in the server)
            const float fGainL = MathUtils::GetLeftPan  ( fPan, false );
            const float fGainR = MathUtils::GetRightPan ( fPan, false );

            for ( i = 0, j = 0; i < iMonoBlockSizeSam; i++, j += 2 )
            {
                // note that the gain is always <= 1, therefore a simple cast is
                // ok since we never can get an overload
                vecsStereoSndCrd[j + 1] = static_cast<int16_t> ( fGainR * vecsStereoSndCrd[j + 1] );
                vecsStereoSndCrd[j]     = static_cast<int16_t> ( fGainL * vecsStereoSndCrd[j] );
            }
        }
        else
        {
            // for mono implement a cross-fade between channels and mix them, for
            // mono-in/stereo-out use no attenuation in pan center
            const float fGainL = MathUtils::GetLeftPan  ( fPan, eAudioChannelConf != CC_MONO_IN_STEREO_OUT );
            const float fGainR = MathUtils::GetRightPan ( fPan, eAudioChannelConf != CC_MONO_IN_STEREO_OUT );

            for ( i = 0, j = 0; i < iMonoBlockSizeSam; i++, j += 2 )
            {
                // note that we need the Float2Short for stereo pan mode
                vecsStereoSndCrd[i] = Float2Short (
                    fGainL * vecsStereoSndCrd[j] + fGainR * vecsStereoSndCrd[j + 1] );
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
            vecsStereoSndCrd[j] = vecsStereoSndCrd[j + 1] = vecsStereoSndCrd[i];
        }
    }

    for ( i = 0; i < iSndCrdFrameSizeFactor; i++ )
    {
        // OPUS encoding
        if ( CurOpusEncoder != nullptr )
        {
            if ( bMuteOutStream )
            {
                iUnused = opus_custom_encode ( CurOpusEncoder,
                                               &vecZeros[i * iNumAudioChannels * iOPUSFrameSizeSamples],
                                               iOPUSFrameSizeSamples,
                                               &vecCeltData[0],
                                               iCeltNumCodedBytes );
            }
            else
            {
                iUnused = opus_custom_encode ( CurOpusEncoder,
                                               &vecsStereoSndCrd[i * iNumAudioChannels * iOPUSFrameSizeSamples],
                                               iOPUSFrameSizeSamples,
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
    // in case of mute stream, store local data
    if ( bMuteOutStream )
    {
        vecsStereoSndCrdMuteStream = vecsStereoSndCrd;
    }

    for ( i = 0; i < iSndCrdFrameSizeFactor; i++ )
    {
        // receive a new block
        const bool bReceiveDataOk =
            ( Channel.GetData ( vecbyNetwData, iCeltNumCodedBytes ) == GS_BUFFER_OK );

        // get pointer to coded data and manage the flags
        if ( bReceiveDataOk )
        {
            pCurCodedData = &vecbyNetwData[0];

            // on any valid received packet, we clear the initialization phase flag
            bIsInitializationPhase = false;
        }
        else
        {
            // for lost packets use null pointer as coded input data
            pCurCodedData = nullptr;

            // invalidate the buffer OK status flag
            bJitterBufferOK = false;
        }

        // OPUS decoding
        if ( CurOpusDecoder != nullptr )
        {
            iUnused = opus_custom_decode ( CurOpusDecoder,
                                           pCurCodedData,
                                           iCeltNumCodedBytes,
                                           &vecsStereoSndCrd[i * iNumAudioChannels * iOPUSFrameSizeSamples],
                                           iOPUSFrameSizeSamples );
        }
    }

    // for muted stream we have to add our local data here
    if ( bMuteOutStream )
    {
        for ( i = 0; i < iStereoBlockSizeSam; i++ )
        {
            vecsStereoSndCrd[i] = Float2Short (
                vecsStereoSndCrd[i] + vecsStereoSndCrdMuteStream[i] * fMuteOutStreamGain );
        }
    }

    // check if channel is connected and if we do not have the initialization phase
    if ( Channel.IsConnected() && ( !bIsInitializationPhase ) )
    {
        if ( eAudioChannelConf == CC_MONO )
        {
            // copy mono data in stereo sound card buffer (note that since the input
            // and output is the same buffer, we have to start from the end not to
            // overwrite input values)
            for ( i = iMonoBlockSizeSam - 1, j = iStereoBlockSizeSam - 2; i >= 0; i--, j -= 2 )
            {
                vecsStereoSndCrd[j] = vecsStereoSndCrd[j + 1] = vecsStereoSndCrd[i];
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

    Q_UNUSED ( iUnused )
}

int CClient::EstimatedOverallDelay ( const int iPingTimeMs )
{
    const float fSystemBlockDurationMs = static_cast<float> ( iOPUSFrameSizeSamples ) /
        SYSTEM_SAMPLE_RATE_HZ * 1000;

    // If the jitter buffers are set effectively, i.e. they are exactly the
    // size of the network jitter, then the delay of the buffer is the buffer
    // length. Since that is usually not the case but the buffers are usually
    // a bit larger than necessary, we introduce some factor for compensation.
    // Consider the jitter buffer on the client and on the server side, too.
    const float fTotalJitterBufferDelayMs = fSystemBlockDurationMs *
        ( GetSockBufNumFrames() + GetServerSockBufNumFrames() ) * 0.7f;

    // consider delay introduced by the sound card conversion buffer by using
    // "GetSndCrdConvBufAdditionalDelayMonoBlSize()"
    float fTotalSoundCardDelayMs = GetSndCrdConvBufAdditionalDelayMonoBlSize() *
        1000.0f / SYSTEM_SAMPLE_RATE_HZ;

    // try to get the actual input/output sound card delay from the audio
    // interface, per definition it is not available if a 0 is returned
    const float fSoundCardInputOutputLatencyMs = Sound.GetInOutLatencyMs();

    if ( fSoundCardInputOutputLatencyMs == 0.0f )
    {
        // use an alternative approach for estimating the sound card delay:
        //
        // we assume that we have two period sizes for the input and one for the
        // output, therefore we have "3 *" instead of "2 *" (for input and output)
        // the actual sound card buffer size
        // "GetSndCrdConvBufAdditionalDelayMonoBlSize"
        fTotalSoundCardDelayMs +=
            ( 3 * GetSndCrdActualMonoBlSize() ) *
            1000.0f / SYSTEM_SAMPLE_RATE_HZ;
    }
    else
    {
        // add the actual sound card latency in ms
        fTotalSoundCardDelayMs += fSoundCardInputOutputLatencyMs;
    }

    // network packets are of the same size as the audio packets per definition
    // if no sound card conversion buffer is used
    const float fDelayToFillNetworkPacketsMs =
        GetSystemMonoBlSize() * 1000.0f / SYSTEM_SAMPLE_RATE_HZ;

    // OPUS additional delay at small frame sizes is half a frame size
    const float fAdditionalAudioCodecDelayMs = fSystemBlockDurationMs / 2;

    const float fTotalBufferDelayMs =
        fDelayToFillNetworkPacketsMs +
        fTotalJitterBufferDelayMs +
        fTotalSoundCardDelayMs +
        fAdditionalAudioCodecDelayMs;

    return MathUtils::round ( fTotalBufferDelayMs + iPingTimeMs );
}
