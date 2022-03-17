/******************************************************************************\
 * Copyright (c) 2004-2022
 *
 * Author(s):
 *  Volker Fischer, revised and maintained by Peter Goderie (pgScorpio)
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

//============================================================================
// Helpers:
//============================================================================

bool ConvertCFStringToQString ( const CFStringRef stringRef, QString& sOut )
{
    bool ok = false;

    sOut.clear();

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
                ok   = true;
            }

            free ( sC_strPropValue );
        }
    }

    return ok;
}

//============================================================================
// Audio Hardware Helpers:
//============================================================================

bool ahGetDefaultDevice ( bool bInput, AudioDeviceID& deviceId )
{
    UInt32                     iPropertySize = sizeof ( AudioDeviceID );
    AudioObjectPropertyAddress stPropertyAddress;

    stPropertyAddress.mSelector = bInput ? kAudioHardwarePropertyDefaultInputDevice : kAudioHardwarePropertyDefaultOutputDevice;
    stPropertyAddress.mScope    = kAudioObjectPropertyScopeGlobal;
    stPropertyAddress.mElement  = kAudioObjectPropertyElementMaster;

    return ( AudioObjectGetPropertyData ( kAudioObjectSystemObject, &stPropertyAddress, 0, NULL, &iPropertySize, &deviceId ) == noErr );
}

bool ahGetAudioDeviceList ( CVector<AudioDeviceID>& vAudioDevices )
{
    UInt32                     iPropertySize = 0;
    AudioObjectPropertyAddress stPropertyAddress;

    stPropertyAddress.mSelector = kAudioHardwarePropertyDevices;
    stPropertyAddress.mScope    = kAudioObjectPropertyScopeGlobal;
    stPropertyAddress.mElement  = kAudioObjectPropertyElementMaster;

    if ( AudioObjectGetPropertyDataSize ( kAudioObjectSystemObject, &stPropertyAddress, 0, NULL, &iPropertySize ) == noErr )
    {
        vAudioDevices.Init ( iPropertySize / sizeof ( AudioDeviceID ) );

        if ( AudioObjectGetPropertyData ( kAudioObjectSystemObject, &stPropertyAddress, 0, NULL, &iPropertySize, &vAudioDevices[0] ) == noErr )
        {
            return true;
        }
    }

    vAudioDevices.clear();
    return false;
}

bool ahGetDeviceStreamIdList ( AudioDeviceID deviceID, bool bInput, CVector<AudioStreamID>& streamIdList )
{
    UInt32                     iPropertySize = 0;
    AudioObjectPropertyAddress stPropertyAddress;

    stPropertyAddress.mSelector = kAudioDevicePropertyStreams;
    stPropertyAddress.mScope    = bInput ? kAudioDevicePropertyScopeInput : kAudioDevicePropertyScopeOutput;
    stPropertyAddress.mElement  = kAudioObjectPropertyElementMaster;

    if ( ( AudioObjectGetPropertyDataSize ( deviceID, &stPropertyAddress, 0, NULL, &iPropertySize ) == noErr ) && ( iPropertySize ) )
    {
        streamIdList.Init ( iPropertySize / sizeof ( AudioStreamID ) );

        if ( AudioObjectGetPropertyData ( deviceID, &stPropertyAddress, 0, NULL, &iPropertySize, &streamIdList[0] ) == noErr )
        {
            return ( streamIdList.Size() > 0 );
        }
    }

    streamIdList.clear();
    return false;
}

bool ahGetStreamFormat ( AudioStreamID streamId, AudioStreamBasicDescription& streamFormat )
{
    UInt32                     iPropertySize;
    AudioObjectPropertyAddress stPropertyAddress;

    stPropertyAddress.mSelector = kAudioStreamPropertyVirtualFormat;
    stPropertyAddress.mScope    = kAudioObjectPropertyScopeGlobal;
    stPropertyAddress.mElement  = kAudioObjectPropertyElementMaster;

    if ( AudioObjectGetPropertyDataSize ( streamId, &stPropertyAddress, 0, NULL, &iPropertySize ) == noErr )
    {
        if ( iPropertySize == sizeof ( AudioStreamBasicDescription ) )
        {
            if ( AudioObjectGetPropertyData ( streamId, &stPropertyAddress, 0, NULL, &iPropertySize, &streamFormat ) == noErr )
            {
                return true;
            }
        }
    }

    memset ( &streamFormat, 0, sizeof ( streamFormat ) );
    return false;
}

bool ahGetStreamLatency ( AudioStreamID streamId, UInt32& iLatencyFrames )
{
    UInt32 iPropertySize = sizeof ( UInt32 );

    AudioObjectPropertyAddress stPropertyAddress;
    stPropertyAddress.mScope    = kAudioObjectPropertyScopeGlobal;
    stPropertyAddress.mElement  = kAudioObjectPropertyElementMaster;
    stPropertyAddress.mSelector = kAudioStreamPropertyLatency;

    // number of frames of latency in the AudioStream
    OSStatus ret = AudioObjectGetPropertyData ( streamId, &stPropertyAddress, 0, NULL, &iPropertySize, &iLatencyFrames );
    if ( ret != noErr )
    {
        iLatencyFrames = 0;
        return false;
    }

    return true;
}

bool ahGetDeviceLatency ( AudioDeviceID deviceId, bool bInput, UInt32& iLatency )
{
    UInt32 deviceLatency = 0;
    UInt32 safetyOffset  = 0;
    UInt32 streamLatency = 0;

    UInt32                     size = sizeof ( deviceLatency );
    AudioObjectPropertyAddress stPropertyAddress;

    iLatency = 0;

    stPropertyAddress.mElement  = kAudioObjectPropertyElementMaster;
    stPropertyAddress.mSelector = kAudioDevicePropertyLatency;
    stPropertyAddress.mScope    = bInput ? kAudioDevicePropertyScopeInput : kAudioDevicePropertyScopeOutput;
    if ( AudioObjectGetPropertyData ( deviceId, &stPropertyAddress, 0, nullptr, &size, &deviceLatency ) == noErr )
    {
        iLatency += deviceLatency;
    }

    size                        = sizeof ( safetyOffset );
    stPropertyAddress.mSelector = kAudioDevicePropertySafetyOffset;
    if ( AudioObjectGetPropertyData ( deviceId, &stPropertyAddress, 0, nullptr, &size, &safetyOffset ) != noErr )
    {
        return false;
    }

    iLatency += safetyOffset;

    // Query stream latency
    stPropertyAddress.mSelector = kAudioDevicePropertyStreams;
    if ( AudioObjectGetPropertyDataSize ( deviceId, &stPropertyAddress, 0, nullptr, &size ) != noErr )
    {
        return false;
    }

    CVector<AudioStreamID> streamList;
    streamList.Init ( size / sizeof ( AudioStreamID ) );

    if ( streamList.Size() == 0 )
    {
        return false;
    }

    if ( AudioObjectGetPropertyData ( deviceId, &stPropertyAddress, 0, nullptr, &size, &streamList[0] ) != noErr )
    {
        return false;
    }

    stPropertyAddress.mSelector = kAudioStreamPropertyLatency;
    size                        = sizeof ( streamLatency );
    // We could check all streams for the device, but it only ever seems to return the stream latency on the first stream
    if ( AudioObjectGetPropertyData ( streamList[0], &stPropertyAddress, 0, nullptr, &size, &streamLatency ) != noErr )
    {
        return false;
    }

    iLatency += streamLatency;

    return true;
}

bool ahGetDeviceBufferList ( AudioDeviceID devID, bool bInput, CVector<AudioBuffer>& vBufferList )
{
    UInt32                     iPropertySize = sizeof ( AudioBufferList );
    AudioObjectPropertyAddress stPropertyAddress;

    stPropertyAddress.mSelector = kAudioDevicePropertyStreamConfiguration;
    stPropertyAddress.mScope    = bInput ? kAudioObjectPropertyScopeInput : kAudioObjectPropertyScopeOutput;
    stPropertyAddress.mElement  = 0;

    vBufferList.clear();
    AudioBufferList* pBufferList = NULL;

    if ( AudioObjectGetPropertyDataSize ( devID, &stPropertyAddress, 0, NULL, &iPropertySize ) == noErr )
    {
        pBufferList = (AudioBufferList*) malloc ( iPropertySize );

        if ( pBufferList && ( AudioObjectGetPropertyData ( devID, &stPropertyAddress, 0, NULL, &iPropertySize, pBufferList ) == noErr ) )
        {
            const unsigned int numBuffers = pBufferList->mNumberBuffers;
            for ( unsigned int i = 0; i < numBuffers; i++ )
            {
                vBufferList.Add ( pBufferList->mBuffers[i] );
            }

            free ( pBufferList );

            return ( vBufferList.Size() > 0 );
        }
    }

    if ( pBufferList )
    {
        free ( pBufferList );
    }

    return false;
}

bool ahGetDeviceName ( AudioDeviceID devId, CFStringRef& stringValue )
{
    UInt32                     iPropertySize = sizeof ( CFStringRef );
    AudioObjectPropertyAddress stPropertyAddress;

    stPropertyAddress.mSelector = kAudioObjectPropertyElementName;
    stPropertyAddress.mScope    = kAudioObjectPropertyScopeGlobal;
    stPropertyAddress.mElement  = kAudioObjectPropertyElementMaster;

    return ( AudioObjectGetPropertyData ( devId, &stPropertyAddress, 0, NULL, &iPropertySize, &stringValue ) == noErr );
}

bool ahGetDeviceChannelName ( AudioDeviceID devID, bool bInput, int chIndex, QString& strName )
{
    bool                       ok            = false;
    UInt32                     iPropertySize = sizeof ( CFStringRef );
    AudioObjectPropertyAddress stPropertyAddress;

    stPropertyAddress.mSelector = kAudioObjectPropertyElementName;
    stPropertyAddress.mScope    = bInput ? kAudioObjectPropertyScopeInput : kAudioObjectPropertyScopeOutput;
    stPropertyAddress.mElement  = chIndex + 1;

    strName.clear();
    CFStringRef cfName = NULL;

    if ( AudioObjectGetPropertyData ( devID, &stPropertyAddress, 0, NULL, &iPropertySize, &cfName ) == noErr )
    {
        ok = ConvertCFStringToQString ( cfName, strName );
        CFRelease ( cfName );
    }

    if ( strName.length() == 0 )
    {
        strName = ( bInput ) ? "In" : "Out";
        strName += " " + QString::number ( stPropertyAddress.mElement );

        return false;
    }

    return ok;
}

bool ahGetDeviceSampleRate ( AudioDeviceID devId, Float64& sampleRate )
{
    UInt32                     iPropertySize = sizeof ( Float64 );
    AudioObjectPropertyAddress stPropertyAddress;

    stPropertyAddress.mSelector = kAudioDevicePropertyNominalSampleRate;
    stPropertyAddress.mScope    = kAudioObjectPropertyScopeGlobal;
    stPropertyAddress.mElement  = kAudioObjectPropertyElementMaster;

    return ( AudioObjectGetPropertyData ( devId, &stPropertyAddress, 0, NULL, &iPropertySize, &sampleRate ) == noErr );
}

bool ahSetDeviceSampleRate ( AudioDeviceID devId, Float64 sampleRate )
{
    UInt32                     iPropertySize = sizeof ( Float64 );
    AudioObjectPropertyAddress stPropertyAddress;

    stPropertyAddress.mSelector = kAudioDevicePropertyNominalSampleRate;
    stPropertyAddress.mScope    = kAudioObjectPropertyScopeGlobal;
    stPropertyAddress.mElement  = kAudioObjectPropertyElementMaster;

    return ( AudioObjectSetPropertyData ( devId, &stPropertyAddress, 0, NULL, iPropertySize, &sampleRate ) == noErr );
}

//============================================================================
// CSound data classes:
//============================================================================

bool CSound::cChannelInfo::IsValid ( bool asAdded )
{
    if ( asAdded )
    {
        // Valid Added Input info ?
        return ( iListIndex < 0 ) || ( ( DeviceId != (AudioDeviceID) 0 ) && ( DeviceId != (AudioDeviceID) -1 ) && ( iBuffer >= 0 ) &&
                                       ( iChanPerFrame > 0 ) && ( iInterlCh >= 0 ) && ( iInterlCh < iChanPerFrame ) );
    }
    else
    {
        // Valid Normal Input or Output info ?
        return ( iListIndex >= 0 ) && ( ( DeviceId != (AudioDeviceID) 0 ) && ( DeviceId != (AudioDeviceID) -1 ) && ( iBuffer >= 0 ) &&
                                        ( iChanPerFrame > 0 ) && ( iInterlCh >= 0 ) && ( iInterlCh < iChanPerFrame ) );
    }
}

bool CSound::cChannelInfo::IsValidFor ( AudioDeviceID forDeviceId, bool asInput, bool asAdded )
{
    if ( asAdded )
    {
        // Valid Added Input info ?
        return ( iListIndex < 0 ) || ( ( DeviceId == forDeviceId ) && ( isInput == asInput ) && ( iBuffer >= 0 ) && ( iChanPerFrame > 0 ) &&
                                       ( iInterlCh >= 0 ) && ( iInterlCh < iChanPerFrame ) );
    }
    else
    {
        // Valid Normal Input or Output info ?
        return ( iListIndex >= 0 ) && ( ( DeviceId == forDeviceId ) && ( isInput == asInput ) && ( iBuffer >= 0 ) && ( iChanPerFrame > 0 ) &&
                                        ( iInterlCh >= 0 ) && ( iInterlCh < iChanPerFrame ) );
    }
}

bool CSound::cDeviceInfo::IsValid()
{
    if ( ( InputDeviceId == 0 ) || ( InputDeviceId == (AudioDeviceID) -1 ) || ( OutputDeviceId == 0 ) || ( OutputDeviceId == (AudioDeviceID) -1 ) ||
         ( lNumInChan <= 0 ) || ( lNumAddedInChan < 0 ) || ( lNumOutChan <= 0 ) )
    {
        return false;
    }

    if ( ( input.Size() != ( lNumInChan + lNumAddedInChan ) ) || ( output.Size() != lNumOutChan ) )
    {
        return false;
    }

    for ( int i = 0; i < lNumInChan; i++ )
    {
        if ( !input[i].IsValidFor ( InputDeviceId, biINPUT, false ) )
        {
            return false;
        }
    }

    for ( int i = 0; i < lNumAddedInChan; i++ )
    {
        if ( !input[lNumInChan + i].IsValidFor ( InputDeviceId, biINPUT, true ) )
        {
            return false;
        }
    }

    for ( int i = 0; i < output.Size(); i++ )
    {
        if ( !output[i].IsValidFor ( OutputDeviceId, biOUTPUT, false ) )
        {
            return false;
        }
    }

    return true;
}

//============================================================================
// CSound class:
//============================================================================

CSound* CSound::pSound = NULL;

CSound::cChannelInfo CSound::noChannelInfo;

CSound::CSound ( void ( *theProcessCallback ) ( CVector<short>& psData, void* arg ), void* theProcessCallbackArg ) :
    CSoundBase ( "CoreAudio", theProcessCallback, theProcessCallbackArg ),
    midiInPortRef ( 0 )
{
    setObjectName ( "CSoundThread" );

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
    //  GetAvailableInOutDevices();

    // Optional MIDI initialization --------------------------------------------
    if ( iCtrlMIDIChannel != INVALID_MIDI_CH )
    {
        // create client and ports
        MIDIClientRef midiClient = 0;
        MIDIClientCreate ( CFSTR ( APP_NAME ), NULL, NULL, &midiClient );
        MIDIInputPortCreate ( midiClient, CFSTR ( "Input port" ), _onMIDI, this, &midiInPortRef );

        // open connections from all sources
        const int iNMIDISources = MIDIGetNumberOfSources();

        for ( int i = 0; i < iNMIDISources; i++ )
        {
            MIDIEndpointRef src = MIDIGetSource ( i );
            MIDIPortConnectSource ( midiInPortRef, src, NULL );
        }
    }

    // set my properties in CSoundbase:
    soundProperties.bHasSetupDialog = false;

    pSound = this;
}

void getAudioDeviceInfos ( int index, const AudioDeviceID DeviceID, QString& strDeviceName, bool& bIsInput, bool& bIsOutput )
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

    strDeviceName.clear();
    // get property name
    CFStringRef sPropertyStringValue = NULL;

    stPropertyAddress.mSelector = kAudioObjectPropertyName;
    stPropertyAddress.mScope    = kAudioObjectPropertyScopeGlobal;
    iPropertySize               = sizeof ( CFStringRef );

    if ( AudioObjectGetPropertyData ( DeviceID, &stPropertyAddress, 0, NULL, &iPropertySize, &sPropertyStringValue ) == noErr )
    {
        // convert string
        ConvertCFStringToQString ( sPropertyStringValue, strDeviceName );

        CFRelease ( sPropertyStringValue );
    }

    if ( strDeviceName.length() == 0 )
    {
        // use a default name in case the conversion did not succeed
        strDeviceName = "Device" + QString::number ( index );
    }
}

bool CSound::setBufferSize ( AudioDeviceID& audioInDeviceID,
                             AudioDeviceID& audioOutDeviceID,
                             unsigned int   iPrefBufferSize,
                             unsigned int&  iActualBufferSize )
{
    bool ok = true;

    UInt32                     iSizeBufValue;
    UInt32                     iRequestedBuffersize = iPrefBufferSize;
    UInt32                     iActualInBufferSize  = 0;
    UInt32                     iActualOutBufferSize = 0;
    AudioObjectPropertyAddress stPropertyAddress;

    stPropertyAddress.mElement  = kAudioObjectPropertyElementMaster;
    stPropertyAddress.mSelector = kAudioDevicePropertyBufferFrameSize;

    if ( audioInDeviceID )
    {
        iSizeBufValue            = sizeof ( UInt32 );
        stPropertyAddress.mScope = kAudioObjectPropertyScopeInput;

        ok &= ( AudioObjectSetPropertyData ( audioInDeviceID, &stPropertyAddress, 0, NULL, iSizeBufValue, &iRequestedBuffersize ) == noErr );
        ok &= ( AudioObjectGetPropertyData ( audioInDeviceID, &stPropertyAddress, 0, NULL, &iSizeBufValue, &iActualInBufferSize ) == noErr );
        ok &= ( iSizeBufValue == sizeof ( UInt32 ) );
    }

    if ( audioOutDeviceID )
    {
        iSizeBufValue            = sizeof ( UInt32 );
        stPropertyAddress.mScope = kAudioObjectPropertyScopeOutput;

        ok &= ( AudioObjectSetPropertyData ( audioOutDeviceID, &stPropertyAddress, 0, NULL, iSizeBufValue, &iRequestedBuffersize ) == noErr );
        ok &= ( AudioObjectGetPropertyData ( audioOutDeviceID, &stPropertyAddress, 0, NULL, &iSizeBufValue, &iActualOutBufferSize ) == noErr );
        ok &= ( iSizeBufValue == sizeof ( UInt32 ) );
    }

    if ( audioInDeviceID && audioOutDeviceID )
    {
        // Return lowest size: (they still might be zero!)
        iActualBufferSize = ( iActualInBufferSize < iActualOutBufferSize ) ? iActualInBufferSize : iActualOutBufferSize;

        return ok && ( iActualBufferSize != 0 ) && ( iActualInBufferSize == iActualOutBufferSize );
    }
    else if ( audioInDeviceID )
    {
        iActualBufferSize = iActualInBufferSize;
        return ok && ( iActualBufferSize != 0 );
    }
    else if ( audioOutDeviceID )
    {
        iActualBufferSize = iActualOutBufferSize;
        return ok && ( iActualBufferSize != 0 );
    }

    iActualBufferSize = 0;
    return false;
}

//============================================================================
// Callbacks:
//============================================================================

OSStatus CSound::onDeviceNotification ( AudioDeviceID, UInt32, const AudioObjectPropertyAddress* inAddresses )
{
    if ( ( inAddresses->mSelector == kAudioDevicePropertyDeviceHasChanged ) || ( inAddresses->mSelector == kAudioDevicePropertyDeviceIsAlive ) ||
         ( inAddresses->mSelector == kAudioHardwarePropertyDefaultOutputDevice ) ||
         ( inAddresses->mSelector == kAudioHardwarePropertyDefaultInputDevice ) )
    {
        if ( ( inAddresses->mSelector == kAudioDevicePropertyDeviceHasChanged ) || ( inAddresses->mSelector == kAudioDevicePropertyDeviceIsAlive ) )
        {
            // if any property of the device has changed, do a full reload
            emit reinitRequest ( tSndCrdResetType::RS_RELOAD_RESTART_AND_INIT );
        }
        else
        {
            // for any other change in audio devices, just initiate a restart which
            // triggers an update of the sound device selection combo box
            emit reinitRequest ( RS_ONLY_RESTART );
        }
    }

    return noErr;
}

OSStatus CSound::onBufferSwitch ( AudioDeviceID deviceID,
                                  const AudioTimeStamp*,
                                  const AudioBufferList* inInputData,
                                  const AudioTimeStamp*,
                                  AudioBufferList* outOutputData,
                                  const AudioTimeStamp* )
{
    // both, the input and output device use the same callback function
    QMutexLocker locker ( &mutexAudioProcessCallback );
    static bool  awaitIn = true;

    if ( IsStarted() )
    {
        if ( awaitIn && inInputData && ( deviceID == selectedDevice.InputDeviceId ) )
        {
            int iSelBufferLeft       = selectedDevice.InLeft.iBuffer;
            int iSelChanPerFrameLeft = selectedDevice.InLeft.iChanPerFrame;
            int iSelInterlChLeft     = selectedDevice.InLeft.iInterlCh;

            int iSelBufferRight       = selectedDevice.InRight.iBuffer;
            int iSelChanPerFrameRight = selectedDevice.InRight.iChanPerFrame;
            int iSelInterlChRight     = selectedDevice.InRight.iInterlCh;

            int iAddBufferLeft       = selectedDevice.AddInLeft.iBuffer;
            int iAddChanPerFrameLeft = selectedDevice.AddInLeft.iChanPerFrame;
            int iAddInterlChLeft     = selectedDevice.AddInLeft.iInterlCh;

            int iAddBufferRight       = selectedDevice.AddInRight.iBuffer;
            int iAddChanPerFrameRight = selectedDevice.AddInRight.iChanPerFrame;
            int iAddInterlChRight     = selectedDevice.AddInRight.iInterlCh;

            Float32* pLeftData  = static_cast<Float32*> ( inInputData->mBuffers[iSelBufferLeft].mData );
            Float32* pRightData = static_cast<Float32*> ( inInputData->mBuffers[iSelBufferRight].mData );

            // copy input data
            for ( unsigned int i = 0; i < iDeviceBufferSize; i++ )
            {
                // copy left and right channels separately
                audioBuffer[2 * i]     = static_cast<int16_t> ( pLeftData[iSelChanPerFrameLeft * i + iSelInterlChLeft] * _MAXSHORT );
                audioBuffer[2 * i + 1] = static_cast<int16_t> ( pRightData[iSelChanPerFrameRight * i + iSelInterlChRight] * _MAXSHORT );
            }

            // add an additional optional channel
            if ( iAddBufferLeft >= 0 )
            {
                pLeftData = static_cast<Float32*> ( inInputData->mBuffers[iAddBufferLeft].mData );

                for ( unsigned int i = 0; i < iDeviceBufferSize; i++ )
                {
                    audioBuffer[2 * i] += static_cast<int16_t> ( pLeftData[iAddChanPerFrameLeft * i + iAddInterlChLeft] * _MAXSHORT );
                }
            }

            if ( iAddBufferRight >= 0 )
            {
                pRightData = static_cast<Float32*> ( inInputData->mBuffers[iAddBufferRight].mData );

                for ( unsigned int i = 0; i < iDeviceBufferSize; i++ )
                {
                    audioBuffer[2 * i + 1] += static_cast<int16_t> ( pRightData[iAddChanPerFrameRight * i + iAddInterlChRight] * _MAXSHORT );
                }
            }

            // call processing callback function
            processCallback ( audioBuffer );
            awaitIn = false;
        }

        if ( !awaitIn && outOutputData && ( deviceID == selectedDevice.OutputDeviceId ) )
        {
            int iSelBufferLeft       = selectedDevice.OutLeft.iBuffer;
            int iSelChanPerFrameLeft = selectedDevice.OutLeft.iChanPerFrame;
            int iSelInterlChLeft     = selectedDevice.OutLeft.iInterlCh;

            int iSelBufferRight       = selectedDevice.OutRight.iBuffer;
            int iSelChanPerFrameRight = selectedDevice.OutRight.iChanPerFrame;
            int iSelInterlChRight     = selectedDevice.OutRight.iInterlCh;

            Float32* pLeftData  = static_cast<Float32*> ( outOutputData->mBuffers[iSelBufferLeft].mData );
            Float32* pRightData = static_cast<Float32*> ( outOutputData->mBuffers[iSelBufferRight].mData );

            // copy output data
            for ( unsigned int i = 0; i < iDeviceBufferSize; i++ )
            {
                // copy left and right channels separately
                pLeftData[iSelChanPerFrameLeft * i + iSelInterlChLeft]    = (Float32) audioBuffer[2 * i] / _MAXSHORT;
                pRightData[iSelChanPerFrameRight * i + iSelInterlChRight] = (Float32) audioBuffer[2 * i + 1] / _MAXSHORT;
            }
            awaitIn = true;
        }
    }
    else
    {
        awaitIn = true;
    }

    return kAudioHardwareNoError;
}

void CSound::onMIDI ( const MIDIPacketList* pktlist )
{
    if ( midiInPortRef )
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

            parseMIDIMessage ( vMIDIPaketBytes );

            midiPacket = MIDIPacketNext ( midiPacket );
        }
    }
}

void CSound::unregisterDeviceCallBacks()
{
    if ( selectedDevice.IsActive() )
    {
        AudioObjectPropertyAddress stPropertyAddress;

        stPropertyAddress.mElement = kAudioObjectPropertyElementMaster;
        stPropertyAddress.mScope   = kAudioObjectPropertyScopeGlobal;

        stPropertyAddress.mSelector = kAudioHardwarePropertyDefaultOutputDevice;
        AudioObjectRemovePropertyListener ( kAudioObjectSystemObject, &stPropertyAddress, _onDeviceNotification, this );

        stPropertyAddress.mSelector = kAudioHardwarePropertyDefaultInputDevice;
        AudioObjectRemovePropertyListener ( kAudioObjectSystemObject, &stPropertyAddress, _onDeviceNotification, this );

        stPropertyAddress.mSelector = kAudioDevicePropertyDeviceHasChanged;
        AudioObjectRemovePropertyListener ( selectedDevice.InputDeviceId, &stPropertyAddress, _onDeviceNotification, this );
        AudioObjectRemovePropertyListener ( selectedDevice.InputDeviceId, &stPropertyAddress, _onDeviceNotification, this );

        stPropertyAddress.mSelector = kAudioDevicePropertyDeviceIsAlive;
        AudioObjectRemovePropertyListener ( selectedDevice.OutputDeviceId, &stPropertyAddress, _onDeviceNotification, this );
        AudioObjectRemovePropertyListener ( selectedDevice.OutputDeviceId, &stPropertyAddress, _onDeviceNotification, this );
    }
}

void CSound::registerDeviceCallBacks()
{
    if ( selectedDevice.IsActive() )
    {
        AudioObjectPropertyAddress stPropertyAddress;

        // register callbacks
        stPropertyAddress.mElement = kAudioObjectPropertyElementMaster;
        stPropertyAddress.mScope   = kAudioObjectPropertyScopeGlobal;

        // setup callbacks for device property changes
        stPropertyAddress.mSelector = kAudioDevicePropertyDeviceHasChanged;
        AudioObjectAddPropertyListener ( selectedDevice.InputDeviceId, &stPropertyAddress, _onDeviceNotification, this );
        AudioObjectAddPropertyListener ( selectedDevice.OutputDeviceId, &stPropertyAddress, _onDeviceNotification, this );

        stPropertyAddress.mSelector = kAudioDevicePropertyDeviceIsAlive;
        AudioObjectAddPropertyListener ( selectedDevice.InputDeviceId, &stPropertyAddress, _onDeviceNotification, this );
        AudioObjectAddPropertyListener ( selectedDevice.OutputDeviceId, &stPropertyAddress, _onDeviceNotification, this );

        stPropertyAddress.mSelector = kAudioHardwarePropertyDefaultOutputDevice;
        AudioObjectAddPropertyListener ( kAudioObjectSystemObject, &stPropertyAddress, _onDeviceNotification, this );

        stPropertyAddress.mSelector = kAudioHardwarePropertyDefaultInputDevice;
        AudioObjectAddPropertyListener ( kAudioObjectSystemObject, &stPropertyAddress, _onDeviceNotification, this );
    }
}

//============================================================================
// Channel Info & Selection:
//============================================================================

bool CSound::setSelectedDevice()
{
    bool ok = true;

    unregisterDeviceCallBacks();

    selectedDevice.clear();

    // Get Channel numbers:
    int iInLeft;
    int iAddLeft;
    getInputSelAndAddChannels ( selectedInputChannels[0], lNumInChan, lNumAddedInChan, iInLeft, iAddLeft );

    int iInRight;
    int iAddRight;
    getInputSelAndAddChannels ( selectedInputChannels[1], lNumInChan, lNumAddedInChan, iInRight, iAddRight );

    int iOutLeft  = selectedOutputChannels[0];
    int iOutRight = selectedOutputChannels[1];

    selectedDevice.InputDeviceId  = device[iCurrentDevice].InputDeviceId;
    selectedDevice.OutputDeviceId = device[iCurrentDevice].OutputDeviceId;
    ok &= ( selectedDevice.InputDeviceId != 0 ) && ( selectedDevice.OutputDeviceId != 0 );

    // Get Channel Ids
    if ( device[iCurrentDevice].input[iInLeft].IsValid ( false ) )
    {
        selectedDevice.InLeft = device[iCurrentDevice].input[iInLeft];
    }
    else
    {
        selectedDevice.InLeft = noChannelInfo;
        ok                    = false;
    }

    if ( device[iCurrentDevice].input[iInRight].IsValid ( false ) )
    {
        selectedDevice.InRight = device[iCurrentDevice].input[iInRight];
    }
    else
    {
        selectedDevice.InRight = noChannelInfo;
        ok                     = false;
    }

    if ( iAddLeft >= 0 )
    {
        if ( device[iCurrentDevice].input[iAddLeft].IsValid ( true ) )
        {
            selectedDevice.AddInLeft = device[iCurrentDevice].input[iAddLeft];
        }
        else
        {
            selectedDevice.AddInLeft = noChannelInfo;
            ok                       = false;
        }
    }
    else
    {
        selectedDevice.AddInLeft = noChannelInfo;
    }

    if ( iAddRight >= 0 )
    {
        if ( device[iCurrentDevice].input[iAddRight].IsValid ( true ) )
        {
            selectedDevice.AddInRight = device[iCurrentDevice].input[iAddRight];
        }
        else
        {
            selectedDevice.AddInRight = noChannelInfo;
            ok                        = false;
        }
    }
    else
    {
        selectedDevice.AddInRight = noChannelInfo;
    }

    if ( device[iCurrentDevice].output[iOutLeft].IsValid ( false ) )
    {
        selectedDevice.OutLeft = device[iCurrentDevice].output[iOutLeft];
    }
    else
    {
        selectedDevice.OutLeft = noChannelInfo;
        ok                     = false;
    }

    if ( device[iCurrentDevice].output[iOutRight].IsValid ( false ) )
    {
        selectedDevice.OutRight = device[iCurrentDevice].output[iOutRight];
    }
    else
    {
        selectedDevice.OutRight = noChannelInfo;
        ok                      = false;
    }

    if ( ok )
    {
        selectedDevice.Activated = true;
        registerDeviceCallBacks();
    }

    return ok;
}

void CSound::setBaseChannelInfo ( cDeviceInfo& deviceSelection )
{
    const int NumInputChannels  = deviceSelection.input.Size();
    const int NumOutputChannels = deviceSelection.output.Size();

    strInputChannelNames.clear();
    for ( int i = 0; i < NumInputChannels; i++ )
    {
        strInputChannelNames.append ( deviceSelection.input[i].strName );
    }

    strOutputChannelNames.clear();
    for ( int i = 0; i < NumOutputChannels; i++ )
    {
        strOutputChannelNames.append ( deviceSelection.output[i].strName );
    }

    lNumInChan      = deviceSelection.lNumInChan;
    lNumAddedInChan = deviceSelection.lNumAddedInChan;
    lNumOutChan     = deviceSelection.lNumOutChan;
}

bool CSound::getDeviceChannelInfo ( cDeviceInfo& deviceInfo, bool bInput )
{
    CSound::cChannelInfo channelInfo;

    AudioDeviceID                  DeviceId     = bInput ? deviceInfo.InputDeviceId : deviceInfo.OutputDeviceId;
    CVector<CSound::cChannelInfo>& vChannelInfo = bInput ? deviceInfo.input : deviceInfo.output;

    vChannelInfo.clear();

    channelInfo.isInput  = bInput;
    channelInfo.DeviceId = DeviceId;
    // Get Primary channels:
    {
        CVector<AudioBuffer> vBufferList;

        if ( ahGetDeviceBufferList ( DeviceId, bInput, vBufferList ) )
        {
            int numBuffers = vBufferList.Size();
            int numChannels;

            for ( int i = 0; i < numBuffers; i++ )
            {
                channelInfo.iBuffer       = i;
                channelInfo.iChanPerFrame = numChannels = vBufferList[i].mNumberChannels;

                for ( int c = 0; c < numChannels; c++ )
                {
                    channelInfo.iInterlCh = c;

                    channelInfo.iListIndex = vChannelInfo.Size();
                    ahGetDeviceChannelName ( DeviceId, bInput, channelInfo.iListIndex, channelInfo.strName );
                    vChannelInfo.Add ( channelInfo );
                }
            }
        }
    }

    if ( bInput )
    {
        long numInChan = deviceInfo.lNumInChan = vChannelInfo.Size();
        long ChannelsToAdd                     = getNumInputChannelsToAdd ( numInChan );

        int iSelChan, iAddChan;

        for ( int i = 0; i < ChannelsToAdd; i++ )
        {
            getInputSelAndAddChannels ( vChannelInfo.Size(), numInChan, ChannelsToAdd, iSelChan, iAddChan );
            channelInfo            = vChannelInfo[iAddChan];
            channelInfo.iListIndex = vChannelInfo.Size();
            channelInfo.strName    = vChannelInfo[iSelChan].strName + " + " + vChannelInfo[iAddChan].strName;
            vChannelInfo.Add ( channelInfo );
        }

        deviceInfo.lNumAddedInChan = ChannelsToAdd;
    }
    else
    {
        deviceInfo.lNumOutChan = vChannelInfo.Size();
    }

    return ( vChannelInfo.Size() > 0 );
}

bool StreamformatOk ( AudioStreamBasicDescription& streamDescription )
{
    return ( ( streamDescription.mFormatID == kAudioFormatLinearPCM ) && ( streamDescription.mFramesPerPacket == 1 ) &&
             ( streamDescription.mBitsPerChannel == 32 ) && ( streamDescription.mFormatFlags & kAudioFormatFlagIsFloat ) &&
             ( streamDescription.mFormatFlags & kAudioFormatFlagIsPacked ) );
}

bool CSound::CheckDeviceCapabilities ( cDeviceInfo& deviceSelection )
{
    bool                        ok                = true;
    bool                        inputDeviceOk     = true;
    bool                        outputDeviceOk    = true;
    const Float64               fSystemSampleRate = static_cast<Float64> ( SYSTEM_SAMPLE_RATE_HZ );
    Float64                     inputSampleRate   = 0;
    Float64                     outputSampleRate  = 0;
    AudioStreamBasicDescription CurDevStreamFormat;
    CVector<AudioStreamID>      vInputStreamIDList;
    CVector<AudioStreamID>      vOutputStreamIDList;

    if ( !ahGetDeviceSampleRate ( deviceSelection.InputDeviceId, inputSampleRate ) )
    {
        strErrorList.append (
            tr ( "The audio %1 device is no longer available. Please check if your %1 device is connected correctly." ).arg ( "input" ) );
        ok = inputDeviceOk = false;
    }

    if ( ( inputDeviceOk ) && ( inputSampleRate != fSystemSampleRate ) )
    {
        // try to change the sample rate
        if ( !ahSetDeviceSampleRate ( deviceSelection.InputDeviceId, fSystemSampleRate ) )
        {
            strErrorList.append ( tr ( "The sample rate on the current %1 device isn't %2 Hz and is therefore incompatible. " )
                                      .arg ( "input" )
                                      .arg ( SYSTEM_SAMPLE_RATE_HZ ) +
                                  tr ( "Try setting the sample rate to % 1 Hz manually via Audio-MIDI-Setup (in Applications->Utilities)." )
                                      .arg ( SYSTEM_SAMPLE_RATE_HZ ) );
            ok = false;
        }
    }

    if ( !ahGetDeviceSampleRate ( deviceSelection.OutputDeviceId, outputSampleRate ) )
    {
        strErrorList.append (
            tr ( "The audio %1 device is no longer available. Please check if your %1 device is connected correctly." ).arg ( "output" ) );
        ok = outputDeviceOk = false;
    }

    if ( outputDeviceOk && ( outputSampleRate != fSystemSampleRate ) )
    {
        // try to change the sample rate
        if ( !ahSetDeviceSampleRate ( deviceSelection.OutputDeviceId, fSystemSampleRate ) )
        {
            strErrorList.append ( tr ( "The sample rate on the current %1 device isn't %2 Hz and is therefore incompatible. " )
                                      .arg ( "output" )
                                      .arg ( SYSTEM_SAMPLE_RATE_HZ ) +
                                  tr ( "Try setting the sample rate to %1 Hz manually via Audio-MIDI-Setup (in Applications->Utilities)." )
                                      .arg ( SYSTEM_SAMPLE_RATE_HZ ) );
            ok = false;
        }
    }

    if ( inputDeviceOk )
    {
        if ( !ahGetDeviceStreamIdList ( deviceSelection.InputDeviceId, biINPUT, vInputStreamIDList ) )
        {
            strErrorList.append ( tr ( "The input device doesn't have any valid inputs." ) );
            ok = inputDeviceOk = false;
        }
    }

    if ( outputDeviceOk )
    {
        // get the stream ID of the output device (at least one stream must always exist)
        if ( !ahGetDeviceStreamIdList ( deviceSelection.OutputDeviceId, biOUTPUT, vOutputStreamIDList ) )
        {
            strErrorList.append ( tr ( "The output device doesn't have any valid outputs." ) );
            ok = outputDeviceOk = false;
        }
    }

    if ( ok )
    {
        ok = setBufferSize ( deviceSelection.InputDeviceId, deviceSelection.OutputDeviceId, iProtocolBufferSize, iDeviceBufferSize );
        if ( ok )
        {
            // According to the AudioHardware documentation: "If the format is a linear PCM
            // format, the data will always be presented as 32 bit, native endian floating
            // point. All conversions to and from the true physical format of the hardware
            // is handled by the devices driver.".
            // check the input
            if ( !ahGetStreamFormat ( vInputStreamIDList[0], CurDevStreamFormat ) || // pgScorpio: this one fails !
                 !StreamformatOk ( CurDevStreamFormat ) )
            {
                strErrorList.append ( tr ( "The stream format on the current input device isn't compatible with %1." ).arg ( APP_NAME ) );
                ok = inputDeviceOk = false;
            }

            if ( !ahGetStreamFormat ( vOutputStreamIDList[0], CurDevStreamFormat ) || !StreamformatOk ( CurDevStreamFormat ) )
            {
                strErrorList.append ( tr ( "The stream format on the current output device isn't compatible with %1." ).arg ( APP_NAME ) );
                ok = outputDeviceOk = false;
            }
        }
    }

    return ok;
}

//============================================================================
// CSoundBase virtuals:
//============================================================================

long CSound::createDeviceList ( bool bRescan ) // Fills strDeviceList. Returns number of devices found
{
    if ( bRescan || ( lNumDevices == 0 ) || ( strDeviceNames.size() != lNumDevices ) || ( device.Size() != lNumDevices ) )
    {
        CVector<AudioDeviceID> vAudioDevices;
        cDeviceInfo            newDevice;

        clearDeviceInfo(); // We will have no more device to select...

        lNumDevices = 0;
        strDeviceNames.clear();
        device.clear();

        if ( !ahGetAudioDeviceList ( vAudioDevices ) )
        {
            strErrorList.append ( tr ( "No audiodevices found." ) );
            return 0;
        }

        const UInt32 iDeviceCount = vAudioDevices.Size();

        // always add system default devices for input and output as first entry
        if ( !( ahGetDefaultDevice ( biINPUT, newDevice.InputDeviceId ) && ahGetDefaultDevice ( biOUTPUT, newDevice.OutputDeviceId ) ) )
        {
            strErrorList.append ( tr ( "No sound card is available in your system." ) );
            return 0;
        }
        getDeviceChannelInfo ( newDevice, biINPUT );
        getDeviceChannelInfo ( newDevice, biOUTPUT );
        device.Add ( newDevice );
        strDeviceNames.append ( tr ( "System Default In/Out Devices" ) );

        QString strDeviceName_i;
        QString strDeviceName_j;
        bool    bIsInput_i;
        bool    bIsOutput_i;
        bool    bIsInput_j;
        bool    bIsOutput_j;

        // First get in + out devices...
        for ( UInt32 i = 0; i < iDeviceCount; i++ )
        {
            getAudioDeviceInfos ( i, vAudioDevices[i], strDeviceName_i, bIsInput_i, bIsOutput_i );
            if ( bIsInput_i && bIsOutput_i )
            {
                newDevice.InputDeviceId  = vAudioDevices[i];
                newDevice.OutputDeviceId = vAudioDevices[i];
                getDeviceChannelInfo ( newDevice, biINPUT );
                getDeviceChannelInfo ( newDevice, biOUTPUT );
                device.Add ( newDevice );
                strDeviceNames.append ( strDeviceName_i );
            }
        }

        // Then find all other in / out combinations...
        for ( UInt32 i = 0; i < iDeviceCount; i++ )
        {
            getAudioDeviceInfos ( i, vAudioDevices[i], strDeviceName_i, bIsInput_i, bIsOutput_i );

            if ( bIsInput_i )
            {
                newDevice.InputDeviceId = vAudioDevices[i];
                getDeviceChannelInfo ( newDevice, biINPUT );

                for ( UInt32 j = 0; j < iDeviceCount; j++ )
                {
                    // Skip primary in + out devices, we already got those...
                    if ( i != j )
                    {
                        getAudioDeviceInfos ( j, vAudioDevices[j], strDeviceName_j, bIsInput_j, bIsOutput_j );

                        if ( bIsOutput_j )
                        {
                            newDevice.OutputDeviceId = vAudioDevices[j];
                            getDeviceChannelInfo ( newDevice, biOUTPUT );
                            device.Add ( newDevice );
                            strDeviceNames.append ( strDeviceName_i + " / " + strDeviceName_j );
                        }
                    }
                }
            }
        }

        lNumDevices = strDeviceNames.length();
    }

    return lNumDevices;
}

bool CSound::getCurrentLatency()
{
    if ( !selectedDevice.IsActive() )
    {
        return false;
    }

    bool                   ok = true;
    CVector<AudioStreamID> streamIdList;
    UInt32                 iInputLatencyFrames  = 0;
    UInt32                 iOutputLatencyFrames = 0;

    if ( !ahGetDeviceLatency ( selectedDevice.InputDeviceId, biINPUT, iInputLatencyFrames ) )
    {
        ok                  = false;
        iInputLatencyFrames = iDeviceBufferSize;
    }

    if ( !ahGetDeviceLatency ( selectedDevice.OutputDeviceId, biOUTPUT, iOutputLatencyFrames ) )
    {
        ok                   = false;
        iOutputLatencyFrames = iDeviceBufferSize;
    }

    iInputLatencyFrames += iDeviceBufferSize;
    iOutputLatencyFrames += iDeviceBufferSize;

    fInOutLatencyMs = ( ( static_cast<float> ( iInputLatencyFrames ) + iOutputLatencyFrames ) * 1000 ) / SYSTEM_SAMPLE_RATE_HZ;

    return ok;
}

bool CSound::checkDeviceChange ( CSoundBase::tDeviceChangeCheck mode, int iDriverIndex ) // Open device sequence handling....
{
    if ( ( iDriverIndex < 0 ) || ( iDriverIndex >= lNumDevices ) )
    {
        return false;
    }

    switch ( mode )
    {
    case CSoundBase::tDeviceChangeCheck::CheckOpen:
        return device[iDriverIndex].IsValid();

    case CSoundBase::tDeviceChangeCheck::CheckCapabilities:
        return CheckDeviceCapabilities ( device[iDriverIndex] );

    case CSoundBase::tDeviceChangeCheck::Activate:
        clearDeviceInfo();
        setBaseChannelInfo ( device[iDriverIndex] );
        resetChannelMapping();

        fInOutLatencyMs = 0.0;
        iCurrentDevice  = iDriverIndex;

        // TODO: ChannelMapping from inifile!

        return setSelectedDevice();

    case CSoundBase::tDeviceChangeCheck::Abort:
        return true;

    default:
        return false;
    }
}

unsigned int CSound::getDeviceBufferSize ( unsigned int iDesiredBufferSize )
{
    CSoundStopper sound ( *this );

    if ( selectedDevice.IsActive() )
    {
        unsigned int iActualBufferSize;

        if ( setBufferSize ( selectedDevice.InputDeviceId, selectedDevice.OutputDeviceId, iDesiredBufferSize, iActualBufferSize ) )
        {
            return iActualBufferSize;
        }
    }

    return 0;
}

void CSound::closeCurrentDevice() // Closes the current driver and Clears Device Info
{
    // Can't close current device while running
    Stop();
    // Clear CSoundBase device info
    clearDeviceInfo();
    // Clear my selected device
    selectedDevice.clear();
}

//============================================================================
// Start/Stop:
//============================================================================

bool CSound::prepareAudio ( bool start )
{
    if ( !selectedDevice.IsActive() || ( iDeviceBufferSize == 0 ) )
    {
        return false;
    }

    // create memory for intermediate audio buffer
    audioBuffer.Init ( PROT_NUM_IN_CHANNELS * iDeviceBufferSize );

    //    selectedDevice.InputIdLeft  = inputInfo[selectedInputChannels[0]].deviceID;  //?????
    //    selectedDevice.OutputIdLeft = inputInfo[selectedOutputChannels[0]].deviceID; //?????

    // register the callback function for input and output
    bool ok = ( AudioDeviceCreateIOProcID ( selectedDevice.InputDeviceId, _onBufferSwitch, this, &audioInputProcID ) == noErr );
    if ( ok )
    {
        ok = ( AudioDeviceCreateIOProcID ( selectedDevice.OutputDeviceId, _onBufferSwitch, this, &audioOutputProcID ) == noErr );

        if ( ok && audioInputProcID && audioOutputProcID && start )
        {
            // start the audio stream
            ok = ( AudioDeviceStart ( selectedDevice.InputDeviceId, audioInputProcID ) == noErr );
            if ( ok )
            {
                ok = ( AudioDeviceStart ( selectedDevice.OutputDeviceId, audioOutputProcID ) == noErr );
                if ( ok )
                {
                    getCurrentLatency();

                    return true;
                }

                AudioDeviceStop ( selectedDevice.InputDeviceId, audioInputProcID );
            }
        }
    }

    if ( !ok )
    {
        if ( audioInputProcID )
        {
            AudioDeviceDestroyIOProcID ( selectedDevice.InputDeviceId, audioInputProcID );
            audioInputProcID = NULL;
        }
        if ( audioOutputProcID )
        {
            AudioDeviceDestroyIOProcID ( selectedDevice.OutputDeviceId, audioOutputProcID );
            audioOutputProcID = NULL;
        }
    }

    return ok;
}

bool CSound::start()
{
    strErrorList.clear();

    if ( !IsStarted() )
    {
        if ( prepareAudio ( true ) )
        {
            return true;
        }
        else
        {
            // We failed!, audio is stopped !
            strErrorList.append ( htmlBold ( tr ( "Failed to start your audio device!." ) ) );
            strErrorList.append ( tr ( "Please check your device settings..." ) );
        }
    }

    return false;
}

bool CSound::stop()
{
    strErrorList.clear();

    if ( IsStarted() )
    {

        // stop the audio stream
        AudioDeviceStop ( selectedDevice.InputDeviceId, audioInputProcID );
        AudioDeviceStop ( selectedDevice.OutputDeviceId, audioOutputProcID );

        // unregister the callback function for input and output
        AudioDeviceDestroyIOProcID ( selectedDevice.InputDeviceId, audioInputProcID );
        AudioDeviceDestroyIOProcID ( selectedDevice.OutputDeviceId, audioOutputProcID );
    }

    return true;
}
