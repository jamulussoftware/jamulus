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
    QObject::connect ( &Timer, SIGNAL ( timeout() ),
        this, SLOT ( OnTimer() ) );
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
                   const int          iMaxDaysHistory,
                   const QString&     strLoggingFileName,
                   const quint16      iPortNumber,
                   const QString&     strHTMLStatusFileName,
                   const QString&     strHistoryFileName,
                   const QString&     strServerNameForHTMLStatusFile,
                   const QString&     strCentralServer,
                   const QString&     strServerInfo,
                   const QString&     strNewWelcomeMessage,
                   const QString&     strRecordingDirName,
                   const bool         bNCentServPingServerInList,
                   const bool         bNDisconnectAllClientsOnQuit,
                   const bool         bNUseDoubleSystemFrameSize,
                   const ELicenceType eNLicenceType ) :
    bUseDoubleSystemFrameSize   ( bNUseDoubleSystemFrameSize ),
    iMaxNumChannels             ( iNewMaxNumChan ),
    Socket                      ( this, iPortNumber ),
    Logging                     ( iMaxDaysHistory ),
    JamRecorder                 ( strRecordingDirName ),
    bWriteStatusHTMLFile        ( false ),
    HighPrecisionTimer          ( bNUseDoubleSystemFrameSize ),
    ServerListManager           ( iPortNumber,
                                  strCentralServer,
                                  strServerInfo,
                                  iNewMaxNumChan,
                                  bNCentServPingServerInList,
                                  &ConnLessProtocol ),
    bAutoRunMinimized           ( false ),
    strWelcomeMessage           ( strNewWelcomeMessage ),
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

    // we always use stereo audio buffers (which is the worst case)
    vecsSendData.Init ( 2 /* stereo */ * DOUBLE_SYSTEM_FRAME_SIZE_SAMPLES /* worst case buffer size */ );

    // allocate worst case memory for the temporary vectors
    vecChanIDsCurConChan.Init          ( iMaxNumChannels );
    vecvecdGains.Init                  ( iMaxNumChannels );
    vecvecdPannings.Init               ( iMaxNumChannels );
    vecvecsData.Init                   ( iMaxNumChannels );
    vecNumAudioChannels.Init           ( iMaxNumChannels );
    vecNumFrameSizeConvBlocks.Init     ( iMaxNumChannels );
    vecUseDoubleSysFraSizeConvBuf.Init ( iMaxNumChannels );
    vecAudioComprType.Init             ( iMaxNumChannels );

    for ( i = 0; i < iMaxNumChannels; i++ )
    {
        // init vectors storing information of all channels
        vecvecdGains[i].Init ( iMaxNumChannels );
        vecvecdPannings[i].Init ( iMaxNumChannels );

        // we always use stereo audio buffers (see "vecsSendData")
        vecvecsData[i].Init ( 2 /* stereo */ * DOUBLE_SYSTEM_FRAME_SIZE_SAMPLES /* worst case buffer size */ );
    }

    // allocate worst case memory for the coded data
    vecbyCodedData.Init ( MAX_SIZE_BYTES_NETW_BUF );

    // allocate worst case memory for the channel levels
    vecChannelLevels.Init     ( iMaxNumChannels );

    // enable history graph (if requested)
    if ( !strHistoryFileName.isEmpty() )
    {
        Logging.EnableHistory ( strHistoryFileName );
    }

    // enable logging (if requested)
    if ( !strLoggingFileName.isEmpty() )
    {
        // in case the history is enabled and a logging file name is
        // given, parse the logging file for old entries which are then
        // added in the history on software startup
        if ( !strHistoryFileName.isEmpty() )
        {
            Logging.ParseLogFile ( strLoggingFileName );
        }

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
        StartStatusHTMLFileWriting ( strHTMLStatusFileName,
            strCurServerNameForHTMLStatusFile + ":" +
            QString().number ( static_cast<int> ( iPortNumber ) ) );
    }

    // enable jam recording (if requested) - kicks off the thread
    if ( !strRecordingDirName.isEmpty() )
    {
        bRecorderInitialised = JamRecorder.Init ( this, iServerFrameSizeSamples );
        bEnableRecording     = bRecorderInitialised;
    }

    // enable all channels (for the server all channel must be enabled the
    // entire life time of the software)
    for ( i = 0; i < iMaxNumChannels; i++ )
    {
        vecChannels[i].SetEnable ( true );
    }


    // Connections -------------------------------------------------------------
    // connect timer timeout signal
    QObject::connect ( &HighPrecisionTimer, SIGNAL ( timeout() ),
        this, SLOT ( OnTimer() ) );

    QObject::connect ( &ConnLessProtocol,
        SIGNAL ( CLMessReadyForSending ( CHostAddress, CVector<uint8_t> ) ),
        this, SLOT ( OnSendCLProtMessage ( CHostAddress, CVector<uint8_t> ) ) );

    QObject::connect ( &ConnLessProtocol,
        SIGNAL ( CLPingReceived ( CHostAddress, int ) ),
        this, SLOT ( OnCLPingReceived ( CHostAddress, int ) ) );

    QObject::connect ( &ConnLessProtocol,
        SIGNAL ( CLPingWithNumClientsReceived ( CHostAddress, int, int ) ),
        this, SLOT ( OnCLPingWithNumClientsReceived ( CHostAddress, int, int ) ) );

    QObject::connect ( &ConnLessProtocol,
        SIGNAL ( CLRegisterServerReceived ( CHostAddress, CHostAddress, CServerCoreInfo ) ),
        this, SLOT ( OnCLRegisterServerReceived ( CHostAddress, CHostAddress, CServerCoreInfo ) ) );

    QObject::connect ( &ConnLessProtocol,
        SIGNAL ( CLUnregisterServerReceived ( CHostAddress ) ),
        this, SLOT ( OnCLUnregisterServerReceived ( CHostAddress ) ) );

    QObject::connect ( &ConnLessProtocol,
        SIGNAL ( CLReqServerList ( CHostAddress ) ),
        this, SLOT ( OnCLReqServerList ( CHostAddress ) ) );

    QObject::connect ( &ConnLessProtocol,
        SIGNAL ( CLRegisterServerResp ( CHostAddress, ESvrRegResult ) ),
        this, SLOT ( OnCLRegisterServerResp ( CHostAddress, ESvrRegResult ) ) );

    QObject::connect ( &ConnLessProtocol,
        SIGNAL ( CLSendEmptyMes ( CHostAddress ) ),
        this, SLOT ( OnCLSendEmptyMes ( CHostAddress ) ) );

    QObject::connect ( &ConnLessProtocol,
        SIGNAL ( CLDisconnection ( CHostAddress ) ),
        this, SLOT ( OnCLDisconnection ( CHostAddress ) ) );

    QObject::connect ( &ConnLessProtocol,
        SIGNAL ( CLReqVersionAndOS ( CHostAddress ) ),
        this, SLOT ( OnCLReqVersionAndOS ( CHostAddress ) ) );

    QObject::connect ( &ConnLessProtocol,
        SIGNAL ( CLReqConnClientsList ( CHostAddress ) ),
        this, SLOT ( OnCLReqConnClientsList ( CHostAddress ) ) );

    QObject::connect ( &ServerListManager,
       SIGNAL ( SvrRegStatusChanged() ),
       SIGNAL ( SvrRegStatusChanged() ) );

    QObject::connect ( &JamRecorder,
        SIGNAL ( RecordingSessionStarted ( QString ) ),
        SIGNAL ( RecordingSessionStarted ( QString ) ) );

    QObject::connect ( QCoreApplication::instance(),
        SIGNAL ( aboutToQuit() ),
        this, SLOT ( OnAboutToQuit() ) );

    QObject::connect ( pSignalHandler,
        SIGNAL ( HandledSignal ( int ) ),
        this, SLOT ( OnHandledSignal ( int ) ) );

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
};

