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

#include "server.h"


// CHighPrecisionTimer implementation ******************************************
#ifdef _WIN32
CHighPrecisionTimer::CHighPrecisionTimer ( const bool bNewUseDoubleSystemFrameSize ) :
    bUseDoubleSystemFrameSize ( bNewUseDoubleSystemFrameSize )
{
    // add some error checking, the high precision timer implementation only
    // supports 64 and 128 samples frame size at 48 kHz sampling rate
#if ( SYSTEM_FRAME_SIZE_SAMPLES != 64 ) && ( DOUBLE_SYSTEM_FRAME_SIZE_SAMPLES != 128 )
# error "Only system frame size of 64 and 128 samples is supported by this module"
#endif
#if ( SYSTEM_SAMPLE_RATE_HZ != 48000 )
# error "Only a system sample rate of 48 kHz is supported by this module"
#endif

    // Since QT only supports a minimum timer resolution of 1 ms but for our
    // server we require a timer interval of 2.333 ms for 128 samples
    // frame size at 48 kHz sampling rate.
    // To support this interval, we use a timer with 2 ms resolution for 128
    // samples frame size and 1 ms resolution for 64 samples frame size.
    // Then we fire the actual frame timer if the error to the actual
    // required interval is minimum.
    veciTimeOutIntervals.Init ( 3 );

    // for 128 sample frame size at 48 kHz sampling rate with 2 ms timer resolution:
    // actual intervals:  0.0  2.666  5.333  8.0
    // quantized to 2 ms: 0    2      6      8 (0)
    // for 64 sample frame size at 48 kHz sampling rate with 1 ms timer resolution:
    // actual intervals:  0.0  1.333  2.666  4.0
    // quantized to 2 ms: 0    1      3      4 (0)
    veciTimeOutIntervals[0] = 0;
    veciTimeOutIntervals[1] = 1;
    veciTimeOutIntervals[2] = 0;

    // connect timer timeout signal
    QObject::connect ( &Timer, &QTimer::timeout,
        this, &CHighPrecisionTimer::OnTimer );
}

void CHighPrecisionTimer::Start()
{
    // reset position pointer and counter
    iCurPosInVector  = 0;
    iIntervalCounter = 0;

    if ( bUseDoubleSystemFrameSize )
    {
        // start internal timer with 2 ms resolution for 128 samples frame size
        Timer.start ( 2 );
    }
    else
    {
        // start internal timer with 1 ms resolution for 64 samples frame size
        Timer.start ( 1 );
    }
}

void CHighPrecisionTimer::Stop()
{
    // stop timer
    Timer.stop();
}

void CHighPrecisionTimer::OnTimer()
{
    // check if maximum number of high precision timer intervals are
    // finished
    if ( veciTimeOutIntervals[iCurPosInVector] == iIntervalCounter )
    {
        // reset interval counter
        iIntervalCounter = 0;

        // go to next position in vector, take care of wrap around
        iCurPosInVector++;
        if ( iCurPosInVector == veciTimeOutIntervals.Size() )
        {
            iCurPosInVector = 0;
        }

        // minimum time error to actual required timer interval is reached,
        // emit signal for server
        emit timeout();
    }
    else
    {
        // next high precision timer interval
        iIntervalCounter++;
    }
}
#else // Mac and Linux
CHighPrecisionTimer::CHighPrecisionTimer ( const bool bUseDoubleSystemFrameSize ) :
    bRun ( false )
{
    // calculate delay in ns
    uint64_t iNsDelay;

    if ( bUseDoubleSystemFrameSize )
    {
        iNsDelay = ( (uint64_t) DOUBLE_SYSTEM_FRAME_SIZE_SAMPLES * 1000000000 ) /
                   (uint64_t) SYSTEM_SAMPLE_RATE_HZ; // in ns
    }
    else
    {
        iNsDelay = ( (uint64_t) SYSTEM_FRAME_SIZE_SAMPLES * 1000000000 ) /
                   (uint64_t) SYSTEM_SAMPLE_RATE_HZ; // in ns
    }

#if defined ( __APPLE__ ) || defined ( __MACOSX )
    // calculate delay in mach absolute time
    struct mach_timebase_info timeBaseInfo;
    mach_timebase_info ( &timeBaseInfo );

    Delay = ( iNsDelay * (uint64_t) timeBaseInfo.denom ) /
        (uint64_t) timeBaseInfo.numer;
#else
    // set delay
    Delay = iNsDelay;
#endif
}

void CHighPrecisionTimer::Start()
{
    // only start if not already running
    if ( !bRun )
    {
        // set run flag
        bRun = true;

        // set initial end time
#if defined ( __APPLE__ ) || defined ( __MACOSX )
        NextEnd = mach_absolute_time() + Delay;
#else
        clock_gettime ( CLOCK_MONOTONIC, &NextEnd );

        NextEnd.tv_nsec += Delay;
        if ( NextEnd.tv_nsec >= 1000000000L )
        {
            NextEnd.tv_sec++;
            NextEnd.tv_nsec -= 1000000000L;
        }
#endif

        // start thread
        QThread::start ( QThread::TimeCriticalPriority );
    }
}

void CHighPrecisionTimer::Stop()
{
    // set flag so that thread can leave the main loop
    bRun = false;

    // give thread some time to terminate
    wait ( 5000 );
}

void CHighPrecisionTimer::run()
{
    // loop until the thread shall be terminated
    while ( bRun )
    {
        // call processing routine by fireing signal

// TODO by emit a signal we leave the high priority thread -> maybe use some
//      other connection type to have something like a true callback, e.g.
//      "Qt::DirectConnection" -> Can this work?

        emit timeout();

        // now wait until the next buffer shall be processed (we
        // use the "increment method" to make sure we do not introduce
        // a timing drift)
#if defined ( __APPLE__ ) || defined ( __MACOSX )
        mach_wait_until ( NextEnd );

        NextEnd += Delay;
#else
        clock_nanosleep ( CLOCK_MONOTONIC,
                          TIMER_ABSTIME,
                          &NextEnd,
                          NULL );

        NextEnd.tv_nsec += Delay;
        if ( NextEnd.tv_nsec >= 1000000000L )
        {
            NextEnd.tv_sec++;
            NextEnd.tv_nsec -= 1000000000L;
        }
#endif
    }
}
#endif


