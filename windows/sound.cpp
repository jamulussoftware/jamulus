/******************************************************************************\
 * Copyright (c) 2004-2009
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
#include <qmutex.h>

// external references
extern AsioDrivers* asioDrivers;
bool   loadAsioDriver ( char *name );

// mutex
QMutex ASIOMutex;

// TODO the following variables should be in the class definition but we cannot
// do it here since we have static callback functions which cannot access the
// class members :-(((

// ASIO stuff
ASIODriverInfo   driverInfo;
ASIOBufferInfo   bufferInfos[2 * NUM_IN_OUT_CHANNELS]; // for input and output buffers -> "2 *"
ASIOChannelInfo  channelInfos[2 * NUM_IN_OUT_CHANNELS];
bool             bASIOPostOutput;
ASIOCallbacks    asioCallbacks;
int              iBufferSizeMono;
int              iBufferSizeStereo;
int              iASIOBufferSizeMono;

// event
HANDLE           m_ASIOEvent;

// wave in
short*           psCaptureBuffer;
int              iBufferPosCapture;
bool             bCaptureBufferOverrun;

// wave out
short*           psPlayBuffer;
int              iBufferPosPlay;
bool             bPlayBufferUnderrun;

int              iMinNumSndBuf;
int              iCurNumSndBufIn;
int              iCurNumSndBufOut;
int              iNewNumSndBufIn;
int              iNewNumSndBufOut;
bool             bSetNumSndBufToMinimumValue;

// we must implement these functions here to get access to global variables
int CSound::GetOutNumBuf() { return iNewNumSndBufOut; }
int CSound::GetInNumBuf()  { return iNewNumSndBufIn; }


/******************************************************************************\
* Wave in                                                                      *
\******************************************************************************/
bool CSound::Read ( CVector<short>& psData )
{
    int  i;
    bool bError;

    // check if device must be opened or reinitialized
    if ( bChangParamIn )
    {
        // reinit sound interface
        InitRecordingAndPlayback();

        // reset flag
        bChangParamIn = false;
    }

    // wait until enough data is available
    int iWaitCount = 0;
    while ( iBufferPosCapture < iBufferSizeStereo )
    {
        if ( bBlockingRec )
        {
            if ( !bCaptureBufferOverrun )
            {
                // regular case
                WaitForSingleObject ( m_ASIOEvent, INFINITE );
            }
            else
            {
                // it seems that the buffers are too small, wait
                // just one time to avoid CPU to go up to 100% and
                // then leave this function
                if ( iWaitCount == 0 )
                {
                    WaitForSingleObject ( m_ASIOEvent, INFINITE );
                    iWaitCount++;
                }
                else
                {
                    return true;
                }
            }
        }
        else
        {
            return true;
        }
    }

    ASIOMutex.lock(); // get mutex lock
    {
        // check for buffer overrun in ASIO thread
        bError = bCaptureBufferOverrun;
        if ( bCaptureBufferOverrun )
        {
            // reset flag
            bCaptureBufferOverrun = false;
        }

        // copy data from sound card capture buffer in function output buffer
        for ( i = 0; i < iBufferSizeStereo; i++ )
        {
            psData[i] = psCaptureBuffer[i];
        }

        // move all other data in buffer
        const int iLenCopyRegion = iBufferPosCapture - iBufferSizeStereo;
        for ( i = 0; i < iLenCopyRegion; i++ )
        {
            psCaptureBuffer[i] = psCaptureBuffer[iBufferSizeStereo + i];
        }

        // adjust "current block to write" pointer
        iBufferPosCapture -= iBufferSizeStereo;

        // in case more than one buffer was ready, reset event
        ResetEvent ( m_ASIOEvent );
    }
    ASIOMutex.unlock();

    return bError;
}

void CSound::SetInNumBuf ( int iNewNum )
{
    // check new parameter
    if ( ( iNewNum < MAX_SND_BUF_IN ) && ( iNewNum >= iMinNumSndBuf ) )
    {
        // change only if parameter is different
        if ( iNewNum != iNewNumSndBufIn )
        {
            iNewNumSndBufIn = iNewNum;
            bChangParamIn   = true;
        }
    }
}


