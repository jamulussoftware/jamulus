/******************************************************************************\
 * Copyright (c) 2004-2026
 *
 * Author(s):
 *  ann0see and ngocdh based on code from Volker Fischer
 *
 * As of Jamulus 3.12.1dev (commit eb172d47): All new source code contributions must be licensed
 * under AGPL 3.0 or any later version.
 *
 * Existing code: Code contributed before 3.12.1dev (commit eb172d47) was licensed under GPL 2.0+.
 * This code will be licensed under GPL 3.0 (or any later version) from
 * 3.12.1dev (commit eb172d47).  When distributed as part of Jamulus, the AGPL 3.0 terms govern
 * the combined work, including network use provisions.
 *
 ******************************************************************************
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 * ---------------------------------------------------------------------------
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
\******************************************************************************/
#include "sound.h"
#include <AVFoundation/AVFoundation.h>

#define kOutputBus 0
#define kInputBus  1

/* Implementation *************************************************************/
CSound::CSound ( void ( *fpNewProcessCallback ) ( CVector<short>& psData, void* arg ), void* arg, const bool, const QString& ) :
    CSoundBase ( "CoreAudio iOS", fpNewProcessCallback, arg ),
    isInitialized ( false ),
    iNumInChan ( 2 ),
    iSelInputLeftChannel ( 0 ),
    iSelInputRightChannel ( 1 )
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

    // allocate the buffer large enough to hold the maximum number of input
    // channels we support (the actual channel count is only known once an
    // input device has been selected and negotiated, see UpdateInputChannelInfo)
    buffer.mNumberChannels    = iNumInChan;
    buffer.mData              = malloc ( 256 * sizeof ( Float32 ) * MAX_NUM_IN_OUT_CHANNELS ); // max size
    bufferList.mNumberBuffers = 1;
    bufferList.mBuffers[0]    = buffer;

    for ( int i = 0; i < MAX_NUM_IN_OUT_CHANNELS; i++ )
    {
        sChannelNamesInput[i] = QString ( "Channel %1" ).arg ( i + 1 );
    }
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

