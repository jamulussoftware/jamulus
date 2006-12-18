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

#if !defined(AFX_SOUNDIN_H__9518A621_7F78_11D3_8C0D_EEBF182CF549__INCLUDED_)
#define AFX_SOUNDIN_H__9518A621_7F78_11D3_8C0D_EEBF182CF549__INCLUDED_

#include <windows.h>
#include <mmsystem.h>
#include <string>
#include "../src/util.h"
#include "../src/global.h"


/* Definitions ****************************************************************/
#define NUM_IN_OUT_CHANNELS     2       /* Stereo recording (but we only
                                           use one channel for recording) */
#define BITS_PER_SAMPLE         16      /* Use all bits of the D/A-converter */
#define BYTES_PER_SAMPLE        2       /* Number of bytes per sample */

#define MAX_SND_BUF_IN          200
#define MAX_SND_BUF_OUT         200

#define NUM_SOUND_BUFFERS_IN    (70 / MIN_BLOCK_DURATION_MS)
#define NUM_SOUND_BUFFERS_OUT   (80 / MIN_BLOCK_DURATION_MS)

/* Maximum number of recognized sound cards installed in the system */
#define MAX_NUMBER_SOUND_CARDS  10


/* Classes ********************************************************************/
class CSound
{
public:
    CSound();
    virtual ~CSound();

    void        InitRecording(int iNewBufferSize, bool bNewBlocking = TRUE);
    void        InitPlayback(int iNewBufferSize, bool bNewBlocking = FALSE);
    bool        Read(CVector<short>& psData);
    bool        Write(CVector<short>& psData);

    int         GetNumDev() {return iNumDevs;}
    std::string GetDeviceName(int iDiD) {return pstrDevices[iDiD];}
    void        SetOutDev(int iNewDev);
    int         GetOutDev() {return iCurOutDev;}
    void        SetInDev(int iNewDev);
    int         GetInDev() {return iCurInDev;}
    void        SetOutNumBuf(int iNewNum);
    int         GetOutNumBuf() {return iCurNumSndBufOut;}
    void        SetInNumBuf(int iNewNum);
    int         GetInNumBuf() {return iCurNumSndBufIn;}

    void        Close();

protected:
    void        OpenInDevice();
    void        OpenOutDevice();
    void        PrepareInBuffer(int iBufNum);
    void        PrepareOutBuffer(int iBufNum);
    void        AddInBuffer();
    void        AddOutBuffer(int iBufNum);
    void        GetDoneBuffer(int& iCntPrepBuf, int& iIndexDoneBuf);

    WAVEFORMATEX    sWaveFormatEx;
    UINT            iNumDevs;
    std::string     pstrDevices[MAX_NUMBER_SOUND_CARDS];
    UINT            iCurInDev;
    UINT            iCurOutDev;
    bool            bChangParamIn;
    bool            bChangParamOut;
    int             iCurNumSndBufIn;
    int             iCurNumSndBufOut;

    /* Wave in */
    WAVEINCAPS      m_WaveInDevCaps;
    HWAVEIN         m_WaveIn;
    HANDLE          m_WaveInEvent;
    WAVEHDR         m_WaveInHeader[MAX_SND_BUF_IN];
    int             iBufferSizeIn;
    int             iWhichBufferIn;
    short*          psSoundcardBuffer[MAX_SND_BUF_IN];
    bool            bBlockingRec;

    /* Wave out */
    int             iBufferSizeOut;
    HWAVEOUT        m_WaveOut;
    short*          psPlaybackBuffer[MAX_SND_BUF_OUT];
    WAVEHDR         m_WaveOutHeader[MAX_SND_BUF_OUT];
    HANDLE          m_WaveOutEvent;
    bool            bBlockingPlay;
};


#endif // !defined(AFX_SOUNDIN_H__9518A621_7F78_11D3_8C0D_EEBF182CF549__INCLUDED_)
