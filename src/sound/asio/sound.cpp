/******************************************************************************\
 * Copyright (c) 2004-2025
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
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 *
\******************************************************************************/

#include "sound.h"

/* Implementation *************************************************************/
// external references
extern AsioDrivers* asioDrivers;
bool                loadAsioDriver ( char* name );

// pointer to our sound object
CSound* pSound;

/******************************************************************************\
* Common                                                                       *
\******************************************************************************/
QString CSound::LoadAndInitializeDriver ( QString strDriverName, bool bOpenDriverSetup )
{
    // find and load driver
    int iDriverIdx = INVALID_INDEX; // initialize with an invalid index

    for ( int i = 0; i < MAX_NUMBER_SOUND_CARDS; i++ )
    {
        if ( strDriverName.compare ( cDriverNames[i] ) == 0 )
        {
            iDriverIdx = i;
        }
    }

    // if the selected driver was not found, return an error message
    if ( iDriverIdx == INVALID_INDEX )
    {
        return tr ( "The selected audio device is no longer present in the system. Please check your audio device." );
    }

    // Save number of channels from last driver
    // Need to save these (but not the driver name) as CheckDeviceCapabilities() overwrites them
    long lNumInChanPrev  = lNumInChan;
    long lNumOutChanPrev = lNumOutChan;

    loadAsioDriver ( cDriverNames[iDriverIdx] );

    // According to the docs, driverInfo.asioVersion and driverInfo.sysRef
    // should be set, but we haven't being doing that and it seems to work
    // okay...
    memset ( &driverInfo, 0, sizeof driverInfo );

    if ( ASIOInit ( &driverInfo ) != ASE_OK )
    {
        // clean up and return error string
        asioDrivers->removeCurrentDriver();
        return tr ( "Couldn't initialise the audio driver. Check if your audio hardware is plugged in and verify your driver settings." );
    }

    // check device capabilities if it fulfills our requirements
    const QString strStat = CheckDeviceCapabilities(); // also sets lNumInChan and lNumOutChan

    // check if device is capable
    if ( strStat.isEmpty() )
    {
        // Reset channel mapping if the sound card name has changed or the number of channels has changed
        if ( ( strCurDevName.compare ( strDriverNames[iDriverIdx] ) != 0 ) || ( lNumInChanPrev != lNumInChan ) || ( lNumOutChanPrev != lNumOutChan ) )
        {
            // In order to fix https://github.com/jamulussoftware/jamulus/issues/796
            // this code runs after a change in the ASIO driver (not when changing the ASIO input selection.)

            // mapping to the defaults (first two available channels)
            ResetChannelMapping();

            // store ID of selected driver if initialization was successful
            strCurDevName = cDriverNames[iDriverIdx];
        }
    }
    else
    {
        // if requested, open ASIO driver setup in case of an error
        if ( bOpenDriverSetup )
        {
            OpenDriverSetup();
            QMessageBox::question ( nullptr,
                                    APP_NAME,
                                    "Are you done with your ASIO driver settings of " + GetDeviceName ( iDriverIdx ) + "?",
                                    QMessageBox::Yes );
        }

        // driver cannot be used, clean up
        asioDrivers->removeCurrentDriver();
    }

    return strStat;
}

void CSound::UnloadCurrentDriver()
{
    // clean up ASIO stuff
    if ( bRun )
    {
        Stop();
    }
    if ( bufferInfos[0].buffers[0] )
    {
        ASIODisposeBuffers();
        bufferInfos[0].buffers[0] = NULL;
    }
    ASIOExit();
    asioDrivers->removeCurrentDriver();
}