// CServer implementation ******************************************************
CServer::CServer ( const int          iNewMaxNumChan,
                   const QString&     strLoggingFileName,
                   const quint16      iPortNumber,
                   const QString&     strHTMLStatusFileName,
                   const QString&     strServerNameForHTMLStatusFile,
                   const QString&     strCentralServer,
                   const QString&     strServerInfo,
                   const QString&     strServerListFilter,
                   const QString&     strNewWelcomeMessage,
                   const QString&     strRecordingDirName,
                   const bool         bNCentServPingServerInList,
                   const bool         bNDisconnectAllClientsOnQuit,
                   const bool         bNUseDoubleSystemFrameSize,
                   const bool         bNUseMultithreading,
                   const ELicenceType eNLicenceType ) :
    bUseDoubleSystemFrameSize   ( bNUseDoubleSystemFrameSize ),
    bUseMultithreading          ( bNUseMultithreading ),
    iMaxNumChannels             ( iNewMaxNumChan ),
    Socket                      ( this, iPortNumber ),
    Logging                     ( ),
    iFrameCount                 ( 0 ),
    bWriteStatusHTMLFile        ( false ),
    HighPrecisionTimer          ( bNUseDoubleSystemFrameSize ),
    ServerListManager           ( iPortNumber,
                                  strCentralServer,
                                  strServerInfo,
                                  strServerListFilter,
                                  iNewMaxNumChan,
                                  bNCentServPingServerInList,
                                  &ConnLessProtocol ),
    bAutoRunMinimized           ( false ),
    eLicenceType                ( eNLicenceType ),
    bDisconnectAllClientsOnQuit ( bNDisconnectAllClientsOnQuit ),
    pSignalHandler              ( CSignalHandler::getSingletonP() )
{
    int iOpusError;
    int i;

    // create OPUS encoder/decoder for each channel (must be done before
    // enabling the channels), create a mono and stereo encoder/decoder
    // for each channel
    for ( i = 0; i < iMaxNumChannels; i++ )
    {
        // init OPUS -----------------------------------------------------------
        OpusMode[i] = opus_custom_mode_create ( SYSTEM_SAMPLE_RATE_HZ,
                                                DOUBLE_SYSTEM_FRAME_SIZE_SAMPLES,
                                                &iOpusError );

        Opus64Mode[i] = opus_custom_mode_create ( SYSTEM_SAMPLE_RATE_HZ,
                                                  SYSTEM_FRAME_SIZE_SAMPLES,
                                                  &iOpusError );

        // init audio encoders and decoders
        OpusEncoderMono[i]     = opus_custom_encoder_create ( OpusMode[i],   1, &iOpusError ); // mono encoder legacy
        OpusDecoderMono[i]     = opus_custom_decoder_create ( OpusMode[i],   1, &iOpusError ); // mono decoder legacy
        OpusEncoderStereo[i]   = opus_custom_encoder_create ( OpusMode[i],   2, &iOpusError ); // stereo encoder legacy
        OpusDecoderStereo[i]   = opus_custom_decoder_create ( OpusMode[i],   2, &iOpusError ); // stereo decoder legacy
        Opus64EncoderMono[i]   = opus_custom_encoder_create ( Opus64Mode[i], 1, &iOpusError ); // mono encoder OPUS64
        Opus64DecoderMono[i]   = opus_custom_decoder_create ( Opus64Mode[i], 1, &iOpusError ); // mono decoder OPUS64
        Opus64EncoderStereo[i] = opus_custom_encoder_create ( Opus64Mode[i], 2, &iOpusError ); // stereo encoder OPUS64
        Opus64DecoderStereo[i] = opus_custom_decoder_create ( Opus64Mode[i], 2, &iOpusError ); // stereo decoder OPUS64

        // we require a constant bit rate
        opus_custom_encoder_ctl ( OpusEncoderMono[i],     OPUS_SET_VBR ( 0 ) );
        opus_custom_encoder_ctl ( OpusEncoderStereo[i],   OPUS_SET_VBR ( 0 ) );
        opus_custom_encoder_ctl ( Opus64EncoderMono[i],   OPUS_SET_VBR ( 0 ) );
        opus_custom_encoder_ctl ( Opus64EncoderStereo[i], OPUS_SET_VBR ( 0 ) );

        // for 64 samples frame size we have to adjust the PLC behavior to avoid loud artifacts
        opus_custom_encoder_ctl ( Opus64EncoderMono[i],   OPUS_SET_PACKET_LOSS_PERC ( 35 ) );
        opus_custom_encoder_ctl ( Opus64EncoderStereo[i], OPUS_SET_PACKET_LOSS_PERC ( 35 ) );

        // we want as low delay as possible
        opus_custom_encoder_ctl ( OpusEncoderMono[i],     OPUS_SET_APPLICATION ( OPUS_APPLICATION_RESTRICTED_LOWDELAY ) );
        opus_custom_encoder_ctl ( OpusEncoderStereo[i],   OPUS_SET_APPLICATION ( OPUS_APPLICATION_RESTRICTED_LOWDELAY ) );
        opus_custom_encoder_ctl ( Opus64EncoderMono[i],   OPUS_SET_APPLICATION ( OPUS_APPLICATION_RESTRICTED_LOWDELAY ) );
        opus_custom_encoder_ctl ( Opus64EncoderStereo[i], OPUS_SET_APPLICATION ( OPUS_APPLICATION_RESTRICTED_LOWDELAY ) );

        // set encoder low complexity for legacy 128 samples frame size
        opus_custom_encoder_ctl ( OpusEncoderMono[i],   OPUS_SET_COMPLEXITY ( 1 ) );
        opus_custom_encoder_ctl ( OpusEncoderStereo[i], OPUS_SET_COMPLEXITY ( 1 ) );


        // init double-to-normal frame size conversion buffers -----------------
        // use worst case memory initialization to avoid allocating memory in
        // the time-critical thread
        DoubleFrameSizeConvBufIn[i].Init  ( 2 /* stereo */ * DOUBLE_SYSTEM_FRAME_SIZE_SAMPLES /* worst case buffer size */ );
        DoubleFrameSizeConvBufOut[i].Init ( 2 /* stereo */ * DOUBLE_SYSTEM_FRAME_SIZE_SAMPLES /* worst case buffer size */ );
    }

    // define colors for chat window identifiers
    vstrChatColors.Init ( 6 );
    vstrChatColors[0] = "mediumblue";
    vstrChatColors[1] = "red";
    vstrChatColors[2] = "darkorchid";
    vstrChatColors[3] = "green";
    vstrChatColors[4] = "maroon";
    vstrChatColors[5] = "coral";

    // set the server frame size
    if ( bUseDoubleSystemFrameSize )
    {
        iServerFrameSizeSamples = DOUBLE_SYSTEM_FRAME_SIZE_SAMPLES;
    }
    else
    {
        iServerFrameSizeSamples = SYSTEM_FRAME_SIZE_SAMPLES;
    }


    // To avoid audio clitches, in the entire realtime timer audio processing
    // routine including the ProcessData no memory must be allocated. Since we
    // do not know the required sizes for the vectors, we allocate memory for
    // the worst case here:

    // allocate worst case memory for the temporary vectors
    vecChanIDsCurConChan.Init          ( iMaxNumChannels );
    vecvecdGains.Init                  ( iMaxNumChannels );
    vecvecdPannings.Init               ( iMaxNumChannels );
    vecvecsData.Init                   ( iMaxNumChannels );
    vecvecsSendData.Init               ( iMaxNumChannels );
    vecvecsIntermediateProcBuf.Init    ( iMaxNumChannels );
    vecvecbyCodedData.Init             ( iMaxNumChannels );
    vecNumAudioChannels.Init           ( iMaxNumChannels );
    vecNumFrameSizeConvBlocks.Init     ( iMaxNumChannels );
    vecUseDoubleSysFraSizeConvBuf.Init ( iMaxNumChannels );
    vecAudioComprType.Init             ( iMaxNumChannels );

    for ( i = 0; i < iMaxNumChannels; i++ )
    {
        // init vectors storing information of all channels
        vecvecdGains[i].Init    ( iMaxNumChannels );
        vecvecdPannings[i].Init ( iMaxNumChannels );

        // we always use stereo audio buffers (which is the worst case)
        vecvecsData[i].Init ( 2 /* stereo */ * DOUBLE_SYSTEM_FRAME_SIZE_SAMPLES /* worst case buffer size */ );

        // (note that we only allocate iMaxNumChannels buffers for the send
        // and coded data because of the OMP implementation)
        vecvecsSendData[i].Init ( 2 /* stereo */ * DOUBLE_SYSTEM_FRAME_SIZE_SAMPLES /* worst case buffer size */ );

        // allocate worst case memory for intermediate processing buffers in double precision
        vecvecsIntermediateProcBuf[i].Init ( 2 /* stereo */ * DOUBLE_SYSTEM_FRAME_SIZE_SAMPLES /* worst case buffer size */ );

        // allocate worst case memory for the coded data
        vecvecbyCodedData[i].Init ( MAX_SIZE_BYTES_NETW_BUF );
    }

    // allocate worst case memory for the channel levels
    vecChannelLevels.Init ( iMaxNumChannels );

    // enable logging (if requested)
    if ( !strLoggingFileName.isEmpty() )
    {
        Logging.Start ( strLoggingFileName );
    }

    // HTML status file writing
    if ( !strHTMLStatusFileName.isEmpty() )
    {
        QString strCurServerNameForHTMLStatusFile = strServerNameForHTMLStatusFile;

        // if server name is empty, substitute a default name
        if ( strCurServerNameForHTMLStatusFile.isEmpty() )
        {
            strCurServerNameForHTMLStatusFile = "[server address]";
        }

        // (the static cast to integer of the port number is required so that it
        // works correctly under Linux)
        StartStatusHTMLFileWriting ( strHTMLStatusFileName,
            strCurServerNameForHTMLStatusFile + ":" +
            QString().number( static_cast<int> ( iPortNumber ) ) );
    }

    // manage welcome message: if the welcome message is a valid link to a local
    // file, the content of that file is used as the welcome message (#361)
    SetWelcomeMessage ( strNewWelcomeMessage ); // first use given text, may be overwritten

    if ( QFileInfo ( strNewWelcomeMessage ).exists() )
    {
        QFile file ( strNewWelcomeMessage );

        if ( file.open ( QIODevice::ReadOnly | QIODevice::Text ) )
        {
            // use entire file content for the welcome message
            SetWelcomeMessage ( file.readAll() );
        }
    }

    // enable jam recording (if requested) - kicks off the thread (note
    // that jam recorder needs the frame size which is given to the jam
    // recorder in the SetRecordingDir() function)
    SetRecordingDir ( strRecordingDirName );

    // enable all channels (for the server all channel must be enabled the
    // entire life time of the software)
    for ( i = 0; i < iMaxNumChannels; i++ )
    {
        vecChannels[i].SetEnable ( true );
    }


    // Connections -------------------------------------------------------------
    // connect timer timeout signal
    QObject::connect ( &HighPrecisionTimer, &CHighPrecisionTimer::timeout,
        this, &CServer::OnTimer );

    QObject::connect ( &ConnLessProtocol, &CProtocol::CLMessReadyForSending,
        this, &CServer::OnSendCLProtMessage );

    QObject::connect ( &ConnLessProtocol, &CProtocol::CLPingReceived,
        this, &CServer::OnCLPingReceived );

    QObject::connect ( &ConnLessProtocol, &CProtocol::CLPingWithNumClientsReceived,
        this, &CServer::OnCLPingWithNumClientsReceived );

    QObject::connect ( &ConnLessProtocol, &CProtocol::CLRegisterServerReceived,
        this, &CServer::OnCLRegisterServerReceived );

    QObject::connect ( &ConnLessProtocol, &CProtocol::CLRegisterServerExReceived,
        this, &CServer::OnCLRegisterServerExReceived );

    QObject::connect ( &ConnLessProtocol, &CProtocol::CLUnregisterServerReceived,
        this, &CServer::OnCLUnregisterServerReceived );

    QObject::connect ( &ConnLessProtocol, &CProtocol::CLReqServerList,
        this, &CServer::OnCLReqServerList );

    QObject::connect ( &ConnLessProtocol, &CProtocol::CLRegisterServerResp,
        this, &CServer::OnCLRegisterServerResp );

    QObject::connect ( &ConnLessProtocol, &CProtocol::CLSendEmptyMes,
        this, &CServer::OnCLSendEmptyMes );

    QObject::connect ( &ConnLessProtocol, &CProtocol::CLDisconnection,
        this, &CServer::OnCLDisconnection );

    QObject::connect ( &ConnLessProtocol, &CProtocol::CLReqVersionAndOS,
        this, &CServer::OnCLReqVersionAndOS );

    QObject::connect ( &ConnLessProtocol, &CProtocol::CLVersionAndOSReceived,
        this, &CServer::CLVersionAndOSReceived );

    QObject::connect ( &ConnLessProtocol, &CProtocol::CLReqConnClientsList,
        this, &CServer::OnCLReqConnClientsList );

    QObject::connect ( &ServerListManager, &CServerListManager::SvrRegStatusChanged,
        this, &CServer::SvrRegStatusChanged );

    QObject::connect ( &JamController, &recorder::CJamController::RestartRecorder,
        this, &CServer::RestartRecorder );

    QObject::connect ( &JamController, &recorder::CJamController::StopRecorder,
        this, &CServer::StopRecorder );

    QObject::connect ( &JamController, &recorder::CJamController::RecordingSessionStarted,
        this, &CServer::RecordingSessionStarted );

    QObject::connect ( &JamController, &recorder::CJamController::EndRecorderThread,
        this, &CServer::EndRecorderThread );

    QObject::connect ( this, &CServer::Stopped,
        &JamController, &recorder::CJamController::Stopped );

    QObject::connect ( this, &CServer::ClientDisconnected,
        &JamController, &recorder::CJamController::ClientDisconnected );

    qRegisterMetaType<CVector<int16_t>> ( "CVector<int16_t>" );
    QObject::connect ( this, &CServer::AudioFrame,
        &JamController, &recorder::CJamController::AudioFrame );

    QObject::connect ( QCoreApplication::instance(), &QCoreApplication::aboutToQuit,
        this, &CServer::OnAboutToQuit );

    QObject::connect ( pSignalHandler, &CSignalHandler::HandledSignal,
        this, &CServer::OnHandledSignal );

    connectChannelSignalsToServerSlots<MAX_NUM_CHANNELS>();

    // start the socket (it is important to start the socket after all
    // initializations and connections)
    Socket.Start();
}

