/******************************************************************************\
 * Copyright (c) 2004-2022
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

CSound* CSound::pSound = NULL;

/* Implementation *************************************************************/
CSound::CSound ( void ( *fpProcessCallback ) ( CVector<short>& psData, void* arg ), void* pProcessCallBackArg ) :
    CSoundBase ( "CoreAudio iOS", fpProcessCallback, pProcessCallBackArg ),
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

    soundProperties.bHasAudioDeviceSelection   = true;
    soundProperties.bHasInputChannelSelection  = false;
    soundProperties.bHasOutputChannelSelection = false;
    soundProperties.bHasInputGainSelection     = false;
    // Update default property texts according selected properties
    soundProperties.setDefaultTexts();
    // Set any property text diversions here...

    pSound = this;
}

#ifdef OLD_SOUND_COMPATIBILITY
// Backwards compatibility constructor
CSound::CSound ( void ( *fpProcessCallback ) ( CVector<short>& psData, void* pCallbackArg ),
                 void* pProcessCallBackArg,
                 QString /* strMIDISetup */,
                 bool /* bNoAutoJackConnect */,
                 QString /* strNClientName */ ) :
    CSoundBase ( "CoreAudio iOS", fpProcessCallback, pProcessCallBackArg ),
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

    soundProperties.bHasAudioDeviceSelection   = true;
    soundProperties.bHasInputChannelSelection  = false;
    soundProperties.bHasOutputChannelSelection = false;
    soundProperties.bHasInputGainSelection     = false;
    // Update default property texts according selected properties
    soundProperties.setDefaultTexts();
    // Set any property text diversions here...

    pSound = this;
}
#endif

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
    Q_UNUSED ( inRefCon )
    Q_UNUSED ( inBusNumber )

    //    CSound* pSound = static_cast<CSound*> ( inRefCon );

    // setting up temp buffer
    pSound->buffer.mDataByteSize   = pSound->iDeviceBufferSize * sizeof ( Float32 ) * pSound->buffer.mNumberChannels;
    pSound->bufferList.mBuffers[0] = pSound->buffer;

    // Obtain recorded samples

    // Calling Unit Render to store input data to bufferList
    AudioUnitRender ( pSound->audioUnit, ioActionFlags, inTimeStamp, 1, inNumberFrames, &pSound->bufferList );

    // Now, we have the samples we just read sitting in buffers in bufferList
    // Process the new data
    pSound->processBufferList ( &pSound->bufferList, pSound ); // THIS IS WHERE vecsStereo is filled with data from bufferList

    Float32* pData = (Float32*) ( ioData->mBuffers[0].mData );

    // copy output data
    for ( unsigned int i = 0; i < pSound->iDeviceBufferSize; i++ )
    {
        pData[2 * i]     = (Float32) pSound->audioBuffer[2 * i] / _MAXSHORT;     // left
        pData[2 * i + 1] = (Float32) pSound->audioBuffer[2 * i + 1] / _MAXSHORT; // right
    }

    return noErr;
}

void CSound::processBufferList ( AudioBufferList* inInputData, CSound* pSound ) // got stereo input data
{
    QMutexLocker locker ( &pSound->mutexAudioProcessCallback );
    Float32*     pData = static_cast<Float32*> ( inInputData->mBuffers[0].mData );

    // copy input data
    for ( unsigned int i = 0; i < pSound->iDeviceBufferSize; i++ )
    {
        // copy left and right channels separately
        pSound->audioBuffer[2 * i]     = (short) ( pData[2 * i] * _MAXSHORT );     // left
        pSound->audioBuffer[2 * i + 1] = (short) ( pData[2 * i + 1] * _MAXSHORT ); // right
    }
    pSound->processCallback ( pSound->audioBuffer );
}