QString CSound::CheckDeviceCapabilities()
{
    // This function checks if our required input/output channel
    // properties are supported by the selected device. If the return
    // string is empty, the device can be used, otherwise the error
    // message is returned.

    // check the sample rate
    const ASIOError CanSaRateReturn = ASIOCanSampleRate ( SYSTEM_SAMPLE_RATE_HZ );

    if ( ( CanSaRateReturn == ASE_NoClock ) || ( CanSaRateReturn == ASE_NotPresent ) )
    {
        // return error string
        return QString ( tr ( "The selected audio device is incompatible "
                              "since it doesn't support a sample rate of %1 Hz. Please select another "
                              "device." ) )
            .arg ( SYSTEM_SAMPLE_RATE_HZ );
    }

    // check if sample rate can be set
    const ASIOError SetSaRateReturn = ASIOSetSampleRate ( SYSTEM_SAMPLE_RATE_HZ );

    if ( ( SetSaRateReturn == ASE_NoClock ) || ( SetSaRateReturn == ASE_InvalidMode ) || ( SetSaRateReturn == ASE_NotPresent ) )
    {
        // return error string
        return QString ( tr ( "The current audio device configuration is incompatible "
                              "because the sample rate couldn't be set to %2 Hz. Please check for a hardware switch or "
                              "driver setting to set the sample rate manually and restart %1." ) )
            .arg ( APP_NAME )
            .arg ( SYSTEM_SAMPLE_RATE_HZ );
    }

    // check the number of available channels
    ASIOGetChannels ( &lNumInChan, &lNumOutChan );

    if ( ( lNumInChan < NUM_IN_OUT_CHANNELS ) || ( lNumOutChan < NUM_IN_OUT_CHANNELS ) )
    {
        // return error string
        return QString ( tr ( "The selected audio device is incompatible since it doesn't support "
                              "%1 in/out channels. Please select another device or configuration." ) )
            .arg ( NUM_IN_OUT_CHANNELS );
    }

    // clip number of input/output channels to our maximum
    if ( lNumInChan > MAX_NUM_IN_OUT_CHANNELS )
    {
        lNumInChan = MAX_NUM_IN_OUT_CHANNELS;
    }
    if ( lNumOutChan > MAX_NUM_IN_OUT_CHANNELS )
    {
        lNumOutChan = MAX_NUM_IN_OUT_CHANNELS;
    }

    // query channel infos for all available input channels
    bool bInputChMixingSupported = true;

    for ( int i = 0; i < lNumInChan; i++ )
    {
        // setup for input channels
        channelInfosInput[i].isInput = ASIOTrue;
        channelInfosInput[i].channel = i;

        ASIOGetChannelInfo ( &channelInfosInput[i] );

        // Check supported sample formats.
        // Actually, it would be enough to have at least two channels which
        // support the required sample format. But since we have support for
        // all known sample types, the following check should always pass and
        // therefore we throw the error message on any channel which does not
        // fulfill the sample format requirement (quick hack solution).
        if ( !CheckSampleTypeSupported ( channelInfosInput[i].type ) )
        {
            // return error string
            return tr ( "The selected audio device is incompatible since "
                        "the required audio sample format isn't available. Please use another device." );
        }

        // store the name of the channel and check if channel mixing is supported
        channelInputName[i] = channelInfosInput[i].name;

        if ( !CheckSampleTypeSupportedForCHMixing ( channelInfosInput[i].type ) )
        {
            bInputChMixingSupported = false;
        }
    }

    // query channel infos for all available output channels
    for ( int i = 0; i < lNumOutChan; i++ )
    {
        // setup for output channels
        channelInfosOutput[i].isInput = ASIOFalse;
        channelInfosOutput[i].channel = i;

        ASIOGetChannelInfo ( &channelInfosOutput[i] );

        // Check supported sample formats.
        // Actually, it would be enough to have at least two channels which
        // support the required sample format. But since we have support for
        // all known sample types, the following check should always pass and
        // therefore we throw the error message on any channel which does not
        // fulfill the sample format requirement (quick hack solution).
        if ( !CheckSampleTypeSupported ( channelInfosOutput[i].type ) )
        {
            // return error string
            return tr ( "The selected audio device is incompatible since "
                        "the required audio sample format isn't available. Please use another device." );
        }
    }

    // special case with 4 input channels: support adding channels
    if ( ( lNumInChan == 4 ) && bInputChMixingSupported )
    {
        // add four mixed channels (i.e. 4 normal, 4 mixed channels)
        lNumInChanPlusAddChan = 8;

        for ( int iCh = 0; iCh < lNumInChanPlusAddChan; iCh++ )
        {
            int iSelCH, iSelAddCH;

            GetSelCHAndAddCH ( iCh, lNumInChan, iSelCH, iSelAddCH );

            if ( iSelAddCH >= 0 )
            {
                // for mixed channels, show both audio channel names to be mixed
                channelInputName[iCh] = channelInputName[iSelCH] + " + " + channelInputName[iSelAddCH];
            }
        }
    }
    else
    {
        // regular case: no mixing input channels used
        lNumInChanPlusAddChan = lNumInChan;
    }

    // everything is ok, return empty string for "no error" case
    return "";
}

void CSound::SetLeftInputChannel ( const int iNewChan )
{
    // apply parameter after input parameter check
    if ( ( iNewChan >= 0 ) && ( iNewChan < lNumInChanPlusAddChan ) )
    {
        vSelectedInputChannels[0] = iNewChan;
    }
}

void CSound::SetRightInputChannel ( const int iNewChan )
{
    // apply parameter after input parameter check
    if ( ( iNewChan >= 0 ) && ( iNewChan < lNumInChanPlusAddChan ) )
    {
        vSelectedInputChannels[1] = iNewChan;
    }
}

void CSound::SetLeftOutputChannel ( const int iNewChan )
{
    // apply parameter after input parameter check
    if ( ( iNewChan >= 0 ) && ( iNewChan < lNumOutChan ) )
    {
        vSelectedOutputChannels[0] = iNewChan;
    }
}

void CSound::SetRightOutputChannel ( const int iNewChan )
{
    // apply parameter after input parameter check
    if ( ( iNewChan >= 0 ) && ( iNewChan < lNumOutChan ) )
    {
        vSelectedOutputChannels[1] = iNewChan;
    }
}

