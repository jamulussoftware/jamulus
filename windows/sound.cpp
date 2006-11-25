/******************************************************************************\
 * Copyright (c) 2004-2006
 *
 * Author(s):
 *  Volker Fischer
 *
 * Description:
 * Sound card interface for Windows operating systems
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

#include "Sound.h"


/* Implementation *************************************************************/
/******************************************************************************\
* Wave in                                                                      *
\******************************************************************************/
bool CSound::Read(CVector<short>& psData)
{
    int     i;
    bool    bError;

    /* Check if device must be opened or reinitialized */
    if (bChangParamIn == TRUE)
    {
        OpenInDevice();

        /* Reinit sound interface */
        InitRecording(iBufferSizeIn, bBlockingRec);

        /* Reset flag */
        bChangParamIn = FALSE;
    }

    /* Wait until data is available */
    if (!(m_WaveInHeader[iWhichBufferIn].dwFlags & WHDR_DONE))
    {
        if (bBlockingRec == TRUE)
            WaitForSingleObject(m_WaveInEvent, INFINITE);
        else
            return FALSE;
    }

    /* Check if buffers got lost */
    int iNumInBufDone = 0;
    for (i = 0; i < iCurNumSndBufIn; i++)
    {
        if (m_WaveInHeader[i].dwFlags & WHDR_DONE)
            iNumInBufDone++;
    }

    /* If the number of done buffers equals the total number of buffers, it is
       very likely that a buffer got lost -> set error flag */
    if (iNumInBufDone == iCurNumSndBufIn)
        bError = TRUE;
    else
        bError = FALSE;

    /* Copy data from sound card in output buffer */
    for (i = 0; i < iBufferSizeIn; i++)
        psData[i] = psSoundcardBuffer[iWhichBufferIn][i];

    /* Add the buffer so that it can be filled with new samples */
    AddInBuffer();

    /* In case more than one buffer was ready, reset event */
    ResetEvent(m_WaveInEvent);

    return bError;
}

void CSound::AddInBuffer()
{
    /* Unprepare old wave-header */
    waveInUnprepareHeader(
        m_WaveIn, &m_WaveInHeader[iWhichBufferIn], sizeof(WAVEHDR));

    /* Prepare buffers for sending to sound interface */
    PrepareInBuffer(iWhichBufferIn);

    /* Send buffer to driver for filling with new data */
    waveInAddBuffer(m_WaveIn, &m_WaveInHeader[iWhichBufferIn], sizeof(WAVEHDR));

    /* Toggle buffers */
    iWhichBufferIn++;
    if (iWhichBufferIn == iCurNumSndBufIn)
        iWhichBufferIn = 0;
}

void CSound::PrepareInBuffer(int iBufNum)
{
    /* Set struct entries */
    m_WaveInHeader[iBufNum].lpData = (LPSTR) &psSoundcardBuffer[iBufNum][0];
    m_WaveInHeader[iBufNum].dwBufferLength = iBufferSizeIn * BYTES_PER_SAMPLE;
    m_WaveInHeader[iBufNum].dwFlags = 0;

    /* Prepare wave-header */
    waveInPrepareHeader(m_WaveIn, &m_WaveInHeader[iBufNum], sizeof(WAVEHDR));
}

void CSound::InitRecording(int iNewBufferSize, bool bNewBlocking)
{
    /* Check if device must be opened or reinitialized */
    if (bChangParamIn == TRUE)
    {
        OpenInDevice();

        /* Reset flag */
        bChangParamIn = FALSE;
    }

    /* Set internal parameter */
    iBufferSizeIn = iNewBufferSize;
    bBlockingRec = bNewBlocking;

    /* Reset interface so that all buffers are returned from the interface */
    waveInReset(m_WaveIn);
    waveInStop(m_WaveIn);
    
    /* Reset current buffer ID (it is important to do this BEFORE calling
       "AddInBuffer()" */
    iWhichBufferIn = 0;

    /* Create memory for sound card buffer */
    for (int i = 0; i < iCurNumSndBufIn; i++)
    {
        /* Unprepare old wave-header in case that we "re-initialized" this
           module. Calling "waveInUnprepareHeader()" with an unprepared
           buffer (when the module is initialized for the first time) has
           simply no effect */
        waveInUnprepareHeader(m_WaveIn, &m_WaveInHeader[i], sizeof(WAVEHDR));

        if (psSoundcardBuffer[i] != NULL)
            delete[] psSoundcardBuffer[i];

        psSoundcardBuffer[i] = new short[iBufferSizeIn];


        /* Send all buffers to driver for filling the queue ----------------- */
        /* Prepare buffers before sending them to the sound interface */
        PrepareInBuffer(i);

        AddInBuffer();
    }

    /* Notify that sound capturing can start now */
    waveInStart(m_WaveIn);

    /* This reset event is very important for initialization, otherwise we will
       get errors! */
    ResetEvent(m_WaveInEvent);
}

