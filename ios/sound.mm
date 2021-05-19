/******************************************************************************\
 * Copyright (c) 2004-2020
 *
 * Author(s):
 *  ann0see and ngocdh based on code from Volker Fischer
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
#include <AVFoundation/AVFoundation.h>


#define kOutputBus 0
#define kInputBus 1


void checkStatus(int status){
    if (status) {
        printf("Status not 0! %d\n", status);
    }
}

void checkStatus(int status, char * s){
    if (status) {
        printf("Status not 0! %d - %s \n", status, s);
    }
}

/**
 This callback is called when sound card needs output data to play. And because Jamulus use the same buffer to store input and output data (input is sent to server, then output is fetched from server), we actually use the output callback to read inputdata first, process it, and then copy the output fetched from server to ioData, which will then be played.
 */
static OSStatus recordingCallback(void *inRefCon,
                                  AudioUnitRenderActionFlags *ioActionFlags,
                                  const AudioTimeStamp *inTimeStamp,
                                  UInt32 inBusNumber,
                                  UInt32 inNumberFrames,
                                  AudioBufferList *ioData) {
    
    CSound * pSound = static_cast<CSound*> ( inRefCon );
    
    AudioBuffer buffer;
    
    buffer.mNumberChannels = 2;
    buffer.mDataByteSize = pSound->iCoreAudioBufferSizeMono * sizeof(Float32) * buffer.mNumberChannels;
    buffer.mData = malloc( buffer.mDataByteSize );
    
    // Put buffer in a AudioBufferList
    AudioBufferList bufferList;
    bufferList.mNumberBuffers = 1;
    bufferList.mBuffers[0] = buffer;
    
    
    // Then:
    // Obtain recorded samples
    
    OSStatus status;
    
    // Calling Unit Render to store input data to bufferList
    status = AudioUnitRender(pSound->audioUnit,
                             ioActionFlags,
                             inTimeStamp,
                             1,
                             inNumberFrames,
                             &bufferList);
    //checkStatus(status, (char *)" Just called AudioUnitRender ");
    
    // Now, we have the samples we just read sitting in buffers in bufferList
    // Process the new data
    pSound->processBufferList(&bufferList,pSound); //THIS IS WHERE vecsStereo is filled with data from bufferList
    
    // release the malloc'ed data in the buffer we created earlier
    free(bufferList.mBuffers[0].mData);
    
    Float32* pData = (Float32*) ( ioData->mBuffers[0].mData );

    // copy output data
    for ( int i = 0; i < pSound->iCoreAudioBufferSizeMono; i++ )
    {
        pData[2 * i]    = (Float32) pSound->vecsTmpAudioSndCrdStereo[2 * i] / _MAXSHORT; //left
        pData[2 * i + 1] = (Float32) pSound->vecsTmpAudioSndCrdStereo[2 * i + 1] / _MAXSHORT; //right
    }
    
    return noErr;
}


void CSound::processBufferList(AudioBufferList* inInputData, CSound* pSound) //got stereo input data
{
    QMutexLocker locker ( &pSound->MutexAudioProcessCallback );
    Float32* pData             = static_cast<Float32*> ( inInputData->mBuffers[0].mData );

    // copy input data
    for ( int i = 0; i < pSound->iCoreAudioBufferSizeMono; i++ )
    {
        // copy left and right channels separately
        pSound->vecsTmpAudioSndCrdStereo[2 * i]     = (short) ( pData[2 * i] * _MAXSHORT ); //left
        pSound->vecsTmpAudioSndCrdStereo[2 * i + 1] = (short) ( pData[2 * i + 1] * _MAXSHORT ); //right
    }
    pSound->ProcessCallback(pSound->vecsTmpAudioSndCrdStereo);
}


/* Implementation *************************************************************/
CSound::CSound ( void           (*fpNewProcessCallback) ( CVector<short>& psData, void* arg ),
                 void*          arg,
                 const QString& strMIDISetup,
                 const bool     ,
                 const QString& ) :
    CSoundBase ( "CoreAudio iOS", fpNewProcessCallback, arg, strMIDISetup ),
    midiInPortRef ( static_cast<MIDIPortRef> ( NULL ) )
{
    NSError *audioSessionError = nil;
  
    [[AVAudioSession sharedInstance] setCategory:AVAudioSessionCategoryPlayAndRecord error:&audioSessionError];
    [[AVAudioSession sharedInstance] requestRecordPermission:^(BOOL granted) {
        if (granted) {
            // ok
        }
    }];
    [[AVAudioSession sharedInstance] setMode:AVAudioSessionModeMeasurement error:&audioSessionError];
}

