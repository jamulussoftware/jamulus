/******************************************************************************\
 * Copyright (c) 2004-2025
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
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 *
\******************************************************************************/

#include "sound.h"

/* Implementation *************************************************************/
CSound::CSound ( void ( *fpNewProcessCallback ) ( CVector<short>& psData, void* arg ),
                 void*          arg,
                 const QString& strMIDISetup,
                 const bool,
                 const QString& ) :
    CSoundBase ( "CoreAudio", fpNewProcessCallback, arg, strMIDISetup ),
    midiClient ( static_cast<MIDIClientRef> ( NULL ) ),
    midiInPortRef ( static_cast<MIDIPortRef> ( NULL ) )
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
    AudioObjectPropertyAddress property   = { kAudioHardwarePropertyRunLoop, kAudioObjectPropertyScopeGlobal, kAudioObjectPropertyElementMaster };

    AudioObjectSetPropertyData ( kAudioObjectSystemObject, &property, 0, NULL, sizeof ( CFRunLoopRef ), &theRunLoop );

    // initial query for available input/output sound devices in the system
    GetAvailableInOutDevices();

    // init device index as not initialized (invalid)
    lCurDev                    = INVALID_INDEX;
    CurrentAudioInputDeviceID  = 0;
    CurrentAudioOutputDeviceID = 0;
    iNumInChan                 = 0;
    iNumInChanPlusAddChan      = 0;
    iNumOutChan                = 0;
    iSelInputLeftChannel       = 0;
    iSelInputRightChannel      = 0;
    iSelOutputLeftChannel      = 0;
    iSelOutputRightChannel     = 0;
}

CSound::~CSound()
{
    // Ensure MIDI resources are properly cleaned up
    DestroyMIDIPort(); // This will destroy the port if it exists

    // Explicitly destroy the client if it exists
    if ( midiClient != static_cast<MIDIClientRef> ( NULL ) )
    {
        OSStatus result = MIDIClientDispose ( midiClient );
        if ( result != noErr )
        {
            qWarning() << "Failed to dispose CoreAudio MIDI client in destructor. Error code:" << result;
        }
        midiClient = static_cast<MIDIClientRef> ( NULL );
    }
}

void CSound::GetAvailableInOutDevices()
{
    UInt32                     iPropertySize = 0;
    AudioObjectPropertyAddress stPropertyAddress;

    stPropertyAddress.mScope   = kAudioObjectPropertyScopeGlobal;
    stPropertyAddress.mElement = kAudioObjectPropertyElementMaster;

    // first get property size of devices array and allocate memory
    stPropertyAddress.mSelector = kAudioHardwarePropertyDevices;

    AudioObjectGetPropertyDataSize ( kAudioObjectSystemObject, &stPropertyAddress, 0, NULL, &iPropertySize );

    CVector<AudioDeviceID> vAudioDevices ( iPropertySize );

    // now actually query all devices present in the system
    AudioObjectGetPropertyData ( kAudioObjectSystemObject, &stPropertyAddress, 0, NULL, &iPropertySize, &vAudioDevices[0] );

    // calculate device count based on size of returned data array
    const UInt32 iDeviceCount = iPropertySize / sizeof ( AudioDeviceID );

    // always add system default devices for input and output as first entry
    lNumDevs                 = 0;
    strDriverNames[lNumDevs] = "System Default In/Out Devices";

    iPropertySize               = sizeof ( AudioDeviceID );
    stPropertyAddress.mSelector = kAudioHardwarePropertyDefaultInputDevice;

    if ( AudioObjectGetPropertyData ( kAudioObjectSystemObject, &stPropertyAddress, 0, NULL, &iPropertySize, &audioInputDevice[lNumDevs] ) )
    {
        throw CGenErr ( tr ( "No sound card is available in your system. "
                             "CoreAudio input AudioHardwareGetProperty call failed." ) );
    }

    iPropertySize               = sizeof ( AudioDeviceID );
    stPropertyAddress.mSelector = kAudioHardwarePropertyDefaultOutputDevice;

    if ( AudioObjectGetPropertyData ( kAudioObjectSystemObject, &stPropertyAddress, 0, NULL, &iPropertySize, &audioOutputDevice[lNumDevs] ) )
    {
        throw CGenErr ( tr ( "No sound card is available in the system. "
                             "CoreAudio output AudioHardwareGetProperty call failed." ) );
    }

    lNumDevs++; // next device

    // add detected devices
    //
    // we add combined entries for input and output for each device so that we
    // do not need two combo boxes in the GUI for input and output (therefore
    // all possible combinations are required which can be a large number)
    for ( UInt32 i = 0; i < iDeviceCount; i++ )
    {
        for ( UInt32 j = 0; j < iDeviceCount; j++ )
        {
            // get device infos for both current devices
            QString strDeviceName_i;
            QString strDeviceName_j;
            bool    bIsInput_i;
            bool    bIsInput_j;
            bool    bIsOutput_i;
            bool    bIsOutput_j;

            GetAudioDeviceInfos ( vAudioDevices[i], strDeviceName_i, bIsInput_i, bIsOutput_i );

            GetAudioDeviceInfos ( vAudioDevices[j], strDeviceName_j, bIsInput_j, bIsOutput_j );

            // check if i device is input and j device is output and that we are
            // in range
            if ( bIsInput_i && bIsOutput_j && ( lNumDevs < MAX_NUMBER_SOUND_CARDS ) )
            {
                strDriverNames[lNumDevs] = "in: " + strDeviceName_i + "/out: " + strDeviceName_j;

                // store audio device IDs
                audioInputDevice[lNumDevs]  = vAudioDevices[i];
                audioOutputDevice[lNumDevs] = vAudioDevices[j];

                lNumDevs++; // next device
            }
        }
    }
}

