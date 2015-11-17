/******************************************************************************\
 * Copyright (c) 2004-2015
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
CSound::CSound ( void (*fpNewProcessCallback) ( CVector<short>& psData, void* arg ), void* arg ) :
    CSoundBase ( "CoreAudio", true, fpNewProcessCallback, arg )
{
    // Apple Mailing Lists: Subject: GUI Apps should set kAudioHardwarePropertyRunLoop
    // in the HAL, From: Jeff Moore, Date: Fri, 6 Dec 2002
    // Most GUI applciations have several threads on which they receive
    // notifications already, so the having the HAL's thread around is wasteful.
    // Here is what you should do: On the thread you want the HAL to use for
    // notifications (for most apps, this will be the main thread), add the
    // following lines of code:
    // tell the HAL to use the current thread as it's run loop
    CFRunLoopRef               theRunLoop = CFRunLoopGetCurrent();
    AudioObjectPropertyAddress property   = { kAudioHardwarePropertyRunLoop,
                                              kAudioObjectPropertyScopeGlobal,
                                              kAudioObjectPropertyElementMaster };

    AudioObjectSetPropertyData ( kAudioObjectSystemObject,
                                 &property,
                                 0,
                                 NULL,
                                 sizeof ( CFRunLoopRef ),
                                 &theRunLoop );


    // Get available input/output devices --------------------------------------
    UInt32                     iPropertySize;
    AudioObjectPropertyAddress stPropertyAddress;

    stPropertyAddress.mScope   = kAudioObjectPropertyScopeGlobal;
    stPropertyAddress.mElement = kAudioObjectPropertyElementMaster;

    // first get property size of devices array and allocate memory
    stPropertyAddress.mSelector = kAudioHardwarePropertyDevices;

    AudioObjectGetPropertyDataSize ( kAudioObjectSystemObject,
                                     &stPropertyAddress,
                                     0,
                                     NULL,
                                     &iPropertySize );

    AudioDeviceID* audioDevices = (AudioDeviceID*) malloc ( iPropertySize );

    // now actually query all devices present in the system
    AudioObjectGetPropertyData ( kAudioObjectSystemObject,
                                 &stPropertyAddress,
                                 0,
                                 NULL,
                                 &iPropertySize,
                                 audioDevices );

    // calculate device count based on size of returned data array
    const UInt32 deviceCount = iPropertySize / sizeof ( AudioDeviceID );

    // always add system default devices for input and output as first entry
    lNumDevs                 = 0;
    strDriverNames[lNumDevs] = "System Default In/Out Devices";

    iPropertySize               = sizeof ( AudioDeviceID );
    stPropertyAddress.mSelector = kAudioHardwarePropertyDefaultInputDevice;

    if ( AudioObjectGetPropertyData ( kAudioObjectSystemObject,
                                      &stPropertyAddress,
                                      0,
                                      NULL,
                                      &iPropertySize,
                                      &audioInputDevice[lNumDevs] ) )
    {
        throw CGenErr ( tr ( "CoreAudio input AudioHardwareGetProperty call failed. "
                             "It seems that no sound card is available in the system." ) );
    }

    iPropertySize               = sizeof ( AudioDeviceID );
    stPropertyAddress.mSelector = kAudioHardwarePropertyDefaultOutputDevice;

    if ( AudioObjectGetPropertyData ( kAudioObjectSystemObject,
                                      &stPropertyAddress,
                                      0,
                                      NULL,
                                      &iPropertySize,
                                      &audioOutputDevice[lNumDevs] ) )
    {
        throw CGenErr ( tr ( "CoreAudio output AudioHardwareGetProperty call failed. "
                             "It seems that no sound card is available in the system." ) );
    }

    lNumDevs++; // next device

    // add detected devices
    //
    // we add combined entries for input and output for each device so that we
    // do not need two combo boxes in the GUI for input and output (therefore
    // all possible combinations are required which can be a large number)
    for ( UInt32 i = 0; i < deviceCount; i++ )
    {
        for ( UInt32 j = 0; j < deviceCount; j++ )
        {
            // get device infos for both current devices
            QString strDeviceName_i;
            QString strDeviceName_j;
            bool    bIsInput_i;
            bool    bIsInput_j;
            bool    bIsOutput_i;
            bool    bIsOutput_j;

            GetAudioDeviceInfos ( audioDevices[i],
                                  strDeviceName_i,
                                  bIsInput_i,
                                  bIsOutput_i );

            GetAudioDeviceInfos ( audioDevices[j],
                                  strDeviceName_j,
                                  bIsInput_j,
                                  bIsOutput_j );

            // check if i device is input and j device is output and that we are
            // in range
            if ( bIsInput_i && bIsOutput_j && ( lNumDevs < MAX_NUMBER_SOUND_CARDS ) )
            {
                strDriverNames[lNumDevs] = "in: " +
                    strDeviceName_i + "/out: " +
                    strDeviceName_j;

                // store audio device IDs
                audioInputDevice[lNumDevs]  = audioDevices[i];
                audioOutputDevice[lNumDevs] = audioDevices[j];

                lNumDevs++; // next device
            }
        }
    }

    free ( audioDevices );

    // init device index as not initialized (invalid)
    lCurDev                   = INVALID_SNC_CARD_DEVICE;
    CurrentAudioInputDeviceID = 0;
}

void CSound::GetAudioDeviceInfos ( const AudioDeviceID DeviceID,
                                   QString&            strDeviceName,
                                   bool&               bIsInput,
                                   bool&               bIsOutput )
{
    UInt32                     iPropertySize;
    AudioObjectPropertyAddress stPropertyAddress;

    // init return values
    strDeviceName = "UNKNOWN"; // init value in case no name is available
    bIsInput      = false;
    bIsOutput     = false;

    // check if device is input or output or both (is that possible?)
    stPropertyAddress.mSelector = kAudioDevicePropertyStreams;
    stPropertyAddress.mElement  = kAudioObjectPropertyElementMaster;

    // input check
    stPropertyAddress.mScope = kAudioDevicePropertyScopeInput;

    AudioObjectGetPropertyDataSize ( DeviceID,
                                     &stPropertyAddress,
                                     0,
                                     NULL,
                                     &iPropertySize );

    bIsInput = ( iPropertySize > 0 ); // check if any input streams are available

    // output check
    stPropertyAddress.mScope = kAudioDevicePropertyScopeOutput;

    AudioObjectGetPropertyDataSize ( DeviceID,
                                     &stPropertyAddress,
                                     0,
                                     NULL,
                                     &iPropertySize );

    bIsOutput = ( iPropertySize > 0 ); // check if any output streams are available

    // get property name
    CFStringRef sPropertyStringValue;

    stPropertyAddress.mSelector = kAudioObjectPropertyName;
    stPropertyAddress.mScope    = kAudioObjectPropertyScopeGlobal;
    iPropertySize               = sizeof ( CFStringRef );

    AudioObjectGetPropertyData ( DeviceID,
                                 &stPropertyAddress,
                                 0,
                                 NULL,
                                 &iPropertySize,
                                 &sPropertyStringValue );

    // first check if the string is not empty
    if ( CFStringGetLength ( sPropertyStringValue ) > 0 )
    {
        // convert CFString in c-string (quick hack!) and then in QString
        char* sC_strPropValue =
            (char*) malloc ( CFStringGetLength ( sPropertyStringValue ) + 1 );

        if ( CFStringGetCString ( sPropertyStringValue,
                                  sC_strPropValue,
                                  CFStringGetLength ( sPropertyStringValue ) + 1,
                                  kCFStringEncodingISOLatin1 ) )
        {
            strDeviceName = sC_strPropValue;
        }

        free ( sC_strPropValue );
    }
}

QString CSound::LoadAndInitializeDriver ( int iDriverIdx )
{
    // check device capabilities if it fullfills our requirements
    const QString strStat = CheckDeviceCapabilities ( iDriverIdx );

    // check if device is capable
    if ( strStat.isEmpty() )
    {
        // store ID of selected driver if initialization was successful
        lCurDev                   = iDriverIdx;
        CurrentAudioInputDeviceID = audioInputDevice[iDriverIdx];
    }

    return strStat;
}

QString CSound::CheckDeviceCapabilities ( const int iDriverIdx )
{
    UInt32                      iPropertySize;
    Float64                     inputSampleRate;
    AudioStreamBasicDescription	CurDevStreamFormat;
    const Float64               fSystemSampleRate = static_cast<Float64> ( SYSTEM_SAMPLE_RATE_HZ );
    AudioObjectPropertyAddress  stPropertyAddress;

    stPropertyAddress.mScope    = kAudioObjectPropertyScopeGlobal;
    stPropertyAddress.mElement  = kAudioObjectPropertyElementMaster;

    // check input device sample rate
    stPropertyAddress.mSelector = kAudioDevicePropertyNominalSampleRate;
    iPropertySize               = sizeof ( Float64 );

    AudioObjectGetPropertyData ( audioInputDevice[iDriverIdx],
                                 &stPropertyAddress,
                                 0,
                                 NULL,
                                 &iPropertySize,
                                 &inputSampleRate );

    if ( inputSampleRate != fSystemSampleRate )
    {
        // try to change the sample rate
        if ( AudioObjectSetPropertyData ( audioInputDevice[iDriverIdx],
                                          &stPropertyAddress,
                                          0,
                                          NULL,
                                          sizeof ( Float64 ),
                                          &fSystemSampleRate ) != noErr )
        {
            return QString ( tr ( "Current system audio input device sample "
                "rate of %1 Hz is not supported. Please open the Audio-MIDI-Setup in "
                "Applications->Utilities and try to set a sample rate of %2 Hz." ) ).arg (
                static_cast<int> ( inputSampleRate ) ).arg ( SYSTEM_SAMPLE_RATE_HZ );
        }
    }

    // check output device sample rate
    iPropertySize = sizeof ( Float64 );
    Float64 outputSampleRate;

    AudioObjectGetPropertyData ( audioOutputDevice[iDriverIdx],
                                 &stPropertyAddress,
                                 0,
                                 NULL,
                                 &iPropertySize,
                                 &outputSampleRate );

    if ( outputSampleRate != fSystemSampleRate )
    {
        // try to change the sample rate
        if ( AudioObjectSetPropertyData ( audioOutputDevice[iDriverIdx],
                                          &stPropertyAddress,
                                          0,
                                          NULL,
                                          sizeof ( Float64 ),
                                          &fSystemSampleRate ) != noErr )
        {
            return QString ( tr ( "Current system audio output device sample "
                "rate of %1 Hz is not supported. Please open the Audio-MIDI-Setup in "
                "Applications->Utilities and try to set a sample rate of %2 Hz." ) ).arg (
                static_cast<int> ( outputSampleRate ) ).arg ( SYSTEM_SAMPLE_RATE_HZ );
        }
    }

    // According to the AudioHardware documentation: "If the format is a linear PCM
    // format, the data will always be presented as 32 bit, native endian floating
    // point. All conversions to and from the true physical format of the hardware
    // is handled by the devices driver."
    // So we check for the fixed values here.
    iPropertySize               = sizeof ( AudioStreamBasicDescription );
    stPropertyAddress.mSelector = kAudioStreamPropertyVirtualFormat;

    AudioObjectGetPropertyData ( audioOutputDevice[iDriverIdx],
                                 &stPropertyAddress,
                                 0,
                                 NULL,
                                 &iPropertySize,
                                 &CurDevStreamFormat );

    if ( ( CurDevStreamFormat.mFormatID         != kAudioFormatLinearPCM ) ||
         ( CurDevStreamFormat.mFramesPerPacket  != 1 ) ||
         ( CurDevStreamFormat.mBytesPerFrame    != 8 ) ||
         ( CurDevStreamFormat.mBytesPerPacket   != 8 ) ||
         ( CurDevStreamFormat.mChannelsPerFrame != 2 ) ||
         ( CurDevStreamFormat.mBitsPerChannel   != 32 ) )
    {
        return QString ( tr ( "The audio stream format for this audio device is"
                              "not compatible with the requirements." ) );
    }

// TODO     mSampleRate, mFormatFlags      = kAudioFormatFlagIsSignedInteger

// TODO check input device, too!

qDebug() << "mBitsPerChannel" << CurDevStreamFormat.mBitsPerChannel;
qDebug() << "mBytesPerFrame" << CurDevStreamFormat.mBytesPerFrame;
qDebug() << "mBytesPerPacket" << CurDevStreamFormat.mBytesPerPacket;
qDebug() << "mChannelsPerFrame" << CurDevStreamFormat.mChannelsPerFrame;
qDebug() << "mFramesPerPacket" << CurDevStreamFormat.mFramesPerPacket;
qDebug() << "mSampleRate" << CurDevStreamFormat.mSampleRate;



    // everything is ok, return empty string for "no error" case
    return "";
}

void CSound::Start()
{
    // setup callback for xruns (only for input is enough)
    AudioObjectPropertyAddress stPropertyAddress;

    stPropertyAddress.mSelector = kAudioDeviceProcessorOverload;
    stPropertyAddress.mElement  = kAudioObjectPropertyElementMaster;
    stPropertyAddress.mScope    = kAudioObjectPropertyScopeGlobal;

    AudioObjectAddPropertyListener ( audioInputDevice[lCurDev],
                                     &stPropertyAddress,
                                     deviceNotification,
                                     this );

    // register the callback function for input and output
    AudioDeviceCreateIOProcID ( audioInputDevice[lCurDev],
                                callbackIO,
                                this,
                                &audioInputProcID );

    AudioDeviceCreateIOProcID ( audioOutputDevice[lCurDev],
                                callbackIO,
                                this,
                                &audioOutputProcID );

    // start the audio stream
    AudioDeviceStart ( audioInputDevice[lCurDev], audioInputProcID );
    AudioDeviceStart ( audioOutputDevice[lCurDev], audioOutputProcID );

    // call base class
    CSoundBase::Start();
}

void CSound::Stop()
{
    // stop the audio stream
    AudioDeviceStop ( audioInputDevice[lCurDev], audioInputProcID );
    AudioDeviceStop ( audioOutputDevice[lCurDev], audioOutputProcID );

    // unregister the callback function for input and output
    AudioDeviceDestroyIOProcID ( audioInputDevice[lCurDev], audioInputProcID );
    AudioDeviceDestroyIOProcID ( audioOutputDevice[lCurDev], audioOutputProcID );

    // unregister the callback function for xruns
    AudioObjectPropertyAddress stPropertyAddress;

    stPropertyAddress.mSelector = kAudioDeviceProcessorOverload;
    stPropertyAddress.mElement  = kAudioObjectPropertyElementMaster;
    stPropertyAddress.mScope    = kAudioObjectPropertyScopeGlobal;

    AudioObjectRemovePropertyListener( audioInputDevice[lCurDev],
                                       &stPropertyAddress,
                                       deviceNotification,
                                       this );

    // call base class
    CSoundBase::Stop();
}

int CSound::Init ( const int iNewPrefMonoBufferSize )
{
    UInt32 iActualMonoBufferSize;

    // Error message string: in case buffer sizes on input and output cannot be
    // set to the same value
    const QString strErrBufSize = tr ( "The buffer sizes of the current "
        "input and output audio device cannot be set to a common value. Please "
        "choose other input/output audio devices in your system settings." );

    // try to set input buffer size
    iActualMonoBufferSize =
        SetBufferSize ( audioInputDevice[lCurDev], true, iNewPrefMonoBufferSize );

    if ( iActualMonoBufferSize != static_cast<UInt32> ( iNewPrefMonoBufferSize ) )
    {
        // try to set the input buffer size to the output so that we
        // have a matching pair
        if ( SetBufferSize ( audioOutputDevice[lCurDev], false, iActualMonoBufferSize ) !=
             iActualMonoBufferSize )
        {
            throw CGenErr ( strErrBufSize );
        }
    }
    else
    {
        // try to set output buffer size
        if ( SetBufferSize ( audioOutputDevice[lCurDev], false, iNewPrefMonoBufferSize ) !=
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
    iCoreAudioBufferSizeStereo = 2 * iCoreAudioBufferSizeMono;

    // create memory for intermediate audio buffer
    vecsTmpAudioSndCrdStereo.Init ( iCoreAudioBufferSizeStereo );

    return iCoreAudioBufferSizeMono;
}

UInt32 CSound::SetBufferSize ( AudioDeviceID& audioDeviceID,
                               const bool     bIsInput,
                               UInt32         iPrefBufferSize )
{
    AudioObjectPropertyAddress stPropertyAddress;
    stPropertyAddress.mSelector = kAudioDevicePropertyBufferFrameSize;

    if ( bIsInput )
    {
        stPropertyAddress.mScope = kAudioDevicePropertyScopeInput;
    }
    else
    {
        stPropertyAddress.mScope = kAudioDevicePropertyScopeOutput;
    }

    stPropertyAddress.mElement = kAudioObjectPropertyElementMaster;

    // first set the value
    UInt32 iSizeBufValue = sizeof ( UInt32 );
    AudioObjectSetPropertyData ( audioDeviceID,
                                 &stPropertyAddress,
                                 0,
                                 NULL,
                                 iSizeBufValue,
                                 &iPrefBufferSize );

    // read back which value is actually used
    UInt32 iActualMonoBufferSize;
    AudioObjectGetPropertyData ( audioDeviceID,
                                 &stPropertyAddress,
                                 0,
                                 NULL,
                                 &iSizeBufValue,
                                 &iActualMonoBufferSize );

    return iActualMonoBufferSize;
}

OSStatus CSound::deviceNotification ( AudioDeviceID,
                                      UInt32,
                                      const AudioObjectPropertyAddress* /* inAddresses */,
                                      void*                             /* inRefCon */ )
{
// TODO: Do we need this anymore? If not, we can completely remove this function...
/*
    CSound* pSound = static_cast<CSound*> ( inRefCon );

    if ( inAddresses->mSelector == kAudioDeviceProcessorOverload )
    {
        // xrun handling (it is important to act on xruns under CoreAudio
        // since it seems that the xrun situation stays stable for a
        // while and would give you a long time bad audio)
        pSound->EmitReinitRequestSignal ( RS_ONLY_RESTART );
    }
*/

    return noErr;
}

