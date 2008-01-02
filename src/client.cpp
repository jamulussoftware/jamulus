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
CClient::CClient() : bRun ( false ), Socket ( &Channel ),
        iAudioInFader ( AUD_FADER_IN_MAX / 2 ),
        iReverbLevel ( AUD_REVERB_MAX / 6 ),
        bReverbOnLeftChan ( false ),
        iNetwBufSizeFactIn ( DEF_NET_BLOCK_SIZE_FACTOR ),
        strIPAddress ( "" ), strName ( "" )
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
    Channel.CreateReqConnClientsList();
}

bool CClient::SetServerAddr ( QString strNAddr )
{
    QHostAddress InetAddr;

    // first try if this is an IP number an can directly applied to QHostAddress
    if ( !InetAddr.setAddress ( strNAddr ) )
    {

// TODO implement the IP number query with QT objects (this is not possible with
// QT 2 so we have to implement a workaround solution here

        // it was no vaild IP address, try to get host by name, assuming
        // that the string contains a valid host name string
        const hostent* HostInf = gethostbyname ( strNAddr.latin1() );

        if ( HostInf )
        {
            uint32_t dwIPNumber;

            // copy IP address of first found host in host list
            memcpy ( (char*) &dwIPNumber, HostInf->h_addr_list[0], sizeof ( dwIPNumber ) );

            // apply IP address to QT object (change byte order, too)
            InetAddr.setAddress ( htonl ( dwIPNumber ) );
        }
        else
        {
            return false; // invalid address
        }
    }

    // apply address (the server port is fixed and always the same)
    Channel.SetAddress ( CHostAddress ( InetAddr, LLCON_PORT_NUMBER ) );

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
    // set block sizes (in samples)
    iBlockSizeSam = MIN_BLOCK_SIZE_SAMPLES;
    iSndCrdBlockSizeSam = MIN_SND_CRD_BLOCK_SIZE_SAMPLES;

    vecsAudioSndCrd.Init  ( iSndCrdBlockSizeSam * 2 ); // stereo
    vecdAudioSndCrdL.Init ( iSndCrdBlockSizeSam );
    vecdAudioSndCrdR.Init ( iSndCrdBlockSizeSam );

    vecdAudioL.Init ( iBlockSizeSam );
    vecdAudioR.Init ( iBlockSizeSam );

    Sound.InitRecording ( iSndCrdBlockSizeSam * 2 ); // stereo
    Sound.InitPlayback  ( iSndCrdBlockSizeSam * 2 ); // stereo

    // resample objects are always initialized with the input block size
    // record
    ResampleObjDownL.Init ( iSndCrdBlockSizeSam, SND_CRD_SAMPLE_RATE, SAMPLE_RATE );
    ResampleObjDownR.Init ( iSndCrdBlockSizeSam, SND_CRD_SAMPLE_RATE, SAMPLE_RATE );

    // playback
    ResampleObjUpL.Init ( iBlockSizeSam, SAMPLE_RATE, SND_CRD_SAMPLE_RATE );
    ResampleObjUpR.Init ( iBlockSizeSam, SAMPLE_RATE, SND_CRD_SAMPLE_RATE );

    // init network buffers
    vecsNetwork.Init  ( iBlockSizeSam );
    vecdNetwData.Init ( iBlockSizeSam );

    // init moving average buffer for response time evaluation
    RespTimeMoAvBuf.Init ( LEN_MOV_AV_RESPONSE );

    // init time for response time evaluation
    TimeLastBlock = QTime::currentTime();

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
    // set the process to realtime privs, taken from
    // "http://www.gardena.net/benno/linux/audio" but does not seem to work,
    // maybe a problem with user rights
    struct sched_param schp;
    memset ( &schp, 0, sizeof ( schp ) );
    schp.sched_priority = sched_get_priority_max ( SCHED_FIFO );
    sched_setscheduler ( 0, SCHED_FIFO, &schp );
#endif

    // init object
    Init();


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
            // first attenuation amplification factor
            const double dRevLev = (double) iReverbLevel / AUD_REVERB_MAX / 2;

            if ( bReverbOnLeftChan )
            {
                for (i = 0; i < iBlockSizeSam; i++)
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
            Channel.GetAddress () );

        // receive a new block
        if ( Channel.GetData ( vecdNetwData ) == GS_BUFFER_OK )
        {
            PostWinMessage ( MS_JIT_BUF_GET, MUL_COL_LED_GREEN );
        }
        else
        {
            PostWinMessage ( MS_JIT_BUF_GET, MUL_COL_LED_RED );
        }

#ifdef _DEBUG_
#if 0
#if 0
/* Determine network delay. We can do this very simple if only this client is
   connected to the server. In this case, exactly the same audio material is
   coming back and we can simply compare the samples */
/* store send data instatic buffer (may delay is 100 ms) */
const int iMaxDelaySamples = (int) ((float)       0.3     /*0.1*/ * SAMPLE_RATE);
static CVector<short> vecsOutBuf(iMaxDelaySamples);

/* update buffer */
const int iBufDiff = iMaxDelaySamples - iBlockSizeSam;
for (i = 0; i < iBufDiff; i++)
    vecsOutBuf[i + iBlockSizeSam] = vecsOutBuf[i];
for (i = 0; i < iBlockSizeSam; i++)
    vecsOutBuf[i] = vecsNetwork[i];

/* now search for equal samples */
int iDelaySamples = 0;
for (i = 0; i < iMaxDelaySamples - 1; i++)
{
    /* compare two successive samples */
    if ((vecsOutBuf[i] == (short) vecdNetwData[0]) &&
        (vecsOutBuf[i + 1] == (short) vecdNetwData[1]))
    {
        iDelaySamples = i;
    }
}

static FILE* pFileDelay = fopen("delay.dat", "w");
fprintf(pFileDelay, "%d\n", iDelaySamples);
fflush(pFileDelay);
#else
/* just store both, input and output, streams */
// fid=fopen('v.dat','r');x=fread(fid,'int16');fclose(fid);
static FILE* pFileDelay = fopen("v.dat", "wb");
short sData[2];
for (i = 0; i < iBlockSizeSam; i++)
{
    sData[0] = vecsNetwork[i];
    sData[1] = (short) vecdNetwData[i];
    fwrite(&sData, size_t(2), size_t(2), pFileDelay);
}
fflush(pFileDelay);
#endif
#endif
#endif


/*
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
        const QTime CurTime = QTime::currentTime();

        // we want to calculate the standard deviation (we assume that the mean
        // is correct at the block period time)
        const double dCurAddVal =
            ( (double) TimeLastBlock.msecsTo ( CurTime ) - MIN_BLOCK_DURATION_MS );

/*
// TEST
static FILE* pFileTest = fopen("sti.dat", "w");
fprintf(pFileTest, "%e\n", dCurAddVal);
fflush(pFileTest);
*/

        RespTimeMoAvBuf.Add ( dCurAddVal * dCurAddVal ); // add squared value

        // store old time value
        TimeLastBlock = CurTime;
    }

    // disable channel
    Channel.SetEnable ( false );

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
