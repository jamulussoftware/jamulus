/******************************************************************************\
 * Copyright (c) 2004-2020
 *
 * Author(s):
 *  Volker Fischer, hselasky
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
#include "../src/util.h"
#include "../src/global.h"
#include "../src/soundbase.h"

// copy the ASIO SDK in the llcon/windows directory: "llcon/windows/ASIOSDK2" to
// get it work
#include "asiosys.h"
#include "asio.h"
#include "asiodrivers.h"


/* Definitions ****************************************************************/
// stereo for input and output
#define NUM_IN_OUT_CHANNELS         2


/* Classes ********************************************************************/
class CSound : public CSoundBase
{
public:
    CSound ( void           (*fpNewCallback) ( CVector<float>& pfData, void* arg ),
             void*          arg,
             const int      iCtrlMIDIChannel,
             const bool     ,
             const QString& );
    
    virtual ~CSound() { UnloadCurrentDriver(); }

    virtual int  Init ( const int iNewPrefMonoBufferSize );
    virtual void Start();
    virtual void Stop();

    virtual void OpenDriverSetup() { ASIOControlPanel(); }

    // channel selection
    virtual int     GetNumInputChannels() { return static_cast<int> ( lNumInChanPlusAddChan ); }
    virtual QString GetInputChannelName ( const int iDiD ) { return channelInputName[iDiD]; }
    virtual void    SetLeftInputChannel  ( const int iNewChan );
    virtual void    SetRightInputChannel ( const int iNewChan );
    virtual int     GetLeftInputChannel()  { return vSelectedInputChannels[0]; }
    virtual int     GetRightInputChannel() { return vSelectedInputChannels[1]; }

    virtual int     GetNumOutputChannels() { return static_cast<int> ( lNumOutChan ); }
    virtual QString GetOutputChannelName ( const int iDiD ) { return channelInfosOutput[iDiD].name; }
    virtual void    SetLeftOutputChannel  ( const int iNewChan );
    virtual void    SetRightOutputChannel ( const int iNewChan );
    virtual int     GetLeftOutputChannel()  { return vSelectedOutputChannels[0]; }
    virtual int     GetRightOutputChannel() { return vSelectedOutputChannels[1]; }

    virtual double  GetInOutLatencyMs() { return dInOutLatencyMs; }

protected:
    virtual QString LoadAndInitializeDriver ( int  iIdx,
                                              bool bOpenDriverSetup );
    virtual void    UnloadCurrentDriver();
    int             GetActualBufferSize ( const int iDesiredBufferSizeMono );
    QString         CheckDeviceCapabilities();
    bool            CheckSampleTypeSupported ( const ASIOSampleType SamType );
    void            ResetChannelMapping();

    int             iASIOBufferSizeMono;
    int             iASIOBufferSizeStereo;

    long            lNumInChan;
    long            lNumInChanPlusAddChan; // includes additional "added" channels
    long            lNumOutChan;
    double          dInOutLatencyMs;
    CVector<int>    vSelectedInputChannels;
    CVector<int>    vSelectedOutputChannels;

    CVector<float>  vecfMultChanAudioSndCrd;

    QMutex          ASIOMutex;

    // audio hardware buffer info
    struct sHWBufferInfo
    {
        long lMinSize;
        long lMaxSize;
        long lPreferredSize;
        long lGranularity;
    } HWBufferInfo;

    // ASIO stuff
    ASIODriverInfo   driverInfo;
    ASIOBufferInfo   bufferInfos[2 * MAX_NUM_IN_OUT_CHANNELS]; // for input and output buffers -> "2 *"
    ASIOChannelInfo  channelInfosInput[MAX_NUM_IN_OUT_CHANNELS];
    QString          channelInputName[MAX_NUM_IN_OUT_CHANNELS];
    ASIOChannelInfo  channelInfosOutput[MAX_NUM_IN_OUT_CHANNELS];
    bool             bASIOPostOutput;
    ASIOCallbacks    asioCallbacks;

    // templates
    template <typename T> void bufferSwitchImport ( const int  iGain,
                                                    const long index,
                                                    const int  iCH)
    {
        int iSelAddCH;
        int iSelCH;

        GetSelCHAndAddCH ( vSelectedInputChannels[iCH], lNumInChan, iSelCH, iSelAddCH );

        const T* pASIOBuf = static_cast<const T*> ( bufferInfos[iSelCH].buffers[index] );

        for ( int iCurSample = 0; iCurSample < iASIOBufferSizeMono; iCurSample++ )
        {
            vecfMultChanAudioSndCrd[2 * iCurSample + iCH] = pASIOBuf[iCurSample].Get() * iGain;
        }

        if ( iSelAddCH >= 0 )
        {
            // mix input channels case
            const T* pASIOBufAdd = static_cast<T*> ( bufferInfos[iSelAddCH].buffers[index] );

            for ( int iCurSample = 0; iCurSample < iASIOBufferSizeMono; iCurSample++ )
            {
                vecfMultChanAudioSndCrd[2 * iCurSample + iCH] =
                    ClipFloat ( vecfMultChanAudioSndCrd[2 * iCurSample + iCH] +
                                pASIOBufAdd[iCurSample].Get() * iGain );
            }
        }
    }

    template <typename T> void bufferSwitchExport ( const int  iGain,
                                                    const long index,
                                                    const int  iCH )
    {
        const int iSelCH = lNumInChan + vSelectedOutputChannels[iCH];

        T* pASIOBuf = static_cast<T*> ( bufferInfos[iSelCH].buffers[index] );

        for ( int iCurSample = 0; iCurSample < iASIOBufferSizeMono; iCurSample++ )
        {
            pASIOBuf[iCurSample].Put ( vecfMultChanAudioSndCrd[2 * iCurSample + iCH] / iGain );
        }
    }

    // callbacks
    static void      bufferSwitch ( long index, ASIOBool processNow );
    static ASIOTime* bufferSwitchTimeInfo ( ASIOTime* timeInfo, long index, ASIOBool processNow );
    static void      sampleRateChanged ( ASIOSampleRate ) {}
    static long      asioMessages ( long selector, long value, void* message, double* opt );

    char* cDriverNames[MAX_NUMBER_SOUND_CARDS];
};