template<>
inline void CServer::connectChannelSignalsToServerSlots<0>() {};

void CServer::CreateAndSendJitBufMessage ( const int iCurChanID,
                                           const int iNNumFra )
{
    vecChannels[iCurChanID].CreateJitBufMes ( iNNumFra );
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
    // updated (implicitely).
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
    if ( !strWelcomeMessage.isEmpty() )
    {
        // create formated server welcome message and send it just to
        // the client which just connected to the server
        const QString strWelcomeMessageFormated =
            "<b>Server Welcome Message:</b> " + strWelcomeMessage;

        vecChannels[iChID].CreateChatTextMes ( strWelcomeMessageFormated );
    }

    // send licence request message (if enabled)
    if ( eLicenceType != LT_NO_LICENCE )
    {
        vecChannels[iChID].CreateLicReqMes ( eLicenceType );
    }

    // send version info (for, e.g., feature activation in the client)
    vecChannels[iChID].CreateVersionAndOSMes();

    // A new client connected to the server and needs to know whether
    // its audio is being recorded or not.  Subsequent changes
    // will be sent out as and when they happen.
    ConnLessProtocol.CreateCLRecorderStateChange ( RecHostAddr, GetRecorderState() );

    // reset the conversion buffers
    DoubleFrameSizeConvBufIn[iChID].Reset();
    DoubleFrameSizeConvBufOut[iChID].Reset();

    // logging of new connected channel
    Logging.AddNewConnection ( RecHostAddr.InetAddr );
}

