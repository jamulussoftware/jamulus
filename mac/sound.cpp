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

	if ( OpenAComponent ( comp, &audioInputUnit ) )
	{
		throw CGenErr ( tr ( "CoreAudio creating input component instance failed" ) );
	}
		
	if ( OpenAComponent ( comp, &audioOutputUnit ) )
	{
		throw CGenErr ( tr ( "CoreAudio creating output component instance failed" ) );
	}

    // we enable input and disable output for input component
    UInt32 enableIO = 1;
    err = AudioUnitSetProperty ( audioInputUnit,
        kAudioOutputUnitProperty_EnableIO,
        kAudioUnitScope_Input,
        1, // input element
        &enableIO,
        sizeof ( enableIO ) );

    enableIO = 0;
    err = AudioUnitSetProperty ( audioInputUnit,
        kAudioOutputUnitProperty_EnableIO,
        kAudioUnitScope_Output,
        0, // output element
        &enableIO,
        sizeof ( enableIO ) );

    // set input device
    size = sizeof ( AudioDeviceID );
    AudioDeviceID inputDevice;
    if ( AudioHardwareGetProperty ( kAudioHardwarePropertyDefaultInputDevice,
                                    &size,
                                    &inputDevice ) )
    {
        throw CGenErr ( tr ( "CoreAudio input AudioHardwareGetProperty call failed" ) );
    }

    if ( AudioUnitSetProperty ( audioInputUnit,
                                kAudioOutputUnitProperty_CurrentDevice,
                                kAudioUnitScope_Global,
                                1,
                                &inputDevice,
                                sizeof ( inputDevice ) ) )
    {
        throw CGenErr ( tr ( "CoreAudio input AudioUnitSetProperty call failed" ) );
    }

    // set up a callback function for new input data
    AURenderCallbackStruct input;
    input.inputProc       = processInput;
    input.inputProcRefCon = this;

    if ( AudioUnitSetProperty ( audioInputUnit,
                                kAudioOutputUnitProperty_SetInputCallback,//kAudioUnitProperty_SetRenderCallback,//
                                kAudioUnitScope_Global,
                                0,
                                &input,
                                sizeof ( input ) ) )
    {
        throw CGenErr ( tr ( "CoreAudio audio unit set property failed" ) );
    }

    // set output device
    size = sizeof ( AudioDeviceID );
    AudioDeviceID outputDevice;
    if ( AudioHardwareGetProperty ( kAudioHardwarePropertyDefaultOutputDevice,
                                    &size,
                                    &outputDevice ) )
    {
        throw CGenErr ( tr ( "CoreAudio output AudioHardwareGetProperty call failed" ) );
    }

    if ( AudioUnitSetProperty ( audioOutputUnit,
                                kAudioOutputUnitProperty_CurrentDevice,
                                kAudioUnitScope_Global,
                                0,
                                &outputDevice,
                                sizeof ( outputDevice ) ) )
    {
        throw CGenErr ( tr ( "CoreAudio output AudioUnitSetProperty call failed" ) );
    }	

    // set up a callback function for new output data
    AURenderCallbackStruct output;
    output.inputProc       = processOutput;
    output.inputProcRefCon = this;

    if ( AudioUnitSetProperty ( audioOutputUnit,
                                kAudioUnitProperty_SetRenderCallback,//kAudioOutputUnitProperty_SetInputCallback,//
                                kAudioUnitScope_Global,
                                0,
                                &output,
                                sizeof ( output ) ) )
    {
        throw CGenErr ( tr ( "CoreAudio audio unit set property failed" ) );
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
    if ( AudioUnitSetProperty ( audioOutputUnit,
                                kAudioUnitProperty_StreamFormat,
                                kAudioUnitScope_Input,
                                0,
                                &streamFormat,
                                sizeof(streamFormat) ) )
    {
        throw CGenErr ( tr ( "CoreAudio stream format set property failed" ) );
    }

    // our input
    if ( AudioUnitSetProperty ( audioInputUnit,
                                kAudioUnitProperty_StreamFormat,
                                kAudioUnitScope_Output,
                                1,
                                &streamFormat,
                                sizeof(streamFormat) ) )
    {
        throw CGenErr ( tr ( "CoreAudio stream format set property failed" ) );
    }


	
// TODO check for required sample rate, if not available, throw error message	
	
	
// TEST
StoreInOutStreamProps ( audioInputUnit );
StoreInOutStreamProps ( audioOutputUnit );


}

