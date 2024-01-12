/******************************************************************************\
 * Copyright (c) 2004-2024
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
#define kInputBus  1

/* Implementation *************************************************************/
CSound::CSound ( void ( *fpNewProcessCallback ) ( CVector<short>& psData, void* arg ),
                 void*          arg,
                 const QString& strMIDISetup,
                 const bool,
                 const QString& ) :
    CSoundBase ( "CoreAudio iOS", fpNewProcessCallback, arg, strMIDISetup ),
    isInitialized ( false )
{
    try
    {
        NSError* audioSessionError = nil;

        [[AVAudioSession sharedInstance] setCategory:AVAudioSessionCategoryPlayAndRecord error:&audioSessionError];
        [[AVAudioSession sharedInstance] requestRecordPermission:^( BOOL granted ) {
            if ( granted )
            {
                // ok
            }
            else
            {
                // TODO - alert user
            }
        }];
        [[AVAudioSession sharedInstance] setMode:AVAudioSessionModeMeasurement error:&audioSessionError];
    }
    catch ( const CGenErr& generr )
    {
        QMessageBox::warning ( nullptr, "Sound exception", generr.GetErrorText() );
    }

    buffer.mNumberChannels    = 2;
    buffer.mData              = malloc ( 256 * sizeof ( Float32 ) * buffer.mNumberChannels ); // max size
    bufferList.mNumberBuffers = 1;
    bufferList.mBuffers[0]    = buffer;
}

CSound::~CSound() { free ( buffer.mData ); }

/**
 This callback is called when sound card needs output data to play.
 And because Jamulus uses the same buffer to store input and output data (input is sent to server, then output is fetched from server), we actually
 use the output callback to read inputdata first, process it, and then copy the output fetched from server to ioData, which will then be played.
 */
OSStatus CSound::recordingCallback ( void*                       inRefCon,
                                     AudioUnitRenderActionFlags* ioActionFlags,
                                     const AudioTimeStamp*       inTimeStamp,
                                     UInt32                      inBusNumber,
                                     UInt32                      inNumberFrames,
                                     AudioBufferList*            ioData )
{

    CSound* pSound = static_cast<CSound*> ( inRefCon );

    // setting up temp buffer
    pSound->buffer.mDataByteSize   = pSound->iCoreAudioBufferSizeMono * sizeof ( Float32 ) * pSound->buffer.mNumberChannels;
    pSound->bufferList.mBuffers[0] = pSound->buffer;

    // Obtain recorded samples

    // Calling Unit Render to store input data to bufferList
    AudioUnitRender ( pSound->audioUnit, ioActionFlags, inTimeStamp, 1, inNumberFrames, &pSound->bufferList );

    // Now, we have the samples we just read sitting in buffers in bufferList
    // Process the new data
    pSound->processBufferList ( &pSound->bufferList, pSound ); // THIS IS WHERE vecsStereo is filled with data from bufferList

    Float32* pData = (Float32*) ( ioData->mBuffers[0].mData );

    // copy output data
    for ( int i = 0; i < pSound->iCoreAudioBufferSizeMono; i++ )
    {
        pData[2 * i]     = (Float32) pSound->vecsTmpAudioSndCrdStereo[2 * i] / _MAXSHORT;     // left
        pData[2 * i + 1] = (Float32) pSound->vecsTmpAudioSndCrdStereo[2 * i + 1] / _MAXSHORT; // right
    }

    return noErr;
}

void CSound::processBufferList ( AudioBufferList* inInputData, CSound* pSound ) // got stereo input data
{
    QMutexLocker locker ( &pSound->MutexAudioProcessCallback );
    Float32*     pData = static_cast<Float32*> ( inInputData->mBuffers[0].mData );

    // copy input data
    for ( int i = 0; i < pSound->iCoreAudioBufferSizeMono; i++ )
    {
        // copy left and right channels separately
        pSound->vecsTmpAudioSndCrdStereo[2 * i]     = (short) ( pData[2 * i] * _MAXSHORT );     // left
        pSound->vecsTmpAudioSndCrdStereo[2 * i + 1] = (short) ( pData[2 * i + 1] * _MAXSHORT ); // right
    }
    pSound->ProcessCallback ( pSound->vecsTmpAudioSndCrdStereo );
}

