/******************************************************************************\
 * Copyright (c) 2004-2005
 *
 * Author(s):
 *	Volker Fischer, Alexander Kurpiers
 *
 * This code is based on the Open-Source sound interface implementation of
 * the Dream DRM Receiver project.
 *
\******************************************************************************/

#include "sound.h"

#ifdef WITH_SOUND
/* Wave in ********************************************************************/
void CSound::InitRecording(int iNewBufferSize, bool bNewBlocking)
{
	int	err;

	/* set internal buffer size for read */
	iBufferSizeIn = iNewBufferSize / NUM_IN_OUT_CHANNELS; /* mono size */

	/* if recording device was already open, close it first */
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
	/* either "hw:0,0" or "plughw:0,0" */
	if ( err = snd_pcm_open ( &rhandle, "hw:0,0", SND_PCM_STREAM_CAPTURE, 0 ) != 0 )
	{
		qDebug ( "open error: %s", snd_strerror ( err ) );
	}

	/* recording should be blocking */
	if ( err = snd_pcm_nonblock ( rhandle, FALSE ) != 0 )
	{
		qDebug ( "cannot set blocking: %s", snd_strerror ( err ) );
	}

	/* set hardware parameters */
	SetHWParams ( rhandle, iBufferSizeIn, iCurPeriodSizeIn );


	/* sw parameters --------------------------------------------------------- */
	snd_pcm_sw_params_t* swparams;

	// allocate an invalid snd_pcm_sw_params_t using standard malloc
	if ( err = snd_pcm_sw_params_malloc ( &swparams ) != 0 )
	{
		qDebug ( "snd_pcm_sw_params_malloc: %s", snd_strerror ( err ) );
	}

	// get the current swparams
	if ( err = snd_pcm_sw_params_current ( rhandle, swparams ) < 0 )
	{
		qDebug ( "Unable to determine current swparams : %s", snd_strerror ( err ) );
	}

	// start the transfer when the buffer immediately -> no threshold, val: 0
	err = snd_pcm_sw_params_set_start_threshold ( rhandle, swparams, 0 );
	if ( err < 0 )
	{
		qDebug ( "Unable to set start threshold mode : %s", snd_strerror ( err ) );
	}

	// align all transfers to 1 sample
	err = snd_pcm_sw_params_set_xfer_align ( rhandle, swparams, 1 );
	if ( err < 0 )
	{
		qDebug ( "Unable to set transfer align : %s", snd_strerror ( err ) );
	}

	// set avail min inside a software configuration container
	/* Note: This is similar to setting an OSS wakeup point. The valid values
	   for 'val' are determined by the specific hardware. Most PC sound cards
	   can only accept power of 2 frame counts (i.e. 512, 1024, 2048). You
	   cannot use this as a high resolution timer - it is limited to how often
	   the sound card hardware raises an interrupt. Note that you can greatly
	   improve the reponses using snd_pcm_sw_params_set_sleep_min where another
	   timing source is used */
	snd_pcm_uframes_t period_size = iBufferSizeIn;

	err = snd_pcm_sw_params_set_avail_min ( rhandle, swparams, period_size );
	if ( err < 0 )
	{
		qDebug ( "Unable to set avail min : %s", snd_strerror ( err ) );
	}

	// write the parameters to the record/playback device
	err = snd_pcm_sw_params ( rhandle, swparams );
	if ( err < 0 )
	{
		qDebug ( "Unable to set sw params : %s", snd_strerror ( err ) );
	}

	// clean-up
	snd_pcm_sw_params_free ( swparams );


	// start record
	snd_pcm_reset ( rhandle );
	snd_pcm_start ( rhandle );

	qDebug ( "alsa init record done" );
}

