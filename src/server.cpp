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

#include "server.h"


// CHighPrecisionTimer implementation ******************************************
#if defined ( __APPLE__ ) || defined ( __MACOSX )
#include <mach/mach.h>
#include <mach/mach_error.h>
#include <mach/mach_time.h>

CHighPrecisionTimer::CHighPrecisionTimer() :
    bRun ( false )
{
    // calculate delay in mach absolute time
    const uint64_t iNsDelay =
        ( (uint64_t) SYSTEM_FRAME_SIZE_SAMPLES * 1000000000 ) /
        (uint64_t) SYSTEM_SAMPLE_RATE_HZ; // in ns

    struct mach_timebase_info timeBaseInfo;
    mach_timebase_info ( &timeBaseInfo );

    iMachDelay = ( iNsDelay * (uint64_t) timeBaseInfo.denom ) /
        (uint64_t) timeBaseInfo.numer;
}

void CHighPrecisionTimer::Start()
{
    // only start if not already running
    if ( !bRun )
    {
        // set run flag
        bRun = true;

        // set initial end time
        iNextEnd = mach_absolute_time() + iMachDelay;

        // start thread
        QThread::start();
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
        emit timeout();

        // now wait until the next buffer shall be processed (we
        // use the "increment method" to make sure we do not introduce
        // a timing drift)
        mach_wait_until(iNextEnd);
        iNextEnd += iMachDelay;
    }
}
#else
CHighPrecisionTimer::CHighPrecisionTimer()
{
    // add some error checking, the high precision timer implementation only
    // supports 128 samples frame size at 48 kHz sampling rate
#if ( SYSTEM_FRAME_SIZE_SAMPLES != 128 )
# error "Only system frame size of 128 samples is supported by this module"
#endif
#if SYSTEM_SAMPLE_RATE_HZ != 48000
# error "Only a system sample rate of 48 kHz is supported by this module"
#endif

    // Since QT only supports a minimum timer resolution of 1 ms but for our
    // llcon server we require a timer interval of 2.333 ms for 128 samples
    // frame size at 48 kHz sampling rate.
    // To support this interval, we use a timer with 2 ms
    // resolution and fire the actual frame timer if the error to the actual
    // required interval is minimum.
    veciTimeOutIntervals.Init ( 3 );

    // for 128 sample frame size at 48 kHz sampling rate:
    // actual intervals:  0.0  2.666  5.333  8.0
    // quantized to 2 ms: 0    2      6      8 (0)
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

    // start internal timer with 2 ms resolution
    Timer.start ( 2 );
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
#endif


// CServer implementation ******************************************************
CServer::CServer ( const int      iNewNumChan,
                   const QString& strLoggingFileName,
                   const quint16  iPortNumber,
                   const QString& strHTMLStatusFileName,
                   const QString& strHistoryFileName,
                   const QString& strServerNameForHTMLStatusFile,
                   const QString& strCentralServer,
                   const QString& strServerInfo ) :
    iNumChannels         ( iNewNumChan ),
    Socket               ( this, iPortNumber ),
    bWriteStatusHTMLFile ( false ),
    ServerListManager    ( iPortNumber,
                           strCentralServer,
                           strServerInfo,
                           iNewNumChan,
                           &ConnLessProtocol ),
    bAutoRunMinimized    ( false )
{
    int i;

    // create CELT encoder/decoder for each channel (must be done before
    // enabling the channels), create a mono and stereo encoder/decoder
    // for each channel
    for ( i = 0; i < iNumChannels; i++ )
    {
        // init audio endocder/decoder (mono)
        CeltModeMono[i] = celt_mode_create (
            SYSTEM_SAMPLE_RATE_HZ, 1, SYSTEM_FRAME_SIZE_SAMPLES, NULL );

        CeltEncoderMono[i] = celt_encoder_create ( CeltModeMono[i] );
        CeltDecoderMono[i] = celt_decoder_create ( CeltModeMono[i] );

#ifdef USE_LOW_COMPLEXITY_CELT_ENC
        // set encoder low complexity
        celt_encoder_ctl ( CeltEncoderMono[i],
            CELT_SET_COMPLEXITY_REQUEST, celt_int32_t ( 1 ) );
#endif

        // init audio endocder/decoder (stereo)
        CeltModeStereo[i] = celt_mode_create (
            SYSTEM_SAMPLE_RATE_HZ, 2, SYSTEM_FRAME_SIZE_SAMPLES, NULL );

        CeltEncoderStereo[i] = celt_encoder_create ( CeltModeStereo[i] );
        CeltDecoderStereo[i] = celt_decoder_create ( CeltModeStereo[i] );

#ifdef USE_LOW_COMPLEXITY_CELT_ENC
        // set encoder low complexity
        celt_encoder_ctl ( CeltEncoderStereo[i],
            CELT_SET_COMPLEXITY_REQUEST, celt_int32_t ( 1 ) );
#endif
    }

    // define colors for chat window identifiers
    vstrChatColors.Init ( 6 );
    vstrChatColors[0] = "mediumblue";
    vstrChatColors[1] = "red";
    vstrChatColors[2] = "darkorchid";
    vstrChatColors[3] = "green";
    vstrChatColors[4] = "maroon";
    vstrChatColors[5] = "coral";

    // init moving average buffer for response time evaluation
    CycleTimeVariance.Init ( SYSTEM_FRAME_SIZE_SAMPLES,
        SYSTEM_SAMPLE_RATE_HZ, TIME_MOV_AV_RESPONSE_SECONDS );

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
            QString().number( static_cast<int> ( iPortNumber ) ) );
    }

    // enable all channels (for the server all channel must be enabled the
    // entire life time of the software)
    for ( i = 0; i < iNumChannels; i++ )
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
        SIGNAL ( CLRegisterServerReceived ( CHostAddress, CServerCoreInfo ) ),
        this, SLOT ( OnCLRegisterServerReceived ( CHostAddress, CServerCoreInfo ) ) );

    QObject::connect ( &ConnLessProtocol,
        SIGNAL ( CLUnregisterServerReceived ( CHostAddress ) ),
        this, SLOT ( OnCLUnregisterServerReceived ( CHostAddress ) ) );

    QObject::connect ( &ConnLessProtocol,
        SIGNAL ( CLReqServerList ( CHostAddress ) ),
        this, SLOT ( OnCLReqServerList ( CHostAddress ) ) );

    QObject::connect ( &ConnLessProtocol,
        SIGNAL ( CLSendEmptyMes ( CHostAddress ) ),
        this, SLOT ( OnCLSendEmptyMes ( CHostAddress ) ) );

    QObject::connect ( &ConnLessProtocol,
        SIGNAL ( CLDisconnection ( CHostAddress ) ),
        this, SLOT ( OnCLDisconnection ( CHostAddress ) ) );


    // CODE TAG: MAX_NUM_CHANNELS_TAG
    // make sure we have MAX_NUM_CHANNELS connections!!!
    // send message
    QObject::connect ( &vecChannels[0],  SIGNAL ( MessReadyForSending ( CVector<uint8_t> ) ), this, SLOT ( OnSendProtMessCh0  ( CVector<uint8_t> ) ) );
    QObject::connect ( &vecChannels[1],  SIGNAL ( MessReadyForSending ( CVector<uint8_t> ) ), this, SLOT ( OnSendProtMessCh1  ( CVector<uint8_t> ) ) );
    QObject::connect ( &vecChannels[2],  SIGNAL ( MessReadyForSending ( CVector<uint8_t> ) ), this, SLOT ( OnSendProtMessCh2  ( CVector<uint8_t> ) ) );
    QObject::connect ( &vecChannels[3],  SIGNAL ( MessReadyForSending ( CVector<uint8_t> ) ), this, SLOT ( OnSendProtMessCh3  ( CVector<uint8_t> ) ) );
    QObject::connect ( &vecChannels[4],  SIGNAL ( MessReadyForSending ( CVector<uint8_t> ) ), this, SLOT ( OnSendProtMessCh4  ( CVector<uint8_t> ) ) );
    QObject::connect ( &vecChannels[5],  SIGNAL ( MessReadyForSending ( CVector<uint8_t> ) ), this, SLOT ( OnSendProtMessCh5  ( CVector<uint8_t> ) ) );
    QObject::connect ( &vecChannels[6],  SIGNAL ( MessReadyForSending ( CVector<uint8_t> ) ), this, SLOT ( OnSendProtMessCh6  ( CVector<uint8_t> ) ) );
    QObject::connect ( &vecChannels[7],  SIGNAL ( MessReadyForSending ( CVector<uint8_t> ) ), this, SLOT ( OnSendProtMessCh7  ( CVector<uint8_t> ) ) );
    QObject::connect ( &vecChannels[8],  SIGNAL ( MessReadyForSending ( CVector<uint8_t> ) ), this, SLOT ( OnSendProtMessCh8  ( CVector<uint8_t> ) ) );
    QObject::connect ( &vecChannels[9],  SIGNAL ( MessReadyForSending ( CVector<uint8_t> ) ), this, SLOT ( OnSendProtMessCh9  ( CVector<uint8_t> ) ) );
    QObject::connect ( &vecChannels[10], SIGNAL ( MessReadyForSending ( CVector<uint8_t> ) ), this, SLOT ( OnSendProtMessCh10 ( CVector<uint8_t> ) ) );
    QObject::connect ( &vecChannels[11], SIGNAL ( MessReadyForSending ( CVector<uint8_t> ) ), this, SLOT ( OnSendProtMessCh11 ( CVector<uint8_t> ) ) );

    // a connection less protocol message was detected
    QObject::connect ( &vecChannels[0],  SIGNAL ( DetectedCLMessage ( CVector<uint8_t>, int ) ), this, SLOT ( OnDetCLMessCh0  ( CVector<uint8_t>, int ) ) );
    QObject::connect ( &vecChannels[1],  SIGNAL ( DetectedCLMessage ( CVector<uint8_t>, int ) ), this, SLOT ( OnDetCLMessCh1  ( CVector<uint8_t>, int ) ) );
    QObject::connect ( &vecChannels[2],  SIGNAL ( DetectedCLMessage ( CVector<uint8_t>, int ) ), this, SLOT ( OnDetCLMessCh2  ( CVector<uint8_t>, int ) ) );
    QObject::connect ( &vecChannels[3],  SIGNAL ( DetectedCLMessage ( CVector<uint8_t>, int ) ), this, SLOT ( OnDetCLMessCh3  ( CVector<uint8_t>, int ) ) );
    QObject::connect ( &vecChannels[4],  SIGNAL ( DetectedCLMessage ( CVector<uint8_t>, int ) ), this, SLOT ( OnDetCLMessCh4  ( CVector<uint8_t>, int ) ) );
    QObject::connect ( &vecChannels[5],  SIGNAL ( DetectedCLMessage ( CVector<uint8_t>, int ) ), this, SLOT ( OnDetCLMessCh5  ( CVector<uint8_t>, int ) ) );
    QObject::connect ( &vecChannels[6],  SIGNAL ( DetectedCLMessage ( CVector<uint8_t>, int ) ), this, SLOT ( OnDetCLMessCh6  ( CVector<uint8_t>, int ) ) );
    QObject::connect ( &vecChannels[7],  SIGNAL ( DetectedCLMessage ( CVector<uint8_t>, int ) ), this, SLOT ( OnDetCLMessCh7  ( CVector<uint8_t>, int ) ) );
    QObject::connect ( &vecChannels[8],  SIGNAL ( DetectedCLMessage ( CVector<uint8_t>, int ) ), this, SLOT ( OnDetCLMessCh8  ( CVector<uint8_t>, int ) ) );
    QObject::connect ( &vecChannels[9],  SIGNAL ( DetectedCLMessage ( CVector<uint8_t>, int ) ), this, SLOT ( OnDetCLMessCh9  ( CVector<uint8_t>, int ) ) );
    QObject::connect ( &vecChannels[10], SIGNAL ( DetectedCLMessage ( CVector<uint8_t>, int ) ), this, SLOT ( OnDetCLMessCh10 ( CVector<uint8_t>, int ) ) );
    QObject::connect ( &vecChannels[11], SIGNAL ( DetectedCLMessage ( CVector<uint8_t>, int ) ), this, SLOT ( OnDetCLMessCh11 ( CVector<uint8_t>, int ) ) );

    // request jitter buffer size
    QObject::connect ( &vecChannels[0],  SIGNAL ( NewConnection() ), this, SLOT ( OnNewConnectionCh0() ) );
    QObject::connect ( &vecChannels[1],  SIGNAL ( NewConnection() ), this, SLOT ( OnNewConnectionCh1() ) );
    QObject::connect ( &vecChannels[2],  SIGNAL ( NewConnection() ), this, SLOT ( OnNewConnectionCh2() ) );
    QObject::connect ( &vecChannels[3],  SIGNAL ( NewConnection() ), this, SLOT ( OnNewConnectionCh3() ) );
    QObject::connect ( &vecChannels[4],  SIGNAL ( NewConnection() ), this, SLOT ( OnNewConnectionCh4() ) );
    QObject::connect ( &vecChannels[5],  SIGNAL ( NewConnection() ), this, SLOT ( OnNewConnectionCh5() ) );
    QObject::connect ( &vecChannels[6],  SIGNAL ( NewConnection() ), this, SLOT ( OnNewConnectionCh6() ) );
    QObject::connect ( &vecChannels[7],  SIGNAL ( NewConnection() ), this, SLOT ( OnNewConnectionCh7() ) );
    QObject::connect ( &vecChannels[8],  SIGNAL ( NewConnection() ), this, SLOT ( OnNewConnectionCh8() ) );
    QObject::connect ( &vecChannels[9],  SIGNAL ( NewConnection() ), this, SLOT ( OnNewConnectionCh9() ) );
    QObject::connect ( &vecChannels[10], SIGNAL ( NewConnection() ), this, SLOT ( OnNewConnectionCh10() ) );
    QObject::connect ( &vecChannels[11], SIGNAL ( NewConnection() ), this, SLOT ( OnNewConnectionCh11() ) );

    // request connected clients list
    QObject::connect ( &vecChannels[0],  SIGNAL ( ReqConnClientsList() ), this, SLOT ( OnReqConnClientsListCh0() ) );
    QObject::connect ( &vecChannels[1],  SIGNAL ( ReqConnClientsList() ), this, SLOT ( OnReqConnClientsListCh1() ) );
    QObject::connect ( &vecChannels[2],  SIGNAL ( ReqConnClientsList() ), this, SLOT ( OnReqConnClientsListCh2() ) );
    QObject::connect ( &vecChannels[3],  SIGNAL ( ReqConnClientsList() ), this, SLOT ( OnReqConnClientsListCh3() ) );
    QObject::connect ( &vecChannels[4],  SIGNAL ( ReqConnClientsList() ), this, SLOT ( OnReqConnClientsListCh4() ) );
    QObject::connect ( &vecChannels[5],  SIGNAL ( ReqConnClientsList() ), this, SLOT ( OnReqConnClientsListCh5() ) );
    QObject::connect ( &vecChannels[6],  SIGNAL ( ReqConnClientsList() ), this, SLOT ( OnReqConnClientsListCh6() ) );
    QObject::connect ( &vecChannels[7],  SIGNAL ( ReqConnClientsList() ), this, SLOT ( OnReqConnClientsListCh7() ) );
    QObject::connect ( &vecChannels[8],  SIGNAL ( ReqConnClientsList() ), this, SLOT ( OnReqConnClientsListCh8() ) );
    QObject::connect ( &vecChannels[9],  SIGNAL ( ReqConnClientsList() ), this, SLOT ( OnReqConnClientsListCh9() ) );
    QObject::connect ( &vecChannels[10], SIGNAL ( ReqConnClientsList() ), this, SLOT ( OnReqConnClientsListCh10() ) );
    QObject::connect ( &vecChannels[11], SIGNAL ( ReqConnClientsList() ), this, SLOT ( OnReqConnClientsListCh11() ) );

    // channel name has changed
    QObject::connect ( &vecChannels[0],  SIGNAL ( NameHasChanged() ), this, SLOT ( OnNameHasChangedCh0() ) );
    QObject::connect ( &vecChannels[1],  SIGNAL ( NameHasChanged() ), this, SLOT ( OnNameHasChangedCh1() ) );
    QObject::connect ( &vecChannels[2],  SIGNAL ( NameHasChanged() ), this, SLOT ( OnNameHasChangedCh2() ) );
    QObject::connect ( &vecChannels[3],  SIGNAL ( NameHasChanged() ), this, SLOT ( OnNameHasChangedCh3() ) );
    QObject::connect ( &vecChannels[4],  SIGNAL ( NameHasChanged() ), this, SLOT ( OnNameHasChangedCh4() ) );
    QObject::connect ( &vecChannels[5],  SIGNAL ( NameHasChanged() ), this, SLOT ( OnNameHasChangedCh5() ) );
    QObject::connect ( &vecChannels[6],  SIGNAL ( NameHasChanged() ), this, SLOT ( OnNameHasChangedCh6() ) );
    QObject::connect ( &vecChannels[7],  SIGNAL ( NameHasChanged() ), this, SLOT ( OnNameHasChangedCh7() ) );
    QObject::connect ( &vecChannels[8],  SIGNAL ( NameHasChanged() ), this, SLOT ( OnNameHasChangedCh8() ) );
    QObject::connect ( &vecChannels[9],  SIGNAL ( NameHasChanged() ), this, SLOT ( OnNameHasChangedCh9() ) );
    QObject::connect ( &vecChannels[10], SIGNAL ( NameHasChanged() ), this, SLOT ( OnNameHasChangedCh10() ) );
    QObject::connect ( &vecChannels[11], SIGNAL ( NameHasChanged() ), this, SLOT ( OnNameHasChangedCh11() ) );

    // chat text received
    QObject::connect ( &vecChannels[0],  SIGNAL ( ChatTextReceived ( QString ) ), this, SLOT ( OnChatTextReceivedCh0 ( QString ) ) );
    QObject::connect ( &vecChannels[1],  SIGNAL ( ChatTextReceived ( QString ) ), this, SLOT ( OnChatTextReceivedCh1 ( QString ) ) );
    QObject::connect ( &vecChannels[2],  SIGNAL ( ChatTextReceived ( QString ) ), this, SLOT ( OnChatTextReceivedCh2 ( QString ) ) );
    QObject::connect ( &vecChannels[3],  SIGNAL ( ChatTextReceived ( QString ) ), this, SLOT ( OnChatTextReceivedCh3 ( QString ) ) );
    QObject::connect ( &vecChannels[4],  SIGNAL ( ChatTextReceived ( QString ) ), this, SLOT ( OnChatTextReceivedCh4 ( QString ) ) );
    QObject::connect ( &vecChannels[5],  SIGNAL ( ChatTextReceived ( QString ) ), this, SLOT ( OnChatTextReceivedCh5 ( QString ) ) );
    QObject::connect ( &vecChannels[6],  SIGNAL ( ChatTextReceived ( QString ) ), this, SLOT ( OnChatTextReceivedCh6 ( QString ) ) );
    QObject::connect ( &vecChannels[7],  SIGNAL ( ChatTextReceived ( QString ) ), this, SLOT ( OnChatTextReceivedCh7 ( QString ) ) );
    QObject::connect ( &vecChannels[8],  SIGNAL ( ChatTextReceived ( QString ) ), this, SLOT ( OnChatTextReceivedCh8 ( QString ) ) );
    QObject::connect ( &vecChannels[9],  SIGNAL ( ChatTextReceived ( QString ) ), this, SLOT ( OnChatTextReceivedCh9 ( QString ) ) );
    QObject::connect ( &vecChannels[10], SIGNAL ( ChatTextReceived ( QString ) ), this, SLOT ( OnChatTextReceivedCh10 ( QString ) ) );
    QObject::connect ( &vecChannels[11], SIGNAL ( ChatTextReceived ( QString ) ), this, SLOT ( OnChatTextReceivedCh11 ( QString ) ) );

    // auto socket buffer size change
    QObject::connect ( &vecChannels[0],  SIGNAL ( ServerAutoSockBufSizeChange ( int ) ), this, SLOT ( OnServerAutoSockBufSizeChangeCh0 ( int ) ) );
    QObject::connect ( &vecChannels[1],  SIGNAL ( ServerAutoSockBufSizeChange ( int ) ), this, SLOT ( OnServerAutoSockBufSizeChangeCh1 ( int ) ) );
    QObject::connect ( &vecChannels[2],  SIGNAL ( ServerAutoSockBufSizeChange ( int ) ), this, SLOT ( OnServerAutoSockBufSizeChangeCh2 ( int ) ) );
    QObject::connect ( &vecChannels[3],  SIGNAL ( ServerAutoSockBufSizeChange ( int ) ), this, SLOT ( OnServerAutoSockBufSizeChangeCh3 ( int ) ) );
    QObject::connect ( &vecChannels[4],  SIGNAL ( ServerAutoSockBufSizeChange ( int ) ), this, SLOT ( OnServerAutoSockBufSizeChangeCh4 ( int ) ) );
    QObject::connect ( &vecChannels[5],  SIGNAL ( ServerAutoSockBufSizeChange ( int ) ), this, SLOT ( OnServerAutoSockBufSizeChangeCh5 ( int ) ) );
    QObject::connect ( &vecChannels[6],  SIGNAL ( ServerAutoSockBufSizeChange ( int ) ), this, SLOT ( OnServerAutoSockBufSizeChangeCh6 ( int ) ) );
    QObject::connect ( &vecChannels[7],  SIGNAL ( ServerAutoSockBufSizeChange ( int ) ), this, SLOT ( OnServerAutoSockBufSizeChangeCh7 ( int ) ) );
    QObject::connect ( &vecChannels[8],  SIGNAL ( ServerAutoSockBufSizeChange ( int ) ), this, SLOT ( OnServerAutoSockBufSizeChangeCh8 ( int ) ) );
    QObject::connect ( &vecChannels[9],  SIGNAL ( ServerAutoSockBufSizeChange ( int ) ), this, SLOT ( OnServerAutoSockBufSizeChangeCh9 ( int ) ) );
    QObject::connect ( &vecChannels[10], SIGNAL ( ServerAutoSockBufSizeChange ( int ) ), this, SLOT ( OnServerAutoSockBufSizeChangeCh10 ( int ) ) );
    QObject::connect ( &vecChannels[11], SIGNAL ( ServerAutoSockBufSizeChange ( int ) ), this, SLOT ( OnServerAutoSockBufSizeChangeCh11 ( int ) ) );
}

