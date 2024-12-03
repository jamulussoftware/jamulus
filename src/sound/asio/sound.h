/******************************************************************************\
 * Copyright (c) 2004-2024
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

#include <QMutex>
#include <QMessageBox>
#include "../../util.h"
#include "../../global.h"
#include "../soundbase.h"
#include "../midi-win/midi.h"

// The following includes require the ASIO SDK to be placed in
// libs/ASIOSDK2 during build.
// Important:
// - Do not copy parts of ASIO SDK into the Jamulus source tree without
//   further consideration as it would make the license situation more
//   complicated.
// - When building yourself, read and understand the
//   Steinberg ASIO SDK Licensing Agreement and verify whether you might be
//   obliged to sign it as well, especially when considering distribution
//   of Jamulus Windows binaries with ASIO support.
#include "asiosys.h"
#include "asio.h"
#include "asiodrivers.h"

/* Definitions ****************************************************************/
// stereo for input and output
#define NUM_IN_OUT_CHANNELS 2

/* Classes ********************************************************************/
class CSound : public CSoundBase
{
    Q_OBJECT

public:
    CSound ( void ( *fpNewCallback ) ( CVector<int16_t>& psData, void* arg ), void* arg, const QString& strMIDISetup, const bool, const QString& );

    virtual ~CSound();

    virtual int  Init ( const int iNewPrefMonoBufferSize );
    virtual void Start();
    virtual void Stop();

    virtual void OpenDriverSetup() { ASIOControlPanel(); }

    // channel selection
    virtual int     GetNumInputChannels() { return static_cast<int> ( lNumInChanPlusAddChan ); }
    virtual QString GetInputChannelName ( const int iDiD ) { return channelInputName[iDiD]; }
    virtual void    SetLeftInputChannel ( const int iNewChan );
    virtual void    SetRightInputChannel ( const int iNewChan );
    virtual int     GetLeftInputChannel() { return vSelectedInputChannels[0]; }
    virtual int     GetRightInputChannel() { return vSelectedInputChannels[1]; }

    virtual int     GetNumOutputChannels() { return static_cast<int> ( lNumOutChan ); }
    virtual QString GetOutputChannelName ( const int iDiD ) { return channelInfosOutput[iDiD].name; }
    virtual void    SetLeftOutputChannel ( const int iNewChan );
    virtual void    SetRightOutputChannel ( const int iNewChan );
    virtual int     GetLeftOutputChannel() { return vSelectedOutputChannels[0]; }
    virtual int     GetRightOutputChannel() { return vSelectedOutputChannels[1]; }

    virtual float GetInOutLatencyMs() { return fInOutLatencyMs; }

protected:
    virtual QString LoadAndInitializeDriver ( QString strDriverName, bool bOpenDriverSetup );
    virtual void    UnloadCurrentDriver();
    int             GetActualBufferSize ( const int iDesiredBufferSizeMono );
    QString         CheckDeviceCapabilities();
    bool            CheckSampleTypeSupported ( const ASIOSampleType SamType );
    bool            CheckSampleTypeSupportedForCHMixing ( const ASIOSampleType SamType );
    void            ResetChannelMapping();

    int iASIOBufferSizeMono;
    int iASIOBufferSizeStereo;

    long         lNumInChan;
    long         lNumInChanPlusAddChan; // includes additional "added" channels
    long         lNumOutChan;
    float        fInOutLatencyMs;
    CVector<int> vSelectedInputChannels;
    CVector<int> vSelectedOutputChannels;

    CVector<int16_t> vecsMultChanAudioSndCrd;

    QMutex ASIOMutex;

    // utility functions
    static int16_t Flip16Bits ( const int16_t iIn );
    static int32_t Flip32Bits ( const int32_t iIn );
    static int64_t Flip64Bits ( const int64_t iIn );

    // audio hardware buffer info
    struct sHWBufferInfo
    {
        long lMinSize;
        long lMaxSize;
        long lPreferredSize;
        long lGranularity;
    } HWBufferInfo;

    // ASIO stuff
    ASIODriverInfo  driverInfo;
    ASIOBufferInfo  bufferInfos[2 * MAX_NUM_IN_OUT_CHANNELS]; // for input and output buffers -> "2 *"
    ASIOChannelInfo channelInfosInput[MAX_NUM_IN_OUT_CHANNELS];
    QString         channelInputName[MAX_NUM_IN_OUT_CHANNELS];
    ASIOChannelInfo channelInfosOutput[MAX_NUM_IN_OUT_CHANNELS];
    bool            bASIOPostOutput;
    ASIOCallbacks   asioCallbacks;

    // callbacks
    static void      bufferSwitch ( long index, ASIOBool processNow );
    static ASIOTime* bufferSwitchTimeInfo ( ASIOTime* timeInfo, long index, ASIOBool processNow );
    static void      sampleRateChanged ( ASIOSampleRate ) {}
    static long      asioMessages ( long selector, long value, void* message, double* opt );

    char* cDriverNames[MAX_NUMBER_SOUND_CARDS];

    // Windows native MIDI support
    CMidi Midi;
};
