/******************************************************************************\
 * Copyright (c) 2004-2010
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
int              iASIOBufferSizeMono;
int              iASIOBufferSizeStereo;

CVector<int16_t>   vecsTmpAudioSndCrdStereo;

QMutex           ASIOMutex;

// TEST
CSound* pSound;


/******************************************************************************\
* Common                                                                       *
\******************************************************************************/
QString CSound::SetDev ( const int iNewDev )
{
    QString strReturn = ""; // init with no error
    bool    bTryLoadAnyDriver = false;

    // check if an ASIO driver was already initialized
    if ( lCurDev >= 0 )
    {
        // a device was already been initialized and is used, first clean up
        // dispose ASIO buffers
        ASIODisposeBuffers();

        // remove old driver
        ASIOExit();
        asioDrivers->removeCurrentDriver();

        const QString strErrorMessage = LoadAndInitializeDriver ( iNewDev );

        if ( !strErrorMessage.isEmpty() )
        {
            if ( iNewDev != lCurDev )
            {
                // loading and initializing the new driver failed, go back to
                // original driver and display error message
                LoadAndInitializeDriver ( lCurDev );
            }
            else
            {
                // the same driver is used but the driver properties seems to
                // have changed so that they are not compatible to our
                // software anymore
                QMessageBox::critical (
                    0, APP_NAME, QString ( tr ( "The audio driver properties "
                    "have changed to a state which is incompatible to this "
                    "software. The selected audio device could not be used "
                    "because of the following error: <b>" ) ) +
                    strErrorMessage +
                    QString ( tr ( "</b><br><br>Please restart the software." ) ),
                    "Close", 0 );

                _exit ( 0 );
            }

            // store error return message
            strReturn = strErrorMessage;
        }

        Init ( iASIOBufferSizeStereo );
    }
    else
    {
        if ( iNewDev != INVALID_SNC_CARD_DEVICE )
        {
            // This is the first time a driver is to be initialized, we first
            // try to load the selected driver, if this fails, we try to load
            // the first available driver in the system. If this fails, too, we
            // throw an error that no driver is available -> it does not make
            // sense to start the llcon software if no audio hardware is
            // available
            if ( !LoadAndInitializeDriver ( iNewDev ).isEmpty() )
            {
                // loading and initializing the new driver failed, try to find
                // at least one usable driver
                bTryLoadAnyDriver = true;
            }
        }
        else
        {
            // try to find one usable driver (select the first valid driver)
            bTryLoadAnyDriver = true;
        }
    }

    if ( bTryLoadAnyDriver )
    {
        // try to load and initialize any valid driver
        QVector<QString> vsErrorList =
            LoadAndInitializeFirstValidDriver();

        if ( !vsErrorList.isEmpty() )
        {
            // create error message with all details
            QString sErrorMessage = tr ( "<b>No usable ASIO audio device "
                "(driver) found.</b><br><br>"
                "In the following there is a list of all available drivers "
                "with the associated error message:<ul>" );

            for ( int i = 0; i < lNumDevs; i++ )
            {
                sErrorMessage += "<li><b>" + GetDeviceName ( i ) + "</b>: " +
                    vsErrorList[i] + "</li>";
            }
            sErrorMessage += "</ul>";

            throw CGenErr ( sErrorMessage );
        }
    }

    return strReturn;
}

QVector<QString> CSound::LoadAndInitializeFirstValidDriver()
{
    QVector<QString> vsErrorList;

    // load and initialize first valid ASIO driver
    bool bValidDriverDetected = false;
    int  iCurDriverIdx = 0;

    // try all available drivers in the system ("lNumDevs" devices)
    while ( !bValidDriverDetected && ( iCurDriverIdx < lNumDevs ) )
    {
        // try to load and initialize current driver, store error message
        const QString strCurError =
            LoadAndInitializeDriver ( iCurDriverIdx );

        vsErrorList.append ( strCurError );

        if ( strCurError.isEmpty() )
        {
            // initialization was successful
            bValidDriverDetected = true;

            // store ID of selected driver
            lCurDev = iCurDriverIdx;

            // empty error list shows that init was successful
            vsErrorList.clear();
        }

        // try next driver
        iCurDriverIdx++;
    }

    return vsErrorList;
}