int CSound::GetActualBufferSize ( const int iDesiredBufferSizeMono )
{
    int iActualBufferSizeMono;

    // query the usable buffer sizes
    ASIOGetBufferSize ( &HWBufferInfo.lMinSize, &HWBufferInfo.lMaxSize, &HWBufferInfo.lPreferredSize, &HWBufferInfo.lGranularity );

    //### TEST: BEGIN ###//
    /*
    #include <QMessageBox>
    QMessageBox::information ( 0, "APP_NAME", QString("lMinSize: %1, lMaxSize: %2, lPreferredSize: %3, lGranularity: %4").
                               arg(HWBufferInfo.lMinSize).arg(HWBufferInfo.lMaxSize).arg(HWBufferInfo.lPreferredSize).arg(HWBufferInfo.lGranularity)
    );
    _exit(1);
    */
    //### TEST: END ###//

    //### TODO: BEGIN ###//
    // see https://github.com/EddieRingle/portaudio/blob/master/src/hostapi/asio/pa_asio.cpp#L1654
    // (SelectHostBufferSizeForUnspecifiedUserFramesPerBuffer)
    //### TODO: END ###//

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
                if ( ( HWBufferInfo.lGranularity < -1 ) || ( HWBufferInfo.lGranularity == 0 ) )
                {
                    // Special case (seen for EMU audio cards): granularity is
                    // zero or less than zero (make sure to exclude the special
                    // case of -1).
                    // There is no definition of this case in the ASIO SDK
                    // document. We assume here that all buffer sizes in between
                    // minimum and maximum buffer sizes are allowed.
                    iActualBufferSizeMono = iDesiredBufferSizeMono;
                }
                else
                {
                    // General case --------------------------------------------
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
                            if ( ( iTrialBufSize - iDesiredBufferSizeMono ) > ( iDesiredBufferSizeMono - iLastTrialBufSize ) )
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

                            // increment trial buffer size (check for special
                            // case first)
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

                    // clip trial buffer size (it may happen in the while
                    // routine that "iTrialBufSize" is larger than "lMaxSize" in
                    // case "lMaxSize - lMinSize" is not divisible by the
                    // granularity)
                    if ( iTrialBufSize > HWBufferInfo.lMaxSize )
                    {
                        iTrialBufSize = HWBufferInfo.lMaxSize;
                    }

                    // set ASIO buffer size
                    iActualBufferSizeMono = iTrialBufSize;
                }
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
        iASIOBufferSizeMono = GetActualBufferSize ( iNewPrefMonoBufferSize );

        // init base class
        CSoundBase::Init ( iASIOBufferSizeMono );

        // set internal buffer size value and calculate stereo buffer size
        iASIOBufferSizeStereo = 2 * iASIOBufferSizeMono;

        // set the sample rate
        ASIOSetSampleRate ( SYSTEM_SAMPLE_RATE_HZ );

        // create memory for intermediate audio buffer
        vecsMultChanAudioSndCrd.Init ( iASIOBufferSizeStereo );

        // create and activate ASIO buffers (buffer size in samples),
        // dispose old buffers (if any)
        ASIODisposeBuffers();

        // prepare input channels
        for ( int i = 0; i < lNumInChan; i++ )
        {
            bufferInfos[i].isInput    = ASIOTrue;
            bufferInfos[i].channelNum = i;
            bufferInfos[i].buffers[0] = 0;
            bufferInfos[i].buffers[1] = 0;
        }

        // prepare output channels
        for ( int i = 0; i < lNumOutChan; i++ )
        {
            bufferInfos[lNumInChan + i].isInput    = ASIOFalse;
            bufferInfos[lNumInChan + i].channelNum = i;
            bufferInfos[lNumInChan + i].buffers[0] = 0;
            bufferInfos[lNumInChan + i].buffers[1] = 0;
        }

        ASIOCreateBuffers ( bufferInfos, lNumInChan + lNumOutChan, iASIOBufferSizeMono, &asioCallbacks );

        // query the latency of the driver
        long lInputLatency  = 0;
        long lOutputLatency = 0;

        if ( ASIOGetLatencies ( &lInputLatency, &lOutputLatency ) != ASE_NotPresent )
        {
            // add the input and output latencies (returned in number of
            // samples) and calculate the time in ms
            fInOutLatencyMs = ( static_cast<float> ( lInputLatency ) + lOutputLatency ) * 1000 / SYSTEM_SAMPLE_RATE_HZ;
        }
        else
        {
            // no latency available
            fInOutLatencyMs = 0.0f;
        }

        // check whether the driver requires the ASIOOutputReady() optimization
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

    // make sure the working thread is actually done
    // (by checking the locked state)
    if ( ASIOMutex.tryLock ( 5000 ) )
    {
        ASIOMutex.unlock();
    }
}

CSound::CSound ( void ( *fpNewCallback ) ( CVector<int16_t>& psData, void* arg ),
                 void*          arg,
                 const QString& strMIDISetup,
                 const bool,
                 const QString& ) :
    CSoundBase ( "ASIO", fpNewCallback, arg, strMIDISetup ),
    lNumInChan ( 0 ),
    lNumInChanPlusAddChan ( 0 ),
    lNumOutChan ( 0 ),
    fInOutLatencyMs ( 0.0f ), // "0.0" means that no latency value is available
    vSelectedInputChannels ( NUM_IN_OUT_CHANNELS ),
    vSelectedOutputChannels ( NUM_IN_OUT_CHANNELS )
{
    int i;

    // init pointer to our sound object
    pSound = this;

    // We assume NULL'd pointers in this structure indicate that buffers are not
    // allocated yet (see UnloadCurrentDriver).
    memset ( bufferInfos, 0, sizeof bufferInfos );

    // get available ASIO driver names in system
    for ( i = 0; i < MAX_NUMBER_SOUND_CARDS; i++ )
    {
        // allocate memory for driver names
        cDriverNames[i] = new char[32];
    }

    char cDummyName[] = "dummy";
    loadAsioDriver ( cDummyName ); // to initialize external object
    lNumDevs = asioDrivers->getDriverNames ( cDriverNames, MAX_NUMBER_SOUND_CARDS );

    // in case we do not have a driver available, throw error
    if ( lNumDevs == 0 )
    {
        throw CGenErr ( "<b>" + tr ( "No ASIO audio device driver found." ) + "</b><br><br>" +
                        QString ( tr ( "Please install an ASIO driver before running %1. "
                                       "If you own a device with ASIO support, install its official ASIO driver. "
                                       "If not, you'll need to install a universal driver like ASIO4ALL." ) )
                            .arg ( APP_NAME ) );
    }
    asioDrivers->removeCurrentDriver();

    // copy driver names to base class but internally we still have to use
    // the char* variable because of the ASIO API :-(
    for ( i = 0; i < lNumDevs; i++ )
    {
        strDriverNames[i] = cDriverNames[i];
    }

    // init device index as not initialized (invalid)
    strCurDevName = "";

    // init channel mapping
    ResetChannelMapping();

    // set up the asioCallback structure
    asioCallbacks.bufferSwitch         = &bufferSwitch;
    asioCallbacks.sampleRateDidChange  = &sampleRateChanged;
    asioCallbacks.asioMessage          = &asioMessages;
    asioCallbacks.bufferSwitchTimeInfo = &bufferSwitchTimeInfo;

    // Optional MIDI initialization --------------------------------------------
    if ( iCtrlMIDIChannel != INVALID_MIDI_CH )
    {
        Midi.MidiStart();
    }
}

