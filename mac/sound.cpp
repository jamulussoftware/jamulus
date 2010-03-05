/******************************************************************************\
 * Copyright (c) 2004-2010
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

#include "sound.h"


/* Implementation *************************************************************/
void CSound::OpenCoreAudio()
{
    // open the default output unit
    AudioComponentDescription desc;
    desc.componentType         = kAudioUnitType_Output;
    desc.componentSubType      = kAudioUnitSubType_DefaultOutput;
    desc.componentFlags        = 0;
    desc.componentFlagsMask    = 0;
    desc.componentManufacturer = 0;

    AudioComponent comp = AudioComponentFindNext ( NULL, &desc );
    if ( comp == NULL )
    {
        throw CGenErr ( tr ( "No CoreAudio next component found" ) );
    }

    if ( AudioComponentInstanceNew ( comp, &gOutputUnit ) )
    {
        throw CGenErr ( tr ( "CoreAudio creating component instance failed" ) );
    }

    // set up a callback function to generate output to the output unit
    AURenderCallbackStruct input;
    input.inputProc       = process;
    input.inputProcRefCon = this;

    if ( AudioUnitSetProperty ( gOutputUnit,
                                kAudioUnitProperty_SetRenderCallback,
                                kAudioUnitScope_Input,
                                0,
                                &input,
                                sizeof(input) ) )
    {
        throw CGenErr ( tr ( "CoreAudio audio unit set property failed" ) );
    }
}

void CSound::CloseCoreAudio()
{
    // clean up "gOutputUnit"
    AudioUnitUninitialize ( gOutputUnit );
	CloseComponent        ( gOutputUnit );
}

void CSound::Start()
{
	// start the rendering
	if ( AudioOutputUnitStart ( gOutputUnit ) )
    {
        throw CGenErr ( tr ( "CoreAudio starting failed" ) );
    }
}

void CSound::Stop()
{
    // stop the audio stream
	if ( AudioOutputUnitStop ( gOutputUnit ) )
    {
        throw CGenErr ( tr ( "CoreAudio stopping failed" ) );
    }
}

int CSound::Init ( const int iNewPrefMonoBufferSize )
{
    // store buffer size
    iCoreAudioBufferSizeMono = iNewPrefMonoBufferSize;  

    // first uninitialize unit
    AudioUnitUninitialize ( gOutputUnit );

    // set up stream format
    AudioStreamBasicDescription streamFormat;
    streamFormat.mSampleRate       = SYSTEM_SAMPLE_RATE;
    streamFormat.mFormatID         = kAudioFormatLinearPCM;
    streamFormat.mFormatFlags      = kAudioFormatFlagIsSignedInteger;
    streamFormat.mFramesPerPacket  = iCoreAudioBufferSizeMono;
    streamFormat.mBytesPerFrame    = streamFormat.mChannelsPerFrame * 2; // short type
    streamFormat.mBytesPerPacket   = streamFormat.mBytesPerFrame * streamFormat.mFramesPerPacket;
    streamFormat.mChannelsPerFrame = 2; // stereo
    streamFormat.mBitsPerChannel   = 16;

    if ( AudioUnitSetProperty ( gOutputUnit,
                                kAudioUnitProperty_StreamFormat,
                                kAudioUnitScope_Input,
                                0,
                                &streamFormat,
                                sizeof(streamFormat) ) )
    {
        throw CGenErr ( tr ( "CoreAudio stream format set property failed" ) );
    }

    // (re-)initialize unit
	if ( AudioUnitInitialize ( gOutputUnit ) )
    {
        throw CGenErr ( tr ( "Initialization of CoreAudio failed" ) );
    }

    // init base class
    CSoundBase::Init ( iCoreAudioBufferSizeMono );

    // set internal buffer size value and calculate stereo buffer size
    iCoreAudioBufferSizeStero = 2 * iCoreAudioBufferSizeMono;

    // create memory for intermediate audio buffer
    vecsTmpAudioSndCrdStereo.Init ( iCoreAudioBufferSizeStero );

    return iNewPrefMonoBufferSize;
}

OSStatus CSound::process ( void*                       inRefCon,
                           AudioUnitRenderActionFlags* ioActionFlags,
                           const AudioTimeStamp*       inTimeStamp,
                           UInt32                      inBusNumber,
                           UInt32                      inNumberFrames,
                           AudioBufferList*            ioData )
{
    CSound* pSound = reinterpret_cast<CSound*> ( inRefCon );



/*
	for ( UInt32 channel = 1; channel < ioData->mNumberBuffers; channel++)
    {
		memcpy ( ioData->mBuffers[channel].mData,
                 ioData->mBuffers[0].mData,
                 ioData->mBuffers[0].mDataByteSize );
    }
*/

/*
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
*/

    // call processing callback function
    pSound->ProcessCallback ( pSound->vecsTmpAudioSndCrdStereo );

/*
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
*/
/*
    ioData->mBuffers[0].mDataByteSize = 2048;
    ioData->mBuffers[0].mData = lbuf;
    ioData->mBuffers[0].mNumberChannels = 1;
*/

// TEST
printf ( "processing Core Audio\n" );

    return noErr;
}