QString CSound::LoadAndInitializeDriver ( int iDriverIdx )
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
        return tr ( "The audio driver could not be initialized." );
    }

    // check device capabilities if it fullfills our requirements
    const QString strStat = CheckDeviceCapabilities();

    // store ID of selected driver if initialization was successful
    if ( strStat.isEmpty() )
    {
        lCurDev = iDriverIdx;
    }
    else
    {
        // driver cannot be used, clean up
        asioDrivers->removeCurrentDriver();
    }

    return strStat;
}

QString CSound::CheckDeviceCapabilities()
{
    // This function checks if our required input/output channel
    // properties are supported by the selected device. If the return
    // string is empty, the device can be used, otherwise the error
    // message is returned.

    // check the sample rate
    const ASIOError CanSaRateReturn = ASIOCanSampleRate ( SYSTEM_SAMPLE_RATE );
    if ( ( CanSaRateReturn == ASE_NoClock ) ||
         ( CanSaRateReturn == ASE_NotPresent ) )
    {
        // return error string
        return tr ( "The audio device does not support the "
            "required sample rate. The required sample rate is: " ) +
            QString().setNum ( SYSTEM_SAMPLE_RATE ) + " Hz";
    }

    // check the number of available channels
    long lNumInChan;
    long lNumOutChan;
    ASIOGetChannels ( &lNumInChan, &lNumOutChan );
    if ( ( lNumInChan < NUM_IN_OUT_CHANNELS ) ||
         ( lNumOutChan < NUM_IN_OUT_CHANNELS ) )
    {
        // return error string
        return tr ( "The audio device does not support the "
            "required number of channels. The required number of channels "
            "is: " ) + QString().setNum ( NUM_IN_OUT_CHANNELS );
    }

    // check sample format
    for ( int i = 0; i < 2 * NUM_IN_OUT_CHANNELS; i++ )
    {
        // check all used input and output channels
        channelInfos[i].channel = i % NUM_IN_OUT_CHANNELS;
        if ( i < NUM_IN_OUT_CHANNELS )
        {
            channelInfos[i].isInput = ASIOTrue;
        }
        else
        {
            channelInfos[i].isInput = ASIOFalse;
        }
        ASIOGetChannelInfo ( &channelInfos[i] );

        // check supported sample formats
        if ( ( channelInfos[i].type != ASIOSTInt16LSB ) &&
             ( channelInfos[i].type != ASIOSTInt24LSB ) &&
             ( channelInfos[i].type != ASIOSTInt32LSB ) &&
             ( channelInfos[i].type != ASIOSTFloat32LSB ) &&
             ( channelInfos[i].type != ASIOSTFloat64LSB ) &&
             ( channelInfos[i].type != ASIOSTInt32LSB16 ) &&
             ( channelInfos[i].type != ASIOSTInt32LSB18 ) &&
             ( channelInfos[i].type != ASIOSTInt32LSB20 ) &&
             ( channelInfos[i].type != ASIOSTInt32LSB24 ) &&
             ( channelInfos[i].type != ASIOSTInt16MSB ) &&
             ( channelInfos[i].type != ASIOSTInt24MSB ) &&
             ( channelInfos[i].type != ASIOSTInt32MSB ) &&
             ( channelInfos[i].type != ASIOSTFloat32MSB ) &&
             ( channelInfos[i].type != ASIOSTFloat64MSB ) &&
             ( channelInfos[i].type != ASIOSTInt32MSB16 ) &&
             ( channelInfos[i].type != ASIOSTInt32MSB18 ) &&
             ( channelInfos[i].type != ASIOSTInt32MSB20 ) &&
             ( channelInfos[i].type != ASIOSTInt32MSB24 ) )
        {
            // return error string
            return tr ( "Required audio sample format not available." );
        }
    }

    // everything is ok, return empty string for "no error" case
    return "";
}