void CServer::OnServerFull ( CHostAddress RecHostAddr )
{
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

void CServer::OnProtcolCLMessageReceived ( int              iRecID,
                                           CVector<uint8_t> vecbyMesBodyData,
                                           CHostAddress     RecHostAddr )
{
    // connection less messages are always processed
    ConnLessProtocol.ParseConnectionLessMessageBody ( vecbyMesBodyData,
                                                      iRecID,
                                                      RecHostAddr );
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
        SetEnableRecording ( !bEnableRecording );
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

void CServer::RequestNewRecording()
{
    if ( bRecorderInitialised && bEnableRecording )
    {
        emit RestartRecorder();
    }
}

void CServer::SetEnableRecording ( bool bNewEnableRecording )
{
    if ( bRecorderInitialised )
    {
        bEnableRecording = bNewEnableRecording;

        if ( !bEnableRecording )
        {
            emit StopRecorder();
        }
        else if ( !IsRunning() )
        {
            // This dirty hack is for the GUI.  It doesn't care.
            emit StopRecorder();
        }
    }

    CreateAndSendRecorderStateForAllConChannels();
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

        CreateAndSendRecorderStateForAllConChannels();
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

        // by definition there are no connected channels...
        CreateAndSendRecorderStateForAllConChannels();
    }
}

void CServer::OnTimer()
{
    int                i, j, iUnused;
    int                iClientFrameSizeSamples = 0; // initialize to avoid a compiler warning
    OpusCustomDecoder* CurOpusDecoder;
    OpusCustomEncoder* CurOpusEncoder;
    unsigned char*     pCurCodedData;

/*
// TEST do a timer jitter measurement
static CTimingMeas JitterMeas ( 1000, "test2.dat" );
JitterMeas.Measure();
*/

    // Get data from all connected clients -------------------------------------
    // some inits
    int  iNumClients               = 0; // init connected client counter
    bool bChannelIsNowDisconnected = false;
    bool bSendChannelLevels        = false;

    // Make put and get calls thread safe. Do not forget to unlock mutex
    // afterwards!
    Mutex.lock();
    {
        // first, get number and IDs of connected channels
        for ( i = 0; i < iMaxNumChannels; i++ )
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
        for ( i = 0; i < iNumClients; i++ )
        {
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
                DoubleFrameSizeConvBufIn[iCurChanID].SetBufferSize  ( DOUBLE_SYSTEM_FRAME_SIZE_SAMPLES  * vecNumAudioChannels[i] );
                DoubleFrameSizeConvBufOut[iCurChanID].SetBufferSize ( DOUBLE_SYSTEM_FRAME_SIZE_SAMPLES  * vecNumAudioChannels[i] );
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
            for ( j = 0; j < iNumClients; j++ )
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
                    const EGetDataStat eGetStat = vecChannels[iCurChanID].GetData ( vecbyCodedData, iCeltNumCodedBytes );

                    // if channel was just disconnected, set flag that connected
                    // client list is sent to all other clients
                    // and emit the client disconnected signal
                    if ( eGetStat == GS_CHAN_NOW_DISCONNECTED )
                    {
                        if ( bEnableRecording )
                        {
                            emit ClientDisconnected ( iCurChanID ); // TODO do this outside the mutex lock?
                        }

                        bChannelIsNowDisconnected = true;
                    }

                    // get pointer to coded data
                    if ( eGetStat == GS_BUFFER_OK )
                    {
                        pCurCodedData = &vecbyCodedData[0];
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
        // ultra-low frequency updates
        if ( iULFFrameCount > ULTRA_LOW_FREQUENCY_LIMIT )
        {
            iULFFrameCount = 0;

            // things can go missing, so send this every now and then
            CreateAndSendRecorderStateForAllConChannels();
        }
        // low frequency updates
        if ( iFrameCount > LOW_FREQUENCY_UPDATE_INTERVAL )
        {
            iFrameCount = 0;

            // Calculate channel levels if any client has requested them
            for ( int i = 0; i < iNumClients; i++ )
            {
                if ( vecChannels[vecChanIDsCurConChan[i]].ChannelLevelsRequired() )
                {
                    bSendChannelLevels = true;

                    CreateLevelsForAllConChannels ( iNumClients,
                                                    vecNumAudioChannels,
                                                    vecvecsData,
                                                    vecChannelLevels );
                    break;
                }
            }

            iULFFrameCount++;
        }
        iFrameCount++;
        if ( bUseDoubleSystemFrameSize )
        {
            // additional increment needed for double frame size to get to the same time interval
            iFrameCount++;
        }

        for ( int i = 0; i < iNumClients; i++ )
        {
            // get actual ID of current channel
            const int iCurChanID = vecChanIDsCurConChan[i];

            // get number of audio channels of current channel
            const int iCurNumAudChan = vecNumAudioChannels[i];

            // export the audio data for recording purpose
            if ( bEnableRecording )
            {
                emit AudioFrame ( iCurChanID,
                                  vecChannels[iCurChanID].GetName(),
                                  vecChannels[iCurChanID].GetAddress(),
                                  iCurNumAudChan,
                                  vecvecsData[i] );
            }

            // generate a sparate mix for each channel
            // actual processing of audio data -> mix
            ProcessData ( vecvecsData,
                          vecvecdGains[i],
                          vecvecdPannings[i],
                          vecNumAudioChannels,
                          vecsSendData,
                          iCurNumAudChan,
                          iNumClients );

            // get current number of CELT coded bytes
            const int iCeltNumCodedBytes = vecChannels[iCurChanID].GetNetwFrameSize();

            // select the opus encoder and raw audio frame length
            if ( vecAudioComprType[i] == CT_OPUS )
            {
                iClientFrameSizeSamples = DOUBLE_SYSTEM_FRAME_SIZE_SAMPLES;

                if ( vecNumAudioChannels[i] == 1 )
                {
                    CurOpusEncoder = OpusEncoderMono[iCurChanID];
                }
                else
                {
                    CurOpusEncoder = OpusEncoderStereo[iCurChanID];
                }
            }
            else if ( vecAudioComprType[i] == CT_OPUS64 )
            {
                iClientFrameSizeSamples = SYSTEM_FRAME_SIZE_SAMPLES;

                if ( vecNumAudioChannels[i] == 1 )
                {
                    CurOpusEncoder = Opus64EncoderMono[iCurChanID];
                }
                else
                {
                    CurOpusEncoder = Opus64EncoderStereo[iCurChanID];
                }
            }
            else
            {
                CurOpusEncoder = nullptr;
            }

            // If the server frame size is smaller than the received OPUS frame size, we need a conversion
            // buffer which stores the large buffer.
            // Note that we have a shortcut here. If the conversion buffer is not needed, the boolean flag
            // is false and the Get() function is not called at all. Therefore if the buffer is not needed
            // we do not spend any time in the function but go directly inside the if condition.
            if ( ( vecUseDoubleSysFraSizeConvBuf[i] == 0 ) ||
                 DoubleFrameSizeConvBufOut[iCurChanID].Put ( vecsSendData, SYSTEM_FRAME_SIZE_SAMPLES * vecNumAudioChannels[i] ) )
            {
                if ( vecUseDoubleSysFraSizeConvBuf[i] != 0 )
                {
                    // get the large frame from the conversion buffer
                    DoubleFrameSizeConvBufOut[iCurChanID].GetAll ( vecsSendData, DOUBLE_SYSTEM_FRAME_SIZE_SAMPLES * vecNumAudioChannels[i] );
                }

                for ( int iB = 0; iB < vecNumFrameSizeConvBlocks[i]; iB++ )
                {
                    // OPUS encoding
                    if ( CurOpusEncoder != nullptr )
                    {
// TODO find a better place than this: the setting does not change all the time
//      so for speed optimization it would be better to set it only if the network
//      frame size is changed
opus_custom_encoder_ctl ( CurOpusEncoder,
                          OPUS_SET_BITRATE ( CalcBitRateBitsPerSecFromCodedBytes ( iCeltNumCodedBytes, iClientFrameSizeSamples ) ) );

                        iUnused = opus_custom_encode ( CurOpusEncoder,
                                                       &vecsSendData[iB * SYSTEM_FRAME_SIZE_SAMPLES * vecNumAudioChannels[i]],
                                                       iClientFrameSizeSamples,
                                                       &vecbyCodedData[0],
                                                       iCeltNumCodedBytes );
                    }

                    // send separate mix to current clients
                    vecChannels[iCurChanID].PrepAndSendPacket ( &Socket,
                                                                vecbyCodedData,
                                                                iCeltNumCodedBytes );
                }

                // update socket buffer size
                vecChannels[iCurChanID].UpdateSocketBufferSize();

                // send channel levels
                if ( bSendChannelLevels && vecChannels[iCurChanID].ChannelLevelsRequired() )
                {
                    ConnLessProtocol.CreateCLChannelLevelListMes ( vecChannels[iCurChanID].GetAddress(), vecChannelLevels, iNumClients );
                }
            }
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

/// @brief Mix all audio data from all clients together.
void CServer::ProcessData ( const CVector<CVector<int16_t> >& vecvecsData,
                            const CVector<double>&            vecdGains,
                            const CVector<double>&            vecdPannings,
                            const CVector<int>&               vecNumAudioChannels,
                            CVector<int16_t>&                 vecsOutData,
                            const int                         iCurNumAudChan,
                            const int                         iNumClients )
{
    int i, j, k;

    // init return vector with zeros since we mix all channels on that vector
    vecsOutData.Reset ( 0 );

    // distinguish between stereo and mono mode
    if ( iCurNumAudChan == 1 )
    {
        // Mono target channel -------------------------------------------------
        for ( j = 0; j < iNumClients; j++ )
        {
            // get a reference to the audio data and gain of the current client
            const CVector<int16_t>& vecsData = vecvecsData[j];
            const double            dGain    = vecdGains[j];

            // if channel gain is 1, avoid multiplication for speed optimization
            if ( dGain == static_cast<double> ( 1.0 ) )
            {
                if ( vecNumAudioChannels[j] == 1 )
                {
                    // mono
                    for ( i = 0; i < iServerFrameSizeSamples; i++ )
                    {
                        vecsOutData[i] = Double2Short (
                            static_cast<double> ( vecsOutData[i] ) + vecsData[i] );
                    }
                }
                else
                {
                    // stereo: apply stereo-to-mono attenuation
                    for ( i = 0, k = 0; i < iServerFrameSizeSamples; i++, k += 2 )
                    {
                        vecsOutData[i] =
                            Double2Short ( vecsOutData[i] +
                            ( static_cast<double> ( vecsData[k] ) + vecsData[k + 1] ) / 2 );
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
                        vecsOutData[i] = Double2Short (
                            vecsOutData[i] + vecsData[i] * dGain );
                    }
                }
                else
                {
                    // stereo: apply stereo-to-mono attenuation
                    for ( i = 0, k = 0; i < iServerFrameSizeSamples; i++, k += 2 )
                    {
                        vecsOutData[i] =
                            Double2Short ( vecsOutData[i] + dGain *
                            ( static_cast<double> ( vecsData[k] ) + vecsData[k + 1] ) / 2 );
                    }
                }
            }
        }
    }
    else
    {
        // Stereo target channel -----------------------------------------------
        for ( j = 0; j < iNumClients; j++ )
        {
            // get a reference to the audio data and gain/pan of the current client
            const CVector<int16_t>& vecsData = vecvecsData[j];
            const double            dGain    = vecdGains[j];
            const double            dPan     = vecdPannings[j];

            // calculate combined gain/pan for each stereo channel where we define
            // the panning that center equals full gain for both channels
            const double dGainL = std::min ( 0.5, 1 - dPan ) * 2 * dGain;
            const double dGainR = std::min ( 0.5, dPan ) * 2 * dGain;

            // if channel gain is 1, avoid multiplication for speed optimization
            if ( ( dGainL == static_cast<double> ( 1.0 ) ) && ( dGainR == static_cast<double> ( 1.0 ) ) )
            {
                if ( vecNumAudioChannels[j] == 1 )
                {
                    // mono: copy same mono data in both out stereo audio channels
                    for ( i = 0, k = 0; i < iServerFrameSizeSamples; i++, k += 2 )
                    {
                        // left channel
                        vecsOutData[k] = Double2Short (
                            static_cast<double> ( vecsOutData[k] ) + vecsData[i] );

                        // right channel
                        vecsOutData[k + 1] = Double2Short (
                            static_cast<double> ( vecsOutData[k + 1] ) + vecsData[i] );
                    }
                }
                else
                {
                    // stereo
                    for ( i = 0; i < ( 2 * iServerFrameSizeSamples ); i++ )
                    {
                        vecsOutData[i] = Double2Short (
                            static_cast<double> ( vecsOutData[i] ) + vecsData[i] );
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
                        vecsOutData[k]     = Double2Short ( vecsOutData[k] +     vecsData[i] * dGainL );
                        vecsOutData[k + 1] = Double2Short ( vecsOutData[k + 1] + vecsData[i] * dGainR );
                    }
                }
                else
                {
                    // stereo
                    for ( i = 0; i < ( 2 * iServerFrameSizeSamples ); i += 2 )
                    {
                        // left/right channel
                        vecsOutData[i]     = Double2Short ( vecsOutData[i] +     vecsData[i] *     dGainL );
                        vecsOutData[i + 1] = Double2Short ( vecsOutData[i + 1] + vecsData[i + 1] * dGainR );
                    }
                }
            }
        }
    }
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
        QTime::currentTime().toString ( "hh:mm:ss AP" ) + ") <b>" + ChanName +
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

void CServer::CreateAndSendRecorderStateForAllConChannels()
{
    // get the recorder state
    ESvrRecState eState = GetRecorderState();

    // now send connected channels list to all connected clients
    for ( int i = 0; i < iMaxNumChannels; i++ )
    {
        if ( vecChannels[i].IsConnected() )
        {
            // send message
            ConnLessProtocol.CreateCLRecorderStateChange ( vecChannels[i].GetAddress(), eState );
        }
    }
}

ESvrRecState CServer::GetRecorderState()
{
    if ( !bRecorderInitialised )
    {
        return ESvrRecState::RS_NOT_INITIALISED;
    }
    if ( !bEnableRecording )
    {
        return ESvrRecState::RS_NOT_ENABLED;
    }
    return ESvrRecState::RS_ENABLED;
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

void CServer::OnProtcolMessageReceived ( int              iRecCounter,
                                         int              iRecID,
                                         CVector<uint8_t> vecbyMesBodyData,
                                         CHostAddress     RecHostAddr )
{
    Mutex.lock();
    {
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
    Mutex.unlock();
}

bool CServer::PutAudioData ( const CVector<uint8_t>& vecbyRecBuf,
                             const int               iNumBytesRead,
                             const CHostAddress&     HostAdr,
                             int&                    iCurChanID )
{
    bool bNewConnection = false; // init return value
    bool bChanOK        = true; // init with ok, might be overwritten

    Mutex.lock();
    {
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
    }
    Mutex.unlock();

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
    streamFileOut << strServerNameWithPort << endl << "<ul>" << endl;

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
                streamFileOut << "  <li>" << vecChannels[i].GetName() << "</li>" << endl;
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
void CServer::CreateLevelsForAllConChannels ( const int                        iNumClients,
                                              const CVector<int>&              vecNumAudioChannels,
                                              const CVector<CVector<int16_t> > vecvecsData,
                                              CVector<uint16_t>&               vecLevelsOut )
{
    int i, j, k;

    // init return vector with zeros since we mix all channels on that vector
    vecLevelsOut.Reset ( 0 );

    for ( j = 0; j < iNumClients; j++ )
    {
        // get a reference to the audio data
        const CVector<int16_t>& vecsData = vecvecsData[j];

        double dCurLevel = 0.0;

        if ( vecNumAudioChannels[j] == 1 )
        {
            // mono
            for ( i = 0; i < iServerFrameSizeSamples; i += 3 )
            {
                dCurLevel = std::max ( dCurLevel, fabs ( static_cast<double> ( vecsData[i] ) ) );
            }
        }
        else
        {
            // stereo: apply stereo-to-mono attenuation
            for ( i = 0, k = 0; i < iServerFrameSizeSamples; i += 3, k += 6 )
            {
                double sMix = ( static_cast<double> ( vecsData[k] ) + vecsData[k + 1] ) / 2;
                dCurLevel = std::max ( dCurLevel, fabs ( sMix ) );
            }
        }

        // smoothing
        const int iChId = vecChanIDsCurConChan[j];
        dCurLevel       = std::max ( dCurLevel, vecChannels[iChId].GetPrevLevel() * 0.5 );
        vecChannels[iChId].SetPrevLevel ( dCurLevel );

        // logarithmic measure
        double dCurSigLevel = CStereoSignalLevelMeter::CalcLogResult ( dCurLevel );

        // map to signal level meter
        dCurSigLevel -= LOW_BOUND_SIG_METER;
        dCurSigLevel *= NUM_STEPS_LED_BAR / ( UPPER_BOUND_SIG_METER - LOW_BOUND_SIG_METER );

        if ( dCurSigLevel < 0 )
        {
            dCurSigLevel = 0;
        }

        vecLevelsOut[j] = static_cast<uint16_t> ( ceil ( dCurSigLevel ) );
    }
}
