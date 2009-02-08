/******************************************************************************\
 * Copyright (c) 2004-2008
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
CClient::CClient ( const quint16 iPortNumber ) : bRun ( false ),
    iSndCrdBlockSizeSam ( MIN_SND_CRD_BLOCK_SIZE_SAMPLES ),
    Sound ( MIN_SND_CRD_BLOCK_SIZE_SAMPLES * 2 /* stereo */ ),
    Socket ( &Channel, iPortNumber ),
    iAudioInFader ( AUD_FADER_IN_MAX / 2 ),
    iReverbLevel ( AUD_REVERB_MAX / 6 ),
    bReverbOnLeftChan ( false ),
    iNetwBufSizeFactIn ( DEF_NET_BLOCK_SIZE_FACTOR ),
    strIPAddress ( "" ), strName ( "" ),
    bOpenChatOnNewMessage ( true )
{
    // connection for protocol
    QObject::connect ( &Channel,
        SIGNAL ( MessReadyForSending ( CVector<uint8_t> ) ),
        this, SLOT ( OnSendProtMessage ( CVector<uint8_t> ) ) );

    QObject::connect ( &Channel, SIGNAL ( ReqJittBufSize() ),
        this, SLOT ( OnReqJittBufSize() ) );

    QObject::connect ( &Channel, SIGNAL ( ProtocolStatus ( bool ) ),
        this, SLOT ( OnProtocolStatus ( bool ) ) );

    QObject::connect ( &Channel,
        SIGNAL ( ConClientListMesReceived ( CVector<CChannelShortInfo> ) ),
        SIGNAL ( ConClientListMesReceived ( CVector<CChannelShortInfo> ) ) );

    QObject::connect ( &Channel, SIGNAL ( NewConnection() ),
        this, SLOT ( OnNewConnection() ) );

    QObject::connect ( &Channel, SIGNAL ( ChatTextReceived ( QString ) ),
        this, SIGNAL ( ChatTextReceived ( QString ) ) );

    QObject::connect ( &Channel, SIGNAL ( PingReceived ( int ) ),
        this, SLOT ( OnReceivePingMessage ( int ) ) );
}

void CClient::OnSendProtMessage ( CVector<uint8_t> vecMessage )
{

// convert unsigned uint8_t in char, TODO convert all buffers in uint8_t
CVector<unsigned char> vecbyDataConv ( vecMessage.Size() );
for ( int i = 0; i < vecMessage.Size(); i++ ) {
    vecbyDataConv[i] = static_cast<unsigned char> ( vecMessage[i] );
}


    // the protocol queries me to call the function to send the message
    // send it through the network
    Socket.SendPacket ( vecbyDataConv, Channel.GetAddress() );
}

