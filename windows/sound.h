/******************************************************************************\
 * Copyright (c) 2004-2009
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
#include <string>
#include <qmessagebox.h>
#include "../src/util.h"
#include "../src/global.h"

// copy the ASIO SDK in the llcon/windows directory: "llcon/windows/ASIOSDK2" to
// get it work
#include "asiosys.h"
#include "asio.h"
#include "asiodrivers.h"


/* Definitions ****************************************************************/
#define NUM_IN_OUT_CHANNELS     2       /* Stereo recording (but we only
                                           use one channel for recording) */

#define MAX_SND_BUF_IN          100
#define MAX_SND_BUF_OUT         100

#define NUM_SOUND_BUFFERS_IN    4
#define NUM_SOUND_BUFFERS_OUT   4


/* Classes ********************************************************************/
class CSound
{
public:
    CSound ( const int iNewBufferSizeStereo );
    virtual ~CSound();

    void        InitRecording ( const bool bNewBlocking = true )
                {
                    bBlockingRec = bNewBlocking;
                    InitRecordingAndPlayback();
                }
    void        InitPlayback ( const bool bNewBlocking = false )
                {
                    bBlockingPlay = bNewBlocking; 
                    InitRecordingAndPlayback();
                }
    bool        Read ( CVector<short>& psData );
    bool        Write ( CVector<short>& psData );

    int         GetNumDev() { return lNumDevs; }
    std::string GetDeviceName ( const int iDiD ) { return cDriverNames[iDiD]; }

    void        SetDev ( const int iNewDev );
    int         GetDev() { return lCurDev; }

    void        SetOutNumBuf ( const int iNewNum );
    int         GetOutNumBuf();
    void        SetInNumBuf ( const int iNewNum );
    int         GetInNumBuf();

    void        Close();

protected:
    bool        LoadAndInitializeFirstValidDriver();
    std::string LoadAndInitializeDriver ( int iIdx );
    std::string PrepareDriver();
    void        InitRecordingAndPlayback();

    // audio hardware buffer info
    struct sHWBufferInfo
    {
	    long lMinSize;
	    long lMaxSize;
	    long lPreferredSize;
	    long lGranularity;
    } HWBufferInfo;

    // callbacks
    static void      bufferSwitch ( long index, ASIOBool processNow );
    static ASIOTime* bufferSwitchTimeInfo ( ASIOTime *timeInfo, long index, ASIOBool processNow );
    static void      sampleRateChanged ( ASIOSampleRate sRate ) {}
    static long      asioMessages ( long selector, long value, void* message, double* opt );

    long             lNumDevs;
    long             lCurDev;
    char*            cDriverNames[MAX_NUMBER_SOUND_CARDS];

    bool             bChangParamIn;
    bool             bChangParamOut;

    bool             bBlockingRec;
    bool             bBlockingPlay;
};

#endif // !defined ( AFX_SOUNDIN_H__9518A621_7F78_11D3_8C0D_EEBF182CF549__INCLUDED_ )