bool CSound::init() // pgScorpio: Some of these function results could be stored and checked in checkCapabilities()
{
    bool ok = true;

    try
    {
        iDeviceBufferSize = iProtocolBufferSize;

        // create memory for intermediate audio buffer
        audioBuffer.Init ( ( iDeviceBufferSize * 2 ) );

        AVAudioSession* sessionInstance = [AVAudioSession sharedInstance];

        // we are going to play and record so we pick that category
        NSError* error = nil;
        [sessionInstance setCategory:AVAudioSessionCategoryPlayAndRecord
                         withOptions:AVAudioSessionCategoryOptionMixWithOthers | AVAudioSessionCategoryOptionDefaultToSpeaker |
                                     AVAudioSessionCategoryOptionAllowBluetooth | AVAudioSessionCategoryOptionAllowBluetoothA2DP |
                                     AVAudioSessionCategoryOptionAllowAirPlay
                               error:&error];

        // using values from jamulus settings 64 = 2.67ms/2
        NSTimeInterval bufferDuration = iDeviceBufferSize / Float32 ( SYSTEM_SAMPLE_RATE_HZ ); // yeah it's math
        [sessionInstance setPreferredIOBufferDuration:bufferDuration error:&error];

        // set the session's sample rate 48000 - the only supported by Jamulus
        [sessionInstance setPreferredSampleRate:SYSTEM_SAMPLE_RATE_HZ error:&error];
        // pgScorpio store result for checkCapabilities() ??

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
        ok &= ( status == 0 );

        // Enable IO for recording
        UInt32 flag = 1;
        status      = AudioUnitSetProperty ( audioUnit, kAudioOutputUnitProperty_EnableIO, kAudioUnitScope_Input, kInputBus, &flag, sizeof ( flag ) );
        checkStatus ( status );
        ok &= ( status == 0 );

        // Enable IO for playback
        status = AudioUnitSetProperty ( audioUnit, kAudioOutputUnitProperty_EnableIO, kAudioUnitScope_Output, kOutputBus, &flag, sizeof ( flag ) );
        checkStatus ( status );
        ok &= ( status == 0 );

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
        ok &= ( status == 0 );

        status = AudioUnitSetProperty ( audioUnit,
                                        kAudioUnitProperty_StreamFormat,
                                        kAudioUnitScope_Input,
                                        kOutputBus,
                                        &audioFormat,
                                        sizeof ( audioFormat ) );
        checkStatus ( status );
        ok &= ( status == 0 );

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
        ok &= ( status == 0 );

        // Initialise
        status = AudioUnitInitialize ( audioUnit );
        checkStatus ( status );
        ok &= ( status == 0 );

        // SwitchDevice ( strCurDevName ); ????

        if ( !isInitialized && ok )
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
            isInitialized = true;
        }
    }
    catch ( const CGenErr& generr )
    {
        strErrorList.append ( "Sound init exception:" );
        strErrorList.append ( generr.GetErrorText() );
        isInitialized = false;
    }

    return isInitialized;
}

bool CSound::start()
{
    bool ok = false;

    if ( init() )
    {
        AVAudioSession* sessionInstance = [AVAudioSession sharedInstance];
        NSError*        error           = nil;

        // Old SwitchDevice() action:
        if ( iCurrentDevice == 1 )
        {
            [sessionInstance setPreferredInput:sessionInstance.availableInputs[0] error:&error];
        }
        else // system default device
        {
            unsigned long lastInput = sessionInstance.availableInputs.count - 1;
            [sessionInstance setPreferredInput:sessionInstance.availableInputs[lastInput] error:&error];
        }

        try
        {
            OSStatus err = AudioOutputUnitStart ( audioUnit );
            checkStatus ( err );
            ok = ( err == 0 );
        }
        catch ( const CGenErr& generr )
        {
            strErrorList.append ( "Sound start exception:" );
            strErrorList.append ( generr.GetErrorText() );
            return false;
        }
    }

    return ok;
}

bool CSound::stop()
{
    bool ok = false;

    try
    {
        OSStatus err = AudioOutputUnitStop ( audioUnit );
        checkStatus ( err );
        ok = ( err == 0 );
    }
    catch ( const CGenErr& generr )
    {
        strErrorList.append ( "Sound stop exception:" );
        strErrorList.append ( generr.GetErrorText() );
        return false;
    }

    return ok;
}

