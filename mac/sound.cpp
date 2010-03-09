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




// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// TODO remove the following code as soon as the Coreaudio is working!!!!
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// TEST Debug functions
#include <string.h>
void StoreInOutStreamProps ( ComponentInstance in );
void StoreAudioStreamBasicDescription ( AudioStreamBasicDescription in, std::string text );
static FILE* pFile = fopen ( "test.dat", "w" );





// TEST
// allocated to hold buffer data
AudioBufferList* theBufferList; 




/* Implementation *************************************************************/
void CSound::OpenCoreAudio()
{
    UInt32          size;
	ComponentResult err;

    // open the default unit
    ComponentDescription desc;
    desc.componentType         = kAudioUnitType_Output;
    desc.componentSubType      = kAudioUnitSubType_HALOutput;
    desc.componentManufacturer = kAudioUnitManufacturer_Apple;
    desc.componentFlags        = 0;
    desc.componentFlagsMask    = 0;

    Component comp = FindNextComponent ( NULL, &desc );
    if ( comp == NULL )
    {
        throw CGenErr ( tr ( "No CoreAudio next component found" ) );
    }
	
    if ( OpenAComponent ( comp, &audioUnit ) )
    {
        throw CGenErr ( tr ( "CoreAudio creating component instance failed" ) );
    }

    // TODO
    UInt32 enableIO = 0;
    err = AudioUnitSetProperty ( audioUnit,
        kAudioOutputUnitProperty_EnableIO,
        kAudioUnitScope_Input,
        1, // input element
        &enableIO,
        sizeof ( enableIO ) );

    enableIO = 1;
    err = AudioUnitSetProperty ( audioUnit,
        kAudioOutputUnitProperty_EnableIO,
        kAudioUnitScope_Output,
        0, // output element
        &enableIO,
        sizeof ( enableIO ) );

    // set up a callback function to generate output to the output unit
    AURenderCallbackStruct input;
    input.inputProc       = process;
    input.inputProcRefCon = this;

    if ( AudioUnitSetProperty ( audioUnit,
                                kAudioUnitProperty_SetRenderCallback,
                                kAudioUnitScope_Global,
                                0,
                                &input,
                                sizeof ( input ) ) )
    {
        throw CGenErr ( tr ( "CoreAudio audio unit set property failed" ) );
    }

    // set input device
    size = sizeof ( AudioDeviceID );
    AudioDeviceID inputDevice;
    if ( AudioHardwareGetProperty ( kAudioHardwarePropertyDefaultOutputDevice,
                                    &size,
                                    &inputDevice ) )
    {
        throw CGenErr ( tr ( "CoreAudio AudioHardwareGetProperty call failed" ) );
    }

    if ( AudioUnitSetProperty ( audioUnit,
                                kAudioOutputUnitProperty_CurrentDevice,
                                kAudioUnitScope_Global,
                                0,
                                &inputDevice,
                                sizeof ( inputDevice ) ) )
    {
        throw CGenErr ( tr ( "CoreAudio AudioUnitSetProperty call failed" ) );
    }

    // set up stream format
    AudioStreamBasicDescription streamFormat;
    streamFormat.mSampleRate       = SYSTEM_SAMPLE_RATE;
    streamFormat.mFormatID         = kAudioFormatLinearPCM;
    streamFormat.mFormatFlags      = kAudioFormatFlagIsSignedInteger;
    streamFormat.mFramesPerPacket  = 1;
    streamFormat.mBytesPerFrame    = 4;
    streamFormat.mBytesPerPacket   = 4;
    streamFormat.mChannelsPerFrame = 2; // stereo
    streamFormat.mBitsPerChannel   = 16;
 
    // our output
    if ( AudioUnitSetProperty ( audioUnit,
                                kAudioUnitProperty_StreamFormat,
                                kAudioUnitScope_Input,
                                0,
                                &streamFormat,
                                sizeof(streamFormat) ) )
    {
        throw CGenErr ( tr ( "CoreAudio stream format set property failed" ) );
    }

    // our input
    if ( AudioUnitSetProperty ( audioUnit,
                                kAudioUnitProperty_StreamFormat,
                                kAudioUnitScope_Output,
                                1,
                                &streamFormat,
                                sizeof(streamFormat) ) )
    {
        throw CGenErr ( tr ( "CoreAudio stream format set property failed" ) );
    }


// TEST
StoreInOutStreamProps ( audioUnit );


}

