/******************************************************************************\
 * Copyright (c) 2004-2022
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

#pragma once

#include <CoreAudio/CoreAudio.h>
#include <AudioToolbox/AudioToolbox.h>
#include <CoreMIDI/CoreMIDI.h>
#include <QMutex>
#include <QMessageBox>
#include "../soundbase.h"
#include "../../global.h"

/* Classes ********************************************************************/
class CSound : public CSoundBase
{
    Q_OBJECT

public:
    CSound ( void ( *fpNewProcessCallback ) ( CVector<short>& psData, void* arg ),
             void*          arg,
             const QString& strMIDISetup,
             const bool,
             const QString& );

    virtual int  Init ( const int iNewPrefMonoBufferSize );
    virtual void Start();
    virtual void Stop();

    // channel selection
    virtual int     GetNumInputChannels() { return iNumInChanPlusAddChan; }
    virtual QString GetInputChannelName ( const int iDiD ) { return sChannelNamesInput[iDiD]; }
    virtual void    SetLeftInputChannel ( const int iNewChan );
    virtual void    SetRightInputChannel ( const int iNewChan );
    virtual int     GetLeftInputChannel() { return iSelInputLeftChannel; }
    virtual int     GetRightInputChannel() { return iSelInputRightChannel; }

    virtual int     GetNumOutputChannels() { return iNumOutChan; }
    virtual QString GetOutputChannelName ( const int iDiD ) { return sChannelNamesOutput[iDiD]; }
    virtual void    SetLeftOutputChannel ( const int iNewChan );
    virtual void    SetRightOutputChannel ( const int iNewChan );
    virtual int     GetLeftOutputChannel() { return iSelOutputLeftChannel; }
    virtual int     GetRightOutputChannel() { return iSelOutputRightChannel; }

    // these variables/functions should be protected but cannot since we want
    // to access them from the callback function
    CVector<short> vecsTmpAudioSndCrdStereo;
    int            iCoreAudioBufferSizeMono;
    int            iCoreAudioBufferSizeStereo;
    AudioDeviceID  CurrentAudioInputDeviceID;
    AudioDeviceID  CurrentAudioOutputDeviceID;
    long           lCurDev;
    int            iNumInChan;
    int            iNumInChanPlusAddChan; // includes additional "added" channels
    int            iNumOutChan;
    int            iSelInputLeftChannel;
    int            iSelInputRightChannel;
    int            iSelOutputLeftChannel;
    int            iSelOutputRightChannel;
    int            iSelInBufferLeft;
    int            iSelInBufferRight;
    int            iSelInInterlChLeft;
    int            iSelInInterlChRight;
    int            iSelAddInBufferLeft;
    int            iSelAddInBufferRight;
    int            iSelAddInInterlChLeft;
    int            iSelAddInInterlChRight;
    int            iSelOutBufferLeft;
    int            iSelOutBufferRight;
    int            iSelOutInterlChLeft;
    int            iSelOutInterlChRight;
    CVector<int>   vecNumInBufChan;
    CVector<int>   vecNumOutBufChan;

protected:
    virtual QString LoadAndInitializeDriver ( QString strDriverName, bool );

    QString CheckDeviceCapabilities ( const int iDriverIdx );
    void    UpdateChSelection();
    void    GetAvailableInOutDevices();

    int CountChannels ( AudioDeviceID devID, bool isInput );

    UInt32 SetBufferSize ( AudioDeviceID& audioDeviceID, const bool bIsInput, UInt32 iPrefBufferSize );

    void GetAudioDeviceInfos ( const AudioDeviceID DeviceID, QString& strDeviceName, bool& bIsInput, bool& bIsOutput );

    bool ConvertCFStringToQString ( const CFStringRef stringRef, QString& sOut );

    // callbacks
    static OSStatus deviceNotification ( AudioDeviceID, UInt32, const AudioObjectPropertyAddress* inAddresses, void* inRefCon );

    static OSStatus callbackIO ( AudioDeviceID inDevice,
                                 const AudioTimeStamp*,
                                 const AudioBufferList* inInputData,
                                 const AudioTimeStamp*,
                                 AudioBufferList* outOutputData,
                                 const AudioTimeStamp*,
                                 void* inRefCon );

    static void callbackMIDI ( const MIDIPacketList* pktlist, void* refCon, void* );

    AudioDeviceID       audioInputDevice[MAX_NUMBER_SOUND_CARDS];
    AudioDeviceID       audioOutputDevice[MAX_NUMBER_SOUND_CARDS];
    AudioDeviceIOProcID audioInputProcID;
    AudioDeviceIOProcID audioOutputProcID;

    MIDIPortRef midiInPortRef;

    QString sChannelNamesInput[MAX_NUM_IN_OUT_CHANNELS];
    QString sChannelNamesOutput[MAX_NUM_IN_OUT_CHANNELS];

    QMutex Mutex;
};