void CSound::CloseCoreAudio()
{
    // clean up "gOutputUnit"
    AudioUnitUninitialize ( audioInputUnit );
    AudioUnitUninitialize ( audioOutputUnit );
	CloseComponent        ( audioInputUnit );
	CloseComponent        ( audioOutputUnit );
}

void CSound::Start()
{
	// start the rendering
	AudioOutputUnitStart ( audioInputUnit );
	AudioOutputUnitStart ( audioOutputUnit );

    // call base class
    CSoundBase::Start();
}

void CSound::Stop()
{
    // stop the audio stream
	AudioOutputUnitStop ( audioInputUnit );
	AudioOutputUnitStop ( audioOutputUnit );

    // call base class
    CSoundBase::Stop();
}

int CSound::Init ( const int iNewPrefMonoBufferSize )
{
	
// TODO try to set the preferred buffer size to the audio unit	
	
	// get the audio unit buffer size
	UInt32 bufferSizeFrames;
	UInt32 propertySize = sizeof(UInt32);
	
	
// TEST
AudioUnitGetProperty ( audioOutputUnit, kAudioDevicePropertyBufferFrameSize,
	kAudioUnitScope_Global, 1, &bufferSizeFrames, &propertySize );
printf("out buf size: %d\n",bufferSizeFrames);

	AudioUnitGetProperty ( audioInputUnit, kAudioDevicePropertyBufferFrameSize,
	    kAudioUnitScope_Global, 1, &bufferSizeFrames, &propertySize );

    // store buffer size
    iCoreAudioBufferSizeMono = bufferSizeFrames;  

    // init base class
    CSoundBase::Init ( iCoreAudioBufferSizeMono );
	
    // set internal buffer size value and calculate stereo buffer size
    iCoreAudioBufferSizeStero = 2 * iCoreAudioBufferSizeMono;
	
    // create memory for intermediate audio buffer
    vecsTmpAudioSndCrdStereo.Init ( iCoreAudioBufferSizeStero );

	
// TEST
printf ( "Buffer_Size: %d", (int) bufferSizeFrames );

	
// TODO	
// fill audio unit buffer struct
theBufferList = (AudioBufferList*) malloc ( offsetof ( AudioBufferList,
    mBuffers[0] ) + sizeof(AudioBuffer) );

//(sizeof(AudioBufferList) + (numChannels-1)) * sizeof(AudioBuffer)	


	theBufferList->mNumberBuffers = 1;
    theBufferList->mBuffers[0].mNumberChannels = 2; // stereo
    theBufferList->mBuffers[0].mDataByteSize = bufferSizeFrames * 4; // 2 bytes, 2 channels
	theBufferList->mBuffers[0].mData = &vecsTmpAudioSndCrdStereo[0];
	
    // initialize unit
	if ( AudioUnitInitialize ( audioInputUnit ) )
    {
        throw CGenErr ( tr ( "Initialization of CoreAudio failed" ) );
    }

	if ( AudioUnitInitialize ( audioOutputUnit ) )
    {
        throw CGenErr ( tr ( "Initialization of CoreAudio failed" ) );
    }

    return iCoreAudioBufferSizeMono;
}

OSStatus CSound::processInput ( void*                       inRefCon,
                                AudioUnitRenderActionFlags* ioActionFlags,
                                const AudioTimeStamp*       inTimeStamp,
                                UInt32                      inBusNumber,
                                UInt32                      inNumberFrames,
                                AudioBufferList*            ioData )
{
    CSound* pSound = reinterpret_cast<CSound*> ( inRefCon );
	
    // get the new audio data
    ComponentResult err =
        AudioUnitRender ( pSound->audioInputUnit,
                          ioActionFlags,
                          inTimeStamp,
                          inBusNumber,
                          inNumberFrames,
                          theBufferList );

    // call processing callback function
    pSound->ProcessCallback ( pSound->vecsTmpAudioSndCrdStereo );

    return noErr;
}

OSStatus CSound::processOutput ( void*                       inRefCon,
                                 AudioUnitRenderActionFlags* ioActionFlags,
                                 const AudioTimeStamp*       inTimeStamp,
                                 UInt32                      inBusNumber,
                                 UInt32                      inNumberFrames,
                                 AudioBufferList*            ioData )
{
    CSound* pSound = reinterpret_cast<CSound*> ( inRefCon );

	memcpy ( ioData->mBuffers[0].mData,
		&pSound->vecsTmpAudioSndCrdStereo[0],
		theBufferList->mBuffers[0].mDataByteSize);

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