template<unsigned int slotId>
inline void CServer::connectChannelSignalsToServerSlots()
{
    int iCurChanID = slotId - 1;

    void ( CServer::* pOnSendProtMessCh )( CVector<uint8_t> ) =
        &CServerSlots<slotId>::OnSendProtMessCh;

    void ( CServer::* pOnReqConnClientsListCh )() =
        &CServerSlots<slotId>::OnReqConnClientsListCh;

    void ( CServer::* pOnChatTextReceivedCh )( QString ) =
        &CServerSlots<slotId>::OnChatTextReceivedCh;

    void ( CServer::* pOnMuteStateHasChangedCh )( int, bool ) =
        &CServerSlots<slotId>::OnMuteStateHasChangedCh;

    void ( CServer::* pOnServerAutoSockBufSizeChangeCh )( int ) =
        &CServerSlots<slotId>::OnServerAutoSockBufSizeChangeCh;

    // send message
    QObject::connect ( &vecChannels[iCurChanID], &CChannel::MessReadyForSending,
                       this, pOnSendProtMessCh );

    // request connected clients list
    QObject::connect ( &vecChannels[iCurChanID], &CChannel::ReqConnClientsList,
                       this, pOnReqConnClientsListCh );

    // channel info has changed
    QObject::connect ( &vecChannels[iCurChanID], &CChannel::ChanInfoHasChanged,
                       this, &CServer::CreateAndSendChanListForAllConChannels );

    // chat text received
    QObject::connect ( &vecChannels[iCurChanID], &CChannel::ChatTextReceived,
                       this, pOnChatTextReceivedCh );

    // other mute state has changed
    QObject::connect ( &vecChannels[iCurChanID], &CChannel::MuteStateHasChanged,
                       this, pOnMuteStateHasChangedCh );

    // auto socket buffer size change
    QObject::connect ( &vecChannels[iCurChanID], &CChannel::ServerAutoSockBufSizeChange,
                       this, pOnServerAutoSockBufSizeChangeCh );

    connectChannelSignalsToServerSlots<slotId - 1>();
}

template<>
inline void CServer::connectChannelSignalsToServerSlots<0>() {}

void CServer::CreateAndSendJitBufMessage ( const int iCurChanID,
                                           const int iNNumFra )
{
    vecChannels[iCurChanID].CreateJitBufMes ( iNNumFra );
}