void CSound::OpenInDevice()
{
    /* Open wave-input and set call-back mechanism to event handle */
    if (m_WaveIn != NULL)
    {
        waveInReset(m_WaveIn);
        waveInClose(m_WaveIn);
    }

    MMRESULT result = waveInOpen(&m_WaveIn, iCurInDev, &sWaveFormatEx,
        (DWORD) m_WaveInEvent, NULL, CALLBACK_EVENT);

    if (result != MMSYSERR_NOERROR)
    {
        throw CGenErr("Sound Interface Start, waveInOpen() failed. This error "
            "usually occurs if another application blocks the sound in.");
    }
}

void CSound::SetInDev(int iNewDev)
{
    /* Set device to wave mapper if iNewDev is invalid */
    if ((iNewDev >= iNumDevs) || (iNewDev < 0))
        iNewDev = WAVE_MAPPER;

    /* Change only in case new device id is not already active */
    if (iNewDev != iCurInDev)
    {
        iCurInDev = iNewDev;
        bChangParamIn = TRUE;
    }
}

void CSound::SetInNumBuf(int iNewNum)
{
    /* check new parameter */
    if ((iNewNum >= MAX_SND_BUF_IN) || (iNewNum < 1))
        iNewNum = NUM_SOUND_BUFFERS_IN;

    /* Change only if parameter is different */
    if (iNewNum != iCurNumSndBufIn)
    {
        iCurNumSndBufIn = iNewNum;
        bChangParamIn = TRUE;
    }
}


/******************************************************************************\
* Wave out                                                                     *
\******************************************************************************/
bool CSound::Write(CVector<short>& psData)
{
    int     i, j;
    int     iCntPrepBuf;
    int     iIndexDoneBuf;
    bool    bError;

    /* Check if device must be opened or reinitialized */
    if (bChangParamOut == TRUE)
    {
        OpenOutDevice();

        /* Reinit sound interface */
        InitPlayback(iBufferSizeOut, bBlockingPlay);

        /* Reset flag */
        bChangParamOut = FALSE;
    }

    /* Get number of "done"-buffers and position of one of them */
    GetDoneBuffer(iCntPrepBuf, iIndexDoneBuf);

    /* Now check special cases (Buffer is full or empty) */
    if (iCntPrepBuf == 0)
    {
        if (bBlockingPlay == TRUE)
        {
            /* Blocking wave out routine. Needed for transmitter. Always
               ensure that the buffer is completely filled to avoid buffer
               underruns */
            while (iCntPrepBuf == 0)
            {
                WaitForSingleObject(m_WaveOutEvent, INFINITE);

                GetDoneBuffer(iCntPrepBuf, iIndexDoneBuf);
            }
        }
        else
        {
            /* All buffers are filled, dump new block ----------------------- */
// It would be better to kill half of the buffer blocks to set the start
// back to the middle: TODO
            return TRUE; /* An error occurred */
        }
    }
    else if (iCntPrepBuf == iCurNumSndBufOut)
    {
        /* ---------------------------------------------------------------------
           Buffer is empty -> send as many cleared blocks to the sound-
           interface until half of the buffer size is reached */
        /* Send half of the buffer size blocks to the sound-interface */
        for (j = 0; j < iCurNumSndBufOut / 2; j++)
        {
            /* First, clear these buffers */
            for (i = 0; i < iBufferSizeOut; i++)
                psPlaybackBuffer[j][i] = 0;

            /* Then send them to the interface */
            AddOutBuffer(j);
        }

        /* Set index for done buffer */
        iIndexDoneBuf = iCurNumSndBufOut / 2;

        bError = TRUE;
    }
    else
        bError = FALSE;

    /* Copy stereo data from input in soundcard buffer */
    for (i = 0; i < iBufferSizeOut; i++)
        psPlaybackBuffer[iIndexDoneBuf][i] = psData[i];

    /* Now, send the current block */
    AddOutBuffer(iIndexDoneBuf);

    return bError;
}

void CSound::GetDoneBuffer(int& iCntPrepBuf, int& iIndexDoneBuf)
{
    /* Get number of "done"-buffers and position of one of them */
    iCntPrepBuf = 0;
    for (int i = 0; i < iCurNumSndBufOut; i++)
    {
        if (m_WaveOutHeader[i].dwFlags & WHDR_DONE)
        {
            iCntPrepBuf++;
            iIndexDoneBuf = i;
        }
    }
}

