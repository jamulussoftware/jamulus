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
void CSound::OpenJack ( const bool  bNoAutoJackConnect,
                        const char* jackClientName )
{
    jack_status_t JackStatus;

    // try to become a client of the JACK server
    pJackClient = jack_client_open ( jackClientName, JackNullOption, &JackStatus );

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

    // create four ports (two for input, two for output -> stereo)
    input_port_left = jack_port_register ( pJackClient, "input left",
        JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput, 0 );

    input_port_right = jack_port_register ( pJackClient, "input right",
        JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput, 0 );

    output_port_left = jack_port_register ( pJackClient, "output left",
        JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0 );

    output_port_right = jack_port_register ( pJackClient, "output right",
        JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0 );

    if ( ( input_port_left   == nullptr ) ||
         ( input_port_right  == nullptr ) ||
         ( output_port_left  == nullptr ) ||
         ( output_port_right == nullptr ) )
    {
        throw CGenErr ( tr ( "The Jack port registering failed." ) );
    }

    // optional MIDI initialization
    if ( iCtrlMIDIChannel != INVALID_MIDI_CH )
    {
        input_port_midi = jack_port_register ( pJackClient, "input midi",
            JACK_DEFAULT_MIDI_TYPE, JackPortIsInput, 0 );

        if ( input_port_midi == nullptr )
        {
            throw CGenErr ( tr ( "The Jack port registering failed." ) );
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
        const char** ports;

        // try to connect physical input ports
        if ( ( ports = jack_get_ports ( pJackClient,
                                        nullptr,
                                        nullptr,
                                        JackPortIsPhysical | JackPortIsOutput ) ) != nullptr )
        {
            jack_connect ( pJackClient, ports[0], jack_port_name ( input_port_left ) );

            // before connecting the second stereo channel, check if the input is not mono
            if ( ports[1] )
            {
                jack_connect ( pJackClient, ports[1], jack_port_name ( input_port_right ) );
            }

            jack_free ( ports );
        }

        // try to connect physical output ports
        if ( ( ports = jack_get_ports ( pJackClient,
                                        nullptr,
                                        nullptr,
                                        JackPortIsPhysical | JackPortIsInput ) ) != nullptr )
        {
            jack_connect ( pJackClient, jack_port_name ( output_port_left ), ports[0] );

            // before connecting the second stereo channel, check if the output is not mono
            if ( ports[1] )
            {
                jack_connect ( pJackClient, jack_port_name ( output_port_right ), ports[1] );
            }

            jack_free ( ports );
        }
    }
}

void CSound::CloseJack()
{
    // deactivate client
    jack_deactivate ( pJackClient );

    // unregister ports
    jack_port_unregister ( pJackClient, input_port_left );
    jack_port_unregister ( pJackClient, input_port_right );
    jack_port_unregister ( pJackClient, output_port_left );
    jack_port_unregister ( pJackClient, output_port_right );

    // close client connection to jack server
    jack_client_close ( pJackClient );
}

void CSound::Start()
{
    // call base class
    CSoundBase::Start();
}

void CSound::Stop()
{
    // call base class
    CSoundBase::Stop();
}

int CSound::Init ( const int /* iNewPrefMonoBufferSize */ )
{


// try setting buffer size
// TODO seems not to work! -> no audio after this operation!

// Doesn't this give an infinite loop? The set buffer size function will call our
// registerd callback which calls "EmitReinitRequestSignal()". In that function
// this CSound::Init() function is called...

//jack_set_buffer_size ( pJackClient, iNewPrefMonoBufferSize );


    // get actual buffer size
    iJACKBufferSizeMono = jack_get_buffer_size ( pJackClient );  	

    // init base class
    CSoundBase::Init ( iJACKBufferSizeMono );

    // set internal buffer size value and calculate stereo buffer size
    iJACKBufferSizeStero = 2 * iJACKBufferSizeMono;

    // create memory for intermediate audio buffer
    vecsTmpAudioSndCrdStereo.Init ( iJACKBufferSizeStero );

    return iJACKBufferSizeMono;
}


// JACK callbacks --------------------------------------------------------------
int CSound::process ( jack_nframes_t nframes, void* arg )
{
    CSound* pSound = static_cast<CSound*> ( arg );
    int     i;

    if ( pSound->IsRunning() && ( nframes == static_cast<jack_nframes_t> ( pSound->iJACKBufferSizeMono ) ) )
    {
        // get input data pointer
        jack_default_audio_sample_t* in_left =
            (jack_default_audio_sample_t*) jack_port_get_buffer (
            pSound->input_port_left, nframes );

        jack_default_audio_sample_t* in_right =
            (jack_default_audio_sample_t*) jack_port_get_buffer (
            pSound->input_port_right, nframes );

        // copy input audio data
        if ( ( in_left != nullptr ) && ( in_right != nullptr ) )
        {
            for ( i = 0; i < pSound->iJACKBufferSizeMono; i++ )
            {
                pSound->vecsTmpAudioSndCrdStereo[2 * i] =
                    (short) ( in_left[i] * _MAXSHORT );

                pSound->vecsTmpAudioSndCrdStereo[2 * i + 1] =
                    (short) ( in_right[i] * _MAXSHORT );
            }
        }

        // call processing callback function
        pSound->ProcessCallback ( pSound->vecsTmpAudioSndCrdStereo );

        // get output data pointer
        jack_default_audio_sample_t* out_left =
            (jack_default_audio_sample_t*) jack_port_get_buffer (
            pSound->output_port_left, nframes );

        jack_default_audio_sample_t* out_right =
            (jack_default_audio_sample_t*) jack_port_get_buffer (
            pSound->output_port_right, nframes );

        // copy output data
        if ( ( out_left != nullptr ) && ( out_right != nullptr ) )
        {
            for ( i = 0; i < pSound->iJACKBufferSizeMono; i++ )
            {
                out_left[i] = (jack_default_audio_sample_t)
                    pSound->vecsTmpAudioSndCrdStereo[2 * i] / _MAXSHORT;

                out_right[i] = (jack_default_audio_sample_t)
                    pSound->vecsTmpAudioSndCrdStereo[2 * i + 1] / _MAXSHORT;
            }
        }
    }
    else
    {
        // get output data pointer
        jack_default_audio_sample_t* out_left =
            (jack_default_audio_sample_t*) jack_port_get_buffer (
            pSound->output_port_left, nframes );

        jack_default_audio_sample_t* out_right =
            (jack_default_audio_sample_t*) jack_port_get_buffer (
            pSound->output_port_right, nframes );

        // clear output data
        if ( ( out_left != nullptr ) && ( out_right != nullptr ) )
        {
            memset ( out_left,
                     0,
                     sizeof ( jack_default_audio_sample_t ) * nframes );

            memset ( out_right,
                     0,
                     sizeof ( jack_default_audio_sample_t ) * nframes );
        }
    }

    // akt on MIDI data if MIDI is enabled
    if ( pSound->input_port_midi != nullptr )
    {
        void* in_midi = jack_port_get_buffer ( pSound->input_port_midi, nframes );

        if ( in_midi != 0 )
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