CSound::~CSound()
{
    // stop MIDI if running
    if ( iCtrlMIDIChannel != INVALID_MIDI_CH )
    {
        Midi.MidiStop();
    }

    UnloadCurrentDriver();
}

void CSound::ResetChannelMapping()
{
    // init selected channel numbers with defaults: use first available
    // channels for input and output
    vSelectedInputChannels[0]  = 0;
    vSelectedInputChannels[1]  = 1;
    vSelectedOutputChannels[0] = 0;
    vSelectedOutputChannels[1] = 1;
}

// ASIO callbacks -------------------------------------------------------------
ASIOTime* CSound::bufferSwitchTimeInfo ( ASIOTime*, long index, ASIOBool processNow )
{
    bufferSwitch ( index, processNow );
    return 0L;
}

bool CSound::CheckSampleTypeSupported ( const ASIOSampleType SamType )
{
    // check for supported sample types
    return ( ( SamType == ASIOSTInt16LSB ) || ( SamType == ASIOSTInt24LSB ) || ( SamType == ASIOSTInt32LSB ) || ( SamType == ASIOSTFloat32LSB ) ||
             ( SamType == ASIOSTFloat64LSB ) || ( SamType == ASIOSTInt32LSB16 ) || ( SamType == ASIOSTInt32LSB18 ) ||
             ( SamType == ASIOSTInt32LSB20 ) || ( SamType == ASIOSTInt32LSB24 ) || ( SamType == ASIOSTInt16MSB ) || ( SamType == ASIOSTInt24MSB ) ||
             ( SamType == ASIOSTInt32MSB ) || ( SamType == ASIOSTFloat32MSB ) || ( SamType == ASIOSTFloat64MSB ) || ( SamType == ASIOSTInt32MSB16 ) ||
             ( SamType == ASIOSTInt32MSB18 ) || ( SamType == ASIOSTInt32MSB20 ) || ( SamType == ASIOSTInt32MSB24 ) );
}

bool CSound::CheckSampleTypeSupportedForCHMixing ( const ASIOSampleType SamType )
{
    // check for supported sample types for audio channel mixing (see bufferSwitch)
    return ( ( SamType == ASIOSTInt16LSB ) || ( SamType == ASIOSTInt24LSB ) || ( SamType == ASIOSTInt32LSB ) );
}