void CSound::AddOutBuffer(int iBufNum)
{
    /* Unprepare old wave-header */
    waveOutUnprepareHeader(
        m_WaveOut, &m_WaveOutHeader[iBufNum], sizeof(WAVEHDR));

    /* Prepare buffers for sending to sound interface */
    PrepareOutBuffer(iBufNum);

    /* Send buffer to driver for filling with new data */
    waveOutWrite(m_WaveOut, &m_WaveOutHeader[iBufNum], sizeof(WAVEHDR));
}

void CSound::PrepareOutBuffer(int iBufNum)
{
    /* Set Header data */
    m_WaveOutHeader[iBufNum].lpData = (LPSTR) &psPlaybackBuffer[iBufNum][0];
    m_WaveOutHeader[iBufNum].dwBufferLength = iBufferSizeOut * BYTES_PER_SAMPLE;
    m_WaveOutHeader[iBufNum].dwFlags = 0;

    /* Prepare wave-header */
    waveOutPrepareHeader(m_WaveOut, &m_WaveOutHeader[iBufNum], sizeof(WAVEHDR));
}

void CSound::InitPlayback(int iNewBufferSize, bool bNewBlocking)
{
    int i, j;

    /* Check if device must be opened or reinitialized */
    if (bChangParamOut == TRUE)
    {
        OpenOutDevice();

        /* Reset flag */
        bChangParamOut = FALSE;
    }

    /* Set internal parameters */
    iBufferSizeOut = iNewBufferSize;
    bBlockingPlay = bNewBlocking;

    /* Reset interface */
    waveOutReset(m_WaveOut);

    for (j = 0; j < iCurNumSndBufOut; j++)
    {
        /* Unprepare old wave-header (in case header was not prepared before,
           simply nothing happens with this function call */
        waveOutUnprepareHeader(m_WaveOut, &m_WaveOutHeader[j], sizeof(WAVEHDR));

        /* Create memory for playback buffer */
        if (psPlaybackBuffer[j] != NULL)
            delete[] psPlaybackBuffer[j];

        psPlaybackBuffer[j] = new short[iBufferSizeOut];

        /* Clear new buffer */
        for (i = 0; i < iBufferSizeOut; i++)
            psPlaybackBuffer[j][i] = 0;

        /* Prepare buffer for sending to the sound interface */
        PrepareOutBuffer(j);

        /* Initially, send all buffers to the interface */
        AddOutBuffer(j);
    }
}

void CSound::OpenOutDevice()
{
    if (m_WaveOut != NULL)
    {
        waveOutReset(m_WaveOut);
        waveOutClose(m_WaveOut);
    }

    MMRESULT result = waveOutOpen(&m_WaveOut, iCurOutDev, &sWaveFormatEx,
        (DWORD) m_WaveOutEvent, NULL, CALLBACK_EVENT);

    if (result != MMSYSERR_NOERROR)
        throw CGenErr("Sound Interface Start, waveOutOpen() failed.");
}

void CSound::SetOutDev(int iNewDev)
{
    /* Set device to wave mapper if iNewDev is invalid */
    if ((iNewDev >= iNumDevs) || (iNewDev < 0))
        iNewDev = WAVE_MAPPER;

    /* Change only in case new device id is not already active */
    if (iNewDev != iCurOutDev)
    {
        iCurOutDev = iNewDev;
        bChangParamOut = TRUE;
    }
}

void CSound::SetOutNumBuf(int iNewNum)
{
    /* check new parameter */
    if ((iNewNum >= MAX_SND_BUF_OUT) || (iNewNum < 1))
        iNewNum = NUM_SOUND_BUFFERS_OUT;

    /* Change only if parameter is different */
    if (iNewNum != iCurNumSndBufOut)
    {
        iCurNumSndBufOut = iNewNum;
        bChangParamOut = TRUE;
    }
}