/******************************************************************************\
* Wave out                                                                     *
\******************************************************************************/
bool CSound::Write ( CVector<short>& psData )
{
    bool bError;

    // check if device must be opened or reinitialized
    if ( bChangParamOut )
    {
        // reinit sound interface
        InitRecordingAndPlayback();

        // reset flag
        bChangParamOut = false;
    }

    ASIOMutex.lock(); // get mutex lock
    {
        // check for buffer underrun in ASIO thread
        bError = bPlayBufferUnderrun;
        if ( bPlayBufferUnderrun )
        {
            // reset flag
            bPlayBufferUnderrun = false;
        }

        // first check if enough data in buffer is available
        const int iPlayBufferLen = iCurNumSndBufOut * iBufferSizeStereo;

        if ( iBufferPosPlay + iBufferSizeStereo > iPlayBufferLen )
        {
            // buffer overrun, return error
            bError = true;
        }
        else
        {
            // copy stereo data from function input in soundcard play buffer
            for ( int i = 0; i < iBufferSizeStereo; i++ )
            {
                psPlayBuffer[iBufferPosPlay + i] = psData[i];
            }

            iBufferPosPlay += iBufferSizeStereo;
        }
    }
    ASIOMutex.unlock();

    return bError;
}

void CSound::SetOutNumBuf ( int iNewNum )
{
    // check new parameter
    if ( ( iNewNum < MAX_SND_BUF_OUT ) && ( iNewNum >= iMinNumSndBuf ) )
    {
        // change only if parameter is different
        if ( iNewNum != iNewNumSndBufOut )
        {
            iNewNumSndBufOut = iNewNum;
            bChangParamOut   = true;
        }
    }
}


/******************************************************************************\
* Common                                                                       *
\******************************************************************************/
void CSound::SetDev ( const int iNewDev )
{
    // check if an ASIO driver was already initialized
    if ( lCurDev >= 0 )
    {
        // the new driver was not selected before, use default settings for
        // buffer sizes
        bSetNumSndBufToMinimumValue = true;

        // a device was already been initialized and is used, kill working
        // thread and clean up
        // stop driver
        ASIOStop();

        // set event to ensure that thread leaves the waiting function
        if ( m_ASIOEvent != NULL )
        {
            SetEvent ( m_ASIOEvent );
        }

        // wait for the thread to terminate
        Sleep ( 500 );

        // dispose ASIO buffers
        ASIODisposeBuffers();

        // remove old driver
        ASIOExit();
        asioDrivers->removeCurrentDriver();

        const std::string strErrorMessage = LoadAndInitializeDriver ( iNewDev );

        if ( !strErrorMessage.empty() )
        {
            // The new driver initializing was not successful, try to preserve
            // the old buffer settings -> this is possible, if errornous driver
            // had failed before the buffer setting was done. If it failed after
            // setting the minimum buffer sizes, the following flag modification
            // does not have any effect which means the old settings cannot be
            // recovered anymore (TODO better solution)
            bSetNumSndBufToMinimumValue = false;

            // loading and initializing the new driver failed, go back to original
            // driver and display error message
            LoadAndInitializeDriver ( lCurDev );
            InitRecordingAndPlayback();

            throw CGenErr ( strErrorMessage.c_str() );
        }
        InitRecordingAndPlayback();
    }
    else
    {
        if ( iNewDev != INVALID_SNC_CARD_DEVICE )
        {
            // This is the first time a driver is to be initialized, we first try
            // to load the selected driver, if this fails, we try to load the first
            // available driver in the system. If this fails, too, we throw an error
            // that no driver is available -> it does not make sense to start the llcon
            // software if no audio hardware is available
            const std::string strErrorMessage = LoadAndInitializeDriver ( iNewDev );

            if ( !strErrorMessage.empty() )
            {
                // loading and initializing the new driver failed, try to find at
                // least one usable driver
                if ( !LoadAndInitializeFirstValidDriver() )
                {
                    throw CGenErr ( "No usable ASIO audio device (driver) found." );
                }
            }
        }
        else
        {
            // the new driver was not selected before, use default settings for
            // buffer sizes
            bSetNumSndBufToMinimumValue = true;

            // try to find one usable driver (select the first valid driver)
            if ( !LoadAndInitializeFirstValidDriver() )
            {
                throw CGenErr ( "No usable ASIO audio device (driver) found." );
            }
        }
    }
}

