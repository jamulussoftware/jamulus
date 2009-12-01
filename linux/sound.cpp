/******************************************************************************\
 * Copyright (c) 2004-2009
 *
 * Author(s):
 *  Volker Fischer
 *
 * This code is based on the simple_client example of the Jack audio interface.
\******************************************************************************/

#include "sound.h"

#ifdef WITH_SOUND
void CSound::OpenJack()
{
    jack_status_t JackStatus;

    // try to become a client of the JACK server
    pJackClient = jack_client_open ( "llcon", JackNullOption, &JackStatus );
    if ( pJackClient == NULL )
    {
        throw CGenErr ( tr ( "Jack server not running" ) );
    }

    // tell the JACK server to call "process()" whenever
    // there is work to be done
    jack_set_process_callback ( pJackClient, process, this );

    // register a "buffer size changed" callback function
    jack_set_buffer_size_callback ( pJackClient, bufferSizeCallback, this );

    // register shutdown callback function
    jack_on_shutdown ( pJackClient, shutdownCallback, this );

// TEST check sample rate, if not correct, just fire error
if ( jack_get_sample_rate ( pJackClient ) != SYSTEM_SAMPLE_RATE )
{
    throw CGenErr ( tr ( "Jack server sample rate is different from "
        "required one" ) );
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

    const char** ports;

    // tell the JACK server that we are ready to roll
    if ( jack_activate ( pJackClient ) )
    {
        throw CGenErr ( tr ( "Cannot activate client" ) );
    }

    // connect the ports, note: you cannot do this before
    // the client is activated, because we cannot allow
    // connections to be made to clients that are not
    // running
    if ( ( ports = jack_get_ports ( pJackClient, NULL, NULL,
           JackPortIsPhysical | JackPortIsOutput ) ) == NULL )
    {
        throw CGenErr ( tr ( "Cannot find any physical capture ports" ) );
    }

    if ( !ports[1] )
    {
        throw CGenErr ( tr ( "Cannot find enough physical capture ports" ) );
    }

    if ( jack_connect ( pJackClient, ports[0], jack_port_name ( input_port_left ) ) )
    {
        throw CGenErr ( tr ( "Cannot connect input ports" ) );
    }
    if ( jack_connect ( pJackClient, ports[1], jack_port_name ( input_port_right ) ) )
    {
        throw CGenErr ( tr ( "Cannot connect input ports" ) );
    }

    free ( ports );

    if ( ( ports = jack_get_ports ( pJackClient, NULL, NULL,
           JackPortIsPhysical | JackPortIsInput ) ) == NULL )
    {
        throw CGenErr ( tr ( "Cannot find any physical playback ports" ) );
    }

    if ( !ports[1] )
    {
        throw CGenErr ( tr ( "Cannot find enough physical playback ports" ) );
    }

    if ( jack_connect ( pJackClient, jack_port_name ( output_port_left ), ports[0] ) )
    {
        throw CGenErr ( tr ( "Cannot connect output ports" ) );
    }
    if ( jack_connect ( pJackClient, jack_port_name ( output_port_right ), ports[1] ) )
    {
        throw CGenErr ( tr ( "Cannot connect output ports" ) );
    }

    free ( ports );
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

int CSound::Init ( const int iNewPrefMonoBufferSize )
{
// try setting buffer size
// TODO seems not to work! -> no audio after this operation!
//jack_set_buffer_size ( pJackClient, iNewPrefMonoBufferSize );

    // get actual buffer size
    iJACKBufferSizeMono = jack_get_buffer_size ( pJackClient );  	

    // init base clasee
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
    CSound* pSound = reinterpret_cast<CSound*> ( arg );
    int     i;

    if ( pSound->IsRunning() )
    {
        // get input data pointer
        jack_default_audio_sample_t* in_left =
            (jack_default_audio_sample_t*) jack_port_get_buffer (
            pSound->input_port_left, nframes );

        jack_default_audio_sample_t* in_right =
            (jack_default_audio_sample_t*) jack_port_get_buffer (
            pSound->input_port_right, nframes );

        // copy input data
        for ( i = 0; i < pSound->iJACKBufferSizeMono; i++ )
        {
            pSound->vecsTmpAudioSndCrdStereo[2 * i]     = (short) ( in_left[i] * _MAXSHORT );
            pSound->vecsTmpAudioSndCrdStereo[2 * i + 1] = (short) ( in_right[i] * _MAXSHORT );
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
        for ( i = 0; i < pSound->iJACKBufferSizeMono; i++ )
        {
            out_left[i] = (jack_default_audio_sample_t)
                pSound->vecsTmpAudioSndCrdStereo[2 * i] / _MAXSHORT;

            out_right[i] = (jack_default_audio_sample_t)
                pSound->vecsTmpAudioSndCrdStereo[2 * i + 1] / _MAXSHORT;
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
        for ( i = 0; i < pSound->iJACKBufferSizeMono; i++ )
        {
            out_left[i]  = 0;
            out_right[i] = 0;
        }
    }

    return 0; // zero on success, non-zero on error 
}

int CSound::bufferSizeCallback ( jack_nframes_t nframes, void *arg )
{
    CSound* pSound = reinterpret_cast<CSound*> ( arg );

    pSound->EmitReinitRequestSignal();

    return 0; // zero on success, non-zero on error
}

void CSound::shutdownCallback ( void *arg )
{
    // without a Jack server, our software makes no sense to run, throw
    // error message
    throw CGenErr ( tr ( "Jack server was shut down" ) );
}
#endif // WITH_SOUND
