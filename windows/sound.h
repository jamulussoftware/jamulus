/******************************************************************************\
 * Copyright (c) 2004-2006
 *
 * Author(s):
 *  Volker Fischer
 *
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

#if !defined ( AFX_SOUNDIN_H__9518A621_7F78_11D3_8C0D_EEBF182CF549__INCLUDED_ )
#define AFX_SOUNDIN_H__9518A621_7F78_11D3_8C0D_EEBF182CF549__INCLUDED_

#include <windows.h>
#include <mmsystem.h>
#include <string>
#include "../src/util.h"
#include "../src/global.h"


/* Definitions ****************************************************************/
// switch here between ASIO (Steinberg) or native Windows(TM) sound interface
#undef USE_ASIO_SND_INTERFACE
//#define USE_ASIO_SND_INTERFACE


#define NUM_IN_OUT_CHANNELS     2       /* Stereo recording (but we only
                                           use one channel for recording) */
#define BITS_PER_SAMPLE         16      // use all bits of the D/A-converter
#define BYTES_PER_SAMPLE        2       // number of bytes per sample

#define MAX_SND_BUF_IN          200
#define MAX_SND_BUF_OUT         200

#define NUM_SOUND_BUFFERS_IN    ( 70 / MIN_BLOCK_DURATION_MS )
#define NUM_SOUND_BUFFERS_OUT   ( 80 / MIN_BLOCK_DURATION_MS )

// maximum number of recognized sound cards installed in the system
#define MAX_NUMBER_SOUND_CARDS  10


/* Classes ********************************************************************/
#ifdef USE_ASIO_SND_INTERFACE

// copy the ASIO SDK in the llcon/windows directory: "llcon/windows/ASIOSDK2" to
// get it work

#include "asiosys.h"
#include "asio.h"
#include "asiodrivers.h"


class CSound
{
public:
    CSound();
    virtual ~CSound();

    void        InitRecording ( int iNewBufferSize, bool bNewBlocking = true ) { InitRecordingAndPlayback ( iNewBufferSize ); }
    void        InitPlayback ( int iNewBufferSize, bool bNewBlocking = false ) { InitRecordingAndPlayback ( iNewBufferSize ); }
    bool        Read ( CVector<short>& psData );
    bool        Write ( CVector<short>& psData );

    int         GetNumDev() { return iNumDevs; }
    std::string GetDeviceName ( int iDiD ) { return pstrDevices[iDiD]; }
    void        SetOutDev ( int iNewDev ) {} // not supported
    int         GetOutDev() { return 0; }  // not supported
    void        SetInDev ( int iNewDev ) {}  // not supported
    int         GetInDev() { return 0; }  // not supported
    void        SetOutNumBuf ( int iNewNum );
    int         GetOutNumBuf() { return iCurNumSndBufOut; }
    void        SetInNumBuf ( int iNewNum );
    int         GetInNumBuf() { return iCurNumSndBufIn; }

    void        Close();

protected:
    void        InitRecordingAndPlayback ( int iNewBufferSize );
    void        PrepareInBuffer ( int iBufNum );
    void        PrepareOutBuffer ( int iBufNum );
    void        AddInBuffer();
    void        AddOutBuffer ( int iBufNum );
    void        GetDoneBuffer ( int& iCntPrepBuf, int& iIndexDoneBuf );

    // ASIO stuff
    ASIODriverInfo   driverInfo;
    ASIOBufferInfo   bufferInfos[2 * NUM_IN_OUT_CHANNELS]; // for input and output buffers -> "2 *"
	ASIOChannelInfo  channelInfos[2 * NUM_IN_OUT_CHANNELS];
    bool             bASIOPostOutput;
    ASIOCallbacks    asioCallbacks;

    // callbacks
    static void      bufferSwitch ( long index, ASIOBool processNow );
    static ASIOTime* bufferSwitchTimeInfo ( ASIOTime *timeInfo, long index, ASIOBool processNow );
    static void      sampleRateChanged ( ASIOSampleRate sRate ) {}
    static long      asioMessages ( long selector, long value, void* message, double* opt );

    int              iNumDevs;
    std::string      pstrDevices[MAX_NUMBER_SOUND_CARDS];
    bool             bChangParamIn;
    bool             bChangParamOut;
    int              iCurNumSndBufIn;
    int              iCurNumSndBufOut;

    int              iBufferSize;

    // wave in
    HANDLE           m_WaveInEvent;
    int              iWhichBufferIn;
    short*           psSoundcardBuffer[MAX_SND_BUF_IN];

    // wave out
    short*           psPlaybackBuffer[MAX_SND_BUF_OUT];
    HANDLE           m_WaveOutEvent;
};

#else // USE_ASIO_SND_INTERFACE

class CSound
{
public:
    CSound();
    virtual ~CSound();

    void        InitRecording ( int iNewBufferSize, bool bNewBlocking = true );
    void        InitPlayback ( int iNewBufferSize, bool bNewBlocking = false );
    bool        Read ( CVector<short>& psData );
    bool        Write ( CVector<short>& psData );

    int         GetNumDev() { return iNumDevs; }
    std::string GetDeviceName ( int iDiD ) { return pstrDevices[iDiD]; }
    void        SetOutDev ( int iNewDev );
    int         GetOutDev() { return iCurOutDev; }
    void        SetInDev ( int iNewDev );
    int         GetInDev() { return iCurInDev; }
    void        SetOutNumBuf ( int iNewNum );
    int         GetOutNumBuf() { return iCurNumSndBufOut; }
    void        SetInNumBuf ( int iNewNum );
    int         GetInNumBuf() { return iCurNumSndBufIn; }

    void        Close();

protected:
    void        OpenInDevice();
    void        OpenOutDevice();
    void        PrepareInBuffer ( int iBufNum );
    void        PrepareOutBuffer ( int iBufNum );
    void        AddInBuffer();
    void        AddOutBuffer ( int iBufNum );
    void        GetDoneBuffer ( int& iCntPrepBuf, int& iIndexDoneBuf );

    WAVEFORMATEX    sWaveFormatEx;
    UINT            iNumDevs;
    std::string     pstrDevices[MAX_NUMBER_SOUND_CARDS];
    UINT            iCurInDev;
    UINT            iCurOutDev;
    bool            bChangParamIn;
    bool            bChangParamOut;
    int             iCurNumSndBufIn;
    int             iCurNumSndBufOut;

    // wave in
    WAVEINCAPS      m_WaveInDevCaps;
    HWAVEIN         m_WaveIn;
    HANDLE          m_WaveInEvent;
    WAVEHDR         m_WaveInHeader[MAX_SND_BUF_IN];
    int             iBufferSizeIn;
    int             iWhichBufferIn;
    short*          psSoundcardBuffer[MAX_SND_BUF_IN];
    bool            bBlockingRec;

    // wave out
    int             iBufferSizeOut;
    HWAVEOUT        m_WaveOut;
    short*          psPlaybackBuffer[MAX_SND_BUF_OUT];
    WAVEHDR         m_WaveOutHeader[MAX_SND_BUF_OUT];
    HANDLE          m_WaveOutEvent;
    bool            bBlockingPlay;
};

#endif // USE_ASIO_SND_INTERFACE

#endif // !defined ( AFX_SOUNDIN_H__9518A621_7F78_11D3_8C0D_EEBF182CF549__INCLUDED_ )