std::string CSound::LoadAndInitializeDriver ( int iDriverIdx )
{
    // first check and correct input parameter
    if ( iDriverIdx >= lNumDevs )
    {
        // we assume here that at least one driver is in the system
        iDriverIdx = 0;
    }

    // load driver
    loadAsioDriver ( cDriverNames[iDriverIdx] );
    if ( ASIOInit ( &driverInfo ) != ASE_OK )
    {
        // clean up and return error string
        asioDrivers->removeCurrentDriver();
        return "The audio driver could not be initialized.";
    }

    const std::string strStat = PrepareDriver();

    // store ID of selected driver if initialization was successful
    if ( strStat.empty() )
    {
        lCurDev = iDriverIdx;
    }

    return strStat;
}

bool CSound::LoadAndInitializeFirstValidDriver()
{
    // load and initialize first valid ASIO driver
    bool bValidDriverDetected = false;
    int  iCurDriverIdx = 0;

    // try all available drivers in the system ("lNumDevs" devices)
    while ( !bValidDriverDetected && iCurDriverIdx < lNumDevs )
    {
        if ( loadAsioDriver ( cDriverNames[iCurDriverIdx] ) )
        {
            if ( ASIOInit ( &driverInfo ) == ASE_OK )
            {
                if ( PrepareDriver().empty() )
                {
                    // initialization was successful
                    bValidDriverDetected = true;

                    // store ID of selected driver
                    lCurDev = iCurDriverIdx;
                }
                else
                {
                    // driver could not be loaded, free memory
                    asioDrivers->removeCurrentDriver();
                }
            }
            else
            {
                // driver could not be loaded, free memory
                asioDrivers->removeCurrentDriver();
            }
        }

        // try next driver
        iCurDriverIdx++;
    }

    return bValidDriverDetected;
}