void CServer::OnSendProtMessage ( int iChID, CVector<uint8_t> vecMessage )
{
    // the protocol queries me to call the function to send the message
    // send it through the network
    Socket.SendPacket ( vecMessage, vecChannels[iChID].GetAddress() );
}

void CServer::OnSendCLProtMessage ( CHostAddress     InetAddr,
                                    CVector<uint8_t> vecMessage )
{
    // the protocol queries me to call the function to send the message
    // send it through the network
    Socket.SendPacket ( vecMessage, InetAddr );
}

void CServer::OnDetCLMess ( const CVector<uint8_t>& vecbyData,
                            const int               iNumBytes,
                            const CHostAddress&     InetAddr )
{
    // this is a special case: we received a connection less message but we are
    // in a connection
    ConnLessProtocol.ParseConnectionLessMessage ( vecbyData,
                                                  iNumBytes,
                                                  InetAddr );
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

void CServer::Start()
{
    // only start if not already running
    if ( !IsRunning() )
    {
        // start timer
        HighPrecisionTimer.Start();

        // init time for response time evaluation
        CycleTimeVariance.Reset();

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
    int i, j;

    CVector<int>               vecChanID;
    CVector<CVector<double> >  vecvecdGains;
    CVector<CVector<int16_t> > vecvecsData;
    CVector<int>               vecNumAudioChannels;

    // Get data from all connected clients -------------------------------------
    bool bChannelIsNowDisconnected = false;

    // Make put and get calls thread safe. Do not forget to unlock mutex
    // afterwards!
    Mutex.lock();
    {
        // first, get number and IDs of connected channels
        vecChanID.Init ( 0 );
        for ( i = 0; i < iNumChannels; i++ )
        {
            if ( vecChannels[i].IsConnected() )
            {
                // add ID and data
                vecChanID.Add ( i );
            }
        }

        // process connected channels
        const int iNumCurConnChan = vecChanID.Size();

        // init temporary vectors
        vecvecdGains.Init        ( iNumCurConnChan );
        vecvecsData.Init         ( iNumCurConnChan );
        vecNumAudioChannels.Init ( iNumCurConnChan );

        for ( i = 0; i < iNumCurConnChan; i++ )
        {
            // get actual ID of current channel
            const int iCurChanID = vecChanID[i];

            // get and store number of audio channels
            const int iCurNumAudChan =
                vecChannels[iCurChanID].GetNumAudioChannels();

            vecNumAudioChannels[i] = iCurNumAudChan;

            // init vectors storing information of all channels
            vecvecdGains[i].Init ( iNumCurConnChan );
            vecvecsData[i].Init  ( iCurNumAudChan * SYSTEM_FRAME_SIZE_SAMPLES );

            // get gains of all connected channels
            for ( j = 0; j < iNumCurConnChan; j++ )
            {
                // The second index of "vecvecdGains" does not represent
                // the channel ID! Therefore we have to use "vecChanID" to
                // query the IDs of the currently connected channels
                vecvecdGains[i][j] =
                    vecChannels[iCurChanID].GetGain( vecChanID[j] );
            }

            // get current number of CELT coded bytes
            const int iCeltNumCodedBytes =
                vecChannels[iCurChanID].GetNetwFrameSize();

            // init temporal data vector and clear input buffers
            CVector<uint8_t> vecbyData ( iCeltNumCodedBytes );

            // get data
            const EGetDataStat eGetStat =
                vecChannels[iCurChanID].GetData ( vecbyData );

            // if channel was just disconnected, set flag that connected
            // client list is sent to all other clients
            if ( eGetStat == GS_CHAN_NOW_DISCONNECTED )
            {
                bChannelIsNowDisconnected = true;
            }

            // CELT decode received data stream
            if ( eGetStat == GS_BUFFER_OK )
            {
                if ( iCurNumAudChan == 1 )
                {
                    // mono
                    celt_decode ( CeltDecoderMono[iCurChanID],
                                  &vecbyData[0],
                                  iCeltNumCodedBytes,
                                  &vecvecsData[i][0] );
                }
                else
                {
                    // stereo
                    celt_decode ( CeltDecoderStereo[iCurChanID],
                                  &vecbyData[0],
                                  iCeltNumCodedBytes,
                                  &vecvecsData[i][0] );
                }
            }
            else
            {
                // lost packet
                if ( iCurNumAudChan == 1 )
                {
                    // mono
                    celt_decode ( CeltDecoderMono[iCurChanID],
                                  NULL,
                                  0,
                                  &vecvecsData[i][0] );
                }
                else
                {
                    // stereo
                    celt_decode ( CeltDecoderStereo[iCurChanID],
                                  NULL,
                                  0,
                                  &vecvecsData[i][0] );
                }
            }

            // send message for get status (for GUI)
            if ( eGetStat == GS_BUFFER_OK )
            {
                PostWinMessage ( MS_JIT_BUF_GET, MUL_COL_LED_GREEN, iCurChanID );
            }
            else
            {
                PostWinMessage ( MS_JIT_BUF_GET, MUL_COL_LED_RED, iCurChanID );
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
    const int iNumClients = vecChanID.Size();

    // Check if at least one client is connected. If not, stop server until
    // one client is connected.
    if ( iNumClients != 0 )
    {
        for ( int i = 0; i < iNumClients; i++ )
        {
            // get actual ID of current channel
            const int iCurChanID = vecChanID[i];

            // generate a sparate mix for each channel
            // actual processing of audio data -> mix
            CVector<short> vecsSendData ( ProcessData ( i,
                                                        vecvecsData,
                                                        vecvecdGains[i],
                                                        vecNumAudioChannels ) );

            // get current number of CELT coded bytes
            const int iCeltNumCodedBytes =
                vecChannels[iCurChanID].GetNetwFrameSize();

            // CELT encoding
            CVector<unsigned char> vecCeltData ( iCeltNumCodedBytes );

            if ( vecChannels[iCurChanID].GetNumAudioChannels() == 1 )
            {
                // mono
                celt_encode ( CeltEncoderMono[iCurChanID],
                              &vecsSendData[0],
                              NULL,
                              &vecCeltData[0],
                              iCeltNumCodedBytes );
            }
            else
            {
                // stereo
                celt_encode ( CeltEncoderStereo[iCurChanID],
                              &vecsSendData[0],
                              NULL,
                              &vecCeltData[0],
                              iCeltNumCodedBytes );
            }

            // send separate mix to current clients
            Socket.SendPacket (
                vecChannels[iCurChanID].PrepSendPacket ( vecCeltData ),
                vecChannels[iCurChanID].GetAddress() );

            // update socket buffer size
            vecChannels[iCurChanID].UpdateSocketBufferSize (
                CycleTimeVariance.GetStdDev() );
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

CVector<int16_t> CServer::ProcessData ( const int                   iCurIndex,
                                        CVector<CVector<int16_t> >& vecvecsData,
                                        CVector<double>&            vecdGains,
                                        CVector<int>&               vecNumAudioChannels )
{
    int i, j, k;

    // get number of audio channels of current channel
    const int iCurNumAudChan = vecNumAudioChannels[iCurIndex];

    // number of samples for output vector
    const int iNumOutSamples = iCurNumAudChan * SYSTEM_FRAME_SIZE_SAMPLES;

    // init return vector with zeros since we mix all channels on that vector
    CVector<int16_t> vecsOutData ( iNumOutSamples, 0 );

    const int iNumClients = vecvecsData.Size();

    // mix all audio data from all clients together
    if ( iCurNumAudChan == 1 )
    {
        // Mono target channel -------------------------------------------------
        for ( j = 0; j < iNumClients; j++ )
        {
            // if channel gain is 1, avoid multiplication for speed optimization
            if ( vecdGains[j] == static_cast<double> ( 1.0 ) )
            {
                if ( vecNumAudioChannels[j] == 1 )
                {
                    // mono
                    for ( i = 0; i < SYSTEM_FRAME_SIZE_SAMPLES; i++ )
                    {
                        vecsOutData[i] =
                            Double2Short ( vecsOutData[i] + vecvecsData[j][i] );
                    }
                }
                else
                {
                    // stereo: apply stereo-to-mono attenuation
                    for ( i = 0, k = 0; i < SYSTEM_FRAME_SIZE_SAMPLES; i++, k += 2 )
                    {
                        vecsOutData[i] =
                            Double2Short ( vecsOutData[i] +
                            ( vecvecsData[j][k] + vecvecsData[j][k + 1] ) / 2 );
                    }
                }
            }
            else
            {
                if ( vecNumAudioChannels[j] == 1 )
                {
                    // mono
                    for ( i = 0; i < SYSTEM_FRAME_SIZE_SAMPLES; i++ )
                    {
                        vecsOutData[i] =
                            Double2Short ( vecsOutData[i] +
                            vecvecsData[j][i] * vecdGains[j] );
                    }
                }
                else
                {
                    // stereo: apply stereo-to-mono attenuation
                    for ( i = 0, k = 0; i < SYSTEM_FRAME_SIZE_SAMPLES; i++, k += 2 )
                    {
                        vecsOutData[i] =
                            Double2Short ( vecsOutData[i] + vecdGains[j] *
                            ( vecvecsData[j][k] + vecvecsData[j][k + 1] ) / 2 );
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
            // if channel gain is 1, avoid multiplication for speed optimization
            if ( vecdGains[j] == static_cast<double> ( 1.0 ) )
            {
                if ( vecNumAudioChannels[j] == 1 )
                {
                    // mono: copy same mono data in both out stereo audio channels
                    for ( i = 0, k = 0; i < SYSTEM_FRAME_SIZE_SAMPLES; i++, k += 2 )
                    {
                        // left channel
                        vecsOutData[k] =
                            Double2Short ( vecsOutData[k] + vecvecsData[j][i] );

                        // right channel
                        vecsOutData[k + 1] =
                            Double2Short ( vecsOutData[k + 1] + vecvecsData[j][i] );
                    }
                }
                else
                {
                    // stereo
                    for ( i = 0; i < iNumOutSamples; i++ )
                    {
                        vecsOutData[i] =
                            Double2Short ( vecsOutData[i] + vecvecsData[j][i] );
                    }
                }
            }
            else
            {
                if ( vecNumAudioChannels[j] == 1 )
                {
                    // mono: copy same mono data in both out stereo audio channels
                    for ( i = 0, k = 0; i < SYSTEM_FRAME_SIZE_SAMPLES; i++, k += 2 )
                    {
                        // left channel
                        vecsOutData[k] = Double2Short (
                            vecsOutData[k] + vecvecsData[j][i] * vecdGains[j] );

                        // right channel
                        vecsOutData[k + 1] = Double2Short (
                            vecsOutData[k + 1] + vecvecsData[j][i] * vecdGains[j] );
                    }
                }
                else
                {
                    // stereo
                    for ( i = 0; i < iNumOutSamples; i++ )
                    {
                        vecsOutData[i] =
                            Double2Short ( vecsOutData[i] +
                            vecvecsData[j][i] * vecdGains[j] );
                    }
                }
            }
        }
    }

    return vecsOutData;
}

CVector<CChannelShortInfo> CServer::CreateChannelList()
{
    CVector<CChannelShortInfo> vecChanInfo ( 0 );

    // look for free channels
    for ( int i = 0; i < iNumChannels; i++ )
    {
        if ( vecChannels[i].IsConnected() )
        {
            // append channel ID, IP address and channel name to storing vectors
            vecChanInfo.Add ( CChannelShortInfo (
                i, // ID
                vecChannels[i].GetAddress().InetAddr.toIPv4Address(), // IP address
                vecChannels[i].GetName() /* name */ ) );
        }
    }

    return vecChanInfo;
}

void CServer::CreateAndSendChanListForAllConChannels()
{
    // create channel list
    CVector<CChannelShortInfo> vecChanInfo ( CreateChannelList() );

    // now send connected channels list to all connected clients
    for ( int i = 0; i < iNumChannels; i++ )
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
    CVector<CChannelShortInfo> vecChanInfo ( CreateChannelList() );

    // now send connected channels list to the channel with the ID "iCurChanID"
    vecChannels[iCurChanID].CreateConClientListMes ( vecChanInfo );
}

void CServer::CreateAndSendChatTextForAllConChannels ( const int      iCurChanID,
                                                       const QString& strChatText )
{
    // Create message which is sent to all connected clients -------------------
    // get client name, if name is empty, use IP address instead
    QString ChanName = vecChannels[iCurChanID].GetName();
    if ( ChanName.isEmpty() )
    {
        // convert IP address to text and show it
        ChanName = vecChannels[iCurChanID].GetAddress().
            toString ( CHostAddress::SM_IP_NO_LAST_BYTE );
    }

    // add time and name of the client at the beginning of the message text and
    // use different colors
    QString sCurColor = vstrChatColors[iCurChanID % vstrChatColors.Size()];
    const QString strActualMessageText =
        "<font color=""" + sCurColor + """>(" +
        QTime::currentTime().toString ( "hh:mm:ss AP" ) + ") <b>" + ChanName +
        "</b></font> " + strChatText;


    // Send chat text to all connected clients ---------------------------------
    for ( int i = 0; i < iNumChannels; i++ )
    {
        if ( vecChannels[i].IsConnected() )
        {
            // send message
            vecChannels[i].CreateChatTextMes ( strActualMessageText );
        }
    }
}

int CServer::GetFreeChan()
{
    // look for a free channel
    for ( int i = 0; i < iNumChannels; i++ )
    {
        if ( !vecChannels[i].IsConnected() )
        {
            return i;
        }
    }

    // no free channel found, return invalid ID
    return INVALID_CHANNEL_ID;
}

int CServer::FindChannel ( const CHostAddress& InetAddr )
{
    // look for a channel with the given internet address
    for ( int i = 0; i < iNumChannels; i++ )
    {
        if ( vecChannels[i].GetAddress() == InetAddr )
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
    for ( int i = 0; i < iNumChannels; i++ )
    {
        if ( vecChannels[i].IsConnected() )
        {
            // this channel is connected, increment counter
            iNumConnClients += 1;
        }
    }

    return iNumConnClients;
}

int CServer::CheckAddr ( const CHostAddress& Addr )
{
    CHostAddress InetAddr;

    // check for all possible channels if IP is already in use
    for ( int i = 0; i < iNumChannels; i++ )
    {
        if ( vecChannels[i].IsConnected() )
        {
            if ( vecChannels[i].GetAddress ( InetAddr ) )
            {
                // IP found, return channel number
                if ( InetAddr == Addr )
                {
                    return i;
                }
            }
        }
    }

    // IP not found, return invalid ID
    return INVALID_CHANNEL_ID;
}

bool CServer::PutData ( const CVector<uint8_t>& vecbyRecBuf,
                        const int               iNumBytesRead,
                        const CHostAddress&     HostAdr )
{
    bool bChanOK                        = true; // init with ok, might be overwritten
    bool bNewChannelReserved            = false;
    bool bIsNotEvaluatedProtocolMessage = false;

    Mutex.lock();
    {
        // Get channel ID ------------------------------------------------------
        // check address
        int iCurChanID = CheckAddr ( HostAdr );

        if ( iCurChanID == INVALID_CHANNEL_ID )
        {
            // this is a new client, we then first check if this is a connection
            // less message before we create a new official channel
            if ( ConnLessProtocol.ParseConnectionLessMessage ( vecbyRecBuf,
                                                               iNumBytesRead, 
                                                               HostAdr ) )
            {
                // a new client is calling, look for free channel
                iCurChanID = GetFreeChan();

                if ( iCurChanID != INVALID_CHANNEL_ID )
                {
                    // initialize current channel by storing the calling host
                    // address
                    vecChannels[iCurChanID].SetAddress ( HostAdr );

                    // reset channel name
                    vecChannels[iCurChanID].ResetName();

                    // reset the channel gains of current channel, at the same
                    // time reset gains of this channel ID for all other channels
                    for ( int i = 0; i < iNumChannels; i++ )
                    {
                        vecChannels[iCurChanID].SetGain ( i, (double) 1.0 );

                        // other channels (we do not distinguish the case if
                        // i == iCurChanID for simplicity)
                        vecChannels[i].SetGain ( iCurChanID, (double) 1.0 );
                    }

                    // set flag for new reserved channel
                    bNewChannelReserved = true;
                }
                else
                {
                    // no free channel available
                    bChanOK = false;

                    // create and send "server full" message
                    ConnLessProtocol.CreateCLServerFullMes ( HostAdr );
                }
            }
            else
            {
                // this was a connection less protocol message, return according
                // state
                bChanOK = false;
            }
        }


        // Put received data in jitter buffer ----------------------------------
        if ( bChanOK )
        {
            // put packet in socket buffer
            switch ( vecChannels[iCurChanID].PutData ( vecbyRecBuf, iNumBytesRead ) )
            {
            case PS_AUDIO_OK:
                PostWinMessage ( MS_JIT_BUF_PUT, MUL_COL_LED_GREEN, iCurChanID );
                break;

            case PS_AUDIO_ERR:
                PostWinMessage ( MS_JIT_BUF_PUT, MUL_COL_LED_RED, iCurChanID );
                break;

            case PS_PROT_ERR:
                PostWinMessage ( MS_JIT_BUF_PUT, MUL_COL_LED_YELLOW, iCurChanID );
                break;

            case PS_PROT_OK_MESS_NOT_EVALUATED:
                bIsNotEvaluatedProtocolMessage = true; // set flag
                break;

            case PS_GEN_ERROR:
            case PS_PROT_OK:
                // for these cases, do nothing
                break;
            }
        }

        // act on new channel connection
        if ( bNewChannelReserved && ( !bIsNotEvaluatedProtocolMessage ) )
        {
            // logging of new connected channel
            Logging.AddNewConnection ( HostAdr.InetAddr );

            // A new client connected to the server, the channel list
            // at all clients have to be updated. This is done by sending
            // a channel name request to the client which causes a channel
            // name message to be transmitted to the server. If the server
            // receives this message, the channel list will be automatically
            // updated (implicitely).
            // To make sure the protocol message is transmitted, the channel
            // first has to be marked as connected.
            //
            // Usually it is not required to send the channel list to the
            // client currently connecting since it automatically requests
            // the channel list on a new connection (as a result, he will
            // usually get the list twice which has no impact on functionality
            // but will only increase the network load a tiny little bit). But
            // in case the client thinks he is still connected but the server
            // was restartet, it is important that we send the channel list
            // at this place.
            vecChannels[iCurChanID].ResetTimeOutCounter();
            vecChannels[iCurChanID].CreateReqChanNameMes();

// COMPATIBILITY ISSUE
// since old versions of the llcon software did not implement the channel name
// request message, we have to explicitely send the channel list here
CreateAndSendChanListForAllConChannels();
        }
    }
    Mutex.unlock();

    // we do not want the server to be started on a protocol message but only on
    // an audio packet -> consider "bIsNotEvaluatedProtocolMessage", too
    return bChanOK && ( !bIsNotEvaluatedProtocolMessage );
}

void CServer::GetConCliParam ( CVector<CHostAddress>& vecHostAddresses,
                               CVector<QString>&      vecsName,
                               CVector<int>&          veciJitBufNumFrames,
                               CVector<int>&          veciNetwFrameSizeFact )
{
    CHostAddress InetAddr;

    // init return values
    vecHostAddresses.Init      ( iNumChannels );
    vecsName.Init              ( iNumChannels );
    veciJitBufNumFrames.Init   ( iNumChannels );
    veciNetwFrameSizeFact.Init ( iNumChannels );

    // check all possible channels
    for ( int i = 0; i < iNumChannels; i++ )
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
    // create channel list
    CVector<CChannelShortInfo> vecChanInfo ( CreateChannelList() );

    // prepare file and stream
    QFile serverFileListFile ( strServerHTMLFileListName );
    if ( !serverFileListFile.open ( QIODevice::WriteOnly | QIODevice::Text ) )
    {
        return;
    }

    QTextStream streamFileOut ( &serverFileListFile );
    streamFileOut << strServerNameWithPort << endl << "<ul>" << endl;

    // get the number of connected clients
    int iNumConnClients = 0;
    for ( int i = 0; i < iNumChannels; i++ )
    {
        if ( vecChannels[i].IsConnected() )
        {
            iNumConnClients++;
        }
    }

    // depending on number of connected clients write list
    if ( iNumConnClients == 0 )
    {
        // no clients are connected -> empty server
        streamFileOut << "  No client connected" << endl;
    }
    else
    {
        // write entry for each connected client
        for ( int i = 0; i < iNumChannels; i++ )
        {
            if ( vecChannels[i].IsConnected() )
            {
                QString strCurChanName = vecChannels[i].GetName();

                // if text is empty, show IP address instead
                if ( strCurChanName.isEmpty() )
                {
                    // convert IP address to text and show it, remove last
                    // digits
                    strCurChanName = vecChannels[i].GetAddress().
                        toString ( CHostAddress::SM_IP_NO_LAST_BYTE );
                }

                streamFileOut << "  <li>" << strCurChanName << "</li>" << endl;
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
        const int iMessType = ( (CLlconEvent*) pEvent )->iMessType;

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
