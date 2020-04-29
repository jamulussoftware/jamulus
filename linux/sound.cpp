/******************************************************************************\
 * Copyright (c) 2004-2020
 *
 * Author(s):
 *  Volker Fischer
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
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
\******************************************************************************/

#include "sound.h"

#ifdef WITH_SOUND
void CSound::OpenJack()
{
    jack_status_t JackStatus;

    // try to become a client of the JACK server
    pJackClient = jack_client_open ( APP_NAME, JackNullOption, &JackStatus );

    if ( pJackClient == nullptr )
    {
        throw CGenErr ( tr ( "The Jack server is not running. This software "
            "requires a Jack server to run. Normally if the Jack server is "
            "not running this software will automatically start the Jack server. "
            "It seems that this auto start has not worked. Try to start the Jack "
            "server manually." ) );
    }

    // tell the JACK server to call "process()" whenever
    // there is work to be done
    jack_set_process_callback ( pJackClient, process, this );

    // register a "buffer size changed" callback function
    jack_set_buffer_size_callback ( pJackClient, bufferSizeCallback, this );

    // register shutdown callback function
    jack_on_shutdown ( pJackClient, shutdownCallback, this );

    // check sample rate, if not correct, just fire error
    if ( jack_get_sample_rate ( pJackClient ) != SYSTEM_SAMPLE_RATE_HZ )
    {
        throw CGenErr ( tr ( "The Jack server sample rate is different from "
            "the required one. The required sample rate is: <b>" ) +
            QString().setNum ( SYSTEM_SAMPLE_RATE_HZ ) + tr ( " Hz</b>. You can "
            "use a tool like <i><a href=""http://qjackctl.sourceforge.net"">QJackCtl</a></i> "
            "to adjust the Jack server sample rate.<br>Make sure to set the "
            "<b>Frames/Period</b> to a low value like <b>" ) +
            QString().setNum ( DOUBLE_SYSTEM_FRAME_SIZE_SAMPLES ) +
            tr ( "</b> to achieve a low delay." ) );
    }

    // get physical input and output ports (note that JackPortIsOutput/Input must be reverted)
    const char** inPorts  = jack_get_ports ( pJackClient, nullptr, nullptr, JackPortIsPhysical | JackPortIsOutput );
    const char** outPorts = jack_get_ports ( pJackClient, nullptr, nullptr, JackPortIsPhysical | JackPortIsInput );

    // get the number of available physical input/output channels
    iNumInChan = 0;
    while ( inPorts[iNumInChan] != nullptr ) iNumInChan++;

    iNumOutChan = 0;
    while ( outPorts[iNumOutChan] != nullptr ) iNumOutChan++;

    // want to have at least two input and two output ports but not more than the
    // maximum allowed, in case of the "no auto jack connect" we always use two
    if ( bNoAutoJackConnect )
    {
        iNumInChan  = 2;
        iNumOutChan = 2;
    }
    else
    {
        iNumInChan  = std::min ( MAX_NUM_IN_OUT_CHANNELS, std::max ( 2, iNumInChan ) );
        iNumOutChan = std::min ( MAX_NUM_IN_OUT_CHANNELS, std::max ( 2, iNumOutChan ) );
    }

    // create input/output ports of this application
    for ( int i = 0; i < iNumInChan; i++ )
    {
        input_port[i] = jack_port_register ( pJackClient, QString ( "input %1" ).arg ( 1 + i ).toStdString().c_str(),
                                             JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput, 0 );

        if ( input_port[i] == nullptr )
        {
            throw CGenErr ( tr ( "The Jack audio input port registering failed." ) );
        }
    }

    for ( int i = 0; i < iNumOutChan; i++ )
    {
        output_port[i] = jack_port_register ( pJackClient, QString ( "output %1" ).arg ( 1 + i ).toStdString().c_str(),
                                              JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0 );

        if ( output_port[i] == nullptr )
        {
            throw CGenErr ( tr ( "The Jack audio output port registering failed." ) );
        }
    }

    // optional MIDI initialization
    if ( iCtrlMIDIChannel != INVALID_MIDI_CH )
    {
        input_port_midi = jack_port_register ( pJackClient, "input midi",
                                               JACK_DEFAULT_MIDI_TYPE, JackPortIsInput, 0 );

        if ( input_port_midi == nullptr )
        {
            throw CGenErr ( tr ( "The Jack midi port registering failed." ) );
        }
    }
    else
    {
        input_port_midi = nullptr;
    }

    // tell the JACK server that we are ready to roll
    if ( jack_activate ( pJackClient ) )
    {
        throw CGenErr ( tr ( "Cannot activate the Jack client." ) );
    }

    if ( !bNoAutoJackConnect )
    {
        // connect the ports, note: you cannot do this before
        // the client is activated, because we cannot allow
        // connections to be made to clients that are not
        // running

        // try to connect physical input ports
        if ( inPorts != nullptr )
        {
            for ( int i = 0; i < iNumInChan; i++ )
            {
                jack_connect ( pJackClient, inPorts[i], jack_port_name ( input_port[i] ) );
            }
        }

        // try to connect physical output ports
        if ( outPorts != nullptr )
        {
            for ( int i = 0; i < iNumOutChan; i++ )
            {
                jack_connect ( pJackClient, jack_port_name ( output_port[i] ), outPorts[i] );
            }
        }
    }

    // free memory
    if ( inPorts != nullptr )
    {
        jack_free ( inPorts );
    }

    if ( outPorts != nullptr )
    {
        jack_free ( outPorts );
    }
}