bool CSound::Read(CVector<short>& psData)
{
	/* Check if device must be opened or reinitialized */
	if (bChangParamIn == true)
	{
		InitRecording ( iBufferSizeIn * NUM_IN_OUT_CHANNELS );

		/* Reset flag */
		bChangParamIn = false;
	}

	int ret = snd_pcm_readi(rhandle, &psData[0], iBufferSizeIn);
//qDebug("ret: %d, iBufferSizeIn: %d", ret, iBufferSizeIn);

	if (ret < 0)
	{
		if (ret == -EPIPE)
		{
			/* Under-run */
			qDebug ( "rprepare" );

			ret = snd_pcm_prepare ( rhandle );

			if ( ret < 0 )
			{
				qDebug ( "Can't recover from underrun, prepare failed: %s", snd_strerror ( ret ) );
			}

			ret = snd_pcm_start ( rhandle );

			if (ret < 0)
			{
				qDebug ( "Can't recover from underrun, start failed: %s", snd_strerror ( ret ) );
			}

			return true;

		}
		else if ( ret == -ESTRPIPE )
		{
			qDebug ( "strpipe" );

			/* Wait until the suspend flag is released */
			while ( ( ret = snd_pcm_resume ( rhandle ) ) == -EAGAIN )
			{
				sleep(1);
			}

			if ( ret < 0 )
			{
				ret = snd_pcm_prepare(rhandle);

				if (ret < 0)
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

void CSound::SetInNumBuf ( int iNewNum )
{
	/* check new parameter */
	if ( ( iNewNum >= MAX_SND_BUF_IN ) || ( iNewNum < 1 ) )
	{
		iNewNum = NUM_PERIOD_BLOCKS_IN;
	}

	/* Change only if parameter is different */
	if ( iNewNum != iCurPeriodSizeIn )
	{
		iCurPeriodSizeIn = iNewNum;
		bChangParamIn = true;
	}
}


/* Wave out *******************************************************************/
void CSound::InitPlayback ( int iNewBufferSize, bool bNewBlocking )
{
	int err;

	// save buffer size
	iBufferSizeOut = iNewBufferSize / NUM_IN_OUT_CHANNELS; // mono size

	// if playback device was already open, close it first
	if ( phandle != NULL )
	{
		snd_pcm_close ( phandle );
	}

	// playback device (either "hw:0,0" or "plughw:0,0")
	if ( err = snd_pcm_open ( &phandle, "hw:0,0",
		SND_PCM_STREAM_PLAYBACK, SND_PCM_NONBLOCK ) != 0)
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



#if 0
	/* sw parameters --------------------------------------------------------- */
	snd_pcm_sw_params_t* swparams;

// TEST
// allocate an invalid snd_pcm_sw_params_t using standard malloc
if ( err = snd_pcm_sw_params_malloc ( &swparams ) != 0 )
{
	qDebug ( "snd_pcm_sw_params_malloc: %s", snd_strerror ( err ) );
}

/* get the current swparams */
err = snd_pcm_sw_params_current(phandle, swparams);
if (err < 0)
{
	qDebug("Unable to determine current swparams for playback: %s\n", snd_strerror(err));
}

/* start the transfer when the buffer is almost full: */
/* (buffer_size / avail_min) * avail_min */
err = snd_pcm_sw_params_set_start_threshold(phandle, swparams, iCurPeriodSizeOut - 1);
if (err < 0) {
	qDebug("Unable to set start threshold mode for playback: %s\n", snd_strerror(err));
}

/* allow the transfer when at least period_size samples can be processed */
err = snd_pcm_sw_params_set_avail_min(phandle, swparams, iBufferSizeOut);
if (err < 0) {
	qDebug("Unable to set avail min for playback: %s\n", snd_strerror(err));
}

/* align all transfers to 1 sample */
err = snd_pcm_sw_params_set_xfer_align(phandle, swparams, 1);
if (err < 0) {
	qDebug("Unable to set transfer align for playback: %s\n", snd_strerror(err));
}

/* write the parameters to the playback device */
err = snd_pcm_sw_params(phandle, swparams);
if (err < 0) {
	qDebug("Unable to set sw params for playback: %s\n", snd_strerror(err));
}

// clean-up
snd_pcm_sw_params_free ( swparams );
#endif






	// start playback
	snd_pcm_start ( phandle );

	qDebug ( "alsa init playback done" );
}

bool CSound::Write ( CVector<short>& psData )
{
	int size = iBufferSizeIn;
	int start = 0;
	int ret;

	/* Check if device must be opened or reinitialized */
	if ( bChangParamOut == true )
	{
		InitPlayback ( iBufferSizeOut * NUM_IN_OUT_CHANNELS );

		/* Reset flag */
		bChangParamOut = false;
	}

	while ( size )
	{
		ret = snd_pcm_writei ( phandle, &psData[start], size );
//qDebug("start: %d, iBufferSizeIn: %d", start, iBufferSizeIn);

		if ( ret < 0 )
		{
			if ( ret == -EPIPE )
			{
				/* under-run */
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
				if ( ( ret = snd_pcm_wait ( phandle, 1 ) ) < 0 )
				{
					qDebug ( "poll failed (%s)", snd_strerror ( ret ) );
					break;
				}
				continue;
			}
			else if ( ret == -ESTRPIPE )
			{
				qDebug("wstrpipe");

				/* wait until the suspend flag is released */
        		while ( (ret = snd_pcm_resume ( phandle ) ) == -EAGAIN )
				{
                	sleep(1);
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

		size  -= ret;
		start += ret;
	}

	return false;
}

void CSound::SetOutNumBuf(int iNewNum)
{
	/* check new parameter */
	if ( ( iNewNum >= MAX_SND_BUF_OUT ) || ( iNewNum < 1 ) )
	{
		iNewNum = NUM_PERIOD_BLOCKS_OUT;
	}

	/* Change only if parameter is different */
	if ( iNewNum != iCurPeriodSizeOut )
	{
		iCurPeriodSizeOut = iNewNum;
		bChangParamOut = true;
	}
}


/* common **********************************************************************/
bool CSound::SetHWParams(snd_pcm_t* handle, const int iBufferSizeIn,
						 const int iNumPeriodBlocks)
{
	int						err;
	snd_pcm_hw_params_t*	hwparams;

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
	if ( err = snd_pcm_hw_params_set_channels(handle, hwparams, NUM_IN_OUT_CHANNELS ) < 0 )
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





	/* set the buffer and period size */

// TEST
snd_pcm_uframes_t BufferFrames = iBufferSizeIn * iNumPeriodBlocks;

	if (err = snd_pcm_hw_params_set_buffer_size_near(handle, hwparams, &BufferFrames) < 0)
	{
		qDebug("cannot set buffer size (%s)\n", snd_strerror (err));
		return true;
	}

// TEST
snd_pcm_uframes_t PeriodSize = iBufferSizeIn;

	if (err = snd_pcm_hw_params_set_period_size_near(handle, hwparams, &PeriodSize, 0) < 0)
//if (err = snd_pcm_hw_params_set_period_size_max(handle, hwparams, &PeriodSize, 0) < 0)
	{
		qDebug("cannot set period size (%s)\n", snd_strerror (err));
		return true;
	}



	/* Write the parameters to device */
	if (err = snd_pcm_hw_params(handle, hwparams) < 0)
	{
		qDebug("Unable to set hw params : %s", snd_strerror(err));
		return true;
	}



/* check period and buffer size */
snd_pcm_uframes_t buffer_size;
if (err = snd_pcm_hw_params_get_buffer_size(hwparams, &buffer_size) < 0) {
	qDebug("Unable to get buffer size for playback: %s\n", snd_strerror(err));
}
qDebug("buffer size: %d (desired: %d)", buffer_size, iBufferSizeIn * iNumPeriodBlocks);

snd_pcm_uframes_t period_size;
err = snd_pcm_hw_params_get_period_size(hwparams, &period_size, 0);
if (err < 0)
{
	qDebug("Unable to get period size for playback: %s\n", snd_strerror(err));
}
qDebug("frame size: %d (desired: %d)", period_size, iBufferSizeIn);



	/* clean-up */
	snd_pcm_hw_params_free ( hwparams );

	return false;
}

void CSound::Close ()
{
	/* read */
	if ( rhandle != NULL )
	{
		snd_pcm_close ( rhandle );
	}

	rhandle = NULL;

	/* playback */
	if ( phandle != NULL )
	{
		snd_pcm_close ( phandle );
	}

	phandle = NULL;
}

#endif /* WITH_SOUND */