void CSound::GetAudioDeviceInfos ( const AudioDeviceID DeviceID, QString& strDeviceName, bool& bIsInput, bool& bIsOutput )
{
    UInt32                     iPropertySize;
    AudioObjectPropertyAddress stPropertyAddress;

    // init return values
    bIsInput  = false;
    bIsOutput = false;

    // check if device is input or output or both (is that possible?)
    stPropertyAddress.mSelector = kAudioDevicePropertyStreams;
    stPropertyAddress.mElement  = kAudioObjectPropertyElementMaster;

    // input check
    iPropertySize            = 0;
    stPropertyAddress.mScope = kAudioDevicePropertyScopeInput;

    AudioObjectGetPropertyDataSize ( DeviceID, &stPropertyAddress, 0, NULL, &iPropertySize );

    bIsInput = ( iPropertySize > 0 ); // check if any input streams are available

    // output check
    iPropertySize            = 0;
    stPropertyAddress.mScope = kAudioDevicePropertyScopeOutput;

    AudioObjectGetPropertyDataSize ( DeviceID, &stPropertyAddress, 0, NULL, &iPropertySize );

    bIsOutput = ( iPropertySize > 0 ); // check if any output streams are available

    // get property name
    CFStringRef sPropertyStringValue = NULL;

    stPropertyAddress.mSelector = kAudioObjectPropertyName;
    stPropertyAddress.mScope    = kAudioObjectPropertyScopeGlobal;
    iPropertySize               = sizeof ( CFStringRef );

    AudioObjectGetPropertyData ( DeviceID, &stPropertyAddress, 0, NULL, &iPropertySize, &sPropertyStringValue );

    // convert string
    if ( !ConvertCFStringToQString ( sPropertyStringValue, strDeviceName ) )
    {
        // use a default name in case the conversion did not succeed
        strDeviceName = "UNKNOWN";
    }
}

int CSound::CountChannels ( AudioDeviceID devID, bool isInput )
{
    OSStatus err;
    UInt32   propSize;
    int      result = 0;

    if ( isInput )
    {
        vecNumInBufChan.Init ( 0 );
    }
    else
    {
        vecNumOutBufChan.Init ( 0 );
    }

    // it seems we have multiple buffers where each buffer has only one channel,
    // in that case we assume that each input channel has its own buffer
    AudioObjectPropertyScope theScope = isInput ? kAudioDevicePropertyScopeInput : kAudioDevicePropertyScopeOutput;

    AudioObjectPropertyAddress theAddress = { kAudioDevicePropertyStreamConfiguration, theScope, 0 };

    AudioObjectGetPropertyDataSize ( devID, &theAddress, 0, NULL, &propSize );

    AudioBufferList* buflist = (AudioBufferList*) malloc ( propSize );

    err = AudioObjectGetPropertyData ( devID, &theAddress, 0, NULL, &propSize, buflist );

    if ( !err )
    {
        for ( UInt32 i = 0; i < buflist->mNumberBuffers; ++i )
        {
            // The correct value mNumberChannels for an AudioBuffer can be derived from the mChannelsPerFrame
            // and the interleaved flag. For non interleaved formats, mNumberChannels is always 1.
            // For interleaved formats, mNumberChannels is equal to mChannelsPerFrame.
            result += buflist->mBuffers[i].mNumberChannels;

            if ( isInput )
            {
                vecNumInBufChan.Add ( buflist->mBuffers[i].mNumberChannels );
            }
            else
            {
                vecNumOutBufChan.Add ( buflist->mBuffers[i].mNumberChannels );
            }
        }
    }
    free ( buflist );

    return result;
}