OSStatus CSound::callbackIO ( AudioDeviceID          inDevice,
                              const AudioTimeStamp*,
                              const AudioBufferList* inInputData,
                              const AudioTimeStamp*,
                              AudioBufferList*       outOutputData,
                              const AudioTimeStamp*,
                              void*                  inRefCon )
{
    CSound* pSound = static_cast<CSound*> ( inRefCon );

    // both, the input and output device use the same callback function
    QMutexLocker locker ( &pSound->Mutex );

    if ( inDevice == pSound->CurrentAudioInputDeviceID )
    {
        // check size (float32 has four bytes)
        if ( inInputData->mBuffers[0].mDataByteSize ==
             static_cast<UInt32> ( pSound->iCoreAudioBufferSizeStereo * 4 ) )
        {
            // get a pointer to the input data of the correct type
            Float32* pInData = static_cast<Float32*> ( inInputData->mBuffers[0].mData );

            // copy input data
            for ( int i = 0; i < pSound->iCoreAudioBufferSizeStereo; i++ )
            {
                pSound->vecsTmpAudioSndCrdStereo[i] =
                    (short) ( pInData[i] * _MAXSHORT );
            }

            // call processing callback function
            pSound->ProcessCallback ( pSound->vecsTmpAudioSndCrdStereo );
        }
    }
    else
    {
        // check size (float32 has four bytes)
        if ( outOutputData->mBuffers[0].mDataByteSize ==
             static_cast<UInt32> ( pSound->iCoreAudioBufferSizeStereo * 4 ) )
        {
            // get a pointer to the input data of the correct type
            Float32* pOutData = static_cast<Float32*> ( outOutputData->mBuffers[0].mData );

            // copy output data
            for ( int i = 0; i < pSound->iCoreAudioBufferSizeStereo; i++ )
            {
                pOutData[i] = (Float32)
                    pSound->vecsTmpAudioSndCrdStereo[i] / _MAXSHORT;
            }
        }
    }

    return kAudioHardwareNoError;
}
