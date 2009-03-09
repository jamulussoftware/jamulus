/******************************************************************************\
 * Copyright (c) 2004-2009
 *
 * Author(s):
 *  Volker Fischer, Alexander Kurpiers
 *
 * This code is based on the Open-Source sound interface implementation of
 * the Dream DRM Receiver project.
 *
\******************************************************************************/

#include "sound.h"

#ifdef WITH_SOUND
# if USE_JACK
CSound::CSound ( void (*fpNewCallback) ( CVector<short>& psData, void* arg ),
                 void* arg ) :
    CSoundBase ( true, fpNewCallback, arg )
{
    jack_status_t JackStatus;

    // try to become a client of the JACK server
    pJackClient = jack_client_open ( "llcon", JackNullOption, &JackStatus );
    if ( pJackClient == NULL )
    {
        throw CGenErr ( "Jack server not running" );
    }
 
    // tell the JACK server to call "process()" whenever
    // there is work to be done
    jack_set_process_callback ( pJackClient, process, this );

// TEST check sample rate, if not correct, just fire error
if ( jack_get_sample_rate ( pJackClient ) != SND_CRD_SAMPLE_RATE )
{
    throw CGenErr ( "Jack server sample rate is different from "
        "required one" );
}

    // create four ports
    input_port_left = jack_port_register ( pJackClient, "input left",
        JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput, 0 );
    input_port_right = jack_port_register ( pJackClient, "input right",
        JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput, 0 );

    output_port_left = jack_port_register ( pJackClient, "output left",
        JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0 );
    output_port_right = jack_port_register ( pJackClient, "output right",
        JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0 );
}

void CSound::Start()
{
    const char** ports;

    // tell the JACK server that we are ready to roll
    if ( jack_activate ( pJackClient ) )
    {
        throw CGenErr ( "Cannot activate client" );
    }

    // connect the ports, note: you cannot do this before
    // the client is activated, because we cannot allow
    // connections to be made to clients that are not
    // running
    if ( ( ports = jack_get_ports ( pJackClient, NULL, NULL,
           JackPortIsPhysical | JackPortIsOutput ) ) == NULL )
    {
        throw CGenErr ( "Cannot find any physical capture ports" );
    }

    if ( !ports[1] )
    {
        throw CGenErr ( "Cannot find enough physical capture ports" );
    }

    if ( jack_connect ( pJackClient, ports[0], jack_port_name ( input_port_left ) ) )
    {
        throw CGenErr ( "Cannot connect input ports" );
    }
    if ( jack_connect ( pJackClient, ports[1], jack_port_name ( input_port_right ) ) )
    {
        throw CGenErr ( "Cannot connect input ports" );
    }

    free ( ports );

    if ( ( ports = jack_get_ports ( pJackClient, NULL, NULL,
           JackPortIsPhysical | JackPortIsInput ) ) == NULL )
    {
        throw CGenErr ( "Cannot find any physical playback ports" );
    }

    if ( !ports[1] )
    {
        throw CGenErr ( "Cannot find enough physical playback ports" );
    }

    if ( jack_connect ( pJackClient, jack_port_name ( output_port_left ), ports[0] ) )
    {
        throw CGenErr ( "Cannot connect output ports" );
    }
    if ( jack_connect ( pJackClient, jack_port_name ( output_port_right ), ports[1] ) )
    {
        throw CGenErr ( "Cannot connect output ports" );
    }

    free ( ports );

    // call base class
    CSoundBase::Start();
}

void CSound::Stop()
{
    // deactivate client
    jack_deactivate ( pJackClient );

    // call base class
    CSoundBase::Stop();
}

int CSound::Init ( const int iNewPrefMonoBufferSize )
{
    // try setting buffer size
    jack_set_buffer_size ( pJackClient, iNewPrefMonoBufferSize );

    // get actual buffer size
    iJACKBufferSizeMono = jack_get_buffer_size ( pJackClient );  	

    // init base clasee
    CSoundBase::Init ( iJACKBufferSizeMono );

    // set internal buffer size value and calculate stereo buffer size
    iJACKBufferSizeStero = 2 * iJACKBufferSizeMono;

    // create memory for intermediate audio buffer
    vecsTmpAudioSndCrdStereo.Init ( iJACKBufferSizeStero );

// TEST
return iJACKBufferSizeMono;

}