std::string CSound::PrepareDriver()
{
    int i;
    int iDesiredBufferSizeMono;

    // check the number of available channels
    long lNumInChan;
    long lNumOutChan;
    ASIOGetChannels ( &lNumInChan, &lNumOutChan );
    if ( ( lNumInChan < NUM_IN_OUT_CHANNELS ) ||
         ( lNumOutChan < NUM_IN_OUT_CHANNELS ) )
    {
        // clean up and return error string
        ASIOExit();
        asioDrivers->removeCurrentDriver();
        return "The audio device does not support the "
            "required number of channels.";
    }

    // set the sample rate and check if sample rate is supported
    ASIOSetSampleRate ( SND_CRD_SAMPLE_RATE );

    ASIOSampleRate sampleRate;
    ASIOGetSampleRate ( &sampleRate );
    if ( sampleRate != SND_CRD_SAMPLE_RATE )
    {
        // clean up and return error string
        ASIOExit();
        asioDrivers->removeCurrentDriver();
        return "The audio device does not support the "
            "required sample rate.";
    }

    // query the usable buffer sizes
    ASIOGetBufferSize ( &HWBufferInfo.lMinSize,
                        &HWBufferInfo.lMaxSize,
                        &HWBufferInfo.lPreferredSize,
                        &HWBufferInfo.lGranularity );

    // calculate the desired mono buffer size

// TEST -> put this in the GUI and implement the code for the Linux driver, too
// setting this variable to false sets the previous behaviour
const bool bPreferPowerOfTwoAudioBufferSize = false;

    if ( bPreferPowerOfTwoAudioBufferSize )
    {
        // use next power of 2 for desired block size mono
        iDesiredBufferSizeMono = LlconMath().NextPowerOfTwo ( iBufferSizeMono );
    }
    else
    {
        iDesiredBufferSizeMono = iBufferSizeMono;
    }

    // calculate "nearest" buffer size and set internal parameter accordingly
    // first check minimum and maximum values
    if ( iDesiredBufferSizeMono < HWBufferInfo.lMinSize )
    {
        iASIOBufferSizeMono = HWBufferInfo.lMinSize;
    }
    else
    {
        if ( iDesiredBufferSizeMono > HWBufferInfo.lMaxSize )
        {
            iASIOBufferSizeMono = HWBufferInfo.lMaxSize;
        }
        else
        {
            // initialization
            int  iTrialBufSize     = HWBufferInfo.lMinSize;
            int  iLastTrialBufSize = HWBufferInfo.lMinSize;
            bool bSizeFound        = false;

            // test loop
            while ( ( iTrialBufSize <= HWBufferInfo.lMaxSize ) && ( !bSizeFound ) )
            {
                if ( iTrialBufSize >= iDesiredBufferSizeMono )
                {
                    // test which buffer size fits better: the old one or the
                    // current one
                    if ( ( iTrialBufSize - iDesiredBufferSizeMono ) >
                         ( iDesiredBufferSizeMono - iLastTrialBufSize ) )
                    {
                        iTrialBufSize = iLastTrialBufSize;
                    }

                    // exit while loop
                    bSizeFound = true;
                }

                if ( !bSizeFound )
                {
                    // store old trial buffer size
                    iLastTrialBufSize = iTrialBufSize;

                    // increment trial buffer size (check for special case first)
                    if ( HWBufferInfo.lGranularity == -1 )
                    {
                        // special case: buffer sizes are a power of 2
                        iTrialBufSize *= 2;
                    }
                    else
                    {
                        iTrialBufSize += HWBufferInfo.lGranularity;
                    }
                }
            }

            // set ASIO buffer size
            iASIOBufferSizeMono = iTrialBufSize;
        }
    }

    // calculate the minimum required number of soundcard buffers
    iMinNumSndBuf = static_cast<int> (
        ceil ( static_cast<double> ( iASIOBufferSizeMono ) / iBufferSizeMono ) );

// TODO better solution
// For some ASIO buffer sizes, the above calculation seems not to work although
// it should be correct. Maybe there is a misunderstanding or a bug in the
// sound interface implementation. As a workaround, we implement a table here, to
// get working parameters for the most common ASIO buffer settings
// Interesting observation: only 256 samples seems to be wrong, all other tested
// buffer sizes like 192, 512, 384, etc. are correct...
if ( iASIOBufferSizeMono == 256 )
{
    iMinNumSndBuf = 4;
}
    Q_ASSERT ( iMinNumSndBuf < MAX_SND_BUF_IN );
    Q_ASSERT ( iMinNumSndBuf < MAX_SND_BUF_OUT );

    // set or just check the sound card buffer sizes
    if ( bSetNumSndBufToMinimumValue )
    {
        // use minimum buffer sizes as default
        iNewNumSndBufIn  = iMinNumSndBuf;
        iNewNumSndBufOut = iMinNumSndBuf;
    }
    else
    {
        // correct number of sound card buffers if required
        iNewNumSndBufIn  = max ( iMinNumSndBuf, iNewNumSndBufIn );
        iNewNumSndBufOut = max ( iMinNumSndBuf, iNewNumSndBufOut );
    }
    iCurNumSndBufIn  = iNewNumSndBufIn;
    iCurNumSndBufOut = iNewNumSndBufOut;

    // display warning in case the ASIO buffer is too big
    if ( iMinNumSndBuf > 6 )
    {
        QMessageBox::critical ( 0, APP_NAME,
            QString ( "The ASIO buffer size of the selected audio driver is ") + 
            QString().number ( iASIOBufferSizeMono ) +
            QString ( " samples which is too large. Please try to modify "
            "the ASIO buffer size value in your ASIO driver settings (most ASIO "
            "drivers like ASIO4All or kx driver allow to change the ASIO buffer size). "
            "Recommended settings are 96 or 128 samples. Please make sure that "
            "before you try to change the ASIO driver buffer size all ASIO "
            "applications including llcon are closed." ), "Ok", 0 );
    }

    // prepare input channels
    for ( i = 0; i < NUM_IN_OUT_CHANNELS; i++ )
    {
        bufferInfos[i].isInput    = ASIOTrue;
        bufferInfos[i].channelNum = i;
        bufferInfos[i].buffers[0] = 0;
        bufferInfos[i].buffers[1] = 0;
    }

    // prepare output channels
    for ( i = 0; i < NUM_IN_OUT_CHANNELS; i++ )
    {
        bufferInfos[NUM_IN_OUT_CHANNELS + i].isInput    = ASIOFalse;
        bufferInfos[NUM_IN_OUT_CHANNELS + i].channelNum = i;
        bufferInfos[NUM_IN_OUT_CHANNELS + i].buffers[0] = 0;
        bufferInfos[NUM_IN_OUT_CHANNELS + i].buffers[1] = 0;
    }

    // create and activate ASIO buffers (buffer size in samples)
    ASIOCreateBuffers ( bufferInfos, 2 /* in/out */ * NUM_IN_OUT_CHANNELS /* stereo */,
        iASIOBufferSizeMono, &asioCallbacks );

    // now get some buffer details
    for ( i = 0; i < 2 * NUM_IN_OUT_CHANNELS; i++ )
    {
        channelInfos[i].channel = bufferInfos[i].channelNum;
        channelInfos[i].isInput = bufferInfos[i].isInput;
        ASIOGetChannelInfo ( &channelInfos[i] );

        // only 16/24/32 LSB is supported
        if ( ( channelInfos[i].type != ASIOSTInt16LSB ) &&
             ( channelInfos[i].type != ASIOSTInt24LSB ) &&
             ( channelInfos[i].type != ASIOSTInt32LSB ) )
        {
            // clean up and return error string
            ASIODisposeBuffers();
            ASIOExit();
            asioDrivers->removeCurrentDriver();
            return "Required audio sample format not available (16/24/32 bit LSB).";
        }
    }

    // check wether the driver requires the ASIOOutputReady() optimization
    // (can be used by the driver to reduce output latency by one block)
    bASIOPostOutput = ( ASIOOutputReady() == ASE_OK );

    return "";
}