QString CSound::LoadAndInitializeDriver ( QString strDriverName, bool )
{
    // secure lNumDevs/strDriverNames access
    QMutexLocker locker ( &Mutex );

    // reload the driver list of available sound devices
    GetAvailableInOutDevices();

    // find driver index from given driver name
    int iDriverIdx = INVALID_INDEX; // initialize with an invalid index

    for ( int i = 0; i < MAX_NUMBER_SOUND_CARDS; i++ )
    {
        if ( strDriverName.compare ( strDriverNames[i] ) == 0 )
        {
            iDriverIdx = i;
        }
    }

    // if the selected driver was not found, return an error message
    if ( iDriverIdx == INVALID_INDEX )
    {
        return tr ( "The currently selected audio device is no longer present. Please check your audio device." );
    }

    // check device capabilities if it fulfills our requirements
    const QString strStat = CheckDeviceCapabilities ( iDriverIdx );

    // check if device is capable and if not the same device is used
    if ( strStat.isEmpty() && ( strCurDevName.compare ( strDriverNames[iDriverIdx] ) != 0 ) )
    {
        AudioObjectPropertyAddress stPropertyAddress;

        // unregister callbacks if previous device was valid
        if ( lCurDev != INVALID_INDEX )
        {
            stPropertyAddress.mElement = kAudioObjectPropertyElementMaster;
            stPropertyAddress.mScope   = kAudioObjectPropertyScopeGlobal;

            // unregister callback functions for device property changes
            stPropertyAddress.mSelector = kAudioHardwarePropertyDefaultOutputDevice;

            AudioObjectRemovePropertyListener ( kAudioObjectSystemObject, &stPropertyAddress, deviceNotification, this );

            stPropertyAddress.mSelector = kAudioHardwarePropertyDefaultInputDevice;

            AudioObjectRemovePropertyListener ( kAudioObjectSystemObject, &stPropertyAddress, deviceNotification, this );

            stPropertyAddress.mSelector = kAudioDevicePropertyDeviceHasChanged;

            AudioObjectRemovePropertyListener ( audioOutputDevice[lCurDev], &stPropertyAddress, deviceNotification, this );

            AudioObjectRemovePropertyListener ( audioInputDevice[lCurDev], &stPropertyAddress, deviceNotification, this );

            stPropertyAddress.mSelector = kAudioDevicePropertyDeviceIsAlive;

            AudioObjectRemovePropertyListener ( audioOutputDevice[lCurDev], &stPropertyAddress, deviceNotification, this );

            AudioObjectRemovePropertyListener ( audioInputDevice[lCurDev], &stPropertyAddress, deviceNotification, this );
        }

        // store ID of selected driver if initialization was successful
        lCurDev                    = iDriverIdx;
        CurrentAudioInputDeviceID  = audioInputDevice[iDriverIdx];
        CurrentAudioOutputDeviceID = audioOutputDevice[iDriverIdx];

        // register callbacks
        stPropertyAddress.mElement = kAudioObjectPropertyElementMaster;
        stPropertyAddress.mScope   = kAudioObjectPropertyScopeGlobal;

        // setup callbacks for device property changes
        stPropertyAddress.mSelector = kAudioDevicePropertyDeviceHasChanged;

        AudioObjectAddPropertyListener ( audioInputDevice[lCurDev], &stPropertyAddress, deviceNotification, this );

        AudioObjectAddPropertyListener ( audioOutputDevice[lCurDev], &stPropertyAddress, deviceNotification, this );

        stPropertyAddress.mSelector = kAudioDevicePropertyDeviceIsAlive;

        AudioObjectAddPropertyListener ( audioInputDevice[lCurDev], &stPropertyAddress, deviceNotification, this );

        AudioObjectAddPropertyListener ( audioOutputDevice[lCurDev], &stPropertyAddress, deviceNotification, this );

        stPropertyAddress.mSelector = kAudioHardwarePropertyDefaultOutputDevice;

        AudioObjectAddPropertyListener ( kAudioObjectSystemObject, &stPropertyAddress, deviceNotification, this );

        stPropertyAddress.mSelector = kAudioHardwarePropertyDefaultInputDevice;

        AudioObjectAddPropertyListener ( kAudioObjectSystemObject, &stPropertyAddress, deviceNotification, this );

        // the device has changed, per definition we reset the channel
        // mapping to the defaults (first two available channels)
        SetLeftInputChannel ( 0 );
        SetRightInputChannel ( 1 );
        SetLeftOutputChannel ( 0 );
        SetRightOutputChannel ( 1 );

        // store the current name of the driver
        strCurDevName = strDriverNames[iDriverIdx];
    }

    return strStat;
}