int CSound::GetActualBufferSize ( const int iDesiredBufferSizeMono )
{
    int iActualBufferSizeMono;

    // query the usable buffer sizes
    ASIOGetBufferSize ( &HWBufferInfo.lMinSize,
                        &HWBufferInfo.lMaxSize,
                        &HWBufferInfo.lPreferredSize,
                        &HWBufferInfo.lGranularity );

    // calculate "nearest" buffer size and set internal parameter accordingly
    // first check minimum and maximum values
    if ( iDesiredBufferSizeMono <= HWBufferInfo.lMinSize )
    {
        iActualBufferSizeMono = HWBufferInfo.lMinSize;
    }
    else
    {
        if ( iDesiredBufferSizeMono >= HWBufferInfo.lMaxSize )
        {
            iActualBufferSizeMono = HWBufferInfo.lMaxSize;
        }
        else
        {
            // ASIO SDK 2.2: "Notes: When minimum and maximum buffer size are 
            // equal, the preferred buffer size has to be the same value as
            // well; granularity should be 0 in this case."
            if ( HWBufferInfo.lMinSize == HWBufferInfo.lMaxSize )
            {
                iActualBufferSizeMono = HWBufferInfo.lMinSize;
            }
            else
            {
                // General case ------------------------------------------------
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
                iActualBufferSizeMono = iTrialBufSize;
            }
        }
    }

    return iActualBufferSizeMono;
}

int CSound::Init ( const int iNewPrefMonoBufferSize )
{
    ASIOMutex.lock(); // get mutex lock
    {
        // get the actual sound card buffer size which is supported
        // by the audio hardware
        iASIOBufferSizeMono =
            GetActualBufferSize ( iNewPrefMonoBufferSize );

        // init base clasee
        CSoundBase::Init ( iASIOBufferSizeMono );

        // set internal buffer size value and calculate stereo buffer size
        iASIOBufferSizeStereo = 2 * iASIOBufferSizeMono;

        // set the sample rate
        ASIOSetSampleRate ( SYSTEM_SAMPLE_RATE );

        // create memory for intermediate audio buffer
        vecsTmpAudioSndCrdStereo.Init ( iASIOBufferSizeStereo );

        // create and activate ASIO buffers (buffer size in samples),
        // dispose old buffers (if any)
        ASIODisposeBuffers();
        ASIOCreateBuffers ( bufferInfos,
            2 /* in/out */ * NUM_IN_OUT_CHANNELS /* stereo */,
            iASIOBufferSizeMono, &asioCallbacks );

        // check wether the driver requires the ASIOOutputReady() optimization
        // (can be used by the driver to reduce output latency by one block)
        bASIOPostOutput = ( ASIOOutputReady() == ASE_OK );
    }
    ASIOMutex.unlock();

    return iASIOBufferSizeMono;
}

void CSound::Start()
{
    // start audio
    ASIOStart();

    // call base class
    CSoundBase::Start();
}

void CSound::Stop()
{
    // stop audio
    ASIOStop();

    // call base class
    CSoundBase::Stop();
}