CServer::~CServer()
{
    for ( int i = 0; i < iMaxNumChannels; i++ )
    {
        // free audio encoders and decoders
        opus_custom_encoder_destroy ( OpusEncoderMono[i] );
        opus_custom_decoder_destroy ( OpusDecoderMono[i] );
        opus_custom_encoder_destroy ( OpusEncoderStereo[i] );
        opus_custom_decoder_destroy ( OpusDecoderStereo[i] );
        opus_custom_encoder_destroy ( Opus64EncoderMono[i] );
        opus_custom_decoder_destroy ( Opus64DecoderMono[i] );
        opus_custom_encoder_destroy ( Opus64EncoderStereo[i] );
        opus_custom_decoder_destroy ( Opus64DecoderStereo[i] );

        // free audio modes
        opus_custom_mode_destroy ( OpusMode[i] );
        opus_custom_mode_destroy ( Opus64Mode[i] );
    }
}

void CServer::SendProtMessage ( int iChID, CVector<uint8_t> vecMessage )
{
    // the protocol queries me to call the function to send the message
    // send it through the network
    Socket.SendPacket ( vecMessage, vecChannels[iChID].GetAddress() );
}

void CServer::OnNewConnection ( int          iChID,
                                CHostAddress RecHostAddr )
{
    QMutexLocker locker ( &Mutex );

    // inform the client about its own ID at the server (note that this
    // must be the first message to be sent for a new connection)
    vecChannels[iChID].CreateClientIDMes ( iChID );

    // on a new connection we query the network transport properties for the
    // audio packets (to use the correct network block size and audio
    // compression properties, etc.)
    vecChannels[iChID].CreateReqNetwTranspPropsMes();

    // this is a new connection, query the jitter buffer size we shall use
    // for this client (note that at the same time on a new connection the
    // client sends the jitter buffer size by default but maybe we have
    // reached a state where this did not happen because of network trouble,
    // client or server thinks that the connection was still active, etc.)
    vecChannels[iChID].CreateReqJitBufMes();

    // A new client connected to the server, the channel list
    // at all clients have to be updated. This is done by sending
    // a channel name request to the client which causes a channel
    // name message to be transmitted to the server. If the server
    // receives this message, the channel list will be automatically
    // updated (implicitly).
    //
    // Usually it is not required to send the channel list to the
    // client currently connecting since it automatically requests
    // the channel list on a new connection (as a result, he will
    // usually get the list twice which has no impact on functionality
    // but will only increase the network load a tiny little bit). But
    // in case the client thinks he is still connected but the server
    // was restartet, it is important that we send the channel list
    // at this place.
    vecChannels[iChID].CreateReqChanInfoMes();

    // send welcome message (if enabled)
    MutexWelcomeMessage.lock();
    {
        if ( !strWelcomeMessage.isEmpty() )
        {
            // create formatted server welcome message and send it just to
            // the client which just connected to the server
            const QString strWelcomeMessageFormated =
                "<b>Server Welcome Message:</b> " + strWelcomeMessage;

            vecChannels[iChID].CreateChatTextMes ( strWelcomeMessageFormated );
        }
    }
    MutexWelcomeMessage.unlock();

    // send licence request message (if enabled)
    if ( eLicenceType != LT_NO_LICENCE )
    {
        vecChannels[iChID].CreateLicReqMes ( eLicenceType );
    }

    // send version info (for, e.g., feature activation in the client)
    vecChannels[iChID].CreateVersionAndOSMes();

    // send recording state message on connection
    vecChannels[iChID].CreateRecorderStateMes ( JamController.GetRecorderState() );

    // reset the conversion buffers
    DoubleFrameSizeConvBufIn[iChID].Reset();
    DoubleFrameSizeConvBufOut[iChID].Reset();

    // logging of new connected channel
    Logging.AddNewConnection ( RecHostAddr.InetAddr, GetNumberOfConnectedClients() );
}

void CServer::OnServerFull ( CHostAddress RecHostAddr )
{
    // note: no mutex required here

    // inform the calling client that no channel is free
    ConnLessProtocol.CreateCLServerFullMes ( RecHostAddr );
}

void CServer::OnSendCLProtMessage ( CHostAddress     InetAddr,
                                    CVector<uint8_t> vecMessage )
{
    // the protocol queries me to call the function to send the message
    // send it through the network
    Socket.SendPacket ( vecMessage, InetAddr );
}

void CServer::OnCLDisconnection ( CHostAddress InetAddr )
{
    // check if the given address is actually a client which is connected to
    // this server, if yes, disconnect it
    const int iCurChanID = FindChannel ( InetAddr );

    if ( iCurChanID != INVALID_CHANNEL_ID )
    {
        vecChannels[iCurChanID].Disconnect();
    }
}

void CServer::OnAboutToQuit()
{
    // if enabled, disconnect all clients on quit
    if ( bDisconnectAllClientsOnQuit )
    {
        Mutex.lock();
        {
            for ( int i = 0; i < iMaxNumChannels; i++ )
            {
                if ( vecChannels[i].IsConnected() )
                {
                    ConnLessProtocol.CreateCLDisconnection ( vecChannels[i].GetAddress() );
                }
            }
        }
        Mutex.unlock(); // release mutex
    }

    Stop();

    // if server was registered at the central server, unregister on shutdown
    if ( GetServerListEnabled() )
    {
        UnregisterSlaveServer();
    }
}

void CServer::OnHandledSignal ( int sigNum )
{
    // show the signal number on the command line (note that this does not work for the Windows command line)
// TODO we should use the ConsoleWriterFactory() instead of qDebug()
    qDebug() << "OnHandledSignal: " << sigNum;

#ifdef _WIN32
    // Windows does not actually get OnHandledSignal triggered
    QCoreApplication::instance()->exit();
    Q_UNUSED ( sigNum )
#else
    switch ( sigNum )
    {
    case SIGUSR1:
        RequestNewRecording();
        break;

    case SIGUSR2:
        SetEnableRecording ( !JamController.GetRecordingEnabled() );
        break;

    case SIGINT:
    case SIGTERM:
        // This should trigger OnAboutToQuit
        QCoreApplication::instance()->exit();
        break;

    default:
        break;
    }
#endif
}

void CServer::Start()
{
    // only start if not already running
    if ( !IsRunning() )
    {
        // start timer
        HighPrecisionTimer.Start();

        // emit start signal
        emit Started();
    }
}

void CServer::Stop()
{
    // Under Mac we have the problem that the timer shutdown might
    // take some time and therefore we get a lot of "server stopped"
    // entries in the log. The following condition shall prevent this.
    // For the other OSs this should not hurt either.
    if ( IsRunning() )
    {
        // stop timer
        HighPrecisionTimer.Stop();

        // logging (add "server stopped" logging entry)
        Logging.AddServerStopped();

        // emit stopped signal
        emit Stopped();
    }
}