QString CSound::CheckDeviceCapabilities ( const int iDriverIdx )
{
    UInt32                      iPropertySize;
    AudioStreamBasicDescription CurDevStreamFormat;
    Float64                     inputSampleRate   = 0;
    Float64                     outputSampleRate  = 0;
    const Float64               fSystemSampleRate = static_cast<Float64> ( SYSTEM_SAMPLE_RATE_HZ );
    AudioObjectPropertyAddress  stPropertyAddress;

    stPropertyAddress.mScope   = kAudioObjectPropertyScopeGlobal;
    stPropertyAddress.mElement = kAudioObjectPropertyElementMaster;

    // check input device sample rate
    stPropertyAddress.mSelector = kAudioDevicePropertyNominalSampleRate;
    iPropertySize               = sizeof ( Float64 );

    if ( AudioObjectGetPropertyData ( audioInputDevice[iDriverIdx], &stPropertyAddress, 0, NULL, &iPropertySize, &inputSampleRate ) )
    {
        return QString ( tr ( "The audio input device is no longer available. Please check if your input device is connected correctly." ) );
    }

    if ( inputSampleRate != fSystemSampleRate )
    {
        // try to change the sample rate
        if ( AudioObjectSetPropertyData ( audioInputDevice[iDriverIdx], &stPropertyAddress, 0, NULL, sizeof ( Float64 ), &fSystemSampleRate ) !=
             noErr )
        {
            return QString ( tr ( "The sample rate on the current input device isn't %1 Hz and is therefore incompatible. "
                                  "Please select another device or try setting the sample rate to %1 Hz "
                                  "manually via Audio-MIDI-Setup (in Applications->Utilities)." ) )
                .arg ( SYSTEM_SAMPLE_RATE_HZ );
        }
    }

    // check output device sample rate
    iPropertySize = sizeof ( Float64 );

    if ( AudioObjectGetPropertyData ( audioOutputDevice[iDriverIdx], &stPropertyAddress, 0, NULL, &iPropertySize, &outputSampleRate ) )
    {
        return QString ( tr ( "The audio output device is no longer available. Please check if your output device is connected correctly." ) );
    }

    if ( outputSampleRate != fSystemSampleRate )
    {
        // try to change the sample rate
        if ( AudioObjectSetPropertyData ( audioOutputDevice[iDriverIdx], &stPropertyAddress, 0, NULL, sizeof ( Float64 ), &fSystemSampleRate ) !=
             noErr )
        {
            return QString ( tr ( "The sample rate on the current output device isn't %1 Hz and is therefore incompatible. "
                                  "Please select another device or try setting the sample rate to %1 Hz "
                                  "manually via Audio-MIDI-Setup (in Applications->Utilities)." ) )
                .arg ( SYSTEM_SAMPLE_RATE_HZ );
        }
    }

    // get the stream ID of the input device (at least one stream must always exist)
    iPropertySize               = 0;
    stPropertyAddress.mSelector = kAudioDevicePropertyStreams;
    stPropertyAddress.mScope    = kAudioObjectPropertyScopeInput;

    AudioObjectGetPropertyDataSize ( audioInputDevice[iDriverIdx], &stPropertyAddress, 0, NULL, &iPropertySize );

    CVector<AudioStreamID> vInputStreamIDList ( iPropertySize );

    AudioObjectGetPropertyData ( audioInputDevice[iDriverIdx], &stPropertyAddress, 0, NULL, &iPropertySize, &vInputStreamIDList[0] );

    const AudioStreamID inputStreamID = vInputStreamIDList[0];

    // get the stream ID of the output device (at least one stream must always exist)
    iPropertySize               = 0;
    stPropertyAddress.mSelector = kAudioDevicePropertyStreams;
    stPropertyAddress.mScope    = kAudioObjectPropertyScopeOutput;

    AudioObjectGetPropertyDataSize ( audioOutputDevice[iDriverIdx], &stPropertyAddress, 0, NULL, &iPropertySize );

    CVector<AudioStreamID> vOutputStreamIDList ( iPropertySize );

    AudioObjectGetPropertyData ( audioOutputDevice[iDriverIdx], &stPropertyAddress, 0, NULL, &iPropertySize, &vOutputStreamIDList[0] );

    const AudioStreamID outputStreamID = vOutputStreamIDList[0];

    // According to the AudioHardware documentation: "If the format is a linear PCM
    // format, the data will always be presented as 32 bit, native endian floating
    // point. All conversions to and from the true physical format of the hardware
    // is handled by the devices driver.".
    // check the input
    iPropertySize               = sizeof ( AudioStreamBasicDescription );
    stPropertyAddress.mSelector = kAudioStreamPropertyVirtualFormat;
    stPropertyAddress.mScope    = kAudioObjectPropertyScopeGlobal;

    AudioObjectGetPropertyData ( inputStreamID, &stPropertyAddress, 0, NULL, &iPropertySize, &CurDevStreamFormat );

    if ( ( CurDevStreamFormat.mFormatID != kAudioFormatLinearPCM ) || ( CurDevStreamFormat.mFramesPerPacket != 1 ) ||
         ( CurDevStreamFormat.mBitsPerChannel != 32 ) || ( !( CurDevStreamFormat.mFormatFlags & kAudioFormatFlagIsFloat ) ) ||
         ( !( CurDevStreamFormat.mFormatFlags & kAudioFormatFlagIsPacked ) ) )
    {
        return QString ( tr ( "The stream format on the current input device isn't "
                              "compatible with this software. Please select another device." ) );
    }

    // check the output
    AudioObjectGetPropertyData ( outputStreamID, &stPropertyAddress, 0, NULL, &iPropertySize, &CurDevStreamFormat );

    if ( ( CurDevStreamFormat.mFormatID != kAudioFormatLinearPCM ) || ( CurDevStreamFormat.mFramesPerPacket != 1 ) ||
         ( CurDevStreamFormat.mBitsPerChannel != 32 ) || ( !( CurDevStreamFormat.mFormatFlags & kAudioFormatFlagIsFloat ) ) ||
         ( !( CurDevStreamFormat.mFormatFlags & kAudioFormatFlagIsPacked ) ) )
    {
        return QString ( tr ( "The stream format on the current output device isn't "
                              "compatible with %1. Please select another device." ) )
            .arg ( APP_NAME );
    }

    // store the input and out number of channels for this device
    iNumInChan  = CountChannels ( audioInputDevice[iDriverIdx], true );
    iNumOutChan = CountChannels ( audioOutputDevice[iDriverIdx], false );

    // clip the number of input/output channels to our allowed maximum
    if ( iNumInChan > MAX_NUM_IN_OUT_CHANNELS )
    {
        iNumInChan = MAX_NUM_IN_OUT_CHANNELS;
    }
    if ( iNumOutChan > MAX_NUM_IN_OUT_CHANNELS )
    {
        iNumOutChan = MAX_NUM_IN_OUT_CHANNELS;
    }

    // get the channel names of the input device
    for ( int iCurInCH = 0; iCurInCH < iNumInChan; iCurInCH++ )
    {
        CFStringRef sPropertyStringValue = NULL;

        stPropertyAddress.mSelector = kAudioObjectPropertyElementName;
        stPropertyAddress.mElement  = iCurInCH + 1;
        stPropertyAddress.mScope    = kAudioObjectPropertyScopeInput;
        iPropertySize               = sizeof ( CFStringRef );

        AudioObjectGetPropertyData ( audioInputDevice[iDriverIdx], &stPropertyAddress, 0, NULL, &iPropertySize, &sPropertyStringValue );

        // convert string
        const bool bConvOK = ConvertCFStringToQString ( sPropertyStringValue, sChannelNamesInput[iCurInCH] );

        // add the "[n]:" at the beginning as is in the Audio-Midi-Setup
        if ( !bConvOK || ( iPropertySize == 0 ) )
        {
            // use a default name in case there was an error or the name is empty
            sChannelNamesInput[iCurInCH] = QString ( "%1: Channel %1" ).arg ( iCurInCH + 1 );
        }
        else
        {
            sChannelNamesInput[iCurInCH].prepend ( QString ( "%1: " ).arg ( iCurInCH + 1 ) );
        }
    }

    // get the channel names of the output device
    for ( int iCurOutCH = 0; iCurOutCH < iNumOutChan; iCurOutCH++ )
    {
        CFStringRef sPropertyStringValue = NULL;

        stPropertyAddress.mSelector = kAudioObjectPropertyElementName;
        stPropertyAddress.mElement  = iCurOutCH + 1;
        stPropertyAddress.mScope    = kAudioObjectPropertyScopeOutput;
        iPropertySize               = sizeof ( CFStringRef );

        AudioObjectGetPropertyData ( audioOutputDevice[iDriverIdx], &stPropertyAddress, 0, NULL, &iPropertySize, &sPropertyStringValue );

        // convert string
        const bool bConvOK = ConvertCFStringToQString ( sPropertyStringValue, sChannelNamesOutput[iCurOutCH] );

        // add the "[n]:" at the beginning as is in the Audio-Midi-Setup
        if ( !bConvOK || ( iPropertySize == 0 ) )
        {
            // use a default name in case there was an error or the name is empty
            sChannelNamesOutput[iCurOutCH] = QString ( "%1: Channel %1" ).arg ( iCurOutCH + 1 );
        }
        else
        {
            sChannelNamesOutput[iCurOutCH].prepend ( QString ( "%1: " ).arg ( iCurOutCH + 1 ) );
        }
    }

    // special case with 4 input channels: support adding channels
    if ( iNumInChan == 4 )
    {
        // add four mixed channels (i.e. 4 normal, 4 mixed channels)
        iNumInChanPlusAddChan = 8;

        for ( int iCh = 0; iCh < iNumInChanPlusAddChan; iCh++ )
        {
            int iSelCH, iSelAddCH;

            GetSelCHAndAddCH ( iCh, iNumInChan, iSelCH, iSelAddCH );

            if ( iSelAddCH >= 0 )
            {
                // for mixed channels, show both audio channel names to be mixed
                sChannelNamesInput[iCh] = sChannelNamesInput[iSelCH] + " + " + sChannelNamesInput[iSelAddCH];
            }
        }
    }
    else
    {
        // regular case: no mixing input channels used
        iNumInChanPlusAddChan = iNumInChan;
    }

    // everything is ok, return empty string for "no error" case
    return "";
}