void CSound::checkStatus ( int status )
{
    if ( status )
    {
        printf ( "Status not 0! %d\n", status );
    }
}

void CSound::closeCurrentDevice() // Closes the current driver and Clears Device Info
{
    Stop();
    clearDeviceInfo();
}

long CSound::createDeviceList ( bool )
{
    strDeviceNames.clear();

    // always add system default devices for input and output as first entry
    strDeviceNames.append ( "System Default In/Out Devices" );

    AVAudioSession* sessionInstance = [AVAudioSession sharedInstance];
    if ( sessionInstance.availableInputs.count > 1 )
    {
        strDeviceNames.append ( "in: Built-in Mic/out: System Default" );
    }

    lNumDevices = strDeviceNames.size();

    // Validate device selection
    if ( lNumDevices == 1 )
    {
        iCurrentDevice = 0;
    }
    else if ( iCurrentDevice < 0 )
    {
        iCurrentDevice = 0;
    }
    else if ( iCurrentDevice >= lNumDevices )
    {
        iCurrentDevice = ( (int) lNumDevices - 1 );
    }

    soundProperties.bHasAudioDeviceSelection = ( lNumDevices > 1 );

    return lNumDevices;
}

bool CSound::setBaseValues()
{
    createDeviceList();

    clearDeviceInfo();

    // Set the input channel names:
    strInputChannelNames.append ( "input left" );
    strInputChannelNames.append ( "input right" );
    lNumInChan = 2;

    // Set added input channels: ( Always 0 for iOS )
    addAddedInputChannelNames();

    // Set the output channel names:
    strOutputChannelNames.append ( "output left" );
    strOutputChannelNames.append ( "output right" );
    lNumOutChan = 2;

    // Select input and output channels:
    resetChannelMapping();

    // Set the In/Out Latency:
    fInOutLatencyMs = 0.0;
    // TODO !!!

    return true;
}

bool CSound::checkCapabilities()
{
    // TODO ???
    //  For now anything is OK
    return true;
}

/*
void CSound::SwitchDevice ( QString strDriverName ) pgScorpio: This functionality has been moved to start()
{
    // find driver index from given driver name
    int iDriverIdx = GetIndexOfDevice(strDriverName);

    NSError* error = nil;

    AVAudioSession* sessionInstance = [AVAudioSession sharedInstance];

    if ( iDriverIdx == 0 ) // system default device
    {
        unsigned long lastInput = sessionInstance.availableInputs.count - 1;
        [sessionInstance setPreferredInput:sessionInstance.availableInputs[lastInput] error:&error];
    }
    else if ( iDriverIdx == 1 )
    {
        [sessionInstance setPreferredInput:sessionInstance.availableInputs[0] error:&error];
    }
}
*/

bool CSound::checkDeviceChange ( CSoundBase::tDeviceChangeCheck mode, int iDriverIndex ) // Open device sequence handling....
{
    if ( ( iDriverIndex < 0 ) || ( iDriverIndex >= lNumDevices ) )
    {
        return false;
    }

    switch ( mode )
    {
    case CSoundBase::tDeviceChangeCheck::CheckOpen:
        // We have no other choice than trying to start the device already...
        Stop();
        iCurrentDevice = iDriverIndex;
        return Start();

    case CSoundBase::tDeviceChangeCheck::CheckCapabilities:
        return checkCapabilities();

    case CSoundBase::tDeviceChangeCheck::Activate:
        return setBaseValues();

    case CSoundBase::tDeviceChangeCheck::Abort:
        Stop();
        setBaseValues(); // Still set Base values, since we still might have changed device !
        return true;

    default:
        return false;
    }
}

unsigned int CSound::getDeviceBufferSize ( unsigned int iDesiredBufferSize )
{
    if ( iDesiredBufferSize < 64 )
    {
        return 64;
    }

    return iDesiredBufferSize;
}
