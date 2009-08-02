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
    iSndCrdPreferredMonoBlSizeIndex ( FRAME_SIZE_FACTOR_DEFAULT )
{
    // connection for protocol
    QObject::connect ( &Channel,
        SIGNAL ( MessReadyForSending ( CVector<uint8_t> ) ),
        this, SLOT ( OnSendProtMessage ( CVector<uint8_t> ) ) );

    QObject::connect ( &Channel, SIGNAL ( ReqJittBufSize() ),
        this, SLOT ( OnReqJittBufSize() ) );

    QObject::connect ( &Channel,
        SIGNAL ( ConClientListMesReceived ( CVector<CChannelShortInfo> ) ),
        SIGNAL ( ConClientListMesReceived ( CVector<CChannelShortInfo> ) ) );

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

void CClient::SetSndCrdPreferredMonoBlSizeIndex ( const int iNewIdx )
{
    // right now we simply set the internal value
    if ( ( iNewIdx == FRAME_SIZE_FACTOR_PREFERRED ) ||
         ( iNewIdx == FRAME_SIZE_FACTOR_DEFAULT ) ||
         ( iNewIdx == FRAME_SIZE_FACTOR_SAFE ) )
    {
        iSndCrdPreferredMonoBlSizeIndex = iNewIdx;

        // init with new parameter, if client was running then first
        // stop it and restart again after new initialization
        const bool bWasRunning = Sound.IsRunning();
        if ( bWasRunning )
        {
            Sound.Stop();
        }

        // init with new block size index parameter
        Init ( iSndCrdPreferredMonoBlSizeIndex );

        if ( bWasRunning )
        {
            Sound.Start();
        }
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
    Init ( iSndCrdPreferredMonoBlSizeIndex );

    if ( bWasRunning )
    {
        Sound.Start();
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
    Init ( iSndCrdPreferredMonoBlSizeIndex );

    if ( bWasRunning )
    {
        Sound.Start();
    }
}

void CClient::Start()
{
    // init object
    Init ( iSndCrdPreferredMonoBlSizeIndex );

    // enable channel
    Channel.SetEnable ( true );

    // start audio interface
    Sound.Start();
}

void CClient::Stop()
{
    // stop audio interface
    Sound.Stop();

    // send disconnect message to server (since we disable our protocol
    // receive mechanism with the next command, we do not evaluate any
    // respond from the server, therefore we just hope that the message
    // gets its way to the server, if not, the old behaviour time-out
    // disconnects the connection anyway)
    Channel.CreateDisconnectionMes();

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

void CClient::Init ( const int iPrefMonoBlockSizeSamIndexAtSndCrdSamRate )
{
    // translate block size index in actual block size
    const int iPrefMonoBlockSizeSamAtSndCrdSamRate =
        iPrefMonoBlockSizeSamIndexAtSndCrdSamRate * SYSTEM_BLOCK_FRAME_SAMPLES;

    // get actual sound card buffer size using preferred size
    iMonoBlockSizeSam   = Sound.Init ( iPrefMonoBlockSizeSamAtSndCrdSamRate );
    iStereoBlockSizeSam = 2 * iMonoBlockSizeSam;

    vecsAudioSndCrdStereo.Init ( iStereoBlockSizeSam );

    vecdAudioStereo.Init ( iStereoBlockSizeSam );

    // init response time evaluation
    CycleTimeVariance.Init ( iMonoBlockSizeSam,
        SYSTEM_SAMPLE_RATE, TIME_MOV_AV_RESPONSE );

    CycleTimeVariance.Reset();

    // init reverberation
    AudioReverb.Init ( SYSTEM_SAMPLE_RATE );

    // init audio endocder/decoder (mono)
    CeltMode = celt_mode_create (
        SYSTEM_SAMPLE_RATE, 1, iMonoBlockSizeSam, NULL );

    CeltEncoder = celt_encoder_create ( CeltMode );
    CeltDecoder = celt_decoder_create ( CeltMode );

    // 16: low/normal quality   132 kbsp (128) / 90 kbps (256)
    // 40: high good            204 kbps (128) / 162 kbps (256)
    iCeltNumCodedBytes = 16;

    vecCeltData.Init ( iCeltNumCodedBytes );

    // init network buffers
    vecsNetwork.Init   ( iMonoBlockSizeSam );
    vecbyNetwData.Init ( iCeltNumCodedBytes );

    // set the channel network properties

// TEST right now we only support 128 samples, later 256 and 512, too
Channel.SetNetwFrameSizeAndFact ( iCeltNumCodedBytes,
                                  1 ); // TEST "1"

}

void CClient::ProcessAudioData ( CVector<int16_t>& vecsStereoSndCrd )
{
    int i, j;

    // update stereo signal level meter
    SignalLevelMeter.Update ( vecsStereoSndCrd );

    // convert data from short to double
    for ( i = 0; i < iStereoBlockSizeSam; i++ )
    {
        vecdAudioStereo[i] = (double) vecsStereoSndCrd[i];
    }

    // add reverberation effect if activated
    if ( iReverbLevel != 0 )
    {
        // calculate attenuation amplification factor
        const double dRevLev = (double) iReverbLevel / AUD_REVERB_MAX / 2;

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
            (double) ( AUD_FADER_IN_MIDDLE - abs ( AUD_FADER_IN_MIDDLE - iAudioInFader ) ) /
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

    // send it through the network
//    Socket.SendPacket ( Channel.PrepSendPacket ( vecsNetwork ),
//        Channel.GetAddress() );

celt_encode ( CeltEncoder,
              &vecsNetwork[0],
              NULL,
              &vecCeltData[0],
              iCeltNumCodedBytes );

Socket.SendPacket ( vecCeltData, Channel.GetAddress() );




    // receive a new block
    if ( Channel.GetData ( vecbyNetwData ) == GS_BUFFER_OK )
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
for (i = 0; i < iMonoBlockSizeSam; i++)
{
sData[0] = (short) vecdNetwData[i];
fwrite(&sData, size_t(2), size_t(1), pFileDelay);
}
fflush(pFileDelay);
*/

    // check if channel is connected
    if ( Channel.IsConnected() )
    {
/*
        // convert data from double to short type and copy mono
        // received data in both sound card channels
        for ( i = 0, j = 0; i < iSndCrdMonoBlockSizeSam; i++, j += 2 )
        {
            vecsStereoSndCrd[j] = vecsStereoSndCrd[j + 1] =
                Double2Short ( vecdNetwData[i] );
        }
*/


// TEST CELT
CVector<short> vecsAudioSndCrdMono ( iMonoBlockSizeSam );

celt_decode ( CeltDecoder,
              &vecbyNetwData[0],
              iCeltNumCodedBytes,
              &vecsAudioSndCrdMono[0] );

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
        // Use worst case scenario: We add the block size of input and
        // output. This is not required if the smaller block size is a
        // multiple of the bigger size, but in the general case where
        // the block sizes do not have this relation, we require to have
        // a minimum buffer size of the sum of both sizes
        const double dAudioBufferDurationMs =
            ( 2 * iMonoBlockSizeSam ) * 1000 / SYSTEM_SAMPLE_RATE;

        // accumulate the standard deviations of input network stream and
        // internal timer,
        // add 0.5 to "round up" -> ceil,
        // divide by MIN_SERVER_BLOCK_DURATION_MS because this is the size of
        // one block in the jitter buffer
        const double dEstCurBufSet = ( dAudioBufferDurationMs +
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