void CServer::OnTimer()
{
/*
static CTimingMeas JitterMeas ( 1000, "test2.dat" ); JitterMeas.Measure(); // TEST do a timer jitter measurement
*/
    // Get data from all connected clients -------------------------------------
    // some inits
    int  iUnused;
    int  iNumClients               = 0; // init connected client counter
    bool bChannelIsNowDisconnected = false;
    bool bUpdateChannelLevels      = false;
    bool bSendChannelLevels        = false;

    // Make put and get calls thread safe. Do not forget to unlock mutex
    // afterwards!
    Mutex.lock();
    {
        // first, get number and IDs of connected channels
        for ( int i = 0; i < iMaxNumChannels; i++ )
        {
            if ( vecChannels[i].IsConnected() )
            {
                // add ID and increment counter (note that the vector length is
                // according to the worst case scenario, if the number of
                // connected clients is less, only a subset of elements of this
                // vector are actually used and the others are dummy elements)
                vecChanIDsCurConChan[iNumClients] = i;
                iNumClients++;
            }
        }

        // process connected channels
        for ( int i = 0; i < iNumClients; i++ )
        {
            int                iClientFrameSizeSamples = 0; // initialize to avoid a compiler warning
            OpusCustomDecoder* CurOpusDecoder;
            unsigned char*     pCurCodedData;

            // get actual ID of current channel
            const int iCurChanID = vecChanIDsCurConChan[i];

            // get and store number of audio channels and compression type
            vecNumAudioChannels[i] = vecChannels[iCurChanID].GetNumAudioChannels();
            vecAudioComprType[i]   = vecChannels[iCurChanID].GetAudioCompressionType();

            // get info about required frame size conversion properties
            vecUseDoubleSysFraSizeConvBuf[i] = ( !bUseDoubleSystemFrameSize && ( vecAudioComprType[i] == CT_OPUS ) );

            if ( bUseDoubleSystemFrameSize && ( vecAudioComprType[i] == CT_OPUS64 ) )
            {
                vecNumFrameSizeConvBlocks[i] = 2;
            }
            else
            {
                vecNumFrameSizeConvBlocks[i] = 1;
            }

            // update conversion buffer size (nothing will happen if the size stays the same)
            if ( vecUseDoubleSysFraSizeConvBuf[i] )
            {
                DoubleFrameSizeConvBufIn[iCurChanID].SetBufferSize  ( DOUBLE_SYSTEM_FRAME_SIZE_SAMPLES * vecNumAudioChannels[i] );
                DoubleFrameSizeConvBufOut[iCurChanID].SetBufferSize ( DOUBLE_SYSTEM_FRAME_SIZE_SAMPLES * vecNumAudioChannels[i] );
            }

            // select the opus decoder and raw audio frame length
            if ( vecAudioComprType[i] == CT_OPUS )
            {
                iClientFrameSizeSamples = DOUBLE_SYSTEM_FRAME_SIZE_SAMPLES;

                if ( vecNumAudioChannels[i] == 1 )
                {
                    CurOpusDecoder = OpusDecoderMono[iCurChanID];
                }
                else
                {
                    CurOpusDecoder = OpusDecoderStereo[iCurChanID];
                }
            }
            else if ( vecAudioComprType[i] == CT_OPUS64 )
            {
                iClientFrameSizeSamples = SYSTEM_FRAME_SIZE_SAMPLES;

                if ( vecNumAudioChannels[i] == 1 )
                {
                    CurOpusDecoder = Opus64DecoderMono[iCurChanID];
                }
                else
                {
                    CurOpusDecoder = Opus64DecoderStereo[iCurChanID];
                }
            }
            else
            {
                CurOpusDecoder = nullptr;
            }

            // get gains of all connected channels
            for ( int j = 0; j < iNumClients; j++ )
            {
                // The second index of "vecvecdGains" does not represent
                // the channel ID! Therefore we have to use
                // "vecChanIDsCurConChan" to query the IDs of the currently
                // connected channels
                vecvecdGains[i][j] = vecChannels[iCurChanID].GetGain ( vecChanIDsCurConChan[j] );

                // consider audio fade-in
                vecvecdGains[i][j] *= vecChannels[vecChanIDsCurConChan[j]].GetFadeInGain();

                // panning
                vecvecdPannings[i][j] = vecChannels[iCurChanID].GetPan ( vecChanIDsCurConChan[j] );
            }

            // flag for updating channel levels (if at least one clients wants it)
            if ( vecChannels[iCurChanID].ChannelLevelsRequired() )
            {
                bUpdateChannelLevels = true;
            }

            // If the server frame size is smaller than the received OPUS frame size, we need a conversion
            // buffer which stores the large buffer.
            // Note that we have a shortcut here. If the conversion buffer is not needed, the boolean flag
            // is false and the Get() function is not called at all. Therefore if the buffer is not needed
            // we do not spend any time in the function but go directly inside the if condition.
            if ( ( vecUseDoubleSysFraSizeConvBuf[i] == 0 ) ||
                 !DoubleFrameSizeConvBufIn[iCurChanID].Get ( vecvecsData[i], SYSTEM_FRAME_SIZE_SAMPLES * vecNumAudioChannels[i] ) )
            {
                // get current number of OPUS coded bytes
                const int iCeltNumCodedBytes = vecChannels[iCurChanID].GetNetwFrameSize();

                for ( int iB = 0; iB < vecNumFrameSizeConvBlocks[i]; iB++ )
                {
                    // get data
                    const EGetDataStat eGetStat = vecChannels[iCurChanID].GetData ( vecvecbyCodedData[i], iCeltNumCodedBytes );

                    // if channel was just disconnected, set flag that connected
                    // client list is sent to all other clients
                    // and emit the client disconnected signal
                    if ( eGetStat == GS_CHAN_NOW_DISCONNECTED )
                    {
                        if ( JamController.GetRecordingEnabled() )
                        {
                            emit ClientDisconnected ( iCurChanID ); // TODO do this outside the mutex lock?
                        }

                        bChannelIsNowDisconnected = true;
                    }

                    // get pointer to coded data
                    if ( eGetStat == GS_BUFFER_OK )
                    {
                        pCurCodedData = &vecvecbyCodedData[i][0];
                    }
                    else
                    {
                        // for lost packets use null pointer as coded input data
                        pCurCodedData = nullptr;
                    }

                    // OPUS decode received data stream
                    if ( CurOpusDecoder != nullptr )
                    {
                        iUnused = opus_custom_decode ( CurOpusDecoder,
                                                       pCurCodedData,
                                                       iCeltNumCodedBytes,
                                                       &vecvecsData[i][iB * SYSTEM_FRAME_SIZE_SAMPLES * vecNumAudioChannels[i]],
                                                       iClientFrameSizeSamples );
                    }
                }

                // a new large frame is ready, if the conversion buffer is required, put it in the buffer
                // and read out the small frame size immediately for further processing
                if ( vecUseDoubleSysFraSizeConvBuf[i] != 0 )
                {
                    DoubleFrameSizeConvBufIn[iCurChanID].PutAll ( vecvecsData[i] );
                    DoubleFrameSizeConvBufIn[iCurChanID].Get ( vecvecsData[i], SYSTEM_FRAME_SIZE_SAMPLES * vecNumAudioChannels[i] );
                }
            }
        }

        // a channel is now disconnected, take action on it
        if ( bChannelIsNowDisconnected )
        {
            // update channel list for all currently connected clients
            CreateAndSendChanListForAllConChannels();
        }
    }
    Mutex.unlock(); // release mutex


    // Process data ------------------------------------------------------------
    // Check if at least one client is connected. If not, stop server until
    // one client is connected.
    if ( iNumClients > 0 )
    {
        // calculate levels for all connected clients
        if ( bUpdateChannelLevels )
        {
            bSendChannelLevels = CreateLevelsForAllConChannels ( iNumClients,
                                                                 vecNumAudioChannels,
                                                                 vecvecsData,
                                                                 vecChannelLevels );
        }

        for ( int iChanCnt = 0; iChanCnt < iNumClients; iChanCnt++ )
        {
            // get actual ID of current channel
            const int iCurChanID = vecChanIDsCurConChan[iChanCnt];

            // update socket buffer size
            vecChannels[iCurChanID].UpdateSocketBufferSize();

            // send channel levels
            if ( bSendChannelLevels && vecChannels[iCurChanID].ChannelLevelsRequired() )
            {
                ConnLessProtocol.CreateCLChannelLevelListMes ( vecChannels[iCurChanID].GetAddress(),
                                                               vecChannelLevels,
                                                               iNumClients );
            }

            // export the audio data for recording purpose
            if ( JamController.GetRecordingEnabled() )
            {
                emit AudioFrame ( iCurChanID,
                                  vecChannels[iCurChanID].GetName(),
                                  vecChannels[iCurChanID].GetAddress(),
                                  vecNumAudioChannels[iChanCnt],
                                  vecvecsData[iChanCnt] );
            }

            // processing without multithreading
            if ( !bUseMultithreading )
            {
                // generate a separate mix for each channel, OPUS encode the
                // audio data and transmit the network packet
                MixEncodeTransmitData ( iChanCnt, iNumClients );
            }
        }

        // processing with multithreading
        if ( bUseMultithreading )
        {
// TODO optimization of the MTBlockSize value
            const int iMTBlockSize = 20; // every 20 users a new thread is created
            const int iNumBlocks   = static_cast<int> ( std::ceil ( static_cast<double> ( iNumClients ) / iMTBlockSize ) );

            for ( int iBlockCnt = 0; iBlockCnt < iNumBlocks; iBlockCnt++ )
            {
                // Generate a separate mix for each channel, OPUS encode the
                // audio data and transmit the network packet. The work is
                // distributed over all available processor cores.
                // By using the future synchronizer we make sure that all
                // threads are done when we leave the timer callback function.
                const int iStartChanCnt = iBlockCnt * iMTBlockSize;
                const int iStopChanCnt  = std::min ( ( iBlockCnt + 1 ) * iMTBlockSize - 1, iNumClients - 1 );

                FutureSynchronizer.addFuture ( QtConcurrent::run ( this,
                                                                   &CServer::MixEncodeTransmitDataBlocks,
                                                                   iStartChanCnt,
                                                                   iStopChanCnt,
                                                                   iNumClients ) );
            }

            // make sure all concurrent run threads have finished when we leave this function
            FutureSynchronizer.waitForFinished();
            FutureSynchronizer.clearFutures();
        }
    }
    else
    {
        // Disable server if no clients are connected. In this case the server
        // does not consume any significant CPU when no client is connected.
        Stop();
    }

    Q_UNUSED ( iUnused )
}