void CSound::CloseJack()
{
    // deactivate client
    jack_deactivate ( pJackClient );

    // unregister ports
    for ( int i = 0; i < iNumInChan; i++ )
    {
        jack_port_unregister ( pJackClient, input_port[i] );
    }
    for ( int i = 0; i < iNumOutChan; i++ )
    {
        jack_port_unregister ( pJackClient, output_port[i] );
    }

    // close client connection to jack server
    jack_client_close ( pJackClient );
}

void CSound::Init ( const int /* iNewPrefMonoBufferSize */,
                    int&      iSndCrdBufferSizeMono,
                    int&      iSndCrdNumInputChannels,
                    int&      iSndCrdNumOutputChannels )
{

// try setting buffer size
// TODO seems not to work! -> no audio after this operation!
// Doesn't this give an infinite loop? The set buffer size function will call our
// registerd callback which calls "EmitReinitRequestSignal()". In that function
// this CSound::Init() function is called...
//jack_set_buffer_size ( pJackClient, iNewPrefMonoBufferSize );

    // get actual buffer size
    iJACKBufferSizeMono = jack_get_buffer_size ( pJackClient );  	

    // create memory for intermediate audio buffer
    vecsMultChanAudioSndCrd.Init ( iJACKBufferSizeMono * std::max ( iNumInChan, iNumOutChan ) );

    // apply return values (right now the jack interface supports two input and two output channels)
    iSndCrdBufferSizeMono    = iJACKBufferSizeMono;
    iSndCrdNumInputChannels  = iNumInChan;
    iSndCrdNumOutputChannels = iNumOutChan;
}


// JACK callbacks --------------------------------------------------------------
int CSound::process ( jack_nframes_t nframes, void* arg )
{
    CSound*   pSound = static_cast<CSound*> ( arg );
    const int iJACKBufferSizeMono = pSound->iJACKBufferSizeMono;
    const int iNumInChan          = pSound->iNumInChan;
    const int iNumOutChan         = pSound->iNumOutChan;
    int       i;

    if ( pSound->IsRunning() )
    {
        for ( int iCh = 0; iCh < iNumInChan; iCh++ )
        {
            // get input data pointer
            jack_default_audio_sample_t* in_sample =
                (jack_default_audio_sample_t*) jack_port_get_buffer (
                pSound->input_port[iCh], nframes );

            // copy input audio data
            if ( in_sample != nullptr )
            {
                for ( i = 0; i < iJACKBufferSizeMono; i++ )
                {
                    pSound->vecsMultChanAudioSndCrd[iNumInChan * i + iCh] =
                        (short) ( in_sample[i] * _MAXSHORT );
                }
            }
        }

        // call processing callback function
        pSound->ProcessCallback ( pSound->vecsMultChanAudioSndCrd );

        for ( int iCh = 0; iCh < iNumOutChan; iCh++ )
        {
            // get output data pointer
            jack_default_audio_sample_t* out_sample =
                (jack_default_audio_sample_t*) jack_port_get_buffer (
                pSound->output_port[iCh], nframes );

            // copy output data
            if ( out_sample != nullptr )
            {
                for ( i = 0; i < iJACKBufferSizeMono; i++ )
                {
                    out_sample[i] = (jack_default_audio_sample_t)
                        pSound->vecsMultChanAudioSndCrd[iNumOutChan * i + iCh] / _MAXSHORT;
                }
            }
        }
    }
    else
    {
        for ( int iCh = 0; iCh < iNumOutChan; iCh++ )
        {
            // get output data pointer
            jack_default_audio_sample_t* out_sample =
                (jack_default_audio_sample_t*) jack_port_get_buffer (
                pSound->output_port[iCh], nframes );

            // clear output data
            if ( out_sample != nullptr )
            {
                memset ( out_sample, 0, sizeof ( jack_default_audio_sample_t ) * nframes );
            }
        }
    }

    // akt on MIDI data if MIDI is enabled
    if ( pSound->input_port_midi != nullptr )
    {
        void* in_midi = jack_port_get_buffer ( pSound->input_port_midi, nframes );

        if ( in_midi != nullptr )
        {
            jack_nframes_t event_count = jack_midi_get_event_count ( in_midi );

            for ( jack_nframes_t j = 0; j < event_count; j++ )
            {
                jack_midi_event_t in_event;

                jack_midi_event_get ( &in_event, in_midi, j );

                // copy packet and send it to the MIDI parser
// TODO do not call malloc in real-time callback
                CVector<uint8_t> vMIDIPaketBytes ( in_event.size );

                for ( i = 0; i < static_cast<int> ( in_event.size ); i++ )
                {
                    vMIDIPaketBytes[i] = static_cast<uint8_t> ( in_event.buffer[i] );
                }
                pSound->ParseMIDIMessage ( vMIDIPaketBytes );
            }
        }
    }

    return 0; // zero on success, non-zero on error 
}

int CSound::bufferSizeCallback ( jack_nframes_t, void *arg )
{
    CSound* pSound = static_cast<CSound*> ( arg );

    pSound->EmitReinitRequestSignal ( RS_ONLY_RESTART_AND_INIT );

    return 0; // zero on success, non-zero on error
}

void CSound::shutdownCallback ( void* )
{
    // without a Jack server, our software makes no sense to run, throw
    // error message
    throw CGenErr ( tr ( "The Jack server was shut down. This software "
        "requires a Jack server to run. Try to restart the software to "
        "solve the issue." ) );
}
#endif // WITH_SOUND
