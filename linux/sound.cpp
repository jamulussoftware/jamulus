/******************************************************************************\
 * Copyright (c) 2004-2022
 *
 * Author(s):
 *  Volker Fischer, revised and maintained by Peter Goderie (pgScorpio)
 *
 * This code is based on the simple_client example of the Jack audio interface.
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

#include "sound.h"
#include "jackclient.h"
#include "global.h"

//============================================================================
// CSound:
//============================================================================

CSound* CSound::pSound = NULL;

CSound::CSound ( void ( *theProcessCallback ) ( CVector<short>& psData, void* arg ), void* theProcessCallbackArg ) :
    CSoundBase ( "Jack", theProcessCallback, theProcessCallbackArg ),
    jackClient ( strClientName ), // ClientName from CSoundBase !
    bJackWasShutDown ( false ),
    bAutoConnect ( true ),
    iJackNumInputs ( 2 )
{
    CCommandlineOptions cCommandlineOptions;
    double              dValue;

    setObjectName ( "CSoundThread" );

    if ( cCommandlineOptions.GetFlagArgument ( CMDLN_NOJACKCONNECT ) )
    {
        bAutoConnect = false;
    }

    if ( cCommandlineOptions.GetNumericArgument ( CMDLN_JACKINPUTS, 2, 16, dValue ) )
    {
        iJackNumInputs = static_cast<int> ( dValue );
    }

    soundProperties.bHasSetupDialog = false;

    pSound = this;
}

//===============================
// JACK callbacks:
//===============================

int CSound::onBufferSwitch ( jack_nframes_t nframes, void* /* arg */ )
{
    // make sure we are locked during execution
    QMutexLocker locker ( &mutexAudioProcessCallback );

    if ( !IsStarted() )
    {
        return -1;
    }

    // get output selections
    int iSelOutLeft  = selectedOutputChannels[0];
    int iSelOutRight = selectedOutputChannels[1];

    // get input selections
    int iSelInLeft, iSelAddInLeft;
    int iSelInRight, iSelAddInRight;
    getInputSelAndAddChannels ( selectedInputChannels[0], lNumInChan, lNumAddedInChan, iSelInLeft, iSelAddInLeft );
    getInputSelAndAddChannels ( selectedInputChannels[1], lNumInChan, lNumAddedInChan, iSelInRight, iSelAddInRight );

    if ( nframes == static_cast<jack_nframes_t> ( iDeviceBufferSize ) )
    {
        // get input data pointers
        jack_default_audio_sample_t* in_left  = (jack_default_audio_sample_t*) jackClient.AudioInput[iSelInLeft].GetBuffer ( nframes );
        jack_default_audio_sample_t* in_right = (jack_default_audio_sample_t*) jackClient.AudioInput[iSelInRight].GetBuffer ( nframes );
        jack_default_audio_sample_t* add_left =
            ( iSelAddInLeft >= 0 ) ? (jack_default_audio_sample_t*) jackClient.AudioInput[iSelAddInLeft].GetBuffer ( nframes ) : nullptr;
        jack_default_audio_sample_t* add_right =
            ( iSelAddInRight >= 0 ) ? (jack_default_audio_sample_t*) jackClient.AudioInput[iSelAddInRight].GetBuffer ( nframes ) : nullptr;

        // copy input audio data
        if ( ( in_left != nullptr ) && ( in_right != nullptr ) )
        {
            for ( unsigned int i = 0; i < iDeviceBufferSize; i++ )
            {
                unsigned int i_dest = ( i + i );
                audioBuffer[i_dest] = Float2Short ( in_left[i] * _MAXSHORT );
                if ( add_left != nullptr )
                    audioBuffer[i_dest] += Float2Short ( add_left[i] * _MAXSHORT );

                i_dest++;
                audioBuffer[i_dest] = Float2Short ( in_right[i] * _MAXSHORT );
                if ( add_right != nullptr )
                    audioBuffer[i_dest] += Float2Short ( add_right[i] * _MAXSHORT );
            }
        }

        // call processing callback function
        processCallback ( audioBuffer );

        // get output data pointer
        jack_default_audio_sample_t* out_left  = (jack_default_audio_sample_t*) jackClient.AudioOutput[iSelOutLeft].GetBuffer ( nframes );
        jack_default_audio_sample_t* out_right = (jack_default_audio_sample_t*) jackClient.AudioOutput[iSelOutRight].GetBuffer ( nframes );

        // copy output data
        if ( ( out_left != nullptr ) && ( out_right != nullptr ) )
        {
            for ( unsigned int i = 0; i < iDeviceBufferSize; i++ )
            {
                unsigned int i_src = ( i + i );
                out_left[i]        = (jack_default_audio_sample_t) audioBuffer[i_src] / _MAXSHORT;

                out_right[i] = (jack_default_audio_sample_t) audioBuffer[++i_src] / _MAXSHORT;
            }
        }
    }
    else
    {
        // get output data pointer
        jack_default_audio_sample_t* out_left  = (jack_default_audio_sample_t*) jackClient.AudioOutput[iSelOutLeft].GetBuffer ( nframes );
        jack_default_audio_sample_t* out_right = (jack_default_audio_sample_t*) jackClient.AudioOutput[iSelOutRight].GetBuffer ( nframes );

        // clear output data
        if ( ( out_left != nullptr ) && ( out_right != nullptr ) )
        {
            memset ( out_left, 0, sizeof ( jack_default_audio_sample_t ) * nframes );
            memset ( out_right, 0, sizeof ( jack_default_audio_sample_t ) * nframes );
        }
    }

    // act on MIDI data if MIDI is enabled
    if ( jackClient.MidiInput.size() )
    {
        void* in_midi = jackClient.MidiInput[0].GetBuffer ( nframes );

        if ( in_midi != 0 )
        {
            jack_nframes_t event_count = jackClient.MidiInput[0].GetEventCount();

            for ( jack_nframes_t j = 0; j < event_count; j++ )
            {
                jack_midi_event_t in_event;

                jack_midi_event_get ( &in_event, in_midi, j );

                // copy packet and send it to the MIDI parser
                // clang-format off
// TODO do not call malloc in real-time callback
                // clang-format on
                CVector<uint8_t> vMIDIPaketBytes ( (int) in_event.size );

                for ( unsigned int i = 0; i < in_event.size; i++ )
                {
                    vMIDIPaketBytes[i] = static_cast<uint8_t> ( in_event.buffer[i] );
                }

                parseMIDIMessage ( vMIDIPaketBytes );
            }
        }
    }

    return 0; // zero on success, non-zero on error
}