void CSound::InitRecordingAndPlayback()
{
    // first, stop audio and dispose ASIO buffers
    ASIOStop();

    ASIOMutex.lock(); // get mutex lock
    {
        // store new buffer number values
        iCurNumSndBufIn  = iNewNumSndBufIn;
        iCurNumSndBufOut = iNewNumSndBufOut;

        // initialize write block pointer in and overrun flag
        iBufferPosCapture     = 0;
        bCaptureBufferOverrun = false;

        // create memory for capture buffer
        if ( psCaptureBuffer != NULL )
        {
            delete[] psCaptureBuffer;
        }
        psCaptureBuffer = new short[iCurNumSndBufIn * iBufferSizeStereo];

        // initialize write block pointer out and underrun flag
        iBufferPosPlay      = 0;
        bPlayBufferUnderrun = false;

        // create memory for play buffer
        if ( psPlayBuffer != NULL )
        {
            delete[] psPlayBuffer;
        }
        psPlayBuffer = new short[iCurNumSndBufOut * iBufferSizeStereo];

        // clear new buffer
        for ( int i = 0; i < iCurNumSndBufOut * iBufferSizeStereo; i++ )
        {
            psPlayBuffer[i] = 0;
        }

        // reset event
        ResetEvent ( m_ASIOEvent );
    }
    ASIOMutex.unlock();

    // initialization is done, (re)start audio
    ASIOStart();
}

void CSound::Close()
{
    // stop driver
    ASIOStop();

    // set event to ensure that thread leaves the waiting function
    if ( m_ASIOEvent != NULL )
    {
        SetEvent ( m_ASIOEvent );
    }

    // wait for the thread to terminate
    Sleep ( 500 );

    // set flag to open devices the next time it is initialized
    bChangParamIn  = true;
    bChangParamOut = true;
}