CSound::CSound ( void (*fpNewCallback) ( CVector<int16_t>& psData, void* arg ), void* arg ) :
    CSoundBase ( true, fpNewCallback, arg )
{
    int i;

// TEST
pSound = this;


    // get available ASIO driver names in system
    for ( i = 0; i < MAX_NUMBER_SOUND_CARDS; i++ )
    {
        cDriverNames[i] = new char[32];
    }

    loadAsioDriver ( "dummy" ); // to initialize external object
    lNumDevs = asioDrivers->getDriverNames ( cDriverNames, MAX_NUMBER_SOUND_CARDS );

    // in case we do not have a driver available, throw error
    if ( lNumDevs == 0 )
    {
        throw CGenErr ( tr ( "<b>No ASIO audio device (driver) found.</b><br><br>"
            "The " ) + APP_NAME + tr ( " software requires the low latency audio "
            "interface <b>ASIO</b> to work properly. This is no standard "
            "Windows audio interface and therefore a special audio driver is "
            "required. Either your sound card has a native ASIO driver (which "
            "is recommended) or you might want to use alternative drivers like "
            "the ASIO4All or kX driver." ) );
    }
    asioDrivers->removeCurrentDriver();

    // init device index with illegal value to show that driver is not initialized
    lCurDev = -1;

    // init buffer infos, we always want to have two input and
    // two output channels
    for ( i = 0; i < NUM_IN_OUT_CHANNELS; i++ )
    {
        // prepare input channels
        bufferInfos[i].isInput    = ASIOTrue;
        bufferInfos[i].channelNum = i;
        bufferInfos[i].buffers[0] = 0;
        bufferInfos[i].buffers[1] = 0;

        // prepare output channels
        bufferInfos[NUM_IN_OUT_CHANNELS + i].isInput    = ASIOFalse;
        bufferInfos[NUM_IN_OUT_CHANNELS + i].channelNum = i;
        bufferInfos[NUM_IN_OUT_CHANNELS + i].buffers[0] = 0;
        bufferInfos[NUM_IN_OUT_CHANNELS + i].buffers[1] = 0;
    }

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
ASIOTime* CSound::bufferSwitchTimeInfo ( ASIOTime* timeInfo,
                                         long      index,
                                         ASIOBool  processNow )
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
                    // no type conversion required, just copy operation
                    for ( iCurSample = 0; iCurSample < iASIOBufferSizeMono; iCurSample++ )
                    {
                        vecsTmpAudioSndCrdStereo[2 * iCurSample + bufferInfos[i].channelNum] =
                            static_cast<int16_t*> ( bufferInfos[i].buffers[index] )[iCurSample];
                    }
                    break;

                case ASIOSTInt24LSB:
// NOT YET TESTED
                    for ( iCurSample = 0; iCurSample < iASIOBufferSizeMono; iCurSample++ )
                    {
                        int iCurSam = 0;
                        memcpy ( &iCurSam, ( (char*) bufferInfos[i].buffers[index] ) + iCurSample * 3, 3 );
                        iCurSam >>= 8;

                        vecsTmpAudioSndCrdStereo[2 * iCurSample + bufferInfos[i].channelNum] =
                            static_cast<int16_t> ( iCurSam );
                    }
                    break;

                case ASIOSTInt32LSB:
// NOT YET TESTED
                    for ( iCurSample = 0; iCurSample < iASIOBufferSizeMono; iCurSample++ )
                    {
                        vecsTmpAudioSndCrdStereo[2 * iCurSample + bufferInfos[i].channelNum] =
                            static_cast<int16_t> ( static_cast<int32_t*> (
                            bufferInfos[i].buffers[index] )[iCurSample] >> 16 );
                    }
                    break;

                case ASIOSTFloat32LSB: // IEEE 754 32 bit float, as found on Intel x86 architecture
// NOT YET TESTED
                    for ( iCurSample = 0; iCurSample < iASIOBufferSizeMono; iCurSample++ )
                    {
                        vecsTmpAudioSndCrdStereo[2 * iCurSample + bufferInfos[i].channelNum] =
                            static_cast<int16_t> ( static_cast<float*> (
                            bufferInfos[i].buffers[index] )[iCurSample] * _MAXSHORT );
                    }
	                break;

                case ASIOSTFloat64LSB: // IEEE 754 64 bit double float, as found on Intel x86 architecture
// NOT YET TESTED
                    for ( iCurSample = 0; iCurSample < iASIOBufferSizeMono; iCurSample++ )
                    {
                        vecsTmpAudioSndCrdStereo[2 * iCurSample + bufferInfos[i].channelNum] =
                            static_cast<int16_t> ( static_cast<double*> (
                            bufferInfos[i].buffers[index] )[iCurSample] * _MAXSHORT );
                    }
	                break;

		        case ASIOSTInt32LSB16: // 32 bit data with 16 bit alignment
// NOT YET TESTED
                    for ( iCurSample = 0; iCurSample < iASIOBufferSizeMono; iCurSample++ )
                    {
                        vecsTmpAudioSndCrdStereo[2 * iCurSample + bufferInfos[i].channelNum] =
                            static_cast<int16_t> ( static_cast<int32_t*> (
                            bufferInfos[i].buffers[index] )[iCurSample] & 0xFFFF );
                    }
	                break;

		        case ASIOSTInt32LSB18: // 32 bit data with 18 bit alignment
// NOT YET TESTED
                    for ( iCurSample = 0; iCurSample < iASIOBufferSizeMono; iCurSample++ )
                    {
                        vecsTmpAudioSndCrdStereo[2 * iCurSample + bufferInfos[i].channelNum] =
                            static_cast<int16_t> ( ( static_cast<int32_t*> (
                            bufferInfos[i].buffers[index] )[iCurSample] & 0x3FFFF ) >> 2 );
                    }
	                break;

		        case ASIOSTInt32LSB20: // 32 bit data with 20 bit alignment
// NOT YET TESTED
                    for ( iCurSample = 0; iCurSample < iASIOBufferSizeMono; iCurSample++ )
                    {
                        vecsTmpAudioSndCrdStereo[2 * iCurSample + bufferInfos[i].channelNum] =
                            static_cast<int16_t> ( ( static_cast<int32_t*> (
                            bufferInfos[i].buffers[index] )[iCurSample] & 0xFFFFF ) >> 4 );
                    }
	                break;

		        case ASIOSTInt32LSB24: // 32 bit data with 24 bit alignment
// NOT YET TESTED
                    for ( iCurSample = 0; iCurSample < iASIOBufferSizeMono; iCurSample++ )
                    {
                        vecsTmpAudioSndCrdStereo[2 * iCurSample + bufferInfos[i].channelNum] =
                            static_cast<int16_t> ( ( static_cast<int32_t*> (
                            bufferInfos[i].buffers[index] )[iCurSample] & 0xFFFFFF ) >> 8 );
                    }
	                break;

                case ASIOSTInt16MSB:
// NOT YET TESTED
                    // flip bits
                    for ( iCurSample = 0; iCurSample < iASIOBufferSizeMono; iCurSample++ )
                    {
                        vecsTmpAudioSndCrdStereo[2 * iCurSample + bufferInfos[i].channelNum] =
                            Flip16Bits ( ( static_cast<int16_t*> (
                            bufferInfos[i].buffers[index] ) )[iCurSample] );
                    }
	                break;

                case ASIOSTInt24MSB:
// NOT YET TESTED
	                // TODO
	                break;

                case ASIOSTInt32MSB:
// NOT YET TESTED
                    for ( iCurSample = 0; iCurSample < iASIOBufferSizeMono; iCurSample++ )
                    {
                        // flip bits and convert to 16 bit
                        vecsTmpAudioSndCrdStereo[2 * iCurSample + bufferInfos[i].channelNum] =
                            static_cast<int16_t> ( Flip32Bits ( static_cast<int32_t*> (
                            bufferInfos[i].buffers[index] )[iCurSample] ) >> 16 );
                    }
	                break;

                case ASIOSTFloat32MSB: // IEEE 754 32 bit float, as found on Intel x86 architecture
// NOT YET TESTED
	                // TODO
	                break;

                case ASIOSTFloat64MSB: // IEEE 754 64 bit double float, as found on Intel x86 architecture
// NOT YET TESTED
	                // TODO
	                break;

                case ASIOSTInt32MSB16: // 32 bit data with 16 bit alignment
// NOT YET TESTED
	                // TODO
	                break;

                case ASIOSTInt32MSB18: // 32 bit data with 18 bit alignment
// NOT YET TESTED
	                // TODO
	                break;

                case ASIOSTInt32MSB20: // 32 bit data with 20 bit alignment
// NOT YET TESTED
	                // TODO
	                break;

                case ASIOSTInt32MSB24: // 32 bit data with 24 bit alignment
// NOT YET TESTED
	                // TODO
	                break;
                }
            }
        }

        // call processing callback function
        pSound->ProcessCallback ( vecsTmpAudioSndCrdStereo );

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
                    // no type conversion required, just copy operation
                    for ( iCurSample = 0; iCurSample < iASIOBufferSizeMono; iCurSample++ )
                    {
                        static_cast<int16_t*> ( bufferInfos[i].buffers[index] )[iCurSample] =
                            vecsTmpAudioSndCrdStereo[2 * iCurSample + bufferInfos[i].channelNum];
                    }
                    break;

                case ASIOSTInt24LSB:
// NOT YET TESTED
                    for ( iCurSample = 0; iCurSample < iASIOBufferSizeMono; iCurSample++ )
                    {
                        // convert current sample in 24 bit format
                        int32_t iCurSam = static_cast<int32_t> (
                            vecsTmpAudioSndCrdStereo[2 * iCurSample + bufferInfos[i].channelNum] );

                        iCurSam <<= 8;

                        memcpy ( ( (char*) bufferInfos[i].buffers[index] ) + iCurSample * 3, &iCurSam, 3 );
                    }
                    break;

                case ASIOSTInt32LSB:
// NOT YET TESTED
                    for ( iCurSample = 0; iCurSample < iASIOBufferSizeMono; iCurSample++ )
                    {
                        // convert to 32 bit
                        const int32_t iCurSam = static_cast<int32_t> (
                            vecsTmpAudioSndCrdStereo[2 * iCurSample + bufferInfos[i].channelNum] );

                        static_cast<int32_t*> ( bufferInfos[i].buffers[index] )[iCurSample] =
                            ( iCurSam << 16 );
                    }
                    break;

                case ASIOSTFloat32LSB: // IEEE 754 32 bit float, as found on Intel x86 architecture
// NOT YET TESTED
                    for ( iCurSample = 0; iCurSample < iASIOBufferSizeMono; iCurSample++ )
                    {
                        const float fCurSam = static_cast<float> (
                            vecsTmpAudioSndCrdStereo[2 * iCurSample + bufferInfos[i].channelNum] );

                        static_cast<float*> ( bufferInfos[i].buffers[index] )[iCurSample] =
                            fCurSam / _MAXSHORT;
                    }
	                break;

                case ASIOSTFloat64LSB: // IEEE 754 64 bit double float, as found on Intel x86 architecture