// JACK callbacks --------------------------------------------------------------
int CSound::process ( jack_nframes_t nframes, void* arg )
{
    int     i;
    CSound* pSound = reinterpret_cast<CSound*> ( arg );

    // get input data
    jack_default_audio_sample_t* in_left =
        (jack_default_audio_sample_t*) jack_port_get_buffer ( pSound->input_port_left, nframes );

    jack_default_audio_sample_t* in_right =
        (jack_default_audio_sample_t*) jack_port_get_buffer ( pSound->input_port_right, nframes );

    // copy input data
    for ( i = 0; i < pSound->iJACKBufferSizeMono; i++ )
    {

// TODO better conversion from float to short

        pSound->vecsTmpAudioSndCrdStereo[2 * i]     = (short) in_left[i];
        pSound->vecsTmpAudioSndCrdStereo[2 * i + 1] = (short) in_right[i];
    }

    // call processing callback function
    pSound->Callback ( pSound->vecsTmpAudioSndCrdStereo );

    // put output data
    jack_default_audio_sample_t* out_left =
        (jack_default_audio_sample_t*) jack_port_get_buffer ( pSound->output_port_left, nframes );

    jack_default_audio_sample_t* out_right =
        (jack_default_audio_sample_t*) jack_port_get_buffer ( pSound->output_port_right, nframes );

    // copy output data
    for ( i = 0; i < pSound->iJACKBufferSizeMono; i++ )
    {
        out_left[i]  = pSound->vecsTmpAudioSndCrdStereo[2 * i];
        out_right[i] = pSound->vecsTmpAudioSndCrdStereo[2 * i + 1];
    }

    return 0; // zero on success, non-zero on error 
}
# else
// Wave in *********************************************************************
void CSound::InitRecording()
{
    int err;

    // if recording device was already open, close it first
    if ( rhandle != NULL )
    {
        snd_pcm_close ( rhandle );
    }

    /* record device: The most important ALSA interfaces to the PCM devices are
       the "plughw" and the "hw" interface. If you use the "plughw" interface,
       you need not care much about the sound hardware. If your soundcard does
       not support the sample rate or sample format you specify, your data will
       be automatically converted. This also applies to the access type and the
       number of channels. With the "hw" interface, you have to check whether
       your hardware supports the configuration you would like to use */
    // either "hw:0,0" or "plughw:0,0"
    if ( err = snd_pcm_open ( &rhandle, "hw:0,0", SND_PCM_STREAM_CAPTURE, 0 ) != 0 )
    {
        qDebug ( "open error: %s", snd_strerror ( err ) );
    }

    // recording should be blocking
    if ( err = snd_pcm_nonblock ( rhandle, FALSE ) != 0 )
    {
        qDebug ( "cannot set blocking: %s", snd_strerror ( err ) );
    }

    // set hardware parameters
    SetHWParams ( rhandle, iBufferSizeIn, iCurPeriodSizeIn );

    // start record
    snd_pcm_reset ( rhandle );
    snd_pcm_start ( rhandle );

    qDebug ( "alsa init record done" );
}

bool CSound::Read ( CVector<short>& psData )
{
    int ret;

    // check if device must be opened or reinitialized
    if ( bChangParamIn == true )
    {
        InitRecording();

        // reset flag
        bChangParamIn = false;
    }

    ret = snd_pcm_readi ( rhandle, &psData[0], iBufferSizeIn );

    if ( ret < 0 )
    {
        if ( ret == -EPIPE )
        {
            // under-run
            qDebug ( "rprepare" );

            ret = snd_pcm_prepare ( rhandle );

            if ( ret < 0 )
            {
                qDebug ( "Can't recover from underrun, prepare failed: %s", snd_strerror ( ret ) );
            }

            ret = snd_pcm_start ( rhandle );

            if ( ret < 0 )
            {
                qDebug ( "Can't recover from underrun, start failed: %s", snd_strerror ( ret ) );
            }

            return true;

        }
        else if ( ret == -ESTRPIPE )
        {
            qDebug ( "strpipe" );

            // wait until the suspend flag is released
            while ( ( ret = snd_pcm_resume ( rhandle ) ) == -EAGAIN )
            {
                sleep ( 1 );
            }

            if ( ret < 0 )
            {
                ret = snd_pcm_prepare ( rhandle );

                if ( ret < 0 )
                {
                    qDebug ( "Can't recover from suspend, prepare failed: %s", snd_strerror ( ret ) );
                }
                throw CGenErr ( "CSound:Read" );
            }

            return true;
        }
        else
        {
            qDebug ( "CSound::Read: %s", snd_strerror ( ret ) );
            throw CGenErr ( "CSound:Read" );
        }
    }
    else
    {
        return false;
    }
}