CSound::CSound ( const int iNewBufferSizeStereo )
{
    // set internal buffer size value and calculate mono buffer size
    iBufferSizeStereo = iNewBufferSizeStereo;
    iBufferSizeMono   = iBufferSizeStereo / 2;

    // init number of sound buffers
    iNewNumSndBufIn  = NUM_SOUND_BUFFERS_IN;
    iCurNumSndBufIn  = NUM_SOUND_BUFFERS_IN;
    iNewNumSndBufOut = NUM_SOUND_BUFFERS_OUT;
    iCurNumSndBufOut = NUM_SOUND_BUFFERS_OUT;
    iMinNumSndBuf    = 1;

    // should be initialized because an error can occur during init
    m_ASIOEvent = NULL;

    // get available ASIO driver names in system
    for ( int i = 0; i < MAX_NUMBER_SOUND_CARDS; i++ )
    {
        cDriverNames[i] = new char[32];
    }

    loadAsioDriver ( "dummy" ); // to initialize external object
    lNumDevs = asioDrivers->getDriverNames ( cDriverNames, MAX_NUMBER_SOUND_CARDS );

    // in case we do not have a driver available, throw error
    if ( lNumDevs == 0 )
    {
        throw CGenErr ( "No ASIO audio device (driver) found." );
    }

    asioDrivers->removeCurrentDriver();

    // init device index with illegal value to show that driver is not initialized
    lCurDev = -1;

    // set up the asioCallback structure
    asioCallbacks.bufferSwitch         = &bufferSwitch;
    asioCallbacks.sampleRateDidChange  = &sampleRateChanged;
    asioCallbacks.asioMessage          = &asioMessages;
    asioCallbacks.bufferSwitchTimeInfo = &bufferSwitchTimeInfo;

    // init buffer pointer to zero
    psCaptureBuffer = NULL;
    psPlayBuffer    = NULL;

    // we use an event controlled structure
    // create event
    m_ASIOEvent = CreateEvent ( NULL, FALSE, FALSE, NULL );

    // init flags
    bChangParamIn               = false;
    bChangParamOut              = false;
    bSetNumSndBufToMinimumValue = false;
}

CSound::~CSound()
{
    // cleanup ASIO stuff
    ASIOStop();
    ASIODisposeBuffers();
    ASIOExit();
    asioDrivers->removeCurrentDriver();

    // delete allocated memory
    if ( psCaptureBuffer != NULL )
    {
        delete[] psCaptureBuffer;
    }
    if ( psPlayBuffer != NULL )
    {
        delete[] psPlayBuffer;
    }

    // close the handle for the event
    if ( m_ASIOEvent != NULL )
    {
        CloseHandle ( m_ASIOEvent );
    }
}

// ASIO callbacks -------------------------------------------------------------
ASIOTime* CSound::bufferSwitchTimeInfo ( ASIOTime *timeInfo,
                                         long index,
                                         ASIOBool processNow )
{
    bufferSwitch ( index, processNow );
    return 0L;
}

