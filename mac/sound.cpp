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
    if ( AudioHardwareGetProperty ( kAudioHardwarePropertyDefaultInputDevice,
                                    &size,
                                    &audioInputDevice ) )
    {
        throw CGenErr ( tr ( "CoreAudio input AudioHardwareGetProperty call failed" ) );
    }

    if ( AudioUnitSetProperty ( audioInputUnit,
                                kAudioOutputUnitProperty_CurrentDevice,
                                kAudioUnitScope_Global,
                                1,
                                &audioInputDevice,
                                sizeof ( audioInputDevice ) ) )
    {
        throw CGenErr ( tr ( "CoreAudio input AudioUnitSetProperty call failed" ) );
    }

    // set up a callback function for new input data
    AURenderCallbackStruct input;
    input.inputProc       = processInput;
    input.inputProcRefCon = this;

    if ( AudioUnitSetProperty ( audioInputUnit,
                                kAudioOutputUnitProperty_SetInputCallback,
                                kAudioUnitScope_Global,
                                0,
                                &input,
                                sizeof ( input ) ) )
    {
        throw CGenErr ( tr ( "CoreAudio audio unit set property failed" ) );
    }

    // set output device
    size = sizeof ( AudioDeviceID );
    if ( AudioHardwareGetProperty ( kAudioHardwarePropertyDefaultOutputDevice,
                                    &size,
                                    &audioOutputDevice ) )
    {
        throw CGenErr ( tr ( "CoreAudio output AudioHardwareGetProperty call failed" ) );
    }

    if ( AudioUnitSetProperty ( audioOutputUnit,
                                kAudioOutputUnitProperty_CurrentDevice,
                                kAudioUnitScope_Global,
                                0,
                                &audioOutputDevice,
                                sizeof ( audioOutputDevice ) ) )
    {
        throw CGenErr ( tr ( "CoreAudio output AudioUnitSetProperty call failed" ) );
    }

    // set up a callback function for new output data
    AURenderCallbackStruct output;
    output.inputProc       = processOutput;
    output.inputProcRefCon = this;

    if ( AudioUnitSetProperty ( audioOutputUnit,
                                kAudioUnitProperty_SetRenderCallback,
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

    // check input device sample rate
    size = sizeof ( Float64 );
    Float64 inputSampleRate;
    AudioUnitGetProperty ( audioInputUnit,
                           kAudioUnitProperty_SampleRate,
                           kAudioUnitScope_Input,
                           1,
                           &inputSampleRate,
                           &size );

    if ( static_cast<int> ( inputSampleRate ) != SYSTEM_SAMPLE_RATE )
    {
        throw CGenErr ( QString ( tr ( "Current system audio input device sample "
            "rate of %1 Hz is not supported. Please open the Audio-MIDI-Setup in "
            "Applications->Utilities and try to set a sample rate of %2 Hz." ) ).arg (
            static_cast<int> ( inputSampleRate ) ).arg ( SYSTEM_SAMPLE_RATE ) );
    }

    // check output device sample rate
    size = sizeof ( Float64 );
    Float64 outputSampleRate;
    AudioUnitGetProperty ( audioOutputUnit,
                           kAudioUnitProperty_SampleRate,
                           kAudioUnitScope_Output,
                           0,
                           &outputSampleRate,
                           &size );

    if ( static_cast<int> ( outputSampleRate ) != SYSTEM_SAMPLE_RATE )
    {
        throw CGenErr ( QString ( tr ( "Current system audio output device sample "
            "rate of %1 Hz is not supported. Please open the Audio-MIDI-Setup in "
            "Applications->Utilities and try to set a sample rate of %2 Hz." ) ).arg (
            static_cast<int> ( outputSampleRate ) ).arg ( SYSTEM_SAMPLE_RATE ) );
    }

    // allocate memory for buffer struct
    pBufferList = (AudioBufferList*) malloc ( offsetof ( AudioBufferList,
        mBuffers[0] ) + sizeof ( AudioBuffer ) );
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
    UInt32 iActualMonoBufferSize;

    // Error message string: in case buffer sizes on input and output cannot be set to the same value
    const QString strErrBufSize = tr ( "The buffer sizes of the current "
        "input and output audio device cannot be set to a common value. Please "
        "choose other input/output audio devices in your system settings." );

    // try to set input buffer size
    iActualMonoBufferSize =
        SetBufferSize ( audioInputDevice, true, iNewPrefMonoBufferSize );

    if ( iActualMonoBufferSize != static_cast<UInt32> ( iNewPrefMonoBufferSize ) )
    {
        // try to set the input buffer size to the output so that we
        // have a matching pair
        if ( SetBufferSize ( audioOutputDevice, false, iActualMonoBufferSize ) !=
             iActualMonoBufferSize )
        {
            throw CGenErr ( strErrBufSize );
        }
    }
    else
    {
        // try to set output buffer size
        if ( SetBufferSize ( audioOutputDevice, false, iNewPrefMonoBufferSize ) !=
             static_cast<UInt32> ( iNewPrefMonoBufferSize ) )
        {
            throw CGenErr ( strErrBufSize );
        }
    }

    // store buffer size
    iCoreAudioBufferSizeMono = iActualMonoBufferSize;  

    // init base class
    CSoundBase::Init ( iCoreAudioBufferSizeMono );

    // set internal buffer size value and calculate stereo buffer size
    iCoreAudioBufferSizeStero = 2 * iCoreAudioBufferSizeMono;

    // create memory for intermediate audio buffer
    vecsTmpAudioSndCrdStereo.Init ( iCoreAudioBufferSizeStero );

    // fill audio unit buffer struct
    pBufferList->mNumberBuffers = 1;
    pBufferList->mBuffers[0].mNumberChannels = 2; // stereo
    pBufferList->mBuffers[0].mDataByteSize = iCoreAudioBufferSizeMono * 4; // 2 bytes, 2 channels
    pBufferList->mBuffers[0].mData = &vecsTmpAudioSndCrdStereo[0];

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

UInt32 CSound::SetBufferSize ( AudioDeviceID& audioDeviceID,
                               const bool     bIsInput,
                               UInt32         iPrefBufferSize )
{
    // first set the value
    UInt32 iSizeBufValue = sizeof ( UInt32 );
    AudioDeviceSetProperty ( audioDeviceID,
                             NULL,
                             0,
                             bIsInput,
                             kAudioDevicePropertyBufferFrameSize,
                             iSizeBufValue,
                             &iPrefBufferSize );

    // read back which value is actually used
    UInt32 iActualMonoBufferSize;
    AudioDeviceGetProperty ( audioDeviceID,
                             0,
                             bIsInput,
                             kAudioDevicePropertyBufferFrameSize,
                             &iSizeBufValue,
                             &iActualMonoBufferSize );

    return iActualMonoBufferSize;
}

OSStatus CSound::processInput ( void*                       inRefCon,
                                AudioUnitRenderActionFlags* ioActionFlags,
                                const AudioTimeStamp*       inTimeStamp,
                                UInt32                      inBusNumber,
                                UInt32                      inNumberFrames,
                                AudioBufferList* )
{
    CSound* pSound = reinterpret_cast<CSound*> ( inRefCon );

    QMutexLocker locker ( &pSound->Mutex );

    // get the new audio data
    AudioUnitRender ( pSound->audioInputUnit,
                      ioActionFlags,
                      inTimeStamp,
                      inBusNumber,
                      inNumberFrames,
                      pSound->pBufferList );

    // call processing callback function
    pSound->ProcessCallback ( pSound->vecsTmpAudioSndCrdStereo );

    return noErr;
}

OSStatus CSound::processOutput ( void* inRefCon,
                                 AudioUnitRenderActionFlags*,
                                 const AudioTimeStamp*,
                                 UInt32,
                                 UInt32,
                                 AudioBufferList* ioData )
{
    CSound* pSound = reinterpret_cast<CSound*> ( inRefCon );

    QMutexLocker locker ( &pSound->Mutex );

    memcpy ( ioData->mBuffers[0].mData,
        &pSound->vecsTmpAudioSndCrdStereo[0],
        pSound->pBufferList->mBuffers[0].mDataByteSize);

    return noErr;
}