int CSound::onBufferSizeCallback()
{
    QMutexLocker locker ( &mutexAudioProcessCallback );

    if ( isRunning() )
    {
        emit reinitRequest ( RS_ONLY_RESTART_AND_INIT );
    }

    return 0; // zero on success, non-zero on error
}

void CSound::onShutdownCallback()
{
    QMutexLocker locker ( &mutexAudioProcessCallback );

    bJackWasShutDown = true;

    // Trying to restart should generate the error....
    emit reinitRequest ( RS_ONLY_RESTART );
}

//============================================================================
//  CSoundBase:
//============================================================================

void CSound::closeCurrentDevice()
{
    jackClient.Close();
    clearDeviceInfo();
}

unsigned int CSound::getDeviceBufferSize ( unsigned int iDesiredBufferSize )
{
    // We can't get the actual buffersize without actualy changing the buffersize,
    // so we must first stop and adjust the buffersize
    CSoundStopper sound ( *this );

    iProtocolBufferSize = iDesiredBufferSize;
    jackClient.SetBufferSize ( iProtocolBufferSize );

    iDeviceBufferSize = jackClient.GetBufferSize();

    audioBuffer.Init ( iDeviceBufferSize * 2 );

    return iDeviceBufferSize;
}

long CSound::createDeviceList ( bool bRescan )
{
    Q_UNUSED ( bRescan );

    strDeviceNames.clear();
    strDeviceNames.append ( SystemDriverTechniqueName() );

    // Just one device, so we force selection !
    iCurrentDevice = 0;

    return lNumDevices = 1;
}

bool CSound::startJack()
{
    CSoundStopper sound ( *this );

    // make sure jackClient is closed before starting
    jackClient.Close();

    if ( jackClient.Open() )
    {
        // since we can open jackClient Jack is not, or no longer, shut down...
        bJackWasShutDown = false;
    }
    else
    {
        if ( bJackWasShutDown )
        {
            strErrorList.append ( tr ( "JACK was shut down. %1 requires JACK to run. Please start JACK again." ).arg ( APP_NAME ) );
        }
        else
        {
            strErrorList.append (
                tr ( "JACK is not running. %1 requires JACK to run. Please start JACK first  and check for error messages." ).arg ( APP_NAME ) );
        }
    }

    if ( jackClient.IsOpen() )
    {
        // create ports
        if ( iJackNumInputs == 2 )
        {
            jackClient.AddAudioPort ( "input left", biINPUT );
            jackClient.AddAudioPort ( "input right", biINPUT );
        }
        else
        {
            for ( int i = 1; i <= iJackNumInputs; i++ )
            {

                jackClient.AddAudioPort ( QString ( "input " ) + ::QString::number ( i ), biINPUT );
            }
        }

        jackClient.AddAudioPort ( "output left", biOUTPUT );
        jackClient.AddAudioPort ( "output right", biOUTPUT );

        if ( iCtrlMIDIChannel != INVALID_MIDI_CH )
        {
            jackClient.AddMidiPort ( "input midi", biINPUT );
        }

        // set callbacks
        jackClient.SetShutDownCallBack ( _ShutdownCallback, this );
        jackClient.SetProcessCallBack ( _BufferSwitch, this );
        jackClient.SetBuffersizeCallBack ( _BufferSizeCallback, this );

        // prepare buffers
        jackClient.SetBufferSize ( iProtocolBufferSize );
        iDeviceBufferSize = jackClient.GetBufferSize();
        audioBuffer.Init ( iDeviceBufferSize * 2 );

        // activate jack
        if ( jackClient.Activate() )
        {
            // connect inputs and outputs
            if ( bAutoConnect )
            {
                // Auto connect the audio ports
                jackClient.AutoConnect();
            }

            // Don't we need to connect the midi port ????
            if ( iCtrlMIDIChannel != INVALID_MIDI_CH ) {}
        }
    }

    return jackClient.IsOpen() && jackClient.IsActive();
}