void CSound::bufferSwitch ( long index, ASIOBool )
{
    int iCurSample;

    // get references to class members
    int&              iASIOBufferSizeMono     = pSound->iASIOBufferSizeMono;
    CVector<int16_t>& vecsMultChanAudioSndCrd = pSound->vecsMultChanAudioSndCrd;

    // perform the processing for input and output
    pSound->ASIOMutex.lock(); // get mutex lock
    {
        // CAPTURE -------------------------------------------------------------
        for ( int i = 0; i < NUM_IN_OUT_CHANNELS; i++ )
        {
            int iSelCH, iSelAddCH;

            GetSelCHAndAddCH ( pSound->vSelectedInputChannels[i], pSound->lNumInChan, iSelCH, iSelAddCH );

            // copy new captured block in thread transfer buffer (copy
            // mono data interleaved in stereo buffer)
            switch ( pSound->channelInfosInput[iSelCH].type )
            {
            case ASIOSTInt16LSB:
            {
                // no type conversion required, just copy operation
                int16_t* pASIOBuf = static_cast<int16_t*> ( pSound->bufferInfos[iSelCH].buffers[index] );

                for ( iCurSample = 0; iCurSample < iASIOBufferSizeMono; iCurSample++ )
                {
                    vecsMultChanAudioSndCrd[2 * iCurSample + i] = pASIOBuf[iCurSample];
                }

                if ( iSelAddCH >= 0 )
                {
                    // mix input channels case:
                    int16_t* pASIOBufAdd = static_cast<int16_t*> ( pSound->bufferInfos[iSelAddCH].buffers[index] );

                    for ( iCurSample = 0; iCurSample < iASIOBufferSizeMono; iCurSample++ )
                    {
                        vecsMultChanAudioSndCrd[2 * iCurSample + i] =
                            Float2Short ( (float) vecsMultChanAudioSndCrd[2 * iCurSample + i] + (float) pASIOBufAdd[iCurSample] );
                    }
                }
                break;
            }

            case ASIOSTInt24LSB:
                for ( iCurSample = 0; iCurSample < iASIOBufferSizeMono; iCurSample++ )
                {
                    int iCurSam = 0;
                    memcpy ( &iCurSam, ( (char*) pSound->bufferInfos[iSelCH].buffers[index] ) + iCurSample * 3, 3 );
                    iCurSam >>= 8;

                    vecsMultChanAudioSndCrd[2 * iCurSample + i] = static_cast<int16_t> ( iCurSam );
                }

                if ( iSelAddCH >= 0 )
                {
                    // mix input channels case:
                    for ( iCurSample = 0; iCurSample < iASIOBufferSizeMono; iCurSample++ )
                    {
                        int iCurSam = 0;
                        memcpy ( &iCurSam, ( (char*) pSound->bufferInfos[iSelAddCH].buffers[index] ) + iCurSample * 3, 3 );
                        iCurSam >>= 8;

                        vecsMultChanAudioSndCrd[2 * iCurSample + i] =
                            Float2Short ( (float) vecsMultChanAudioSndCrd[2 * iCurSample + i] + (float) static_cast<int16_t> ( iCurSam ) );
                    }
                }
                break;

            case ASIOSTInt32LSB:
            {
                int32_t* pASIOBuf = static_cast<int32_t*> ( pSound->bufferInfos[iSelCH].buffers[index] );

                for ( iCurSample = 0; iCurSample < iASIOBufferSizeMono; iCurSample++ )
                {
                    vecsMultChanAudioSndCrd[2 * iCurSample + i] = static_cast<int16_t> ( pASIOBuf[iCurSample] >> 16 );
                }

                if ( iSelAddCH >= 0 )
                {
                    // mix input channels case:
                    int32_t* pASIOBufAdd = static_cast<int32_t*> ( pSound->bufferInfos[iSelAddCH].buffers[index] );

                    for ( iCurSample = 0; iCurSample < iASIOBufferSizeMono; iCurSample++ )
                    {
                        vecsMultChanAudioSndCrd[2 * iCurSample + i] = Float2Short ( (float) vecsMultChanAudioSndCrd[2 * iCurSample + i] +
                                                                                    (float) static_cast<int16_t> ( pASIOBufAdd[iCurSample] >> 16 ) );
                    }
                }
                break;
            }

            case ASIOSTFloat32LSB: // IEEE 754 32 bit float, as found on Intel x86 architecture
                                   // clang-format off
// NOT YET TESTED
                                   // clang-format on
                for ( iCurSample = 0; iCurSample < iASIOBufferSizeMono; iCurSample++ )
                {
                    vecsMultChanAudioSndCrd[2 * iCurSample + i] =
                        static_cast<int16_t> ( static_cast<float*> ( pSound->bufferInfos[iSelCH].buffers[index] )[iCurSample] * _MAXSHORT );
                }
                break;

            case ASIOSTFloat64LSB: // IEEE 754 64 bit double float, as found on Intel x86 architecture
                                   // clang-format off
// NOT YET TESTED
                                   // clang-format on
                for ( iCurSample = 0; iCurSample < iASIOBufferSizeMono; iCurSample++ )
                {
                    vecsMultChanAudioSndCrd[2 * iCurSample + i] =
                        static_cast<int16_t> ( static_cast<double*> ( pSound->bufferInfos[iSelCH].buffers[index] )[iCurSample] * _MAXSHORT );
                }
                break;

            case ASIOSTInt32LSB16: // 32 bit data with 16 bit alignment
                                   // clang-format off
// NOT YET TESTED
                                   // clang-format on
                for ( iCurSample = 0; iCurSample < iASIOBufferSizeMono; iCurSample++ )
                {
                    vecsMultChanAudioSndCrd[2 * iCurSample + i] =
                        static_cast<int16_t> ( static_cast<int32_t*> ( pSound->bufferInfos[iSelCH].buffers[index] )[iCurSample] & 0xFFFF );
                }
                break;

            case ASIOSTInt32LSB18: // 32 bit data with 18 bit alignment
                                   // clang-format off
// NOT YET TESTED
                                   // clang-format on
                for ( iCurSample = 0; iCurSample < iASIOBufferSizeMono; iCurSample++ )
                {
                    vecsMultChanAudioSndCrd[2 * iCurSample + i] =
                        static_cast<int16_t> ( ( static_cast<int32_t*> ( pSound->bufferInfos[iSelCH].buffers[index] )[iCurSample] & 0x3FFFF ) >> 2 );
                }
                break;

            case ASIOSTInt32LSB20: // 32 bit data with 20 bit alignment
                                   // clang-format off
// NOT YET TESTED
                                   // clang-format on
                for ( iCurSample = 0; iCurSample < iASIOBufferSizeMono; iCurSample++ )
                {
                    vecsMultChanAudioSndCrd[2 * iCurSample + i] =
                        static_cast<int16_t> ( ( static_cast<int32_t*> ( pSound->bufferInfos[iSelCH].buffers[index] )[iCurSample] & 0xFFFFF ) >> 4 );
                }
                break;

            case ASIOSTInt32LSB24: // 32 bit data with 24 bit alignment
                                   // clang-format off
// NOT YET TESTED
                                   // clang-format on
                for ( iCurSample = 0; iCurSample < iASIOBufferSizeMono; iCurSample++ )
                {
                    vecsMultChanAudioSndCrd[2 * iCurSample + i] =
                        static_cast<int16_t> ( ( static_cast<int32_t*> ( pSound->bufferInfos[iSelCH].buffers[index] )[iCurSample] & 0xFFFFFF ) >> 8 );
                }
                break;

            case ASIOSTInt16MSB:
                // clang-format off
// NOT YET TESTED
                // clang-format on
                // flip bits
                for ( iCurSample = 0; iCurSample < iASIOBufferSizeMono; iCurSample++ )
                {
                    vecsMultChanAudioSndCrd[2 * iCurSample + i] =
                        Flip16Bits ( ( static_cast<int16_t*> ( pSound->bufferInfos[iSelCH].buffers[index] ) )[iCurSample] );
                }
                break;

            case ASIOSTInt24MSB:
                // clang-format off
// NOT YET TESTED
                // clang-format on
                for ( iCurSample = 0; iCurSample < iASIOBufferSizeMono; iCurSample++ )
                {
                    // because the bits are flipped, we do not have to perform the
                    // shift by 8 bits
                    int iCurSam = 0;
                    memcpy ( &iCurSam, ( (char*) pSound->bufferInfos[iSelCH].buffers[index] ) + iCurSample * 3, 3 );

                    vecsMultChanAudioSndCrd[2 * iCurSample + i] = Flip16Bits ( static_cast<int16_t> ( iCurSam ) );
                }
                break;

            case ASIOSTInt32MSB:
                // clang-format off
// NOT YET TESTED
                // clang-format on
                for ( iCurSample = 0; iCurSample < iASIOBufferSizeMono; iCurSample++ )
                {
                    // flip bits and convert to 16 bit
                    vecsMultChanAudioSndCrd[2 * iCurSample + i] = static_cast<int16_t> (
                        Flip32Bits ( static_cast<int32_t*> ( pSound->bufferInfos[iSelCH].buffers[index] )[iCurSample] ) >> 16 );
                }
                break;

            case ASIOSTFloat32MSB: // IEEE 754 32 bit float, as found on Intel x86 architecture
                                   // clang-format off
// NOT YET TESTED
                                   // clang-format on
                for ( iCurSample = 0; iCurSample < iASIOBufferSizeMono; iCurSample++ )
                {
                    vecsMultChanAudioSndCrd[2 * iCurSample + i] = static_cast<int16_t> (
                        static_cast<float> ( Flip32Bits ( static_cast<int32_t*> ( pSound->bufferInfos[iSelCH].buffers[index] )[iCurSample] ) ) *
                        _MAXSHORT );
                }
                break;

            case ASIOSTFloat64MSB: // IEEE 754 64 bit double float, as found on Intel x86 architecture
                                   // clang-format off
// NOT YET TESTED
                                   // clang-format on
                for ( iCurSample = 0; iCurSample < iASIOBufferSizeMono; iCurSample++ )
                {
                    vecsMultChanAudioSndCrd[2 * iCurSample + i] = static_cast<int16_t> (
                        static_cast<double> ( Flip64Bits ( static_cast<int64_t*> ( pSound->bufferInfos[iSelCH].buffers[index] )[iCurSample] ) ) *
                        _MAXSHORT );
                }
                break;

            case ASIOSTInt32MSB16: // 32 bit data with 16 bit alignment
                                   // clang-format off
// NOT YET TESTED
                                   // clang-format on
                for ( iCurSample = 0; iCurSample < iASIOBufferSizeMono; iCurSample++ )
                {
                    vecsMultChanAudioSndCrd[2 * iCurSample + i] = static_cast<int16_t> (
                        Flip32Bits ( static_cast<int32_t*> ( pSound->bufferInfos[iSelCH].buffers[index] )[iCurSample] ) & 0xFFFF );
                }
                break;

            case ASIOSTInt32MSB18: // 32 bit data with 18 bit alignment
                                   // clang-format off
// NOT YET TESTED
                                   // clang-format on
                for ( iCurSample = 0; iCurSample < iASIOBufferSizeMono; iCurSample++ )
                {
                    vecsMultChanAudioSndCrd[2 * iCurSample + i] = static_cast<int16_t> (
                        ( Flip32Bits ( static_cast<int32_t*> ( pSound->bufferInfos[iSelCH].buffers[index] )[iCurSample] ) & 0x3FFFF ) >> 2 );
                }
                break;

            case ASIOSTInt32MSB20: // 32 bit data with 20 bit alignment
                                   // clang-format off
// NOT YET TESTED
                                   // clang-format on
                for ( iCurSample = 0; iCurSample < iASIOBufferSizeMono; iCurSample++ )
                {
                    vecsMultChanAudioSndCrd[2 * iCurSample + i] = static_cast<int16_t> (
                        ( Flip32Bits ( static_cast<int32_t*> ( pSound->bufferInfos[iSelCH].buffers[index] )[iCurSample] ) & 0xFFFFF ) >> 4 );
                }
                break;

            case ASIOSTInt32MSB24: // 32 bit data with 24 bit alignment
                                   // clang-format off
// NOT YET TESTED
                                   // clang-format on
                for ( iCurSample = 0; iCurSample < iASIOBufferSizeMono; iCurSample++ )
                {
                    vecsMultChanAudioSndCrd[2 * iCurSample + i] = static_cast<int16_t> (
                        ( Flip32Bits ( static_cast<int32_t*> ( pSound->bufferInfos[iSelCH].buffers[index] )[iCurSample] ) & 0xFFFFFF ) >> 8 );
                }
                break;
            }
        }

        // call processing callback function
        pSound->ProcessCallback ( vecsMultChanAudioSndCrd );

        // PLAYBACK ------------------------------------------------------------
        for ( int i = 0; i < NUM_IN_OUT_CHANNELS; i++ )
        {
            const int iSelCH = pSound->lNumInChan + pSound->vSelectedOutputChannels[i];

            // copy data from sound card in output buffer (copy
            // interleaved stereo data in mono sound card buffer)
            switch ( pSound->channelInfosOutput[pSound->vSelectedOutputChannels[i]].type )
            {
            case ASIOSTInt16LSB:
            {
                // no type conversion required, just copy operation
                int16_t* pASIOBuf = static_cast<int16_t*> ( pSound->bufferInfos[iSelCH].buffers[index] );

                for ( iCurSample = 0; iCurSample < iASIOBufferSizeMono; iCurSample++ )
                {
                    pASIOBuf[iCurSample] = vecsMultChanAudioSndCrd[2 * iCurSample + i];
                }
                break;
            }

            case ASIOSTInt24LSB:
                // clang-format off
// NOT YET TESTED
                // clang-format on
                for ( iCurSample = 0; iCurSample < iASIOBufferSizeMono; iCurSample++ )
                {
                    // convert current sample in 24 bit format
                    int32_t iCurSam = static_cast<int32_t> ( vecsMultChanAudioSndCrd[2 * iCurSample + i] );

                    iCurSam <<= 8;

                    memcpy ( ( (char*) pSound->bufferInfos[iSelCH].buffers[index] ) + iCurSample * 3, &iCurSam, 3 );
                }
                break;

            case ASIOSTInt32LSB:
            {
                int32_t* pASIOBuf = static_cast<int32_t*> ( pSound->bufferInfos[iSelCH].buffers[index] );

                for ( iCurSample = 0; iCurSample < iASIOBufferSizeMono; iCurSample++ )
                {
                    // convert to 32 bit
                    const int32_t iCurSam = static_cast<int32_t> ( vecsMultChanAudioSndCrd[2 * iCurSample + i] );

                    pASIOBuf[iCurSample] = ( iCurSam << 16 );
                }
                break;
            }

            case ASIOSTFloat32LSB: // IEEE 754 32 bit float, as found on Intel x86 architecture
                                   // clang-format off
// NOT YET TESTED
                                   // clang-format on
                for ( iCurSample = 0; iCurSample < iASIOBufferSizeMono; iCurSample++ )
                {
                    const float fCurSam = static_cast<float> ( vecsMultChanAudioSndCrd[2 * iCurSample + i] );

                    static_cast<float*> ( pSound->bufferInfos[iSelCH].buffers[index] )[iCurSample] = fCurSam / _MAXSHORT;
                }
                break;

            case ASIOSTFloat64LSB: // IEEE 754 64 bit double float, as found on Intel x86 architecture
                                   // clang-format off
// NOT YET TESTED
                                   // clang-format on
                for ( iCurSample = 0; iCurSample < iASIOBufferSizeMono; iCurSample++ )
                {
                    const double fCurSam = static_cast<double> ( vecsMultChanAudioSndCrd[2 * iCurSample + i] );

                    static_cast<double*> ( pSound->bufferInfos[iSelCH].buffers[index] )[iCurSample] = fCurSam / _MAXSHORT;
                }
                break;

            case ASIOSTInt32LSB16: // 32 bit data with 16 bit alignment
                                   // clang-format off
// NOT YET TESTED
                                   // clang-format on
                for ( iCurSample = 0; iCurSample < iASIOBufferSizeMono; iCurSample++ )
                {
                    // convert to 32 bit
                    const int32_t iCurSam = static_cast<int32_t> ( vecsMultChanAudioSndCrd[2 * iCurSample + i] );

                    static_cast<int32_t*> ( pSound->bufferInfos[iSelCH].buffers[index] )[iCurSample] = iCurSam;
                }
                break;

            case ASIOSTInt32LSB18: // 32 bit data with 18 bit alignment
                                   // clang-format off
// NOT YET TESTED
                                   // clang-format on
                for ( iCurSample = 0; iCurSample < iASIOBufferSizeMono; iCurSample++ )
                {
                    // convert to 32 bit
                    const int32_t iCurSam = static_cast<int32_t> ( vecsMultChanAudioSndCrd[2 * iCurSample + i] );

                    static_cast<int32_t*> ( pSound->bufferInfos[iSelCH].buffers[index] )[iCurSample] = ( iCurSam << 2 );
                }
                break;

            case ASIOSTInt32LSB20: // 32 bit data with 20 bit alignment
                                   // clang-format off
// NOT YET TESTED
                                   // clang-format on
                for ( iCurSample = 0; iCurSample < iASIOBufferSizeMono; iCurSample++ )
                {
                    // convert to 32 bit
                    const int32_t iCurSam = static_cast<int32_t> ( vecsMultChanAudioSndCrd[2 * iCurSample + i] );

                    static_cast<int32_t*> ( pSound->bufferInfos[iSelCH].buffers[index] )[iCurSample] = ( iCurSam << 4 );
                }
                break;

            case ASIOSTInt32LSB24: // 32 bit data with 24 bit alignment
                                   // clang-format off
// NOT YET TESTED
                                   // clang-format on
                for ( iCurSample = 0; iCurSample < iASIOBufferSizeMono; iCurSample++ )
                {
                    // convert to 32 bit
                    const int32_t iCurSam = static_cast<int32_t> ( vecsMultChanAudioSndCrd[2 * iCurSample + i] );

                    static_cast<int32_t*> ( pSound->bufferInfos[iSelCH].buffers[index] )[iCurSample] = ( iCurSam << 8 );
                }
                break;

            case ASIOSTInt16MSB:
                // clang-format off
// NOT YET TESTED
                // clang-format on
                // flip bits
                for ( iCurSample = 0; iCurSample < iASIOBufferSizeMono; iCurSample++ )
                {
                    ( (int16_t*) pSound->bufferInfos[iSelCH].buffers[index] )[iCurSample] =
                        Flip16Bits ( vecsMultChanAudioSndCrd[2 * iCurSample + i] );
                }
                break;

            case ASIOSTInt24MSB:
                // clang-format off
// NOT YET TESTED
                // clang-format on
                for ( iCurSample = 0; iCurSample < iASIOBufferSizeMono; iCurSample++ )
                {
                    // because the bits are flipped, we do not have to perform the
                    // shift by 8 bits
                    int32_t iCurSam = static_cast<int32_t> ( Flip16Bits ( vecsMultChanAudioSndCrd[2 * iCurSample + i] ) );

                    memcpy ( ( (char*) pSound->bufferInfos[iSelCH].buffers[index] ) + iCurSample * 3, &iCurSam, 3 );
                }
                break;

            case ASIOSTInt32MSB:
                // clang-format off
// NOT YET TESTED
                // clang-format on
                for ( iCurSample = 0; iCurSample < iASIOBufferSizeMono; iCurSample++ )
                {
                    // convert to 32 bit and flip bits
                    int iCurSam = static_cast<int32_t> ( vecsMultChanAudioSndCrd[2 * iCurSample + i] );

                    static_cast<int32_t*> ( pSound->bufferInfos[iSelCH].buffers[index] )[iCurSample] = Flip32Bits ( iCurSam << 16 );
                }
                break;

            case ASIOSTFloat32MSB: // IEEE 754 32 bit float, as found on Intel x86 architecture
                                   // clang-format off
// NOT YET TESTED
                                   // clang-format on
                for ( iCurSample = 0; iCurSample < iASIOBufferSizeMono; iCurSample++ )
                {
                    const float fCurSam = static_cast<float> ( vecsMultChanAudioSndCrd[2 * iCurSample + i] );

                    static_cast<float*> ( pSound->bufferInfos[iSelCH].buffers[index] )[iCurSample] =
                        static_cast<float> ( Flip32Bits ( static_cast<int32_t> ( fCurSam / _MAXSHORT ) ) );
                }
                break;

            case ASIOSTFloat64MSB: // IEEE 754 64 bit double float, as found on Intel x86 architecture
                                   // clang-format off
// NOT YET TESTED
                                   // clang-format on
                for ( iCurSample = 0; iCurSample < iASIOBufferSizeMono; iCurSample++ )
                {
                    const double fCurSam = static_cast<double> ( vecsMultChanAudioSndCrd[2 * iCurSample + i] );

                    static_cast<float*> ( pSound->bufferInfos[iSelCH].buffers[index] )[iCurSample] =
                        static_cast<double> ( Flip64Bits ( static_cast<int64_t> ( fCurSam / _MAXSHORT ) ) );
                }
                break;

            case ASIOSTInt32MSB16: // 32 bit data with 16 bit alignment
                                   // clang-format off
// NOT YET TESTED
                                   // clang-format on
                for ( iCurSample = 0; iCurSample < iASIOBufferSizeMono; iCurSample++ )
                {
                    // convert to 32 bit
                    const int32_t iCurSam = static_cast<int32_t> ( vecsMultChanAudioSndCrd[2 * iCurSample + i] );

                    static_cast<int32_t*> ( pSound->bufferInfos[iSelCH].buffers[index] )[iCurSample] = Flip32Bits ( iCurSam );
                }
                break;

            case ASIOSTInt32MSB18: // 32 bit data with 18 bit alignment
                                   // clang-format off
// NOT YET TESTED
                                   // clang-format on
                for ( iCurSample = 0; iCurSample < iASIOBufferSizeMono; iCurSample++ )
                {
                    // convert to 32 bit
                    const int32_t iCurSam = static_cast<int32_t> ( vecsMultChanAudioSndCrd[2 * iCurSample + i] );

                    static_cast<int32_t*> ( pSound->bufferInfos[iSelCH].buffers[index] )[iCurSample] = Flip32Bits ( iCurSam << 2 );
                }
                break;

            case ASIOSTInt32MSB20: // 32 bit data with 20 bit alignment
                                   // clang-format off
// NOT YET TESTED
                                   // clang-format on
                for ( iCurSample = 0; iCurSample < iASIOBufferSizeMono; iCurSample++ )
                {
                    // convert to 32 bit
                    const int32_t iCurSam = static_cast<int32_t> ( vecsMultChanAudioSndCrd[2 * iCurSample + i] );

                    static_cast<int32_t*> ( pSound->bufferInfos[iSelCH].buffers[index] )[iCurSample] = Flip32Bits ( iCurSam << 4 );
                }
                break;

            case ASIOSTInt32MSB24: // 32 bit data with 24 bit alignment
                                   // clang-format off
// NOT YET TESTED
                                   // clang-format on
                for ( iCurSample = 0; iCurSample < iASIOBufferSizeMono; iCurSample++ )
                {
                    // convert to 32 bit
                    const int32_t iCurSam = static_cast<int32_t> ( vecsMultChanAudioSndCrd[2 * iCurSample + i] );

                    static_cast<int32_t*> ( pSound->bufferInfos[iSelCH].buffers[index] )[iCurSample] = Flip32Bits ( iCurSam << 8 );
                }
                break;
            }
        }

        // Finally if the driver supports the ASIOOutputReady() optimization,
        // do it here, all data are in place -----------------------------------
        if ( pSound->bASIOPostOutput )
        {
            ASIOOutputReady();
        }
    }
    pSound->ASIOMutex.unlock();
}

long CSound::asioMessages ( long selector, long, void*, double* )
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
        pSound->EmitReinitRequestSignal ( RS_ONLY_RESTART_AND_INIT );
        ret = 1L; // 1L if request is accepted or 0 otherwise
        break;

    case kAsioResetRequest:
        pSound->EmitReinitRequestSignal ( RS_RELOAD_RESTART_AND_INIT );
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
        iOut <<= 1;
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
        iOut <<= 1;
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
        iOut <<= 1;
        iMask >>= 1;
    }

    return iOut;
}