void CServer::MixEncodeTransmitDataBlocks ( const int iStartChanCnt,
                                            const int iStopChanCnt,
                                            const int iNumClients )
{
    // loop over all channels in the current block, needed for multithreading support
    for ( int iChanCnt = iStartChanCnt; iChanCnt <= iStopChanCnt; iChanCnt++ )
    {
        MixEncodeTransmitData ( iChanCnt, iNumClients );
    }
}

/// @brief Mix all audio data from all clients together, encode and transmit
void CServer::MixEncodeTransmitData ( const int iChanCnt,
                                      const int iNumClients )
{
    int               i, j, k, iUnused;
    CVector<double>&  vecdIntermProcBuf = vecvecsIntermediateProcBuf[iChanCnt]; // use reference for faster access
    CVector<int16_t>& vecsSendData      = vecvecsSendData[iChanCnt];            // use reference for faster access

    // get actual ID of current channel
    const int iCurChanID = vecChanIDsCurConChan[iChanCnt];

    // init intermediate processing vector with zeros since we mix all channels on that vector
    vecdIntermProcBuf.Reset ( 0 );

    // distinguish between stereo and mono mode
    if ( vecNumAudioChannels[iChanCnt] == 1 )
    {
        // Mono target channel -------------------------------------------------
        for ( j = 0; j < iNumClients; j++ )
        {
            // get a reference to the audio data and gain of the current client
            const CVector<int16_t>& vecsData = vecvecsData[j];
            const double            dGain    = vecvecdGains[iChanCnt][j];

            // if channel gain is 1, avoid multiplication for speed optimization
            if ( dGain == static_cast<double> ( 1.0 ) )
            {
                if ( vecNumAudioChannels[j] == 1 )
                {
                    // mono
                    for ( i = 0; i < iServerFrameSizeSamples; i++ )
                    {
                        vecdIntermProcBuf[i] += vecsData[i];
                    }
                }
                else
                {
                    // stereo: apply stereo-to-mono attenuation
                    for ( i = 0, k = 0; i < iServerFrameSizeSamples; i++, k += 2 )
                    {
                        vecdIntermProcBuf[i] +=
                            ( static_cast<double> ( vecsData[k] ) + vecsData[k + 1] ) / 2;
                    }
                }
            }
            else
            {
                if ( vecNumAudioChannels[j] == 1 )
                {
                    // mono
                    for ( i = 0; i < iServerFrameSizeSamples; i++ )
                    {
                        vecdIntermProcBuf[i] += vecsData[i] * dGain;
                    }
                }
                else
                {
                    // stereo: apply stereo-to-mono attenuation
                    for ( i = 0, k = 0; i < iServerFrameSizeSamples; i++, k += 2 )
                    {
                        vecdIntermProcBuf[i] += dGain *
                            ( static_cast<double> ( vecsData[k] ) + vecsData[k + 1] ) / 2;
                    }
                }
            }
        }

        // convert from double to short with clipping
        for ( i = 0; i < iServerFrameSizeSamples; i++ )
        {
            vecsSendData[i] = Double2Short ( vecdIntermProcBuf[i] );
        }
    }
    else
    {
        // Stereo target channel -----------------------------------------------
        for ( j = 0; j < iNumClients; j++ )
        {
            // get a reference to the audio data and gain/pan of the current client
            const CVector<int16_t>& vecsData = vecvecsData[j];
            const double            dGain    = vecvecdGains[iChanCnt][j];
            const double            dPan     = vecvecdPannings[iChanCnt][j];

            // calculate combined gain/pan for each stereo channel where we define
            // the panning that center equals full gain for both channels
            const double dGainL = MathUtils::GetLeftPan ( dPan, false ) * dGain;
            const double dGainR = MathUtils::GetRightPan ( dPan, false ) * dGain;

            // if channel gain is 1, avoid multiplication for speed optimization
            if ( ( dGainL == static_cast<double> ( 1.0 ) ) && ( dGainR == static_cast<double> ( 1.0 ) ) )
            {
                if ( vecNumAudioChannels[j] == 1 )
                {
                    // mono: copy same mono data in both out stereo audio channels
                    for ( i = 0, k = 0; i < iServerFrameSizeSamples; i++, k += 2 )
                    {
                        // left/right channel
                        vecdIntermProcBuf[k]     += vecsData[i];
                        vecdIntermProcBuf[k + 1] += vecsData[i];
                    }
                }
                else
                {
                    // stereo
                    for ( i = 0; i < ( 2 * iServerFrameSizeSamples ); i++ )
                    {
                        vecdIntermProcBuf[i] += vecsData[i];
                    }
                }
            }
            else
            {
                if ( vecNumAudioChannels[j] == 1 )
                {
                    // mono: copy same mono data in both out stereo audio channels
                    for ( i = 0, k = 0; i < iServerFrameSizeSamples; i++, k += 2 )
                    {
                        // left/right channel
                        vecdIntermProcBuf[k]     += vecsData[i] * dGainL;
                        vecdIntermProcBuf[k + 1] += vecsData[i] * dGainR;
                    }
                }
                else
                {
                    // stereo
                    for ( i = 0; i < ( 2 * iServerFrameSizeSamples ); i += 2 )
                    {
                        // left/right channel
                        vecdIntermProcBuf[i]     += vecsData[i] *     dGainL;
                        vecdIntermProcBuf[i + 1] += vecsData[i + 1] * dGainR;
                    }
                }
            }
        }

        // convert from double to short with clipping
        for ( i = 0; i < ( 2 * iServerFrameSizeSamples ); i++ )
        {
            vecsSendData[i] = Double2Short ( vecdIntermProcBuf[i] );
        }
    }

    int                iClientFrameSizeSamples = 0; // initialize to avoid a compiler warning
    OpusCustomEncoder* pCurOpusEncoder         = nullptr;

    // get current number of CELT coded bytes
    const int iCeltNumCodedBytes = vecChannels[iCurChanID].GetNetwFrameSize();

    // select the opus encoder and raw audio frame length
    if ( vecAudioComprType[iChanCnt] == CT_OPUS )
    {
        iClientFrameSizeSamples = DOUBLE_SYSTEM_FRAME_SIZE_SAMPLES;

        if ( vecNumAudioChannels[iChanCnt] == 1 )
        {
            pCurOpusEncoder = OpusEncoderMono[iCurChanID];
        }
        else
        {
            pCurOpusEncoder = OpusEncoderStereo[iCurChanID];
        }
    }
    else if ( vecAudioComprType[iChanCnt] == CT_OPUS64 )
    {
        iClientFrameSizeSamples = SYSTEM_FRAME_SIZE_SAMPLES;

        if ( vecNumAudioChannels[iChanCnt] == 1 )
        {
            pCurOpusEncoder = Opus64EncoderMono[iCurChanID];
        }
        else
        {
            pCurOpusEncoder = Opus64EncoderStereo[iCurChanID];
        }
    }

    // If the server frame size is smaller than the received OPUS frame size, we need a conversion
    // buffer which stores the large buffer.
    // Note that we have a shortcut here. If the conversion buffer is not needed, the boolean flag
    // is false and the Get() function is not called at all. Therefore if the buffer is not needed
    // we do not spend any time in the function but go directly inside the if condition.
    if ( ( vecUseDoubleSysFraSizeConvBuf[iChanCnt] == 0 ) ||
         DoubleFrameSizeConvBufOut[iCurChanID].Put ( vecsSendData, SYSTEM_FRAME_SIZE_SAMPLES * vecNumAudioChannels[iChanCnt] ) )
    {
        if ( vecUseDoubleSysFraSizeConvBuf[iChanCnt] != 0 )
        {
            // get the large frame from the conversion buffer
            DoubleFrameSizeConvBufOut[iCurChanID].GetAll ( vecsSendData, DOUBLE_SYSTEM_FRAME_SIZE_SAMPLES * vecNumAudioChannels[iChanCnt] );
        }

        for ( int iB = 0; iB < vecNumFrameSizeConvBlocks[iChanCnt]; iB++ )
        {
            // OPUS encoding
            if ( pCurOpusEncoder != nullptr )
            {
// TODO find a better place than this: the setting does not change all the time so for speed
//      optimization it would be better to set it only if the network frame size is changed
opus_custom_encoder_ctl ( pCurOpusEncoder, OPUS_SET_BITRATE ( CalcBitRateBitsPerSecFromCodedBytes ( iCeltNumCodedBytes, iClientFrameSizeSamples ) ) );

                iUnused = opus_custom_encode ( pCurOpusEncoder,
                                               &vecsSendData[iB * SYSTEM_FRAME_SIZE_SAMPLES * vecNumAudioChannels[iChanCnt]],
                                               iClientFrameSizeSamples,
                                               &vecvecbyCodedData[iChanCnt][0],
                                               iCeltNumCodedBytes );
            }

            // send separate mix to current clients
            vecChannels[iCurChanID].PrepAndSendPacket ( &Socket,
                                                        vecvecbyCodedData[iChanCnt],
                                                        iCeltNumCodedBytes );
        }
    }

    Q_UNUSED ( iUnused )
}