// NOT YET TESTED
                    for ( iCurSample = 0; iCurSample < iASIOBufferSizeMono; iCurSample++ )
                    {
                        const double fCurSam = static_cast<double> (
                            vecsTmpAudioSndCrdStereo[2 * iCurSample + bufferInfos[i].channelNum] );

                        static_cast<double*> ( bufferInfos[i].buffers[index] )[iCurSample] =
                            fCurSam / _MAXSHORT;
                    }
	                break;

		        case ASIOSTInt32LSB16: // 32 bit data with 16 bit alignment
// NOT YET TESTED
                    for ( iCurSample = 0; iCurSample < iASIOBufferSizeMono; iCurSample++ )
                    {
                        // convert to 32 bit
                        const int32_t iCurSam = static_cast<int32_t> (
                            vecsTmpAudioSndCrdStereo[2 * iCurSample + bufferInfos[i].channelNum] );

                        static_cast<int32_t*> ( bufferInfos[i].buffers[index] )[iCurSample] =
                            iCurSam;
                    }
	                break;

		        case ASIOSTInt32LSB18: // 32 bit data with 18 bit alignment
// NOT YET TESTED
                    for ( iCurSample = 0; iCurSample < iASIOBufferSizeMono; iCurSample++ )
                    {
                        // convert to 32 bit
                        const int32_t iCurSam = static_cast<int32_t> (
                            vecsTmpAudioSndCrdStereo[2 * iCurSample + bufferInfos[i].channelNum] );

                        static_cast<int32_t*> ( bufferInfos[i].buffers[index] )[iCurSample] =
                            ( iCurSam << 2 );
                    }
	                break;

		        case ASIOSTInt32LSB20: // 32 bit data with 20 bit alignment
// NOT YET TESTED
                    for ( iCurSample = 0; iCurSample < iASIOBufferSizeMono; iCurSample++ )
                    {
                        // convert to 32 bit
                        const int32_t iCurSam = static_cast<int32_t> (
                            vecsTmpAudioSndCrdStereo[2 * iCurSample + bufferInfos[i].channelNum] );

                        static_cast<int32_t*> ( bufferInfos[i].buffers[index] )[iCurSample] =
                            ( iCurSam << 4 );
                    }
	                break;

		        case ASIOSTInt32LSB24: // 32 bit data with 24 bit alignment
// NOT YET TESTED
                    for ( iCurSample = 0; iCurSample < iASIOBufferSizeMono; iCurSample++ )
                    {
                        // convert to 32 bit
                        const int32_t iCurSam = static_cast<int32_t> (
                            vecsTmpAudioSndCrdStereo[2 * iCurSample + bufferInfos[i].channelNum] );

                        static_cast<int32_t*> ( bufferInfos[i].buffers[index] )[iCurSample] =
                            ( iCurSam << 8 );
                    }
	                break;

                case ASIOSTInt16MSB:
// NOT YET TESTED
                    // flip bits
                    for ( iCurSample = 0; iCurSample < iASIOBufferSizeMono; iCurSample++ )
                    {
                        ( (int16_t*) bufferInfos[i].buffers[index] )[iCurSample] =
                            Flip16Bits ( vecsTmpAudioSndCrdStereo[2 * iCurSample + bufferInfos[i].channelNum] );
                    }
	                break;

                case ASIOSTInt24MSB:
// NOT YET TESTED
	                // TODO
	                break;

                case ASIOSTInt32MSB:
// NOT YET TESTED
                    for ( iCurSample = 0; iCurSample < iASIOBufferSizeMono; iCurSample++ )
                    {
                        // convert to 32 bit and flip bits
                        int iCurSam = static_cast<int32_t> (
                            vecsTmpAudioSndCrdStereo[2 * iCurSample + bufferInfos[i].channelNum] );

                        static_cast<int32_t*> ( bufferInfos[i].buffers[index] )[iCurSample] =
                            Flip32Bits ( iCurSam << 16 );
                    }
	                break;

                case ASIOSTFloat32MSB: // IEEE 754 32 bit float, as found on Intel x86 architecture
// NOT YET TESTED
	                // TODO
	                break;

                case ASIOSTFloat64MSB: // IEEE 754 64 bit double float, as found on Intel x86 architecture
// NOT YET TESTED
	                // TODO
	                break;

                case ASIOSTInt32MSB16: // 32 bit data with 16 bit alignment
// NOT YET TESTED
	                // TODO
	                break;

                case ASIOSTInt32MSB18: // 32 bit data with 18 bit alignment
// NOT YET TESTED
	                // TODO
	                break;

                case ASIOSTInt32MSB20: // 32 bit data with 20 bit alignment
// NOT YET TESTED
	                // TODO
	                break;

                case ASIOSTInt32MSB24: // 32 bit data with 24 bit alignment
// NOT YET TESTED
	                // TODO
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

long CSound::asioMessages ( long    selector,
                            long    value,
                            void*   message,
                            double* opt )
{
    long ret = 0;
    switch ( selector )
    {
        case kAsioEngineVersion:
            // return the supported ASIO version of the host application
            ret = 2L; // Host ASIO implementation version, 2 or higher
            break;

        // both messages might be send if the buffer size changes
        case kAsioBufferSizeChange:
        case kAsioResetRequest:
            pSound->EmitReinitRequestSignal();
            ret = 1L; // 1L if request is accepted or 0 otherwise
            break;
    }
    return ret;
}

int16_t CSound::Flip16Bits ( const int16_t iIn )
{
    uint16_t iMask = ( 1 << 15 );
    int16_t  iOut  = 0;

    for ( unsigned int i = 0; i < 16; i++ )
    {
        // copy current bit to correct position
        iOut |= ( iIn & iMask ) ? 1 : 0;

        // shift out value and mask by one bit
        iOut  <<= 1;
        iMask >>= 1;
    }

    return iOut;
}

int32_t CSound::Flip32Bits ( const int32_t iIn )
{
    uint32_t iMask = ( static_cast<uint32_t> ( 1 ) << 31 );
    int32_t  iOut  = 0;

    for ( unsigned int i = 0; i < 32; i++ )
    {
        // copy current bit to correct position
        iOut |= ( iIn & iMask ) ? 1 : 0;

        // shift out value and mask by one bit
        iOut  <<= 1;
        iMask >>= 1;
    }

    return iOut;
}

int64_t CSound::Flip64Bits ( const int64_t iIn )
{
    uint64_t iMask = ( static_cast<uint64_t> ( 1 ) << 63 );
    int64_t  iOut  = 0;

    for ( unsigned int i = 0; i < 64; i++ )
    {
        // copy current bit to correct position
        iOut |= ( iIn & iMask ) ? 1 : 0;

        // shift out value and mask by one bit
        iOut  <<= 1;
        iMask >>= 1;
    }

    return iOut;
}