void CSound::processBufferList ( AudioBufferList* inInputData, CSound* pSound ) // got (possibly multichannel) input data
{
    QMutexLocker locker ( &pSound->MutexAudioProcessCallback );
    Float32*     pData = static_cast<Float32*> ( inInputData->mBuffers[0].mData );

    // the input device may provide more than two channels (e.g. a multichannel
    // USB audio interface), in which case we pick the user-selected left and
    // right channels out of the interleaved buffer
    const int iNumChan = pSound->buffer.mNumberChannels;
    const int iLeftCh  = pSound->iSelInputLeftChannel;
    const int iRightCh = pSound->iSelInputRightChannel;

    // copy input data
    for ( int i = 0; i < pSound->iCoreAudioBufferSizeMono; i++ )
    {
        // copy left and right channels separately
        pSound->vecsTmpAudioSndCrdStereo[2 * i]     = (short) ( pData[iNumChan * i + iLeftCh] * _MAXSHORT );  // left
        pSound->vecsTmpAudioSndCrdStereo[2 * i + 1] = (short) ( pData[iNumChan * i + iRightCh] * _MAXSHORT ); // right
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

        // select the preferred input device (if any was chosen by the user) and
        // negotiate the number of input channels with it. This must happen before
        // we configure the audio unit's input stream format below since multichannel
        // audio interfaces (e.g. USB audio interfaces) may offer more than 2 channels.
        SwitchDevice ( strCurDevName );

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

        // Describe playback format (output bus): always stereo, since the device's
        // speaker/headphone output only ever has 2 channels
        AudioStreamBasicDescription outputAudioFormat;
        outputAudioFormat.mSampleRate       = SYSTEM_SAMPLE_RATE_HZ;
        outputAudioFormat.mFormatID         = kAudioFormatLinearPCM;
        outputAudioFormat.mFormatFlags      = kAudioFormatFlagsNativeEndian | kAudioFormatFlagIsFloat | kAudioFormatFlagIsPacked;
        outputAudioFormat.mFramesPerPacket  = 1;
        outputAudioFormat.mChannelsPerFrame = 2;  // stereo, so 2 interleaved channels
        outputAudioFormat.mBitsPerChannel   = 32; // sizeof float32
        outputAudioFormat.mBytesPerPacket   = 8;  // (sizeof float32) * 2 channels
        outputAudioFormat.mBytesPerFrame    = 8;  //(sizeof float32) * 2 channels

        // Describe recording format (input bus): may have more than 2 interleaved
        // channels when a multichannel input device (e.g. a USB audio interface) is
        // selected. iNumInChan was negotiated above in SwitchDevice().
        AudioStreamBasicDescription inputAudioFormat = outputAudioFormat;
        inputAudioFormat.mChannelsPerFrame            = iNumInChan;
        inputAudioFormat.mBytesPerPacket              = 4 * iNumInChan; // (sizeof float32) * iNumInChan channels
        inputAudioFormat.mBytesPerFrame               = 4 * iNumInChan; // (sizeof float32) * iNumInChan channels

        // Apply formats
        status = AudioUnitSetProperty ( audioUnit,
                                        kAudioUnitProperty_StreamFormat,
                                        kAudioUnitScope_Output,
                                        kInputBus,
                                        &inputAudioFormat,
                                        sizeof ( inputAudioFormat ) );
        checkStatus ( status );
        status = AudioUnitSetProperty ( audioUnit,
                                        kAudioUnitProperty_StreamFormat,
                                        kAudioUnitScope_Input,
                                        kOutputBus,
                                        &outputAudioFormat,
                                        sizeof ( outputAudioFormat ) );
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

    // list every available input port (e.g. built-in mic, headset mic, or an
    // external/multichannel USB or Lightning audio interface) as a selectable
    // device. Output always stays at the system default since iOS does not
    // allow choosing a separate playback device.
    for ( AVAudioSessionPortDescription* port in sessionInstance.availableInputs )
    {
        if ( lNumDevs >= MAX_NUMBER_SOUND_CARDS )
        {
            break;
        }

        strDriverNames[lNumDevs] = QString ( "in: %1/out: System Default" ).arg ( QString::fromNSString ( port.portName ) );
        lNumDevs++;
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

    if ( iDriverIdx <= 0 ) // system default device (or not found -> fall back to default)
    {
        [sessionInstance setPreferredInput:nil error:&error];
    }
    else
    {
        NSArray<AVAudioSessionPortDescription*>* availableInputs = sessionInstance.availableInputs;
        const NSUInteger                         iPortIdx        = static_cast<NSUInteger> ( iDriverIdx - 1 );

        if ( iPortIdx < availableInputs.count )
        {
            [sessionInstance setPreferredInput:availableInputs[iPortIdx] error:&error];
        }
    }

    // ask for as many input channels as the now-selected device can provide so
    // that multichannel input devices are not limited to stereo
    const NSInteger iMaxChannels = [sessionInstance maximumInputNumberOfChannels];

    [sessionInstance setPreferredInputNumberOfChannels:qBound ( static_cast<NSInteger> ( 1 ), iMaxChannels, static_cast<NSInteger> ( MAX_NUM_IN_OUT_CHANNELS ) )
                                                  error:&error];

    UpdateInputChannelInfo();
}

void CSound::UpdateInputChannelInfo()
{
    AVAudioSession* sessionInstance = [AVAudioSession sharedInstance];

    // query how many input channels were actually negotiated with the device
    int iNewNumInChan = static_cast<int> ( sessionInstance.inputNumberOfChannels );

    iNewNumInChan = qBound ( 1, iNewNumInChan, MAX_NUM_IN_OUT_CHANNELS );

    iNumInChan             = iNewNumInChan;
    buffer.mNumberChannels = iNumInChan;

    // try to get descriptive names for each channel from the active input port
    AVAudioSessionPortDescription* inputPort = sessionInstance.currentRoute.inputs.firstObject;
    NSArray<AVAudioSessionChannelDescription*>* channels = inputPort.channels;

    for ( int i = 0; i < iNumInChan; i++ )
    {
        QString strChanName = QString ( "Channel %1" ).arg ( i + 1 );

        if ( channels && ( static_cast<NSUInteger> ( i ) < channels.count ) && channels[i].channelName.length > 0 )
        {
            strChanName = QString::fromNSString ( channels[i].channelName );
        }

        sChannelNamesInput[i] = QString ( "%1: %2" ).arg ( i + 1 ).arg ( strChanName );
    }

    // if the new device has fewer channels than before, clamp the current
    // selection back into range, defaulting to the first (two) channel(s)
    if ( ( iSelInputLeftChannel < 0 ) || ( iSelInputLeftChannel >= iNumInChan ) )
    {
        iSelInputLeftChannel = 0;
    }

    if ( ( iSelInputRightChannel < 0 ) || ( iSelInputRightChannel >= iNumInChan ) )
    {
        iSelInputRightChannel = ( iNumInChan > 1 ) ? 1 : 0;
    }
}

void CSound::SetLeftInputChannel ( const int iNewChan )
{
    // apply parameter after input parameter check
    if ( ( iNewChan >= 0 ) && ( iNewChan < iNumInChan ) )
    {
        iSelInputLeftChannel = iNewChan;
    }
}

void CSound::SetRightInputChannel ( const int iNewChan )
{
    // apply parameter after input parameter check
    if ( ( iNewChan >= 0 ) && ( iNewChan < iNumInChan ) )
    {
        iSelInputRightChannel = iNewChan;
    }
}
