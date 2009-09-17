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


// CHighPrecisionTimer implementation ******************************************
CHighPrecisionTimer::CHighPrecisionTimer()
{
    // add some error checking, the high precision timer implementation only
    // supports 128 samples frame size at 48 kHz sampling rate
#if ( SYSTEM_FRAME_SIZE_SAMPLES != 128 )
# error "Only system frame size of 128 samples is supported by this module"
#endif
#if SYSTEM_SAMPLE_RATE != 48000
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

void CHighPrecisionTimer::start()
{
    // reset position pointer and counter
    iCurPosInVector  = 0;
    iIntervalCounter = 0;

    // start internal timer with 2 ms resolution
    Timer.start ( 2 );
}

void CHighPrecisionTimer::stop()
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


// CServer implementation ******************************************************
CServer::CServer ( const QString& strLoggingFileName,
                   const quint16  iPortNumber,
                   const QString& strHTMLStatusFileName,
                   const QString& strHistoryFileName,
                   const QString& strServerNameForHTMLStatusFile ) :
    Socket ( this, iPortNumber ),
    bWriteStatusHTMLFile ( false )
{
    int i;

    // create CELT encoder/decoder for each channel (must be done before
    // enabling the channels)
    for ( i = 0; i < USED_NUM_CHANNELS; i++ )
    {
        // init audio endocder/decoder (mono)
        CeltMode[i] = celt_mode_create (
            SYSTEM_SAMPLE_RATE, 1, SYSTEM_FRAME_SIZE_SAMPLES, NULL );

        CeltEncoder[i] = celt_encoder_create ( CeltMode[i] );
        CeltDecoder[i] = celt_decoder_create ( CeltMode[i] );

#ifdef USE_LOW_COMPLEXITY_CELT_ENC
        // set encoder low complexity
        celt_encoder_ctl(CeltEncoder[i],
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

    vecsSendData.Init ( SYSTEM_FRAME_SIZE_SAMPLES );

    // init moving average buffer for response time evaluation
    CycleTimeVariance.Init ( SYSTEM_FRAME_SIZE_SAMPLES,
        SYSTEM_SAMPLE_RATE, TIME_MOV_AV_RESPONSE );

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
    for ( i = 0; i < USED_NUM_CHANNELS; i++ )
    {
        vecChannels[i].SetEnable ( true );
    }


    // connections -------------------------------------------------------------
    // connect timer timeout signal
    QObject::connect ( &HighPrecisionTimer, SIGNAL ( timeout() ),
        this, SLOT ( OnTimer() ) );

    // CODE TAG: MAX_NUM_CHANNELS_TAG
    // make sure we have MAX_NUM_CHANNELS connections!!!
    // send message
    QObject::connect ( &vecChannels[0], SIGNAL ( MessReadyForSending ( CVector<uint8_t> ) ), this, SLOT ( OnSendProtMessCh0 ( CVector<uint8_t> ) ) );
    QObject::connect ( &vecChannels[1], SIGNAL ( MessReadyForSending ( CVector<uint8_t> ) ), this, SLOT ( OnSendProtMessCh1 ( CVector<uint8_t> ) ) );
    QObject::connect ( &vecChannels[2], SIGNAL ( MessReadyForSending ( CVector<uint8_t> ) ), this, SLOT ( OnSendProtMessCh2 ( CVector<uint8_t> ) ) );
    QObject::connect ( &vecChannels[3], SIGNAL ( MessReadyForSending ( CVector<uint8_t> ) ), this, SLOT ( OnSendProtMessCh3 ( CVector<uint8_t> ) ) );
    QObject::connect ( &vecChannels[4], SIGNAL ( MessReadyForSending ( CVector<uint8_t> ) ), this, SLOT ( OnSendProtMessCh4 ( CVector<uint8_t> ) ) );
    QObject::connect ( &vecChannels[5], SIGNAL ( MessReadyForSending ( CVector<uint8_t> ) ), this, SLOT ( OnSendProtMessCh5 ( CVector<uint8_t> ) ) );
    QObject::connect ( &vecChannels[6], SIGNAL ( MessReadyForSending ( CVector<uint8_t> ) ), this, SLOT ( OnSendProtMessCh6 ( CVector<uint8_t> ) ) );
    QObject::connect ( &vecChannels[7], SIGNAL ( MessReadyForSending ( CVector<uint8_t> ) ), this, SLOT ( OnSendProtMessCh7 ( CVector<uint8_t> ) ) );
    QObject::connect ( &vecChannels[8], SIGNAL ( MessReadyForSending ( CVector<uint8_t> ) ), this, SLOT ( OnSendProtMessCh8 ( CVector<uint8_t> ) ) );
    QObject::connect ( &vecChannels[9], SIGNAL ( MessReadyForSending ( CVector<uint8_t> ) ), this, SLOT ( OnSendProtMessCh9 ( CVector<uint8_t> ) ) );

    // request jitter buffer size
    QObject::connect ( &vecChannels[0], SIGNAL ( NewConnection() ), this, SLOT ( OnNewConnectionCh0() ) );
    QObject::connect ( &vecChannels[1], SIGNAL ( NewConnection() ), this, SLOT ( OnNewConnectionCh1() ) );
    QObject::connect ( &vecChannels[2], SIGNAL ( NewConnection() ), this, SLOT ( OnNewConnectionCh2() ) );
    QObject::connect ( &vecChannels[3], SIGNAL ( NewConnection() ), this, SLOT ( OnNewConnectionCh3() ) );
    QObject::connect ( &vecChannels[4], SIGNAL ( NewConnection() ), this, SLOT ( OnNewConnectionCh4() ) );
    QObject::connect ( &vecChannels[5], SIGNAL ( NewConnection() ), this, SLOT ( OnNewConnectionCh5() ) );
    QObject::connect ( &vecChannels[6], SIGNAL ( NewConnection() ), this, SLOT ( OnNewConnectionCh6() ) );
    QObject::connect ( &vecChannels[7], SIGNAL ( NewConnection() ), this, SLOT ( OnNewConnectionCh7() ) );
    QObject::connect ( &vecChannels[8], SIGNAL ( NewConnection() ), this, SLOT ( OnNewConnectionCh8() ) );
    QObject::connect ( &vecChannels[9], SIGNAL ( NewConnection() ), this, SLOT ( OnNewConnectionCh9() ) );

    // request connected clients list
    QObject::connect ( &vecChannels[0], SIGNAL ( ReqConnClientsList() ), this, SLOT ( OnReqConnClientsListCh0() ) );
    QObject::connect ( &vecChannels[1], SIGNAL ( ReqConnClientsList() ), this, SLOT ( OnReqConnClientsListCh1() ) );
    QObject::connect ( &vecChannels[2], SIGNAL ( ReqConnClientsList() ), this, SLOT ( OnReqConnClientsListCh2() ) );
    QObject::connect ( &vecChannels[3], SIGNAL ( ReqConnClientsList() ), this, SLOT ( OnReqConnClientsListCh3() ) );
    QObject::connect ( &vecChannels[4], SIGNAL ( ReqConnClientsList() ), this, SLOT ( OnReqConnClientsListCh4() ) );
    QObject::connect ( &vecChannels[5], SIGNAL ( ReqConnClientsList() ), this, SLOT ( OnReqConnClientsListCh5() ) );
    QObject::connect ( &vecChannels[6], SIGNAL ( ReqConnClientsList() ), this, SLOT ( OnReqConnClientsListCh6() ) );
    QObject::connect ( &vecChannels[7], SIGNAL ( ReqConnClientsList() ), this, SLOT ( OnReqConnClientsListCh7() ) );
    QObject::connect ( &vecChannels[8], SIGNAL ( ReqConnClientsList() ), this, SLOT ( OnReqConnClientsListCh8() ) );
    QObject::connect ( &vecChannels[9], SIGNAL ( ReqConnClientsList() ), this, SLOT ( OnReqConnClientsListCh9() ) );

    // channel name has changed
    QObject::connect ( &vecChannels[0], SIGNAL ( NameHasChanged() ), this, SLOT ( OnNameHasChangedCh0() ) );
    QObject::connect ( &vecChannels[1], SIGNAL ( NameHasChanged() ), this, SLOT ( OnNameHasChangedCh1() ) );
    QObject::connect ( &vecChannels[2], SIGNAL ( NameHasChanged() ), this, SLOT ( OnNameHasChangedCh2() ) );
    QObject::connect ( &vecChannels[3], SIGNAL ( NameHasChanged() ), this, SLOT ( OnNameHasChangedCh3() ) );
    QObject::connect ( &vecChannels[4], SIGNAL ( NameHasChanged() ), this, SLOT ( OnNameHasChangedCh4() ) );
    QObject::connect ( &vecChannels[5], SIGNAL ( NameHasChanged() ), this, SLOT ( OnNameHasChangedCh5() ) );
    QObject::connect ( &vecChannels[6], SIGNAL ( NameHasChanged() ), this, SLOT ( OnNameHasChangedCh6() ) );
    QObject::connect ( &vecChannels[7], SIGNAL ( NameHasChanged() ), this, SLOT ( OnNameHasChangedCh7() ) );
    QObject::connect ( &vecChannels[8], SIGNAL ( NameHasChanged() ), this, SLOT ( OnNameHasChangedCh8() ) );
    QObject::connect ( &vecChannels[9], SIGNAL ( NameHasChanged() ), this, SLOT ( OnNameHasChangedCh9() ) );

    // chat text received
    QObject::connect ( &vecChannels[0], SIGNAL ( ChatTextReceived ( QString ) ), this, SLOT ( OnChatTextReceivedCh0 ( QString ) ) );
    QObject::connect ( &vecChannels[1], SIGNAL ( ChatTextReceived ( QString ) ), this, SLOT ( OnChatTextReceivedCh1 ( QString ) ) );
    QObject::connect ( &vecChannels[2], SIGNAL ( ChatTextReceived ( QString ) ), this, SLOT ( OnChatTextReceivedCh2 ( QString ) ) );
    QObject::connect ( &vecChannels[3], SIGNAL ( ChatTextReceived ( QString ) ), this, SLOT ( OnChatTextReceivedCh3 ( QString ) ) );
    QObject::connect ( &vecChannels[4], SIGNAL ( ChatTextReceived ( QString ) ), this, SLOT ( OnChatTextReceivedCh4 ( QString ) ) );
    QObject::connect ( &vecChannels[5], SIGNAL ( ChatTextReceived ( QString ) ), this, SLOT ( OnChatTextReceivedCh5 ( QString ) ) );
    QObject::connect ( &vecChannels[6], SIGNAL ( ChatTextReceived ( QString ) ), this, SLOT ( OnChatTextReceivedCh6 ( QString ) ) );
    QObject::connect ( &vecChannels[7], SIGNAL ( ChatTextReceived ( QString ) ), this, SLOT ( OnChatTextReceivedCh7 ( QString ) ) );
    QObject::connect ( &vecChannels[8], SIGNAL ( ChatTextReceived ( QString ) ), this, SLOT ( OnChatTextReceivedCh8 ( QString ) ) );
    QObject::connect ( &vecChannels[9], SIGNAL ( ChatTextReceived ( QString ) ), this, SLOT ( OnChatTextReceivedCh9 ( QString ) ) );

    // ping message received
    QObject::connect ( &vecChannels[0], SIGNAL ( PingReceived ( int ) ), this, SLOT ( OnPingReceivedCh0 ( int ) ) );
    QObject::connect ( &vecChannels[1], SIGNAL ( PingReceived ( int ) ), this, SLOT ( OnPingReceivedCh1 ( int ) ) );
    QObject::connect ( &vecChannels[2], SIGNAL ( PingReceived ( int ) ), this, SLOT ( OnPingReceivedCh2 ( int ) ) );
    QObject::connect ( &vecChannels[3], SIGNAL ( PingReceived ( int ) ), this, SLOT ( OnPingReceivedCh3 ( int ) ) );
    QObject::connect ( &vecChannels[4], SIGNAL ( PingReceived ( int ) ), this, SLOT ( OnPingReceivedCh4 ( int ) ) );
    QObject::connect ( &vecChannels[5], SIGNAL ( PingReceived ( int ) ), this, SLOT ( OnPingReceivedCh5 ( int ) ) );
    QObject::connect ( &vecChannels[6], SIGNAL ( PingReceived ( int ) ), this, SLOT ( OnPingReceivedCh6 ( int ) ) );
    QObject::connect ( &vecChannels[7], SIGNAL ( PingReceived ( int ) ), this, SLOT ( OnPingReceivedCh7 ( int ) ) );
    QObject::connect ( &vecChannels[8], SIGNAL ( PingReceived ( int ) ), this, SLOT ( OnPingReceivedCh8 ( int ) ) );
    QObject::connect ( &vecChannels[9], SIGNAL ( PingReceived ( int ) ), this, SLOT ( OnPingReceivedCh9 ( int ) ) );
}

void CServer::OnSendProtMessage ( int iChID, CVector<uint8_t> vecMessage )
{
    // the protocol queries me to call the function to send the message
    // send it through the network
    Socket.SendPacket ( vecMessage, vecChannels[iChID].GetAddress() );
}

void CServer::Start()
{
    // only start if not already running
    if ( !IsRunning() )
    {
        // start timer
        HighPrecisionTimer.start();

        // init time for response time evaluation
        CycleTimeVariance.Reset();
    }
}

void CServer::Stop()
{
    // stop timer
    HighPrecisionTimer.stop();

    // logging (add "server stopped" logging entry)
    Logging.AddServerStopped();
}

void CServer::OnTimer()
{
    int i, j;

    CVector<int> vecChanID;
    CVector<CVector<double> >  vecvecdGains;
    CVector<CVector<int16_t> > vecvecsData;

    // Get data from all connected clients -------------------------------------
    bool bChannelIsNowDisconnected = false;

    // Make put and get calls thread safe. Do not forget to unlock mutex
    // afterwards!
    Mutex.lock();
    {
        // first, get number and IDs of connected channels
        vecChanID.Init ( 0 );
        for ( i = 0; i < USED_NUM_CHANNELS; i++ )
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
        vecvecdGains.Init ( iNumCurConnChan );
        vecvecsData.Init  ( iNumCurConnChan );

        for ( i = 0; i < iNumCurConnChan; i++ )
        {
            // get actual ID of current channel
            const int iCurChanID = vecChanID[i];

            // init vectors storing information of all channels
            vecvecdGains[i].Init ( iNumCurConnChan );
            vecvecsData[i].Init  ( SYSTEM_FRAME_SIZE_SAMPLES );

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
                celt_decode ( CeltDecoder[iCurChanID],
                              &vecbyData[0],
                              iCeltNumCodedBytes,
                              &vecvecsData[i][0] );
            }
            else
            {
                // lost packet
                celt_decode ( CeltDecoder[iCurChanID],
                              NULL,
                              0,
                              &vecvecsData[i][0] );
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
            vecsSendData = ProcessData ( vecvecsData, vecvecdGains[i] );

            // get current number of CELT coded bytes
            const int iCeltNumCodedBytes =
                vecChannels[iCurChanID].GetNetwFrameSize();

            // CELT encoding
            CVector<unsigned char> vecCeltData ( iCeltNumCodedBytes );

            celt_encode ( CeltEncoder[iCurChanID],
                          &vecsSendData[0],
                          NULL,
                          &vecCeltData[0],
                          iCeltNumCodedBytes );

            // send separate mix to current clients
            Socket.SendPacket (
                vecChannels[iCurChanID].PrepSendPacket ( vecCeltData ),
                vecChannels[iCurChanID].GetAddress() );
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

CVector<int16_t> CServer::ProcessData ( CVector<CVector<int16_t> >& vecvecsData,
                                        CVector<double>&            vecdGains )
{
    int i;

    // init return vector with zeros since we mix all channels on that vector

// TODO speed optimization: avoid using the zero vector, use the first valid
// data vector for initialization instead (take care of gain of this data, too!)

    CVector<int16_t> vecsOutData ( SYSTEM_FRAME_SIZE_SAMPLES, 0 );

    const int iNumClients = vecvecsData.Size();

    // mix all audio data from all clients together
    for ( int j = 0; j < iNumClients; j++ )
    {
        // if channel gain is 1, avoid multiplication for speed optimization
        if ( vecdGains[j] == static_cast<double> ( 1.0 ) )
        {
            for ( i = 0; i < SYSTEM_FRAME_SIZE_SAMPLES; i++ )
            {
                vecsOutData[i] =
                    Double2Short ( vecsOutData[i] + vecvecsData[j][i] );
            }
        }
        else
        {
            for ( i = 0; i < SYSTEM_FRAME_SIZE_SAMPLES; i++ )
            {
                vecsOutData[i] =
                    Double2Short ( vecsOutData[i] +
                    vecvecsData[j][i] * vecdGains[j] );
            }
        }
    }

    return vecsOutData;
}

CVector<CChannelShortInfo> CServer::CreateChannelList()
{
    CVector<CChannelShortInfo> vecChanInfo ( 0 );

    // look for free channels
    for ( int i = 0; i < USED_NUM_CHANNELS; i++ )
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
    for ( int i = 0; i < USED_NUM_CHANNELS; i++ )
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

void CServer::CreateAndSendChatTextForAllConChannels ( const int iCurChanID,
                                                       const QString& strChatText )
{
    // create message which is sent to all connected clients -------------------
    // get client name, if name is empty, use IP address instead
    QString ChanName = vecChannels[iCurChanID].GetName();
    if ( ChanName.isEmpty() )
    {
        // convert IP address to text and show it
        ChanName =
            vecChannels[iCurChanID].GetAddress().GetIpAddressStringNoLastByte();
    }

    // add time and name of the client at the beginning of the message text and
    // use different colors
    QString sCurColor = vstrChatColors[iCurChanID % vstrChatColors.Size()];
    const QString strActualMessageText =
        "<font color=""" + sCurColor + """>(" +
        QTime::currentTime().toString ( "hh:mm:ss AP" ) + ") <b>" + ChanName +
        "</b></font> " + strChatText;


    // send chat text to all connected clients ---------------------------------
    for ( int i = 0; i < USED_NUM_CHANNELS; i++ )
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
    for ( int i = 0; i < USED_NUM_CHANNELS; i++ )
    {
        if ( !vecChannels[i].IsConnected() )
        {
            return i;
        }
    }

    // no free channel found, return invalid ID
    return INVALID_CHANNEL_ID;
}

int CServer::CheckAddr ( const CHostAddress& Addr )
{
    CHostAddress InetAddr;

    // check for all possible channels if IP is already in use
    for ( int i = 0; i < USED_NUM_CHANNELS; i++ )
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
    bool bAudioOK            = false;
    bool bNewChannelReserved = false;

    Mutex.lock();
    {
        bool bChanOK = true;

        // Get channel ID ------------------------------------------------------
        // check address
        int iCurChanID = CheckAddr ( HostAdr );

        if ( iCurChanID == INVALID_CHANNEL_ID )
        {
            // a new client is calling, look for free channel
            iCurChanID = GetFreeChan();

            if ( iCurChanID != INVALID_CHANNEL_ID )
            {
                // initialize current channel by storing the calling host
                // address
                vecChannels[iCurChanID].SetAddress ( HostAdr );

                // reset the channel gains of current channel, at the same
                // time reset gains of this channel ID for all other channels
                for ( int i = 0; i < USED_NUM_CHANNELS; i++ )
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

// TODO problem: if channel is not officially connected, we cannot send messages

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
                bAudioOK = true; // in case we have an audio packet, return true
                break;

            case PS_AUDIO_ERR:
                PostWinMessage ( MS_JIT_BUF_PUT, MUL_COL_LED_RED, iCurChanID );
                bAudioOK = true; // in case we have an audio packet, return true
                break;

            case PS_PROT_ERR:
                PostWinMessage ( MS_JIT_BUF_PUT, MUL_COL_LED_YELLOW, iCurChanID );

// TEST
bAudioOK = true;

                break;
            }
        }

        // after data is put in channel buffer, create channel list message if
        // requested
        if ( bNewChannelReserved && bAudioOK )
        {
            // logging of new connected channel
            Logging.AddNewConnection ( HostAdr.InetAddr );

            // A new client connected to the server, create and
            // send all clients the updated channel list (the list has to
            // be created after the received data has to be put to the
            // channel first so that this channel is marked as connected)
            //
            // Usually it is not required to send the channel list to the
            // client currently connecting since it automatically requests
            // the channel list on a new connection (as a result, he will
            // usually get the list twice which has no impact on functionality
            // but will only increase the network load a tiny little bit). But
            // in case the client thinks he is still connected but the server
            // was restartet, it is important that we send the channel list
            // at this place.


// TODO this does not work somehow (another problem: the channel name
// is not yet received from the new client)
// possible solution: create (new) request channel name message, if this one
// is received, the channel list for all clients are automatically sent
// by the server

            CreateAndSendChanListForAllConChannels();
        }
    }
    Mutex.unlock();

    return bAudioOK;
}

void CServer::GetConCliParam ( CVector<CHostAddress>& vecHostAddresses,
                               CVector<QString>&      vecsName,
                               CVector<int>&          veciJitBufNumFrames,
                               CVector<int>&          veciNetwFrameSizeFact )
{
    CHostAddress InetAddr;

    // init return values
    vecHostAddresses.Init      ( USED_NUM_CHANNELS );
    vecsName.Init              ( USED_NUM_CHANNELS );
    veciJitBufNumFrames.Init   ( USED_NUM_CHANNELS );
    veciNetwFrameSizeFact.Init ( USED_NUM_CHANNELS );

    // check all possible channels
    for ( int i = 0; i < USED_NUM_CHANNELS; i++ )
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
    for ( int i = 0; i < USED_NUM_CHANNELS; i++ )
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
        for ( int i = 0; i < USED_NUM_CHANNELS; i++ )
        {
            if ( vecChannels[i].IsConnected() )
            {
                QString strCurChanName = vecChannels[i].GetName();

                // if text is empty, show IP address instead
                if ( strCurChanName.isEmpty() )
                {
                    // convert IP address to text and show it, remove last
                    // digits
                    strCurChanName =
                        vecChannels[i].GetAddress().GetIpAddressStringNoLastByte();
                }

                streamFileOut << "  <li>" << strCurChanName << "</li>" << endl;
            }
        }
    }

    // finish list
    streamFileOut << "</ul>" << endl;
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
