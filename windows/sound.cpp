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
// external references
extern AsioDrivers* asioDrivers;
bool   loadAsioDriver ( char *name );

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

CVector<short>   vecsTmpAudioSndCrdStereo;

QMutex           ASIOMutex;

// TEST
CSound* pSound;


/******************************************************************************\
* Common                                                                       *
\******************************************************************************/
void CSound::SetDev ( const int iNewDev )
{
    // check if an ASIO driver was already initialized
    if ( lCurDev >= 0 )
    {
        // a device was already been initialized and is used, kill working
        // thread and clean up
        // stop driver
        ASIOStop();

        // dispose ASIO buffers
        ASIODisposeBuffers();

        // remove old driver
        ASIOExit();
        asioDrivers->removeCurrentDriver();

        const std::string strErrorMessage = LoadAndInitializeDriver ( iNewDev );

        if ( !strErrorMessage.empty() )
        {
            // loading and initializing the new driver failed, go back to original
            // driver and display error message
            LoadAndInitializeDriver ( lCurDev );
            Init ( iBufferSizeStereo );

            throw CGenErr ( strErrorMessage.c_str() );
        }
        Init ( iBufferSizeStereo );
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

/*
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
*/

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

void CSound::Init ( const int iNewStereoBufferSize )
{
    // first, stop audio and dispose ASIO buffers
    ASIOStop();

    ASIOMutex.lock(); // get mutex lock
    {
        // init base clasee
        CSoundBase::Init ( iNewStereoBufferSize );

        // set internal buffer size value and calculate mono buffer size
        iBufferSizeStereo = iNewStereoBufferSize;
        iBufferSizeMono   = iBufferSizeStereo / 2;

// TEST
PrepareDriver();

        // create memory for intermediate audio buffer
        vecsTmpAudioSndCrdStereo.Init ( iBufferSizeStereo );
    }
    ASIOMutex.unlock();

    // initialization is done, (re)start audio
    ASIOStart();
}

void CSound::Close()
{
    // stop driver
    ASIOStop();
}

CSound::CSound ( void (*fpNewCallback) ( CVector<short>& psData, void* arg ), void* arg ) :
    CSoundBase ( true, fpNewCallback, arg )
{

// TEST
pSound = this;


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
}

CSound::~CSound()
{
    // cleanup ASIO stuff
    ASIOStop();
    ASIODisposeBuffers();
    ASIOExit();
    asioDrivers->removeCurrentDriver();
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
        // perform the processing for input and output
        for ( int i = 0; i < 2 * NUM_IN_OUT_CHANNELS; i++ ) // stereo
        {
            if ( bufferInfos[i].isInput == ASIOTrue )
            {
                // CAPTURE -----------------------------------------------------
                // copy new captured block in thread transfer buffer (copy
                // mono data interleaved in stereo buffer)
                switch ( channelInfos[i].type )
                {
                case ASIOSTInt16LSB:
                    for ( iCurSample = 0; iCurSample < iASIOBufferSizeMono; iCurSample++ )
                    {
                        vecsTmpAudioSndCrdStereo[2 * iCurSample + bufferInfos[i].channelNum] =
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

                        vecsTmpAudioSndCrdStereo[2 * iCurSample + bufferInfos[i].channelNum] =
                            static_cast<short> ( iCurSam );
                    }
                    break;

                case ASIOSTInt32LSB:
                    for ( iCurSample = 0; iCurSample < iASIOBufferSizeMono; iCurSample++ )
                    {
                        // convert to 16 bit
                        vecsTmpAudioSndCrdStereo[2 * iCurSample + bufferInfos[i].channelNum] =
                            (((int*) bufferInfos[i].buffers[index])[iCurSample] >> 16);
                    }
                    break;
                }
            }
        }

        // call processing callback function
        pSound->Callback( vecsTmpAudioSndCrdStereo );

        // perform the processing for input and output
        for ( int i = 0; i < 2 * NUM_IN_OUT_CHANNELS; i++ ) // stereo
        {
            if ( bufferInfos[i].isInput != ASIOTrue )
            {
                // PLAYBACK ----------------------------------------------------
                // copy data from sound card in output buffer (copy
                // interleaved stereo data in mono sound card buffer)
                switch ( channelInfos[i].type )
                {
                case ASIOSTInt16LSB:
                    for ( iCurSample = 0; iCurSample < iASIOBufferSizeMono; iCurSample++ )
                    {
                        ( (short*) bufferInfos[i].buffers[index] )[iCurSample] =
                            vecsTmpAudioSndCrdStereo[2 * iCurSample + bufferInfos[i].channelNum];
                    }
                    break;

                case ASIOSTInt24LSB:

// not yet tested, horrible things might happen with the following code ;-)

                    for ( iCurSample = 0; iCurSample < iASIOBufferSizeMono; iCurSample++ )
                    {
                        // convert current sample in 24 bit format
                        int iCurSam = static_cast<int> ( vecsTmpAudioSndCrdStereo[2 * iCurSample + bufferInfos[i].channelNum] );
                        iCurSam <<= 8;

                        memcpy ( ( (char*) bufferInfos[i].buffers[index] ) + iCurSample * 3, &iCurSam, 3 );
                    }
                    break;

                case ASIOSTInt32LSB:
                    for ( iCurSample = 0; iCurSample < iASIOBufferSizeMono; iCurSample++ )
                    {
                        // convert to 32 bit
                        int iCurSam = static_cast<int> ( vecsTmpAudioSndCrdStereo[2 * iCurSample + bufferInfos[i].channelNum] );
                        ( (int*) bufferInfos[i].buffers[index] )[iCurSample] = ( iCurSam << 16 );
                    }
                    break;
                }
            }
        }


        // finally if the driver supports the ASIOOutputReady() optimization,
        // do it here, all data are in place -----------------------------------
        if ( bASIOPostOutput )
        {
            ASIOOutputReady();
        }
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