int CSound::Init ( const int iCoreAudioBufferSizeMono )
{
    // init base class
    //CSoundBase::Init ( iCoreAudioBufferSizeMono ); this does nothing
    this->iCoreAudioBufferSizeMono = iCoreAudioBufferSizeMono;

    // set internal buffer size value and calculate stereo buffer size
    iCoreAudioBufferSizeStereo = 2 * iCoreAudioBufferSizeMono;

    //create memory for intermediate audio buffer
    vecsTmpAudioSndCrdStereo.Init ( iCoreAudioBufferSizeStereo );
    
    AVAudioSession *sessionInstance = [AVAudioSession sharedInstance];
    
    // we are going to play and record so we pick that category
    NSError *error = nil;
    [sessionInstance setCategory:AVAudioSessionCategoryPlayAndRecord withOptions:AVAudioSessionCategoryOptionMixWithOthers | AVAudioSessionCategoryOptionDefaultToSpeaker | AVAudioSessionCategoryOptionAllowBluetooth | AVAudioSessionCategoryOptionAllowBluetoothA2DP | AVAudioSessionCategoryOptionAllowAirPlay error:&error];
    
    // NGOCDH - using values from jamulus settings 64 = 2.67ms/2
    NSTimeInterval bufferDuration = iCoreAudioBufferSizeMono / 48000.0; //yeah it's math
    [sessionInstance setPreferredIOBufferDuration:bufferDuration error:&error];
    
    // set the session's sample rate 48000 - the only supported by Jamulus ?
    [sessionInstance setPreferredSampleRate:48000 error:&error];
    [[AVAudioSession sharedInstance] setActive:YES error:&error];
    
    OSStatus status;
    
    // Describe audio component
    AudioComponentDescription desc;
    desc.componentType = kAudioUnitType_Output;
    desc.componentSubType = kAudioUnitSubType_RemoteIO;
    desc.componentFlags = 0;
    desc.componentFlagsMask = 0;
    desc.componentManufacturer = kAudioUnitManufacturer_Apple;
    
    // Get component
    AudioComponent inputComponent = AudioComponentFindNext(NULL, &desc);
    
    // Get audio units
    status = AudioComponentInstanceNew(inputComponent, &audioUnit);
    checkStatus(status);
    
    // Enable IO for recording
    UInt32 flag = 1;
    status = AudioUnitSetProperty(audioUnit,
                                  kAudioOutputUnitProperty_EnableIO,
                                  kAudioUnitScope_Input,
                                  kInputBus,
                                  &flag,
                                  sizeof(flag));
    checkStatus(status);
    
    // Enable IO for playback
    status = AudioUnitSetProperty(audioUnit,
                                  kAudioOutputUnitProperty_EnableIO,
                                  kAudioUnitScope_Output,
                                  kOutputBus,
                                  &flag,
                                  sizeof(flag));
    checkStatus(status);
    
    // Describe format
    AudioStreamBasicDescription audioFormat;
    audioFormat.mSampleRate            = 48000.00;
    audioFormat.mFormatID            = kAudioFormatLinearPCM;
    audioFormat.mFormatFlags        = kAudioFormatFlagsNativeEndian | kAudioFormatFlagIsFloat | kAudioFormatFlagIsPacked;
    audioFormat.mFramesPerPacket    = 1;
    audioFormat.mChannelsPerFrame    = 2; //steoreo, so 2 interleaved channels
    audioFormat.mBitsPerChannel        = 32;//sizeof float32
    audioFormat.mBytesPerPacket        = 8;// (sizeof float32) * 2 channels
    audioFormat.mBytesPerFrame        = 8;//(sizeof float32) * 2 channels
    
    // Apply format
    status = AudioUnitSetProperty(audioUnit,
                                  kAudioUnitProperty_StreamFormat,
                                  kAudioUnitScope_Output,
                                  kInputBus,
                                  &audioFormat,
                                  sizeof(audioFormat));
    checkStatus(status);
    status = AudioUnitSetProperty(audioUnit,
                                  kAudioUnitProperty_StreamFormat,
                                  kAudioUnitScope_Input,
                                  kOutputBus,
                                  &audioFormat,
                                  sizeof(audioFormat));
    checkStatus(status);
    
    
    // Set callback
    AURenderCallbackStruct callbackStruct;
    callbackStruct.inputProc = recordingCallback;//this is actually the playback callback
    callbackStruct.inputProcRefCon = this;
    status = AudioUnitSetProperty(audioUnit,
                                  kAudioUnitProperty_SetRenderCallback,
                                  kAudioUnitScope_Global,
                                  kOutputBus,
                                  &callbackStruct,
                                  sizeof(callbackStruct));
    checkStatus(status);
    
    // Initialise
    status = AudioUnitInitialize(audioUnit);
    checkStatus(status);
    
  return iCoreAudioBufferSizeMono;
}

void CSound::Start()
{
    // call base class
    CSoundBase::Start();
    OSStatus err = AudioOutputUnitStart(audioUnit);
    checkStatus(err);
}

void CSound::Stop()
{
    OSStatus err = AudioOutputUnitStop(audioUnit);
    checkStatus(err);
    // call base class
    CSoundBase::Stop();
}

void CSound::SetInputDeviceId( int deviceid )
{
    NSError *error = nil;
    bool builtinmic = true;
    
    if (deviceid==0) builtinmic = false; //try external device

    AVAudioSession *sessionInstance = [AVAudioSession sharedInstance];
    //assumming iOS only has max 2 inputs: 0 for builtin mic and 1 for external device
    if ( builtinmic )
    {
        [sessionInstance setPreferredInput:sessionInstance.availableInputs[0] error:&error];
    }
    else
    {
        unsigned long lastInput = sessionInstance.availableInputs.count - 1;
        [sessionInstance setPreferredInput:sessionInstance.availableInputs[lastInput] error:&error];
    }
    //TODO - error checking
}