// Wave out ********************************************************************
void CSound::InitPlayback()
{
    int err;

    // if playback device was already open, close it first
    if ( phandle != NULL )
    {
        snd_pcm_close ( phandle );
    }

    // playback device (either "hw:0,0" or "plughw:0,0")
    if ( err = snd_pcm_open ( &phandle, "hw:0,0",
         SND_PCM_STREAM_PLAYBACK, SND_PCM_NONBLOCK ) != 0 )
    {
        qDebug ( "open error: %s", snd_strerror ( err ) );
    }

    // non-blocking playback
    if ( err = snd_pcm_nonblock ( phandle, TRUE ) != 0 )
    {
        qDebug ( "cannot set blocking: %s", snd_strerror ( err ) );
    }

    // set hardware parameters
    SetHWParams ( phandle, iBufferSizeOut, iCurPeriodSizeOut );

    // start playback
    snd_pcm_start ( phandle );

    qDebug ( "alsa init playback done" );
}

bool CSound::Write ( CVector<short>& psData )
{
    int size  = iBufferSizeOut;
    int start = 0;
    int ret;

    // check if device must be opened or reinitialized
    if ( bChangParamOut == true )
    {
        InitPlayback();

        // reset flag
        bChangParamOut = false;
    }

    while ( size )
    {
        ret = snd_pcm_writei ( phandle, &psData[start], size );

        if ( ret < 0 )
        {
            if ( ret == -EPIPE )
            {
                // under-run
                qDebug ( "wunderrun" );

                ret = snd_pcm_prepare ( phandle );

                if ( ret < 0 )
                {
                    qDebug ( "Can't recover from underrun, prepare failed: %s", snd_strerror ( ret ) );
                }
                continue;
            }
            else if ( ret == -EAGAIN )
            {
                if ( ( ret = snd_pcm_wait ( phandle, 1000 ) ) < 0 )
                {
                    qDebug ( "poll failed (%s)", snd_strerror ( ret ) );
                    break;
                }
                continue;
            }
            else if ( ret == -ESTRPIPE )
            {
                qDebug ( "wstrpipe" );

                // wait until the suspend flag is released
                while ( ( ret = snd_pcm_resume ( phandle ) ) == -EAGAIN )
                {
                    sleep ( 1 );
                }

                if ( ret < 0 )
                {
                    ret = snd_pcm_prepare ( phandle );

                    if ( ret < 0 )
                    {
                        qDebug ( "Can't recover from suspend, prepare failed: %s", snd_strerror ( ret ) );
                    }
                }
                continue;
            }
            else
            {
                qDebug ( "Write error: %s", snd_strerror ( ret ) );
            }
            break; // skip one period
        }

        if ( ret > 0 )
        {
            size  -= ret;
            start += ret;
        }
    }

    return false;
}