// TODO - CSound::Init is called multiple times at launch to verify device capabilities.
// For iOS though, Init takes long, so should better reduce those inits for iOS (Now: 9 times/launch).
int CSound::Init ( const int iCoreAudioBufferSizeMono )
{
    try
    {
        this->iCoreAudioBufferSizeMono = iCoreAudioBufferSizeMono;

        // set internal buffer size value and calculate stereo buffer size
        iCoreAudioBufferSizeStereo = 2 * iCoreAudioBufferSizeMono;

        // create memory for intermediate audio buffer
        vecsTmpAudioSndCrdStereo.Init ( iCoreAudioBufferSizeStereo );

        AVAudioSession* sessionInstance = [AVAudioSession sharedInstance];

        // we are going to play and record so we pick that category
        NSError* error = nil;
        [sessionInstance setCategory:AVAudioSessionCategoryPlayAndRecord
                         withOptions:AVAudioSessionCategoryOptionMixWithOthers | AVAudioSessionCategoryOptionDefaultToSpeaker |
                                     AVAudioSessionCategoryOptionAllowBluetooth | AVAudioSessionCategoryOptionAllowBluetoothA2DP |
                                     AVAudioSessionCategoryOptionAllowAirPlay
                               error:&error];

        // using values from jamulus settings 64 = 2.67ms/2
        NSTimeInterval bufferDuration = iCoreAudioBufferSizeMono / Float32 ( SYSTEM_SAMPLE_RATE_HZ ); // yeah it's math
        [sessionInstance setPreferredIOBufferDuration:bufferDuration error:&error];

        // set the session's sample rate 48000 - the only supported by Jamulus
        [sessionInstance setPreferredSampleRate:SYSTEM_SAMPLE_RATE_HZ error:&error];
        [[AVAudioSession sharedInstance] setActive:YES error:&error];

        OSStatus status;

        // Describe audio component
        AudioComponentDescription desc;
        desc.componentType         = kAudioUnitType_Output;
        desc.componentSubType      = kAudioUnitSubType_RemoteIO;
        desc.componentFlags        = 0;
        desc.componentFlagsMask    = 0;
        desc.componentManufacturer = kAudioUnitManufacturer_Apple;

        // Get component
        AudioComponent inputComponent = AudioComponentFindNext ( NULL, &desc );

        // Get audio units
        status = AudioComponentInstanceNew ( inputComponent, &audioUnit );
        checkStatus ( status );

        // Enable IO for recording
        UInt32 flag = 1;
        status      = AudioUnitSetProperty ( audioUnit, kAudioOutputUnitProperty_EnableIO, kAudioUnitScope_Input, kInputBus, &flag, sizeof ( flag ) );
        checkStatus ( status );

        // Enable IO for playback
        status = AudioUnitSetProperty ( audioUnit, kAudioOutputUnitProperty_EnableIO, kAudioUnitScope_Output, kOutputBus, &flag, sizeof ( flag ) );
        checkStatus ( status );

        // Describe format
        AudioStreamBasicDescription audioFormat;
        audioFormat.mSampleRate       = SYSTEM_SAMPLE_RATE_HZ;
        audioFormat.mFormatID         = kAudioFormatLinearPCM;
        audioFormat.mFormatFlags      = kAudioFormatFlagsNativeEndian | kAudioFormatFlagIsFloat | kAudioFormatFlagIsPacked;
        audioFormat.mFramesPerPacket  = 1;
        audioFormat.mChannelsPerFrame = 2;  // stereo, so 2 interleaved channels
        audioFormat.mBitsPerChannel   = 32; // sizeof float32
        audioFormat.mBytesPerPacket   = 8;  // (sizeof float32) * 2 channels
        audioFormat.mBytesPerFrame    = 8;  //(sizeof float32) * 2 channels

        // Apply format
        status = AudioUnitSetProperty ( audioUnit,
                                        kAudioUnitProperty_StreamFormat,
                                        kAudioUnitScope_Output,
                                        kInputBus,
                                        &audioFormat,
                                        sizeof ( audioFormat ) );
        checkStatus ( status );
        status = AudioUnitSetProperty ( audioUnit,
                                        kAudioUnitProperty_StreamFormat,
                                        kAudioUnitScope_Input,
                                        kOutputBus,
                                        &audioFormat,
                                        sizeof ( audioFormat ) );
        checkStatus ( status );

        // Set callback
        AURenderCallbackStruct callbackStruct;
        callbackStruct.inputProc       = recordingCallback; // this is actually the playback callback
        callbackStruct.inputProcRefCon = this;
        status                         = AudioUnitSetProperty ( audioUnit,
                                        kAudioUnitProperty_SetRenderCallback,
                                        kAudioUnitScope_Global,
                                        kOutputBus,
                                        &callbackStruct,
                                        sizeof ( callbackStruct ) );
        checkStatus ( status );

        // Initialise
        status = AudioUnitInitialize ( audioUnit );
        checkStatus ( status );

        SwitchDevice ( strCurDevName );

        if ( !isInitialized )
        {
            [[NSNotificationCenter defaultCenter]
                addObserverForName:AVAudioSessionRouteChangeNotification
                            object:nil
                             queue:nil
                        usingBlock:^( NSNotification* notification ) {
                            UInt8 reason = [[notification.userInfo valueForKey:AVAudioSessionRouteChangeReasonKey] intValue];
                            if ( reason == AVAudioSessionRouteChangeReasonNewDeviceAvailable or
                                 reason == AVAudioSessionRouteChangeReasonOldDeviceUnavailable )
                            {
                                emit ReinitRequest ( RS_RELOAD_RESTART_AND_INIT ); // reload the available devices frame
                            }
                        }];
        }

        isInitialized = true;
    }
    catch ( const CGenErr& generr )
    {
        QMessageBox::warning ( nullptr, "Sound init exception", generr.GetErrorText() );
    }

    return iCoreAudioBufferSizeMono;
}