CVector<CChannelInfo> CServer::CreateChannelList()
{
    CVector<CChannelInfo> vecChanInfo ( 0 );

    // look for free channels
    for ( int i = 0; i < iMaxNumChannels; i++ )
    {
        if ( vecChannels[i].IsConnected() )
        {
            // append channel ID, IP address and channel name to storing vectors
            vecChanInfo.Add ( CChannelInfo (
                i, // ID
                QHostAddress ( QHostAddress::Null ).toIPv4Address(), // use invalid IP address (for privacy reason, #316)
                vecChannels[i].GetChanInfo() ) );
        }
    }

    return vecChanInfo;
}

void CServer::CreateAndSendChanListForAllConChannels()
{
    // create channel list
    CVector<CChannelInfo> vecChanInfo ( CreateChannelList() );

    // now send connected channels list to all connected clients
    for ( int i = 0; i < iMaxNumChannels; i++ )
    {
        if ( vecChannels[i].IsConnected() )
        {
            // send message
            vecChannels[i].CreateConClientListMes ( vecChanInfo );
        }
    }

    // create status HTML file if enabled
    if ( bWriteStatusHTMLFile )
    {
        WriteHTMLChannelList();
    }
}

void CServer::CreateAndSendChanListForThisChan ( const int iCurChanID )
{
    // create channel list
    CVector<CChannelInfo> vecChanInfo ( CreateChannelList() );

    // now send connected channels list to the channel with the ID "iCurChanID"
    vecChannels[iCurChanID].CreateConClientListMes ( vecChanInfo );
}