// Common ***********************************************************************
bool CSound::SetHWParams ( snd_pcm_t* handle, const int iDesiredBufferSize,
                           const int iNumPeriodBlocks )
{
    int                  err;
    snd_pcm_hw_params_t* hwparams;

    // allocate an invalid snd_pcm_hw_params_t using standard malloc
    if ( err = snd_pcm_hw_params_malloc ( &hwparams ) < 0 )
    {
        qDebug ( "cannot allocate hardware parameter structure (%s)\n", snd_strerror ( err ) );
        return true;
    }

    // fill params with a full configuration space for a PCM
    if ( err = snd_pcm_hw_params_any ( handle, hwparams ) < 0 )
    {
        qDebug ( "cannot initialize hardware parameter structure (%s)\n", snd_strerror ( err ) );
        return true;
    }

    // restrict a configuration space to contain only one access type:
    // set the interleaved read/write format
    if ( err = snd_pcm_hw_params_set_access ( handle, hwparams, SND_PCM_ACCESS_RW_INTERLEAVED ) < 0 )
    {
        qDebug ( "Access type not available : %s", snd_strerror ( err ) );
        return true;
    }

    // restrict a configuration space to contain only one format:
    // set the sample format PCM, 16 bit
    if ( err = snd_pcm_hw_params_set_format ( handle, hwparams, SND_PCM_FORMAT_S16 ) < 0 )
    {
        qDebug ( "Sample format not available : %s", snd_strerror ( err ) );
        return true;
    }

    // restrict a configuration space to contain only one channels count:
    // set the count of channels (usually stereo, 2 channels)
    if ( err = snd_pcm_hw_params_set_channels ( handle, hwparams, NUM_IN_OUT_CHANNELS ) < 0 )
    {
        qDebug ( "Channels count (%i) not available s: %s", NUM_IN_OUT_CHANNELS, snd_strerror ( err ) );
        return true;
    }

    // restrict a configuration space to have rate nearest to a target:
    // set the sample-rate
    unsigned int rrate = SND_CRD_SAMPLE_RATE;
    if ( err = snd_pcm_hw_params_set_rate_near ( handle, hwparams, &rrate, 0 ) < 0 )
    {
        qDebug ( "Rate %iHz not available : %s", rrate, snd_strerror ( err ) );
        return true;
    }
    if ( rrate != SND_CRD_SAMPLE_RATE ) // check if rate is possible
    {
        qDebug ( "Rate doesn't match (requested %iHz, get %iHz)", rrate, err );
        return true;
    }

    // set the period size
    snd_pcm_uframes_t PeriodSize = iDesiredBufferSize;
    if ( err = snd_pcm_hw_params_set_period_size_near ( handle, hwparams, &PeriodSize, 0 ) < 0 )
    {
        qDebug ( "cannot set period size (%s)\n", snd_strerror ( err ) );
        return true;
    }

    // set the buffer size and period size
    snd_pcm_uframes_t BufferFrames = iDesiredBufferSize * iNumPeriodBlocks;
    if ( err = snd_pcm_hw_params_set_buffer_size_near ( handle, hwparams, &BufferFrames ) < 0 )
    {
        qDebug ( "cannot set buffer size (%s)\n", snd_strerror ( err ) );
        return true;
    }

// check period and buffer size
snd_pcm_uframes_t period_size;
err = snd_pcm_hw_params_get_period_size ( hwparams, &period_size, 0 );
if ( err < 0 )
{
    qDebug ( "Unable to get period size: %s\n", snd_strerror ( err ) );
}
qDebug ( "frame size: %d (desired: %d)", (int) period_size, iDesiredBufferSize );

snd_pcm_uframes_t buffer_size;
if ( err = snd_pcm_hw_params_get_buffer_size(hwparams, &buffer_size ) < 0 )
{
    qDebug ( "Unable to get buffer size: %s\n", snd_strerror ( err ) );
}
qDebug ( "buffer size: %d (desired: %d)", (int) buffer_size, iDesiredBufferSize * iNumPeriodBlocks );



    // write the parameters to device
    if ( err = snd_pcm_hw_params ( handle, hwparams ) < 0 )
    {
        qDebug("Unable to set hw params : %s", snd_strerror(err));
        return true;
    }

    // clean-up
    snd_pcm_hw_params_free ( hwparams );

    return false;
}

void CSound::Close()
{
    // read
    if ( rhandle != NULL )
    {
        snd_pcm_close ( rhandle );
    }

    rhandle = NULL;

    // playback
    if ( phandle != NULL )
    {
        snd_pcm_close ( phandle );
    }

    phandle = NULL;
}
# endif // USE_JACK
#endif // WITH_SOUND