void CSound::UpdateChSelection()
{
    // calculate the selected input/output buffer and the selected interleaved
    // channel index in the buffer, note that each buffer can have a different
    // number of interleaved channels
    int iChCnt;
    int iSelCHLeft, iSelAddCHLeft;
    int iSelCHRight, iSelAddCHRight;

    // initialize all buffer indexes with an invalid value
    iSelInBufferLeft     = INVALID_INDEX;
    iSelInBufferRight    = INVALID_INDEX;
    iSelAddInBufferLeft  = INVALID_INDEX; // if no additional channel used, this will stay on the invalid value
    iSelAddInBufferRight = INVALID_INDEX; // if no additional channel used, this will stay on the invalid value
    iSelOutBufferLeft    = INVALID_INDEX;
    iSelOutBufferRight   = INVALID_INDEX;

    // input
    GetSelCHAndAddCH ( iSelInputLeftChannel, iNumInChan, iSelCHLeft, iSelAddCHLeft );
    GetSelCHAndAddCH ( iSelInputRightChannel, iNumInChan, iSelCHRight, iSelAddCHRight );

    iChCnt = 0;

    for ( int iBuf = 0; iBuf < vecNumInBufChan.Size(); iBuf++ )
    {
        iChCnt += vecNumInBufChan[iBuf];

        if ( ( iSelInBufferLeft < 0 ) && ( iChCnt > iSelCHLeft ) )
        {
            iSelInBufferLeft   = iBuf;
            iSelInInterlChLeft = iSelCHLeft - iChCnt + vecNumInBufChan[iBuf];
        }

        if ( ( iSelInBufferRight < 0 ) && ( iChCnt > iSelCHRight ) )
        {
            iSelInBufferRight   = iBuf;
            iSelInInterlChRight = iSelCHRight - iChCnt + vecNumInBufChan[iBuf];
        }

        if ( ( iSelAddCHLeft >= 0 ) && ( iSelAddInBufferLeft < 0 ) && ( iChCnt > iSelAddCHLeft ) )
        {
            iSelAddInBufferLeft   = iBuf;
            iSelAddInInterlChLeft = iSelAddCHLeft - iChCnt + vecNumInBufChan[iBuf];
        }

        if ( ( iSelAddCHRight >= 0 ) && ( iSelAddInBufferRight < 0 ) && ( iChCnt > iSelAddCHRight ) )
        {
            iSelAddInBufferRight   = iBuf;
            iSelAddInInterlChRight = iSelAddCHRight - iChCnt + vecNumInBufChan[iBuf];
        }
    }

    // output
    GetSelCHAndAddCH ( iSelOutputLeftChannel, iNumOutChan, iSelCHLeft, iSelAddCHLeft );
    GetSelCHAndAddCH ( iSelOutputRightChannel, iNumOutChan, iSelCHRight, iSelAddCHRight );

    iChCnt = 0;

    for ( int iBuf = 0; iBuf < vecNumOutBufChan.Size(); iBuf++ )
    {
        iChCnt += vecNumOutBufChan[iBuf];

        if ( ( iSelOutBufferLeft < 0 ) && ( iChCnt > iSelCHLeft ) )
        {
            iSelOutBufferLeft   = iBuf;
            iSelOutInterlChLeft = iSelCHLeft - iChCnt + vecNumOutBufChan[iBuf];
        }

        if ( ( iSelOutBufferRight < 0 ) && ( iChCnt > iSelCHRight ) )
        {
            iSelOutBufferRight   = iBuf;
            iSelOutInterlChRight = iSelCHRight - iChCnt + vecNumOutBufChan[iBuf];
        }
    }
}