void CSound::Start()
{
    // call base class
    CSoundBase::Start();
    try
    {
        OSStatus err = AudioOutputUnitStart ( audioUnit );
        checkStatus ( err );
    }
    catch ( const CGenErr& generr )
    {
        QMessageBox::warning ( nullptr, "Sound start exception", generr.GetErrorText() );
    }
}

void CSound::Stop()
{
    try
    {
        OSStatus err = AudioOutputUnitStop ( audioUnit );
        checkStatus ( err );
    }
    catch ( const CGenErr& generr )
    {
        QMessageBox::warning ( nullptr, "Sound stop exception", generr.GetErrorText() );
    }
    // call base class
    CSoundBase::Stop();
}

void CSound::checkStatus ( int status )
{
    if ( status )
    {
        printf ( "Status not 0! %d\n", status );
    }
}

QString CSound::LoadAndInitializeDriver ( QString strDriverName, bool )
{
    // secure lNumDevs/strDriverNames access
    QMutexLocker locker ( &Mutex );

    // reload the driver list of available sound devices
    GetAvailableInOutDevices();

    // store the current name of the driver
    strCurDevName = strDriverName;

    return "";
}

void CSound::GetAvailableInOutDevices()
{
    // always add system default devices for input and output as first entry
    lNumDevs          = 1;
    strDriverNames[0] = "System Default In/Out Devices";

    AVAudioSession* sessionInstance = [AVAudioSession sharedInstance];

    if ( sessionInstance.availableInputs.count > 1 )
    {
        lNumDevs          = 2;
        strDriverNames[1] = "in: Built-in Mic/out: System Default";
    }
}

void CSound::SwitchDevice ( QString strDriverName )
{
    // find driver index from given driver name
    int iDriverIdx = INVALID_INDEX; // initialize with an invalid index

    for ( int i = 0; i < MAX_NUMBER_SOUND_CARDS; i++ )
    {
        if ( strDriverName.compare ( strDriverNames[i] ) == 0 )
        {
            iDriverIdx = i;
            break;
        }
    }

    NSError* error = nil;

    AVAudioSession* sessionInstance = [AVAudioSession sharedInstance];

    if ( iDriverIdx == 0 ) // system default device
    {
        unsigned long lastInput = sessionInstance.availableInputs.count - 1;
        [sessionInstance setPreferredInput:sessionInstance.availableInputs[lastInput] error:&error];
    }
    else // built-in mic
    {
        [sessionInstance setPreferredInput:sessionInstance.availableInputs[0] error:&error];
    }
}