void CSound::CloseCoreAudio()
{
    // clean up "gOutputUnit"
    AudioUnitUninitialize ( audioUnit );
	CloseComponent        ( audioUnit );
}

void CSound::Start()
{
	// start the rendering
	if ( AudioOutputUnitStart ( audioUnit ) )
    {
        throw CGenErr ( tr ( "CoreAudio starting failed" ) );
    }

    // call base class
    CSoundBase::Start();

// TEST
printf ( "start\n" );
}

void CSound::Stop()
{
    // stop the audio stream
	if ( AudioOutputUnitStop ( audioUnit ) )
    {
        throw CGenErr ( tr ( "CoreAudio stopping failed" ) );
    }

    // call base class
    CSoundBase::Stop();
	
// TEST
printf ( "stop\n" );	
}

int CSound::Init ( const int iNewPrefMonoBufferSize )
{
    // store buffer size
    iCoreAudioBufferSizeMono = iNewPrefMonoBufferSize;  



// TEST
//Get the size of the IO buffer(s)
UInt32 bufferSizeFrames, bufferSizeBytes;

UInt32 propertySize = sizeof(bufferSizeFrames);
AudioUnitGetProperty(audioUnit, kAudioDevicePropertyBufferFrameSize,
					 kAudioUnitScope_Global, 1, &bufferSizeFrames, &propertySize);

bufferSizeBytes = bufferSizeFrames * sizeof(Float32);

printf("Buffer_Size: %d", (int) bufferSizeFrames);


//malloc buffer lists
theBufferList = (AudioBufferList*) malloc(
    offsetof(AudioBufferList, mBuffers[0]) + (sizeof(AudioBuffer) * 2));

theBufferList->mNumberBuffers = 2;

//pre-malloc buffers for AudioBufferLists
for ( UInt32 i = 0; i < theBufferList->mNumberBuffers; i++ )
{
    theBufferList->mBuffers[i].mNumberChannels = 2;
    theBufferList->mBuffers[i].mDataByteSize = bufferSizeBytes;
    theBufferList->mBuffers[i].mData = malloc(bufferSizeBytes);
}





    // (re-)initialize unit
	if ( AudioUnitInitialize ( audioUnit ) )
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

// TEST
for ( UInt32 channel = 0; channel < theBufferList->mNumberBuffers; channel++)
{
	memset ( theBufferList->mBuffers[channel].mData, 0,
             theBufferList->mBuffers[channel].mDataByteSize );
}


    // get the new audio data
    AudioUnitRender ( pSound->audioUnit,
                      ioActionFlags,
                      inTimeStamp,
                      inBusNumber,
                      inNumberFrames,
                      theBufferList);


/*
// TEST
//static FILE* pFile = fopen ( "test.dat", "w" );
for ( int test = 0; test < theBufferList->mBuffers[0].mDataByteSize / 4; test++ )
{
    fprintf ( pFile, "%e\n", static_cast<float*>(theBufferList->mBuffers[0].mData)[test]);
}
fflush ( pFile );
*/

/*
// TEST output seems to work!!!
	for ( UInt32 i = 1; i < ioData->mBuffers[0].mDataByteSize / 2; i++)
    {
        static_cast<short*> ( ioData->mBuffers[0].mData)[i] = (short) ( (double) rand() / RAND_MAX * 10000 );
    }
*/

/*	
// TEST
static FILE* pFile = fopen ( "test.dat", "w" );
fprintf ( pFile, "%d %d %d\n", ioActionFlags, inNumberFrames, err);
fflush ( pFile );
*/

/*
// TEST
static FILE* pFile = fopen ( "test.dat", "w" );
for ( int test = 0; test < theBufferList->mBuffers[0].mDataByteSize / 4; test++ )
{
    fprintf ( pFile, "%e %d %d\n", static_cast<float*>(theBufferList->mBuffers[0].mData)[test],
        static_cast<short*>(theBufferList->mBuffers[0].mData)[test],
        static_cast<short*> ( ioData->mBuffers[0].mData)[test]);
}
fflush ( pFile );
*/

// TEST
    printf ( "processing Core Audio %d, inBusNumber: %d\n", (int) inNumberFrames, (int) inBusNumber );


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
	
/*	
// TEST
for ( int i = 0; i < inNumberFrames; i++ )
{
	pSound->vecsTmpAudioSndCrdStereo[2 * i]     = static_cast<SInt16*>(ioData->mBuffers[0].mData)[i];
	pSound->vecsTmpAudioSndCrdStereo[2 * i + 1] = static_cast<SInt16*>(ioData->mBuffers[0].mData)[i];
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


    return noErr;
}



// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// TODO remove the following code as soon as the Coreaudio is working!!!!
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// TEST Debug functions
void StoreAudioStreamBasicDescription ( AudioStreamBasicDescription in,
                                        std::string text )
{
    fprintf ( pFile, "*** AudioStreamBasicDescription: %s***\n", text.c_str() );
    fprintf ( pFile, "mSampleRate:       %e\n", in.mSampleRate );
    fprintf ( pFile, "mFormatID:         %d\n", (int) in.mFormatID );
    fprintf ( pFile, "mFormatFlags:      %d\n", (int) in.mFormatFlags );
    fprintf ( pFile, "mFramesPerPacket:  %d\n", (int) in.mFramesPerPacket );
    fprintf ( pFile, "mBytesPerFrame:    %d\n", (int) in.mBytesPerFrame );
    fprintf ( pFile, "mBytesPerPacket:   %d\n", (int) in.mBytesPerPacket );
    fprintf ( pFile, "mChannelsPerFrame: %d\n", (int) in.mChannelsPerFrame );
    fprintf ( pFile, "mBitsPerChannel:   %d\n", (int) in.mBitsPerChannel );
//    fprintf ( pFile, "mReserved %d\n", in.mReserved );
    fflush ( pFile );
}

void StoreInOutStreamProps ( ComponentInstance in )
{
    // input bus 1
    AudioStreamBasicDescription DeviceFormatin1;
    UInt32 size = sizeof ( AudioStreamBasicDescription );
    AudioUnitGetProperty ( in,
                           kAudioUnitProperty_StreamFormat,
                           kAudioUnitScope_Input,
                           1,
                           &DeviceFormatin1,
                           &size );
    StoreAudioStreamBasicDescription ( DeviceFormatin1, "Input Bus 1" );

    // output bus 1
    AudioStreamBasicDescription DeviceFormatout1;
    size = sizeof ( AudioStreamBasicDescription );
    AudioUnitGetProperty ( in,
                           kAudioUnitProperty_StreamFormat,
                           kAudioUnitScope_Output,
                           1,
                           &DeviceFormatout1,
                           &size );
    StoreAudioStreamBasicDescription ( DeviceFormatout1, "Output Bus 1" );

    // input bus 0
    AudioStreamBasicDescription DeviceFormatin0;
    size = sizeof ( AudioStreamBasicDescription );
    AudioUnitGetProperty ( in,
                           kAudioUnitProperty_StreamFormat,
                           kAudioUnitScope_Input,
                           0,
                           &DeviceFormatin0,
                           &size );
    StoreAudioStreamBasicDescription ( DeviceFormatin0, "Input Bus 0" );

    // output bus 0
    AudioStreamBasicDescription DeviceFormatout0;
    size = sizeof ( AudioStreamBasicDescription );
    AudioUnitGetProperty ( in,
                           kAudioUnitProperty_StreamFormat,
                           kAudioUnitScope_Output,
                           0,
                           &DeviceFormatout0,
                           &size );                                                                     
    StoreAudioStreamBasicDescription ( DeviceFormatout0, "Output Bus 0" );
}