void CServer::CreateAndSendChatTextForAllConChannels ( const int      iCurChanID,
                                                       const QString& strChatText )
{
    // Create message which is sent to all connected clients -------------------
    // get client name, if name is empty, use IP address instead
    QString ChanName = vecChannels[iCurChanID].GetName();

    // add time and name of the client at the beginning of the message text and
    // use different colors
    QString sCurColor = vstrChatColors[iCurChanID % vstrChatColors.Size()];

    const QString strActualMessageText =
        "<font color=""" + sCurColor + """>(" +
        QTime::currentTime().toString ( "hh:mm:ss AP" ) + ") <b>" +
        ChanName.toHtmlEscaped() +
        "</b></font> " + strChatText;


    // Send chat text to all connected clients ---------------------------------
    for ( int i = 0; i < iMaxNumChannels; i++ )
    {
        if ( vecChannels[i].IsConnected() )
        {
            // send message
            vecChannels[i].CreateChatTextMes ( strActualMessageText );
        }
    }
}

void CServer::CreateAndSendRecorderStateForAllConChannels()
{
    // get recorder state
    ERecorderState eRecorderState = JamController.GetRecorderState();

    // now send recorder state to all connected clients
    for ( int i = 0; i < iMaxNumChannels; i++ )
    {
        if ( vecChannels[i].IsConnected() )
        {
            // send message
            vecChannels[i].CreateRecorderStateMes ( eRecorderState );
        }
    }
}

void CServer::CreateOtherMuteStateChanged ( const int  iCurChanID,
                                            const int  iOtherChanID,
                                            const bool bIsMuted )
{
    if ( vecChannels[iOtherChanID].IsConnected() )
    {
        // send message
        vecChannels[iOtherChanID].CreateMuteStateHasChangedMes ( iCurChanID, bIsMuted );
    }
}

int CServer::GetFreeChan()
{
    // look for a free channel
    for ( int i = 0; i < iMaxNumChannels; i++ )
    {
        if ( !vecChannels[i].IsConnected() )
        {
            return i;
        }
    }

    // no free channel found, return invalid ID
    return INVALID_CHANNEL_ID;
}

int CServer::GetNumberOfConnectedClients()
{
    int iNumConnClients = 0;

    // check all possible channels for connection status
    for ( int i = 0; i < iMaxNumChannels; i++ )
    {
        if ( vecChannels[i].IsConnected() )
        {
            // this channel is connected, increment counter
            iNumConnClients++;
        }
    }

    return iNumConnClients;
}

int CServer::FindChannel ( const CHostAddress& CheckAddr )
{
    CHostAddress InetAddr;

    // check for all possible channels if IP is already in use
    for ( int i = 0; i < iMaxNumChannels; i++ )
    {
        // the "GetAddress" gives a valid address and returns true if the
        // channel is connected
        if ( vecChannels[i].GetAddress ( InetAddr ) )
        {
            // IP found, return channel number
            if ( InetAddr == CheckAddr )
            {
                return i;
            }
        }
    }

    // IP not found, return invalid ID
    return INVALID_CHANNEL_ID;
}

void CServer::OnProtcolCLMessageReceived ( int              iRecID,
                                           CVector<uint8_t> vecbyMesBodyData,
                                           CHostAddress     RecHostAddr )
{
    QMutexLocker locker ( &Mutex );

    // connection less messages are always processed
    ConnLessProtocol.ParseConnectionLessMessageBody ( vecbyMesBodyData,
                                                      iRecID,
                                                      RecHostAddr );
}

void CServer::OnProtcolMessageReceived ( int              iRecCounter,
                                         int              iRecID,
                                         CVector<uint8_t> vecbyMesBodyData,
                                         CHostAddress     RecHostAddr )
{
    QMutexLocker locker ( &Mutex );

    // find the channel with the received address
    const int iCurChanID = FindChannel ( RecHostAddr );

    // if the channel exists, apply the protocol message to the channel
    if ( iCurChanID != INVALID_CHANNEL_ID )
    {
        vecChannels[iCurChanID].PutProtcolData ( iRecCounter,
                                                 iRecID,
                                                 vecbyMesBodyData,
                                                 RecHostAddr );
    }
}

bool CServer::PutAudioData ( const CVector<uint8_t>& vecbyRecBuf,
                             const int               iNumBytesRead,
                             const CHostAddress&     HostAdr,
                             int&                    iCurChanID )
{
    QMutexLocker locker ( &Mutex );

    bool bNewConnection = false; // init return value
    bool bChanOK        = true;  // init with ok, might be overwritten

    // Get channel ID ------------------------------------------------------
    // check address
    iCurChanID = FindChannel ( HostAdr );

    if ( iCurChanID == INVALID_CHANNEL_ID )
    {
        // a new client is calling, look for free channel
        iCurChanID = GetFreeChan();

        if ( iCurChanID != INVALID_CHANNEL_ID )
        {
            // initialize current channel by storing the calling host
            // address
            vecChannels[iCurChanID].SetAddress ( HostAdr );

            // reset channel info
            vecChannels[iCurChanID].ResetInfo();

            // reset the channel gains of current channel, at the same
            // time reset gains of this channel ID for all other channels
            for ( int i = 0; i < iMaxNumChannels; i++ )
            {
                vecChannels[iCurChanID].SetGain ( i, 1.0 );

                // other channels (we do not distinguish the case if
                // i == iCurChanID for simplicity)
                vecChannels[i].SetGain ( iCurChanID, 1.0 );
            }
        }
        else
        {
            // no free channel available
            bChanOK = false;
        }
    }


    // Put received audio data in jitter buffer ----------------------------
    if ( bChanOK )
    {
        // put packet in socket buffer
        if ( vecChannels[iCurChanID].PutAudioData ( vecbyRecBuf,
                                                    iNumBytesRead,
                                                    HostAdr ) == PS_NEW_CONNECTION )
        {
            // in case we have a new connection return this information
            bNewConnection = true;
        }
    }

    // return the state if a new connection was happening
    return bNewConnection;
}

void CServer::GetConCliParam ( CVector<CHostAddress>& vecHostAddresses,
                               CVector<QString>&      vecsName,
                               CVector<int>&          veciJitBufNumFrames,
                               CVector<int>&          veciNetwFrameSizeFact )
{
    CHostAddress InetAddr;

    // init return values
    vecHostAddresses.Init      ( iMaxNumChannels );
    vecsName.Init              ( iMaxNumChannels );
    veciJitBufNumFrames.Init   ( iMaxNumChannels );
    veciNetwFrameSizeFact.Init ( iMaxNumChannels );

    // check all possible channels
    for ( int i = 0; i < iMaxNumChannels; i++ )
    {
        if ( vecChannels[i].GetAddress ( InetAddr ) )
        {
            // get requested data
            vecHostAddresses[i]      = InetAddr;
            vecsName[i]              = vecChannels[i].GetName();
            veciJitBufNumFrames[i]   = vecChannels[i].GetSockBufNumFrames();
            veciNetwFrameSizeFact[i] = vecChannels[i].GetNetwFrameSizeFact();
        }
    }
}

void CServer::SetEnableRecording ( bool bNewEnableRecording )
{
    JamController.SetEnableRecording ( bNewEnableRecording, IsRunning() );

    // the recording state may have changed, send recording state message
    CreateAndSendRecorderStateForAllConChannels();
}

void CServer::SetWelcomeMessage ( const QString& strNWelcMess )
{
    // we need a mutex to secure access
    QMutexLocker locker ( &MutexWelcomeMessage );
    strWelcomeMessage = strNWelcMess;

    // restrict welcome message to maximum allowed length
    strWelcomeMessage = strWelcomeMessage.left ( MAX_LEN_CHAT_TEXT );
}

void CServer::StartStatusHTMLFileWriting ( const QString& strNewFileName,
                                           const QString& strNewServerNameWithPort )
{
    // set important parameters
    strServerHTMLFileListName = strNewFileName;
    strServerNameWithPort     = strNewServerNameWithPort;

    // set flag
    bWriteStatusHTMLFile = true;

    // write initial file
    WriteHTMLChannelList();
}

void CServer::WriteHTMLChannelList()
{
    // prepare file and stream
    QFile serverFileListFile ( strServerHTMLFileListName );

    if ( !serverFileListFile.open ( QIODevice::WriteOnly | QIODevice::Text ) )
    {
        return;
    }

    QTextStream streamFileOut ( &serverFileListFile );
    streamFileOut << strServerNameWithPort.toHtmlEscaped() << endl << "<ul>" << endl;

    // depending on number of connected clients write list
    if ( GetNumberOfConnectedClients() == 0 )
    {
        // no clients are connected -> empty server
        streamFileOut << "  No client connected" << endl;
    }
    else
    {
        // write entry for each connected client
        for ( int i = 0; i < iMaxNumChannels; i++ )
        {
            if ( vecChannels[i].IsConnected() )
            {
                streamFileOut << "  <li>" << vecChannels[i].GetName().toHtmlEscaped() << "</li>" << endl;
            }
        }
    }

    // finish list
    streamFileOut << "</ul>" << endl;
}

void CServer::customEvent ( QEvent* pEvent )
{
    if ( pEvent->type() == QEvent::User + 11 )
    {
        const int iMessType = ( (CCustomEvent*) pEvent )->iMessType;

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

/// @brief Compute frame peak level for each client
bool CServer::CreateLevelsForAllConChannels ( const int                        iNumClients,
                                              const CVector<int>&              vecNumAudioChannels,
                                              const CVector<CVector<int16_t> > vecvecsData,
                                              CVector<uint16_t>&               vecLevelsOut )
{
    bool bLevelsWereUpdated = false;

    // low frequency updates
    if ( iFrameCount > CHANNEL_LEVEL_UPDATE_INTERVAL )
    {
        iFrameCount        = 0;
        bLevelsWereUpdated = true;

        for ( int j = 0; j < iNumClients; j++ )
        {
            // update and get signal level for meter in dB for each channel
            const double dCurSigLevelForMeterdB = vecChannels[vecChanIDsCurConChan[j]].
                UpdateAndGetLevelForMeterdB ( vecvecsData[j],
                                              iServerFrameSizeSamples,
                                              vecNumAudioChannels[j] > 1 );

            // map value to integer for transmission via the protocol (4 bit available)
            vecLevelsOut[j] = static_cast<uint16_t> ( ceil ( dCurSigLevelForMeterdB ) );
        }
    }

    // increment the frame counter needed for low frequency update trigger
    iFrameCount++;

    if ( bUseDoubleSystemFrameSize )
    {
        // additional increment needed for double frame size to get to the same time interval
        iFrameCount++;
    }

    return bLevelsWereUpdated;
}