void CSound::bufferSwitch ( long index, ASIOBool processNow )
{
    int iCurSample;

    ASIOMutex.lock(); // get mutex lock
    {
        // first check buffer state of capture and play buffers
        const int iCaptureBufferLen = iCurNumSndBufIn * iBufferSizeStereo;

        bCaptureBufferOverrun =
            ( iBufferPosCapture + 2 * iASIOBufferSizeMono > iCaptureBufferLen );

        bPlayBufferUnderrun = ( 2 * iASIOBufferSizeMono > iBufferPosPlay );

        // perform the processing for input and output
        for ( int i = 0; i < 2 * NUM_IN_OUT_CHANNELS; i++ ) // stereo
        {
            if ( bufferInfos[i].isInput == ASIOTrue )
            {
                // CAPTURE -----------------------------------------------------
                // first check if space in buffer is available
                if ( !bCaptureBufferOverrun )
                {
                    // copy new captured block in thread transfer buffer (copy
                    // mono data interleaved in stereo buffer)
                    switch ( channelInfos[i].type )
                    {
                    case ASIOSTInt16LSB:
                        for ( iCurSample = 0; iCurSample < iASIOBufferSizeMono; iCurSample++ )
                        {
                            psCaptureBuffer[iBufferPosCapture + 
                                2 * iCurSample + bufferInfos[i].channelNum] =
                                ( (short*) bufferInfos[i].buffers[index] )[iCurSample];
                        }
                        break;

                    case ASIOSTInt24LSB:

// not yet tested, horrible things might happen with the following code ;-)

                        for ( iCurSample = 0; iCurSample < iASIOBufferSizeMono; iCurSample++ )
                        {
                            // convert current sample in 16 bit format
                            int iCurSam = 0;
                            memcpy ( &iCurSam, ( (char*) bufferInfos[i].buffers[index] ) + iCurSample * 3, 3 );
                            iCurSam >>= 8;

                            psCaptureBuffer[iBufferPosCapture + 
                                2 * iCurSample + bufferInfos[i].channelNum] = static_cast<short> ( iCurSam );
                        }
                        break;

                    case ASIOSTInt32LSB:
                        for ( iCurSample = 0; iCurSample < iASIOBufferSizeMono; iCurSample++ )
                        {
                            // convert to 16 bit
                            psCaptureBuffer[iBufferPosCapture + 
                                2 * iCurSample + bufferInfos[i].channelNum] =
                                (((int*) bufferInfos[i].buffers[index])[iCurSample] >> 16);
                        }
                        break;
                    }
                }
            }
            else
            {
                // PLAYBACK ----------------------------------------------------
                if ( !bPlayBufferUnderrun )
                {
                    // copy data from sound card in output buffer (copy
                    // interleaved stereo data in mono sound card buffer)
                    switch ( channelInfos[i].type )
                    {
                    case ASIOSTInt16LSB:
                        for ( iCurSample = 0; iCurSample < iASIOBufferSizeMono; iCurSample++ )
                        {
                            ( (short*) bufferInfos[i].buffers[index] )[iCurSample] =
                                psPlayBuffer[2 * iCurSample + bufferInfos[i].channelNum];
                        }
                        break;

                    case ASIOSTInt24LSB:

// not yet tested, horrible things might happen with the following code ;-)

                        for ( iCurSample = 0; iCurSample < iASIOBufferSizeMono; iCurSample++ )
                        {
                            // convert current sample in 24 bit format
                            int iCurSam = static_cast<int> ( psPlayBuffer[2 * iCurSample + bufferInfos[i].channelNum] );
                            iCurSam <<= 8;

                            memcpy ( ( (char*) bufferInfos[i].buffers[index] ) + iCurSample * 3, &iCurSam, 3 );
                        }
                        break;

                    case ASIOSTInt32LSB:
                        for ( iCurSample = 0; iCurSample < iASIOBufferSizeMono; iCurSample++ )
                        {
                            // convert to 32 bit
                            int iCurSam = static_cast<int> ( psPlayBuffer[2 * iCurSample + bufferInfos[i].channelNum] );
                            ( (int*) bufferInfos[i].buffers[index] )[iCurSample] = ( iCurSam << 16 );
                        }
                        break;
                    }
                }
            }
        }


        // Manage thread interface buffers for input and output ----------------
        // capture
        if ( !bCaptureBufferOverrun )
        {
            iBufferPosCapture += 2 * iASIOBufferSizeMono;
        }

        // play
        if ( !bPlayBufferUnderrun )
        {
            // move all other data in play buffer
            const int iLenCopyRegion = iBufferPosPlay - 2 * iASIOBufferSizeMono;
            for ( iCurSample = 0; iCurSample < iLenCopyRegion; iCurSample++ )
            {
                psPlayBuffer[iCurSample] =
                    psPlayBuffer[2 * iASIOBufferSizeMono + iCurSample];
            }

            // adjust "current block to write" pointer
            iBufferPosPlay -= 2 * iASIOBufferSizeMono;
        }


        // finally if the driver supports the ASIOOutputReady() optimization,
        // do it here, all data are in place -----------------------------------
        if ( bASIOPostOutput )
        {
            ASIOOutputReady();
        }

        // set event
        SetEvent ( m_ASIOEvent );
    }
    ASIOMutex.unlock();
}

long CSound::asioMessages ( long selector, long value, void* message, double* opt )
{
    long ret = 0;
    switch(selector)
    {
        case kAsioEngineVersion:
            // return the supported ASIO version of the host application
            ret = 2L;
            break;
    }
    return ret;
}