void CSound::SetLeftInputChannel ( const int iNewChan )
{
    // apply parameter after input parameter check
    if ( ( iNewChan >= 0 ) && ( iNewChan < iNumInChanPlusAddChan ) )
    {
        iSelInputLeftChannel = iNewChan;
        UpdateChSelection();
    }
}

void CSound::SetRightInputChannel ( const int iNewChan )
{
    // apply parameter after input parameter check
    if ( ( iNewChan >= 0 ) && ( iNewChan < iNumInChanPlusAddChan ) )
    {
        iSelInputRightChannel = iNewChan;
        UpdateChSelection();
    }
}

void CSound::SetLeftOutputChannel ( const int iNewChan )
{
    // apply parameter after input parameter check
    if ( ( iNewChan >= 0 ) && ( iNewChan < iNumOutChan ) )
    {
        iSelOutputLeftChannel = iNewChan;
        UpdateChSelection();
    }
}

void CSound::SetRightOutputChannel ( const int iNewChan )
{
    // apply parameter after input parameter check
    if ( ( iNewChan >= 0 ) && ( iNewChan < iNumOutChan ) )
    {
        iSelOutputRightChannel = iNewChan;
        UpdateChSelection();
    }
}

void CSound::Start()
{
    // register the callback function for input and output
    AudioDeviceCreateIOProcID ( audioInputDevice[lCurDev], callbackIO, this, &audioInputProcID );

    AudioDeviceCreateIOProcID ( audioOutputDevice[lCurDev], callbackIO, this, &audioOutputProcID );

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

    // call base class
    CSoundBase::Stop();
}

void CSound::EnableMIDI ( bool bEnable )
{
    if ( bEnable && ( iCtrlMIDIChannel != INVALID_MIDI_CH ) )
    {
        // Create MIDI port if we have valid MIDI channel and no port exists
        if ( midiInPortRef == static_cast<MIDIPortRef> ( NULL ) )
        {
            CreateMIDIPort();
        }
    }
    else
    {
        // Destroy MIDI port if it exists
        if ( midiInPortRef != static_cast<MIDIPortRef> ( NULL ) )
        {
            DestroyMIDIPort();
        }
    }
}

bool CSound::IsMIDIEnabled() const { return ( midiInPortRef != static_cast<MIDIPortRef> ( NULL ) ); }

void CSound::CreateMIDIPort()
{
    if ( midiClient == static_cast<MIDIClientRef> ( NULL ) )
    {
        // Create MIDI client
        OSStatus result = MIDIClientCreate ( CFSTR ( APP_NAME ), NULL, NULL, &midiClient );
        if ( result != noErr )
        {
            qWarning() << "Failed to create CoreAudio MIDI client. Error code:" << result;
            return;
        }
    }

    if ( midiInPortRef == static_cast<MIDIPortRef> ( NULL ) )
    {
        // Create MIDI input port
        OSStatus result = MIDIInputPortCreate ( midiClient, CFSTR ( "Input port" ), callbackMIDI, this, &midiInPortRef );
        if ( result != noErr )
        {
            qWarning() << "Failed to create CoreAudio MIDI input port. Error code:" << result;
            return;
        }

        // Connect to all available MIDI sources
        const int iNMIDISources = MIDIGetNumberOfSources();
        for ( int i = 0; i < iNMIDISources; i++ )
        {
            MIDIEndpointRef src = MIDIGetSource ( i );
            MIDIPortConnectSource ( midiInPortRef, src, NULL );
        }

        qInfo() << "CoreAudio MIDI port created and connected to" << iNMIDISources << "sources";
    }
}

void CSound::DestroyMIDIPort()
{
    if ( midiInPortRef != static_cast<MIDIPortRef> ( NULL ) )
    {
        // Disconnect from all sources before disposing
        const int iNMIDISources = MIDIGetNumberOfSources();
        for ( int i = 0; i < iNMIDISources; i++ )
        {
            MIDIEndpointRef src = MIDIGetSource ( i );
            MIDIPortDisconnectSource ( midiInPortRef, src );
        }

        // Dispose of the MIDI input port
        OSStatus result = MIDIPortDispose ( midiInPortRef );
        if ( result != noErr )
        {
            qWarning() << "Failed to dispose CoreAudio MIDI input port. Error code:" << result;
        }
        midiInPortRef = static_cast<MIDIPortRef> ( NULL );

        qInfo() << "CoreAudio MIDI port destroyed";
    }
}