void CClient::OnReqJittBufSize()
{
    Channel.CreateJitBufMes ( Channel.GetSockBufSize() );

// FIXME: we set the network buffer size factor here, too -> in the
// future a separate request function for this parameter should be created
    Channel.CreateNetwBlSiFactMes ( iNetwBufSizeFactIn );
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
    Channel.CreateReqConnClientsList();
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
    quint16      iNetPort = LLCON_PORT_NUMBER;

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

void CClient::OnProtocolStatus ( bool bOk )
{
    // show protocol status in GUI
    if ( bOk )
    {
        PostWinMessage ( MS_PROTOCOL, MUL_COL_LED_RED );
    }
    else
    {
        PostWinMessage ( MS_PROTOCOL, MUL_COL_LED_GREEN );
    }
}

void CClient::Init()
{
    // set block size (in samples)
    iBlockSizeSam = MIN_BLOCK_SIZE_SAMPLES;

    vecsAudioSndCrd.Init  ( iSndCrdBlockSizeSam * 2 ); // stereo
    vecdAudioSndCrdL.Init ( iSndCrdBlockSizeSam );
    vecdAudioSndCrdR.Init ( iSndCrdBlockSizeSam );

    vecdAudioL.Init ( iBlockSizeSam );
    vecdAudioR.Init ( iBlockSizeSam );

    Sound.InitRecording();
    Sound.InitPlayback();

    // resample objects are always initialized with the input block size
    // record
    ResampleObjDownL.Init ( iSndCrdBlockSizeSam, SND_CRD_SAMPLE_RATE, SYSTEM_SAMPLE_RATE );
    ResampleObjDownR.Init ( iSndCrdBlockSizeSam, SND_CRD_SAMPLE_RATE, SYSTEM_SAMPLE_RATE );

    // playback
    ResampleObjUpL.Init ( iBlockSizeSam, SYSTEM_SAMPLE_RATE, SND_CRD_SAMPLE_RATE );
    ResampleObjUpR.Init ( iBlockSizeSam, SYSTEM_SAMPLE_RATE, SND_CRD_SAMPLE_RATE );

    // init network buffers
    vecsNetwork.Init  ( iBlockSizeSam );
    vecdNetwData.Init ( iBlockSizeSam );

    // init moving average buffer for response time evaluation
    RespTimeMoAvBuf.Init ( LEN_MOV_AV_RESPONSE );

    // init time for response time evaluation
    TimeLastBlock = PreciseTime.elapsed();

    AudioReverb.Clear();
}

void CClient::run()
{
    int i, iInCnt;

    // Set thread priority (The working thread should have a higher
    // priority than the GUI)
#ifdef _WIN32
    SetThreadPriority ( GetCurrentThread(), THREAD_PRIORITY_ABOVE_NORMAL );
#else
/*
    // set the process to realtime privs, taken from
    // "http://www.gardena.net/benno/linux/audio" but does not seem to work,
    // maybe a problem with user rights
    struct sched_param schp;
    memset ( &schp, 0, sizeof ( schp ) );
    schp.sched_priority = sched_get_priority_max ( SCHED_FIFO );
    sched_setscheduler ( 0, SCHED_FIFO, &schp );
*/
#endif

    // init object
    try
    {
        Init();
    }
    catch ( CGenErr generr )
    {
        // TODO better error management -> should be catched in main thread
        // problem: how to catch errors in a different thread...?

        // quick hack solution
        QMessageBox::critical ( 0, APP_NAME, generr.GetErrorText(), "Quit", 0 );
        exit ( 0 );
    }


    // runtime phase ------------------------------------------------------------
    // enable channel
    Channel.SetEnable ( true );

    bRun = true;

    // main loop of working thread
    while ( bRun )
    {
        // get audio from sound card (blocking function)
        if ( Sound.Read ( vecsAudioSndCrd ) )
        {
            PostWinMessage ( MS_SOUND_IN, MUL_COL_LED_RED );
        }
        else
        {
            PostWinMessage ( MS_SOUND_IN, MUL_COL_LED_GREEN );
        }

        // copy data from one stereo buffer in two separate buffers
        iInCnt = 0;
        for ( i = 0; i < iSndCrdBlockSizeSam; i++ )
        {
            vecdAudioSndCrdL[i] = (double) vecsAudioSndCrd[iInCnt++];
            vecdAudioSndCrdR[i] = (double) vecsAudioSndCrd[iInCnt++];
        }

        // resample data for each channel seaparately
        ResampleObjDownL.Resample ( vecdAudioSndCrdL, vecdAudioL );
        ResampleObjDownR.Resample ( vecdAudioSndCrdR, vecdAudioR );

        // update signal level meters
        SignalLevelMeterL.Update ( vecdAudioL );
        SignalLevelMeterR.Update ( vecdAudioR );

        // add reverberation effect if activated
        if ( iReverbLevel != 0 )
        {
            // calculate attenuation amplification factor
            const double dRevLev = (double) iReverbLevel / AUD_REVERB_MAX / 2;

            if ( bReverbOnLeftChan )
            {
                for ( i = 0; i < iBlockSizeSam; i++ )
                {
                    // left channel
                    vecdAudioL[i] +=
                        dRevLev * AudioReverb.ProcessSample ( vecdAudioL[i] );
                }
            }
            else
            {
                for ( i = 0; i < iBlockSizeSam; i++ )
                {
                    // right channel
                    vecdAudioR[i] +=
                        dRevLev * AudioReverb.ProcessSample ( vecdAudioR[i] );
                }
            }
        }

        // mix both signals depending on the fading setting
        const int iMiddleOfFader = AUD_FADER_IN_MAX / 2;
        const double dAttFact =
            (double) ( iMiddleOfFader - abs ( iMiddleOfFader - iAudioInFader ) ) /
            iMiddleOfFader;

        for ( i = 0; i < iBlockSizeSam; i++ )
        {
            double dMixedSignal;

            if ( iAudioInFader > iMiddleOfFader )
            {
                dMixedSignal = vecdAudioL[i] + dAttFact * vecdAudioR[i];
            }
            else
            {
                dMixedSignal = vecdAudioR[i] + dAttFact * vecdAudioL[i];
            }

            vecsNetwork[i] = Double2Short ( dMixedSignal );
        }

        // send it through the network
        Socket.SendPacket ( Channel.PrepSendPacket ( vecsNetwork ),
            Channel.GetAddress() );

        // receive a new block
        if ( Channel.GetData ( vecdNetwData ) == GS_BUFFER_OK )
        {
            PostWinMessage ( MS_JIT_BUF_GET, MUL_COL_LED_GREEN );
        }
        else
        {
            PostWinMessage ( MS_JIT_BUF_GET, MUL_COL_LED_RED );
        }

/*
// TEST
// fid=fopen('v.dat','r');x=fread(fid,'int16');fclose(fid);
static FILE* pFileDelay = fopen("v.dat", "wb");
short sData[2];
for (i = 0; i < iBlockSizeSam; i++)
{
    sData[0] = (short) vecdNetwData[i];
    fwrite(&sData, size_t(2), size_t(1), pFileDelay);
}
fflush(pFileDelay);
*/

        // check if channel is connected
        if ( Channel.IsConnected() )
        {
            // write mono input signal in both sound-card channels
            for ( i = 0; i < iBlockSizeSam; i++ )
            {
                vecdAudioL[i] = vecdAudioR[i] = vecdNetwData[i];
            }
        }
        else
        {
            // if not connected, clear data
            for ( i = 0; i < iBlockSizeSam; i++ )
            {
                vecdAudioL[i] = vecdAudioR[i] = 0.0;
            }
        }

        // resample data for each channel separately
        ResampleObjUpL.Resample ( vecdAudioL, vecdAudioSndCrdL );
        ResampleObjUpR.Resample ( vecdAudioR, vecdAudioSndCrdR );

        // copy data from one stereo buffer in two separate buffers
        iInCnt = 0;
        for ( i = 0; i < iSndCrdBlockSizeSam; i++ )
        {
            vecsAudioSndCrd[iInCnt++] = Double2Short ( vecdAudioSndCrdL[i] );
            vecsAudioSndCrd[iInCnt++] = Double2Short ( vecdAudioSndCrdR[i] );
        }

        // play the new block
        if ( Sound.Write ( vecsAudioSndCrd ) )
        {
            PostWinMessage ( MS_SOUND_OUT, MUL_COL_LED_RED );
        }
        else
        {
            PostWinMessage ( MS_SOUND_OUT, MUL_COL_LED_GREEN );
        }


        // update response time measurement ------------------------------------
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

    // disable channel
    Channel.SetEnable ( false );

    // disable sound interface
    Sound.Close();

    // reset current signal level and LEDs
    SignalLevelMeterL.Reset();
    SignalLevelMeterR.Reset();
    PostWinMessage ( MS_RESET_ALL, 0 );
}

bool CClient::Stop()
{
    // set flag so that thread can leave the main loop
    bRun = false;

    // give thread some time to terminate, return status
    return wait ( 5000 );
}