bool CSound::stopJack()
{
    Stop();

    jackClient.Deactivate();
    jackClient.Close();

    return !jackClient.IsOpen();
}

bool CSound::checkCapabilities()
{
    if ( jackClient.IsOpen() )
    {
        bool ok = true;
        if ( jackClient.GetSamplerate() != SYSTEM_SAMPLE_RATE_HZ )
        {
            strErrorList.append ( tr ( "JACK isn't running at a sample rate of <b>%1 Hz</b>. Please use "
                                       "a tool like <i><a href=\"https://qjackctl.sourceforge.io\">QjackCtl</a></i> to set the "
                                       "the JACK sample rate to %1 Hz." )
                                      .arg ( SYSTEM_SAMPLE_RATE_HZ ) );
            ok = false;
        }

        int numInputConnections  = jackClient.GetAudioInputConnections();
        int numOutputConnections = jackClient.GetAudioOutputConnections();
        if ( bAutoConnect ) // We don't seem to get any connections when not connected via autoconnect !
        {
            bool connOk = true;
            if ( numInputConnections < DRV_MIN_IN_CHANNELS )
            {
                strErrorList.append ( tr ( "You have less than %1 inputs connected." ).arg ( DRV_MIN_IN_CHANNELS ) );
                ok = connOk = false;
            }

            if ( numOutputConnections < DRV_MIN_OUT_CHANNELS )
            {
                strErrorList.append ( tr ( "You have less than %1 ouputs connected." ).arg ( DRV_MIN_OUT_CHANNELS ) );
                ok = connOk = false;
            }

            if ( !connOk )
            {
                strErrorList.append ( tr ( "Please check your JACK connections." ) );
            }
        }

        return ok;
    }

    strErrorList.append ( tr ( "Not connected to a Jack server." ) );

    return false;
}

bool CSound::setBaseValues()
{
    createDeviceList();

    clearDeviceInfo();
    // Set the input channel names:
    for ( lNumInChan = 0; lNumInChan < jackClient.AudioInput.size(); lNumInChan++ )
    {
        strInputChannelNames.append ( jackClient.AudioInput[lNumInChan].portName ); // pgScoprpio TODO use fixed names ??
    }

    // Set any added input channel names:
    addAddedInputChannelNames();

    // Set the output channel names:
    for ( lNumOutChan = 0; lNumOutChan < jackClient.AudioOutput.size(); lNumOutChan++ )
    {
        strOutputChannelNames.append ( jackClient.AudioOutput[lNumOutChan].portName ); // pgScoprpio TODO use fixed names ??
    }

    // Select input and output channels:
    resetChannelMapping();
    // pgScorpio TODO: reload channel mapping from inifile

    // Set the In/Out Latency:
    fInOutLatencyMs = 0.0;
    if ( ( jackClient.AudioInput.size() ) && ( jackClient.AudioOutput.size() ) )
    {
        // compute latency by using the first input and first output
        // ports and using the most optimistic values

        jack_latency_range_t inputLatencyRange;
        jack_latency_range_t outputLatencyRange;

        jackClient.AudioInput[0].getLatencyRange ( inputLatencyRange );
        jackClient.AudioOutput[0].getLatencyRange ( outputLatencyRange );

        // be optimistic
        fInOutLatencyMs = static_cast<float> ( inputLatencyRange.min + outputLatencyRange.min ) * 1000 / SYSTEM_SAMPLE_RATE_HZ;
    }

    return ( jackClient.IsOpen() && ( lNumInChan >= DRV_MIN_IN_CHANNELS ) && ( lNumOutChan >= DRV_MIN_OUT_CHANNELS ) );
}

// TODO: ChannelMapping from inifile!
bool CSound::checkDeviceChange ( CSoundBase::tDeviceChangeCheck mode, int iDriverIndex ) // Open device sequence handling....
{
    // We have just one device !
    if ( iDriverIndex != 0 )
    {
        return false;
    }

    switch ( mode )
    {
    case CSoundBase::tDeviceChangeCheck::CheckOpen:
    {
        // Now reset jack by closing and re-opening jack
        if ( bJackWasShutDown )
        {
            stopJack();
        }
        if ( !startJack() )
        {
            return false;
        }
    }
        return true;

    case CSoundBase::tDeviceChangeCheck::CheckCapabilities:
        return checkCapabilities();

    case CSoundBase::tDeviceChangeCheck::Activate:
        return setBaseValues();

    case CSoundBase::tDeviceChangeCheck::Abort:
        setBaseValues(); // Still try to set Base values, since there is no other device !
        return true;

    default:
        return false;
    }
}

bool CSound::start()
{
    if ( !IsStarted() && jackClient.IsActive() )
    {
        return true;
    }

    return IsStarted();
}

bool CSound::stop() { return true; }