int CSound::Init ( const int iNewPrefMonoBufferSize )
{
    UInt32 iActualMonoBufferSize;

    // Error message string: in case buffer sizes on input and output cannot be
    // set to the same value
    const QString strErrBufSize = tr ( "The buffer sizes of the current "
                                       "input and output audio device can't be set to a common value. Please "
                                       "select different input/output devices in your system settings." );

    // try to set input buffer size
    iActualMonoBufferSize = SetBufferSize ( audioInputDevice[lCurDev], true, iNewPrefMonoBufferSize );

    if ( iActualMonoBufferSize != static_cast<UInt32> ( iNewPrefMonoBufferSize ) )
    {
        // try to set the input buffer size to the output so that we
        // have a matching pair
        if ( SetBufferSize ( audioOutputDevice[lCurDev], false, iActualMonoBufferSize ) != iActualMonoBufferSize )
        {
            throw CGenErr ( strErrBufSize );
        }
    }
    else
    {
        // try to set output buffer size
        if ( SetBufferSize ( audioOutputDevice[lCurDev], false, iNewPrefMonoBufferSize ) != static_cast<UInt32> ( iNewPrefMonoBufferSize ) )
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

UInt32 CSound::SetBufferSize ( AudioDeviceID& audioDeviceID, const bool bIsInput, UInt32 iPrefBufferSize )
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

    AudioObjectSetPropertyData ( audioDeviceID, &stPropertyAddress, 0, NULL, iSizeBufValue, &iPrefBufferSize );

    // read back which value is actually used
    UInt32 iActualMonoBufferSize = 0;

    AudioObjectGetPropertyData ( audioDeviceID, &stPropertyAddress, 0, NULL, &iSizeBufValue, &iActualMonoBufferSize );

    return iActualMonoBufferSize;
}

OSStatus CSound::deviceNotification ( AudioDeviceID, UInt32, const AudioObjectPropertyAddress* inAddresses, void* inRefCon )
{
    CSound* pSound = static_cast<CSound*> ( inRefCon );

    if ( ( inAddresses->mSelector == kAudioDevicePropertyDeviceHasChanged ) || ( inAddresses->mSelector == kAudioDevicePropertyDeviceIsAlive ) ||
         ( inAddresses->mSelector == kAudioHardwarePropertyDefaultOutputDevice ) ||
         ( inAddresses->mSelector == kAudioHardwarePropertyDefaultInputDevice ) )
    {
        if ( ( inAddresses->mSelector == kAudioDevicePropertyDeviceHasChanged ) || ( inAddresses->mSelector == kAudioDevicePropertyDeviceIsAlive ) )
        {
            // if any property of the device has changed, do a full reload
            pSound->EmitReinitRequestSignal ( RS_RELOAD_RESTART_AND_INIT );
        }
        else
        {
            // for any other change in audio devices, just initiate a restart which
            // triggers an update of the sound device selection combo box
            pSound->EmitReinitRequestSignal ( RS_ONLY_RESTART );
        }
    }

    return noErr;
}

OSStatus CSound::callbackIO ( AudioDeviceID inDevice,
                              const AudioTimeStamp*,
                              const AudioBufferList* inInputData,
                              const AudioTimeStamp*,
                              AudioBufferList* outOutputData,
                              const AudioTimeStamp*,
                              void* inRefCon )
{
    CSound* pSound = static_cast<CSound*> ( inRefCon );

    // both, the input and output device use the same callback function
    QMutexLocker locker ( &pSound->MutexAudioProcessCallback );

    const int           iCoreAudioBufferSizeMono = pSound->iCoreAudioBufferSizeMono;
    const int           iSelInBufferLeft         = pSound->iSelInBufferLeft;
    const int           iSelInBufferRight        = pSound->iSelInBufferRight;
    const int           iSelInInterlChLeft       = pSound->iSelInInterlChLeft;
    const int           iSelInInterlChRight      = pSound->iSelInInterlChRight;
    const int           iSelAddInBufferLeft      = pSound->iSelAddInBufferLeft;
    const int           iSelAddInBufferRight     = pSound->iSelAddInBufferRight;
    const int           iSelAddInInterlChLeft    = pSound->iSelAddInInterlChLeft;
    const int           iSelAddInInterlChRight   = pSound->iSelAddInInterlChRight;
    const int           iSelOutBufferLeft        = pSound->iSelOutBufferLeft;
    const int           iSelOutBufferRight       = pSound->iSelOutBufferRight;
    const int           iSelOutInterlChLeft      = pSound->iSelOutInterlChLeft;
    const int           iSelOutInterlChRight     = pSound->iSelOutInterlChRight;
    const CVector<int>& vecNumInBufChan          = pSound->vecNumInBufChan;
    const CVector<int>& vecNumOutBufChan         = pSound->vecNumOutBufChan;

    if ( ( inDevice == pSound->CurrentAudioInputDeviceID ) && inInputData && pSound->bRun )
    {
        // check sizes (note that float32 has four bytes)
        if ( ( iSelInBufferLeft >= 0 ) && ( iSelInBufferLeft < static_cast<int> ( inInputData->mNumberBuffers ) ) && ( iSelInBufferRight >= 0 ) &&
             ( iSelInBufferRight < static_cast<int> ( inInputData->mNumberBuffers ) ) &&
             ( iSelAddInBufferLeft < static_cast<int> ( inInputData->mNumberBuffers ) ) &&
             ( iSelAddInBufferRight < static_cast<int> ( inInputData->mNumberBuffers ) ) &&
             ( inInputData->mBuffers[iSelInBufferLeft].mDataByteSize ==
               static_cast<UInt32> ( vecNumInBufChan[iSelInBufferLeft] * iCoreAudioBufferSizeMono * 4 ) ) &&
             ( inInputData->mBuffers[iSelInBufferRight].mDataByteSize ==
               static_cast<UInt32> ( vecNumInBufChan[iSelInBufferRight] * iCoreAudioBufferSizeMono * 4 ) ) )
        {
            Float32* pLeftData             = static_cast<Float32*> ( inInputData->mBuffers[iSelInBufferLeft].mData );
            Float32* pRightData            = static_cast<Float32*> ( inInputData->mBuffers[iSelInBufferRight].mData );
            int      iNumChanPerFrameLeft  = vecNumInBufChan[iSelInBufferLeft];
            int      iNumChanPerFrameRight = vecNumInBufChan[iSelInBufferRight];

            // copy input data
            for ( int i = 0; i < iCoreAudioBufferSizeMono; i++ )
            {
                // copy left and right channels separately
                pSound->vecsTmpAudioSndCrdStereo[2 * i]     = (short) ( pLeftData[iNumChanPerFrameLeft * i + iSelInInterlChLeft] * _MAXSHORT );
                pSound->vecsTmpAudioSndCrdStereo[2 * i + 1] = (short) ( pRightData[iNumChanPerFrameRight * i + iSelInInterlChRight] * _MAXSHORT );
            }

            // add an additional optional channel
            if ( iSelAddInBufferLeft >= 0 )
            {
                pLeftData            = static_cast<Float32*> ( inInputData->mBuffers[iSelAddInBufferLeft].mData );
                iNumChanPerFrameLeft = vecNumInBufChan[iSelAddInBufferLeft];

                for ( int i = 0; i < iCoreAudioBufferSizeMono; i++ )
                {
                    pSound->vecsTmpAudioSndCrdStereo[2 * i] = Float2Short ( pSound->vecsTmpAudioSndCrdStereo[2 * i] +
                                                                            pLeftData[iNumChanPerFrameLeft * i + iSelAddInInterlChLeft] * _MAXSHORT );
                }
            }

            if ( iSelAddInBufferRight >= 0 )
            {
                pRightData            = static_cast<Float32*> ( inInputData->mBuffers[iSelAddInBufferRight].mData );
                iNumChanPerFrameRight = vecNumInBufChan[iSelAddInBufferRight];

                for ( int i = 0; i < iCoreAudioBufferSizeMono; i++ )
                {
                    pSound->vecsTmpAudioSndCrdStereo[2 * i + 1] = Float2Short (
                        pSound->vecsTmpAudioSndCrdStereo[2 * i + 1] + pRightData[iNumChanPerFrameRight * i + iSelAddInInterlChRight] * _MAXSHORT );
                }
            }
        }
        else
        {
            // incompatible sizes, clear work buffer
            pSound->vecsTmpAudioSndCrdStereo.Reset ( 0 );
        }

        // call processing callback function
        pSound->ProcessCallback ( pSound->vecsTmpAudioSndCrdStereo );
    }

    if ( ( inDevice == pSound->CurrentAudioOutputDeviceID ) && outOutputData && pSound->bRun )
    {
        // check sizes (note that float32 has four bytes)
        if ( ( iSelOutBufferLeft >= 0 ) && ( iSelOutBufferLeft < static_cast<int> ( outOutputData->mNumberBuffers ) ) &&
             ( iSelOutBufferRight >= 0 ) && ( iSelOutBufferRight < static_cast<int> ( outOutputData->mNumberBuffers ) ) &&
             ( outOutputData->mBuffers[iSelOutBufferLeft].mDataByteSize ==
               static_cast<UInt32> ( vecNumOutBufChan[iSelOutBufferLeft] * iCoreAudioBufferSizeMono * 4 ) ) &&
             ( outOutputData->mBuffers[iSelOutBufferRight].mDataByteSize ==
               static_cast<UInt32> ( vecNumOutBufChan[iSelOutBufferRight] * iCoreAudioBufferSizeMono * 4 ) ) )
        {
            Float32* pLeftData             = static_cast<Float32*> ( outOutputData->mBuffers[iSelOutBufferLeft].mData );
            Float32* pRightData            = static_cast<Float32*> ( outOutputData->mBuffers[iSelOutBufferRight].mData );
            int      iNumChanPerFrameLeft  = vecNumOutBufChan[iSelOutBufferLeft];
            int      iNumChanPerFrameRight = vecNumOutBufChan[iSelOutBufferRight];

            // copy output data
            for ( int i = 0; i < iCoreAudioBufferSizeMono; i++ )
            {
                // copy left and right channels separately
                pLeftData[iNumChanPerFrameLeft * i + iSelOutInterlChLeft]    = (Float32) pSound->vecsTmpAudioSndCrdStereo[2 * i] / _MAXSHORT;
                pRightData[iNumChanPerFrameRight * i + iSelOutInterlChRight] = (Float32) pSound->vecsTmpAudioSndCrdStereo[2 * i + 1] / _MAXSHORT;
            }
        }
    }

    return kAudioHardwareNoError;
}

void CSound::callbackMIDI ( const MIDIPacketList* pktlist, void* refCon, void* )
{
    CSound* pSound = static_cast<CSound*> ( refCon );

    if ( pSound->midiInPortRef != static_cast<MIDIPortRef> ( NULL ) )
    {
        MIDIPacket* midiPacket = const_cast<MIDIPacket*> ( pktlist->packet );

        for ( unsigned int j = 0; j < pktlist->numPackets; j++ )
        {
            // copy packet and send it to the MIDI parser
            CVector<uint8_t> vMIDIPaketBytes ( midiPacket->length );
            for ( int i = 0; i < midiPacket->length; i++ )
            {
                vMIDIPaketBytes[i] = static_cast<uint8_t> ( midiPacket->data[i] );
            }
            pSound->ParseMIDIMessage ( vMIDIPaketBytes );

            midiPacket = MIDIPacketNext ( midiPacket );
        }
    }
}

bool CSound::ConvertCFStringToQString ( const CFStringRef stringRef, QString& sOut )
{
    // check if the string reference is a valid pointer
    if ( stringRef != NULL )
    {
        // first check if the string is not empty
        if ( CFStringGetLength ( stringRef ) > 0 )
        {
            // convert CFString in c-string (quick hack!) and then in QString
            char* sC_strPropValue = (char*) malloc ( CFStringGetLength ( stringRef ) * 3 + 1 );

            if ( CFStringGetCString ( stringRef, sC_strPropValue, CFStringGetLength ( stringRef ) * 3 + 1, kCFStringEncodingUTF8 ) )
            {
                sOut = sC_strPropValue;
                free ( sC_strPropValue );

                return true; // OK
            }
        }

        // release the string reference because it is not needed anymore
        CFRelease ( stringRef );
    }

    return false; // not OK
}