/******************************************************************************\
* Common                                                                       *
\******************************************************************************/
void CSound::Close()
{
    int         i;
    MMRESULT    result;

    /* Reset audio driver */
    if (m_WaveOut != NULL)
    {
        result = waveOutReset(m_WaveOut);
        if (result != MMSYSERR_NOERROR)
            throw CGenErr("Sound Interface, waveOutReset() failed.");
    }

    if (m_WaveIn != NULL)
    {
        result = waveInReset(m_WaveIn);
        if (result != MMSYSERR_NOERROR)
            throw CGenErr("Sound Interface, waveInReset() failed.");
    }

    /* Set event to ensure that thread leaves the waiting function */
    if (m_WaveInEvent != NULL)
        SetEvent(m_WaveInEvent);

    /* Wait for the thread to terminate */
    Sleep(500);

    /* Unprepare wave-headers */
    if (m_WaveIn != NULL)
    {
        for (i = 0; i < iCurNumSndBufIn; i++)
        {
            result = waveInUnprepareHeader(
                m_WaveIn, &m_WaveInHeader[i], sizeof(WAVEHDR));

            if (result != MMSYSERR_NOERROR)
            {
                throw CGenErr("Sound Interface, waveInUnprepareHeader()"
                    " failed.");
            }
        }

        /* Close the sound in device */
        result = waveInClose(m_WaveIn);
        if (result != MMSYSERR_NOERROR)
            throw CGenErr("Sound Interface, waveInClose() failed.");
    }

    if (m_WaveOut != NULL)
    {
        for (i = 0; i < iCurNumSndBufOut; i++)
        {
            result = waveOutUnprepareHeader(
                m_WaveOut, &m_WaveOutHeader[i], sizeof(WAVEHDR));

            if (result != MMSYSERR_NOERROR)
            {
                throw CGenErr("Sound Interface, waveOutUnprepareHeader()"
                    " failed.");
            }
        }

        /* Close the sound out device */
        result = waveOutClose(m_WaveOut);
        if (result != MMSYSERR_NOERROR)
            throw CGenErr("Sound Interface, waveOutClose() failed.");
    }

    /* Set flag to open devices the next time it is initialized */
    bChangParamIn = TRUE;
    bChangParamOut = TRUE;
}

CSound::CSound()
{
    int i;

    /* init number of sound buffers */
    iCurNumSndBufIn = NUM_SOUND_BUFFERS_IN;
    iCurNumSndBufOut = NUM_SOUND_BUFFERS_OUT;

    /* Should be initialized because an error can occur during init */
    m_WaveInEvent = NULL;
    m_WaveOutEvent = NULL;
    m_WaveIn = NULL;
    m_WaveOut = NULL;

    /* Init buffer pointer to zero */
    for (i = 0; i < MAX_SND_BUF_IN; i++)
    {
        memset(&m_WaveInHeader[i], 0, sizeof(WAVEHDR));
        psSoundcardBuffer[i] = NULL;
    }

    for (i = 0; i < MAX_SND_BUF_OUT; i++)
    {
        memset(&m_WaveOutHeader[i], 0, sizeof(WAVEHDR));
        psPlaybackBuffer[i] = NULL;
    }

    /* Init wave-format structure */
    sWaveFormatEx.wFormatTag = WAVE_FORMAT_PCM;
    sWaveFormatEx.nChannels = NUM_IN_OUT_CHANNELS;
    sWaveFormatEx.wBitsPerSample = BITS_PER_SAMPLE;
    sWaveFormatEx.nSamplesPerSec = SND_CRD_SAMPLE_RATE;
    sWaveFormatEx.nBlockAlign = sWaveFormatEx.nChannels *
        sWaveFormatEx.wBitsPerSample / 8;
    sWaveFormatEx.nAvgBytesPerSec = sWaveFormatEx.nBlockAlign *
        sWaveFormatEx.nSamplesPerSec;
    sWaveFormatEx.cbSize = 0;

    /* Get the number of digital audio devices in this computer, check range */
    iNumDevs = waveInGetNumDevs();

    if (iNumDevs > MAX_NUMBER_SOUND_CARDS)
        iNumDevs = MAX_NUMBER_SOUND_CARDS;

    /* At least one device must exist in the system */
    if (iNumDevs == 0)
        throw CGenErr("No audio device found.");

    /* Get info about the devices and store the names */
    for (i = 0; i < iNumDevs; i++)
    {
        if (!waveInGetDevCaps(i, &m_WaveInDevCaps, sizeof(WAVEINCAPS)))
            pstrDevices[i] = m_WaveInDevCaps.szPname;
    }

    /* We use an event controlled wave-in (wave-out) structure */
    /* Create events */
    m_WaveInEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    m_WaveOutEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

    /* Set flag to open devices */
    bChangParamIn = TRUE;
    bChangParamOut = TRUE;

    /* Default device number, "wave mapper" */
    iCurInDev = WAVE_MAPPER;
    iCurOutDev = WAVE_MAPPER;

    /* Non-blocking wave out is default */
    bBlockingPlay = FALSE;

    /* Blocking wave in is default */
    bBlockingRec = TRUE;
}

CSound::~CSound()
{
    int i;

    /* Delete allocated memory */
    for (i = 0; i < iCurNumSndBufIn; i++)
    {
        if (psSoundcardBuffer[i] != NULL)
            delete[] psSoundcardBuffer[i];
    }

    for (i = 0; i < iCurNumSndBufOut; i++)
    {
        if (psPlaybackBuffer[i] != NULL)
            delete[] psPlaybackBuffer[i];
    }

    /* Close the handle for the events */
    if (m_WaveInEvent != NULL)
        CloseHandle(m_WaveInEvent);

    if (m_WaveOutEvent != NULL)
        CloseHandle(m_WaveOutEvent);
}
