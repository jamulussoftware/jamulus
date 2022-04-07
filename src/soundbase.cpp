/******************************************************************************\
 * Copyright (c) 2004-2022
 *
 * Author(s):
 *  Volker Fischer, revised and maintained by Peter Goderie (pgScorpio)
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

#include "soundbase.h"

// This is used as a lookup table for parsing option letters, mapping
// a single character to an EMidiCtlType
char const sMidiCtlChar[] = {
    // Has to follow order of EMidiCtlType
    /* [EMidiCtlType::Fader]       = */ 'f',
    /* [EMidiCtlType::Pan]         = */ 'p',
    /* [EMidiCtlType::Solo]        = */ 's',
    /* [EMidiCtlType::Mute]        = */ 'm',
    /* [EMidiCtlType::MuteMyself]  = */ 'o',
    /* [EMidiCtlType::None]        = */ '\0' };

//============================================================================
// Static helpers:
//============================================================================

//====================================
// Flip bits:
//====================================

int16_t CSoundBase::Flip16Bits ( const int16_t iIn )
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

int32_t CSoundBase::Flip32Bits ( const int32_t iIn )
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

int64_t CSoundBase::Flip64Bits ( const int64_t iIn )
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

//============================================================================
// CSoundProperties:
//============================================================================

void CSoundProperties::setDefaultTexts()
{
    // Audio Device:
    if ( bHasAudioDeviceSelection )
    {
        strAudioDeviceWhatsThis      = ( "<b>" + CSoundBase::tr ( "Audio Device" ) + ":</b> " +
                                    CSoundBase::tr ( "Your audio device (sound card) can be "
                                                          "selected or using %1. If the selected driver is not valid an error "
                                                          "message will be shown. " )
                                        .arg ( APP_NAME ) +
                                    "<br>" +
                                    CSoundBase::tr ( "If the driver is selected during an active connection, the connection "
                                                          "is stopped, the driver is changed and the connection is started again "
                                                          "automatically." ) );
        strAudioDeviceAccessibleName = CSoundBase::tr ( "Audio device selector combo box." );
    }
    else
    {
        strAudioDeviceWhatsThis =
            ( "<b>" + CSoundBase::tr ( "Audio Device" ) + ":</b> " + CSoundBase::tr ( "The audio device used for %1." ).arg ( APP_NAME ) + "<br>" +
              CSoundBase::tr ( "This setting cannot be changed." ) );
        strAudioDeviceAccessibleName = CSoundBase::tr ( "Audio device name." );
    }

    // Setup Button:
    strSetupButtonText      = CSoundBase::tr ( "Device Settings" ),
    strSetupButtonWhatsThis = htmlBold ( CSoundBase::tr ( "Sound card driver settings:" ) ) + htmlNewLine() +
                              CSoundBase::tr ( "This button opens the driver settings of your sound card. Some drivers "
                                               "allow you to change buffer settings, and/or let you choose input or outputs of your device(s). "
                                               "Always make sure to set your Audio Device Sample Rate to 48000Hz (48Khz)."
                                               "More information can be found on jamulus.io." );

    strSetupButtonAccessibleName = CSoundBase::tr ( "Click this button to open Audio Device Settings" );
}

CSoundProperties::CSoundProperties() :
    bHasAudioDeviceSelection ( true ),
    bHasSetupDialog ( false ),
    bHasInputChannelSelection ( true ),
    bHasOutputChannelSelection ( true ),
    bHasInputGainSelection ( true )
{
    setDefaultTexts();
}

//============================================================================
// CSoundBase:
//============================================================================

CSoundBase::CSoundBase ( const QString& systemDriverTechniqueName,
                         void ( *theProcessCallback ) ( CVector<int16_t>& psData, void* pArg ),
                         void* theProcessCallbackArg ) :
    //    iDeviceBufferSize ( 0 ),
    //    lNumDevices ( 0 ),
    //    fInOutLatencyMs ( 0.0 ),
    //    lNumInChan ( 0 ),
    //    lNumAddedInChan ( 0 ),
    //    lNumOutChan ( 0 ),
    iProtocolBufferSize ( 128 ),
    strDriverTechniqueName ( systemDriverTechniqueName ),
    strClientName ( "" ),
    bAutoConnect ( true ),
    bStarted ( false ),
    bActive ( false ),
    iCurrentDevice ( -1 ),
    iCtrlMIDIChannel ( INVALID_MIDI_CH ),
    aMidiCtls ( 128 ),
    fpProcessCallback ( theProcessCallback ),
    pProcessCallbackArg ( theProcessCallbackArg )
{
    CCommandline cCommandline;

    setObjectName ( "CSoundThread" );

    cCommandline.GetStringArgument ( CMDLN_CLIENTNAME, strClientName );

    QString strMIDISetup;
    if ( cCommandline.GetStringArgument ( CMDLN_CTRLMIDICH, strMIDISetup ) )
    {
        parseMIDICommandLineArgument ( strMIDISetup );
    }

    clearDeviceInfo();
    resetInputChannelsGain();
    resetChannelMapping();

    // setup timers
    timerCheckActive.setSingleShot ( true ); // only check once
    QObject::connect ( &timerCheckActive, &QTimer::timeout, this, &CSoundBase::onTimerCheckActive );
}

void CSoundBase::onTimerCheckActive()
{
    if ( !bActive )
    {
        emit SoundActiveTimeout();
        CMsgBoxes::ShowWarning ( htmlBold ( tr ( "Your audio device is not working correctly." ) ) + htmlNewLine() +
                                 tr ( "Please check your device settings or try another device." ) );
    }
}

void CSoundBase::clearDeviceInfo()
{
    // No current device selection
    iCurrentDevice = -1;
    // No input channels
    lNumInChan      = 0;
    lNumAddedInChan = 0;
    strInputChannelNames.clear();
    // No added output channels
    lNumOutChan = 0; // No outnput channels
    strOutputChannelNames.clear();

    {
        // No input selection
        memset ( &selectedInputChannels, 0xFF, sizeof ( selectedInputChannels ) );
        // No output selection
        memset ( &selectedOutputChannels, 0xFF, sizeof ( selectedOutputChannels ) );
    }

    fInOutLatencyMs = 0.0;
}

void CSoundBase::resetInputChannelsGain()
{
    QMutexLocker locker ( &mutexAudioProcessCallback );

    for ( unsigned int i = 0; i < PROT_NUM_IN_CHANNELS; i++ )
    {
        inputChannelsGain[i] = 1;
    }
}

void CSoundBase::resetChannelMapping()
{
    QMutexLocker locker ( &mutexAudioProcessCallback );

    if ( PROT_NUM_IN_CHANNELS > 2 )
    {
        memset ( selectedInputChannels, 0xFF, sizeof ( selectedInputChannels ) );
    }

    // init selected channel numbers with defaults: use first available
    // channels for input and output
    selectedInputChannels[0]  = 0;
    selectedInputChannels[1]  = ( lNumInChan > 1 ) ? 1 : 0;
    selectedOutputChannels[0] = 0;
    selectedOutputChannels[1] = ( lNumOutChan > 1 ) ? 1 : 0;
    ;
}

long CSoundBase::getNumInputChannelsToAdd ( long lNumInChan )
{
    if ( lNumInChan <= 2 )
    {
        return 0;
    }

    return ( lNumInChan - 2 ) * 2;
}

void CSoundBase::getInputSelAndAddChannels ( int iSelChan, long lNumInChan, long lNumAddedInChan, int& iSelChanOut, int& iAddChanOut )
{
    // we have a mixed channel setup, definitions:
    // - mixed channel setup only for 4 physical inputs:
    //   SelCH == 4: Ch 0 + Ch 2
    //   SelCh == 5: Ch 0 + Ch 3
    //   SelCh == 6: Ch 1 + Ch 2
    //   SelCh == 7: Ch 1 + Ch 3

    if ( ( iSelChan >= 0 ) && ( iSelChan < ( lNumInChan + lNumAddedInChan ) ) )
    {
        if ( iSelChan < lNumInChan )
        {
            iSelChanOut = iSelChan;
            iAddChanOut = INVALID_INDEX;

            return;
        }
        else
        {
            long addedChan = ( iSelChan - lNumInChan );
            long numAdds   = ( lNumInChan - 2 );

            iSelChanOut = ( addedChan / numAdds );
            iAddChanOut = ( addedChan % numAdds ) + 2;
        }

        return;
    }

    iSelChanOut = INVALID_INDEX;
    iAddChanOut = INVALID_INDEX;
}

long CSoundBase::addAddedInputChannelNames()
{
    int curInChannelNames = strInputChannelNames.size();

    if ( ( lNumInChan == 0 ) || ( curInChannelNames == 0 ) || ( curInChannelNames != lNumInChan ) )
    {
        // Error !!! lNumInChan and strInputChannelNames must be set and match !!
        return -1;
    }

    // Determine number of channels to add
    lNumAddedInChan = getNumInputChannelsToAdd ( lNumInChan );

    // Set any added input channel names
    for ( int i = 0; i < lNumAddedInChan; i++ )
    {
        int iSelChan, iAddChan;

        getInputSelAndAddChannels ( lNumInChan + i, lNumInChan, lNumAddedInChan, iSelChan, iAddChan );

        strInputChannelNames.append ( strInputChannelNames[iSelChan] + " + " + strInputChannelNames[iAddChan] );
    }

    return lNumAddedInChan;
}

//============================================================================
// Public Interface to CClient:
//============================================================================

QString CSoundBase::getErrorString()
{
    QString str ( strErrorList.join ( htmlNewLine() ) );
    strErrorList.clear();
    return str;
}

//============================================================================
//
//============================================================================

QStringList CSoundBase::GetDeviceNames()
{
    QMutexLocker locker ( &mutexDeviceProperties );

    return strDeviceNames;
}

QString CSoundBase::GetDeviceName ( const int index )
{
    QMutexLocker locker ( &mutexDeviceProperties );

    return ( ( index >= 0 ) && ( index < strDeviceNames.length() ) ) ? strDeviceNames[index] : QString();
}

QString CSoundBase::GetCurrentDeviceName()
{
    QMutexLocker locker ( &mutexDeviceProperties );

    return ( ( iCurrentDevice >= 0 ) && ( iCurrentDevice < strDeviceNames.count() ) ) ? strDeviceNames[iCurrentDevice] : QString();
}

int CSoundBase::GetIndexOfDevice ( const QString& strDeviceName )
{
    QMutexLocker locker ( &mutexDeviceProperties );

    return strDeviceNames.indexOf ( strDeviceName );
}

int CSoundBase::GetNumInputChannels()
{
    QMutexLocker locker ( &mutexDeviceProperties );

    return ( lNumInChan + lNumAddedInChan );
}

QString CSoundBase::GetInputChannelName ( const int index )
{
    QMutexLocker locker ( &mutexDeviceProperties );

    return ( ( index >= 0 ) && ( index < strInputChannelNames.count() ) ) ? strInputChannelNames[index] : QString();
}

QString CSoundBase::GetOutputChannelName ( const int index )
{
    QMutexLocker locker ( &mutexDeviceProperties );

    return ( ( index >= 0 ) && ( index < strOutputChannelNames.count() ) ) ? strOutputChannelNames[index] : QString();
}

void CSoundBase::SetLeftInputChannel ( const int iNewChan )
{
    if ( !soundProperties.bHasInputChannelSelection )
    {
        return;
    }

    // apply parameter after input parameter check
    if ( ( iNewChan >= 0 ) && ( iNewChan < ( lNumInChan + lNumAddedInChan ) ) && ( iNewChan != selectedInputChannels[0] ) )
    {
        {
            QMutexLocker locker ( &mutexAudioProcessCallback );

            selectedInputChannels[0] = iNewChan;
        }

        onChannelSelectionChanged();
    }
}

void CSoundBase::SetRightInputChannel ( const int iNewChan )
{
    if ( !soundProperties.bHasInputChannelSelection )
    {
        return;
    }

    // apply parameter after input parameter check
    if ( ( iNewChan >= 0 ) && ( iNewChan < ( lNumInChan + lNumAddedInChan ) ) && ( iNewChan != selectedInputChannels[1] ) )
    {
        {
            QMutexLocker locker ( &mutexAudioProcessCallback );

            selectedInputChannels[1] = iNewChan;
        }

        onChannelSelectionChanged();
    }
}

void CSoundBase::SetLeftOutputChannel ( const int iNewChan )
{
    if ( !soundProperties.bHasOutputChannelSelection )
    {
        return;
    }

    // apply parameter after input parameter check
    if ( ( iNewChan >= 0 ) && ( iNewChan < lNumOutChan ) && ( iNewChan != selectedOutputChannels[0] ) )
    {
        {
            QMutexLocker locker ( &mutexAudioProcessCallback );

            int prevChan              = selectedOutputChannels[0];
            selectedOutputChannels[0] = iNewChan;

            // Dirty hack, since we can't easily manage 2x same output
            if ( selectedOutputChannels[1] == iNewChan )
            {
                if ( ( prevChan >= 0 ) && ( prevChan != iNewChan ) )
                {
                    // Swap channels...
                    selectedOutputChannels[1] = prevChan;
                }
                else
                {
                    // Select higher channel number for left
                    int otherChan = iNewChan + 1;
                    if ( otherChan >= lNumOutChan )
                    {
                        otherChan = 0;
                    }
                    selectedInputChannels[1] = otherChan;
                }
            }
        }

        onChannelSelectionChanged();
    }
}

void CSoundBase::SetRightOutputChannel ( const int iNewChan )
{
    if ( !soundProperties.bHasOutputChannelSelection )
    {
        return;
    }

    // apply parameter after input parameter check
    if ( ( iNewChan >= 0 ) && ( iNewChan < lNumOutChan ) && ( iNewChan != selectedOutputChannels[1] ) )
    {
        {
            QMutexLocker locker ( &mutexAudioProcessCallback );

            int prevChan              = selectedOutputChannels[1];
            selectedOutputChannels[1] = iNewChan;

            // Dirty hack, since we can't easily manage 2x same output
            if ( selectedOutputChannels[0] == iNewChan )
            {
                if ( ( prevChan >= 0 ) && ( prevChan != iNewChan ) )
                {
                    // Swap channels...
                    selectedOutputChannels[0] = prevChan;
                }
                else
                {
                    // Select lower channel number for left
                    int otherChan = iNewChan - 1;
                    if ( otherChan < 0 )
                    {
                        otherChan = lNumOutChan - 1;
                    }
                    selectedOutputChannels[0] = otherChan;
                }
            }
        }

        onChannelSelectionChanged();
    }
}

int CSoundBase::SetLeftInputGain ( int iGain )
{
    if ( !soundProperties.bHasInputGainSelection )
    {
        return 1;
    }

    QMutexLocker locker ( &mutexAudioProcessCallback ); // get mutex lock

    if ( iGain < 1 )
    {
        iGain = 1;
    }
    else if ( iGain > DRV_MAX_INPUT_GAIN )
    {
        iGain = DRV_MAX_INPUT_GAIN;
    }

    inputChannelsGain[0] = iGain;

    return iGain;
}

int CSoundBase::SetRightInputGain ( int iGain )
{
    if ( !soundProperties.bHasInputGainSelection )
    {
        return 1;
    }

    QMutexLocker locker ( &mutexAudioProcessCallback ); // get mutex lock

    if ( iGain < 1 )
    {
        iGain = 1;
    }
    else if ( iGain > DRV_MAX_INPUT_GAIN )
    {
        iGain = DRV_MAX_INPUT_GAIN;
    }

    inputChannelsGain[1] = iGain;

    return iGain;
}

bool CSoundBase::OpenDeviceSetup()
{
    if ( !openDeviceSetup() )
    {
        // With ASIO there always is a setup button, but not all ASIO drivers support a setup dialog!

        CMsgBoxes::ShowInfo ( htmlBold ( tr ( "This audio device does not support setup from jamulus." ) ) + htmlNewLine() +
                              tr ( "Check the device manual to see how you can check and change it's settings." ) );
        return false;
    }

    return true;
}

//============================================================================
// Device handling
//============================================================================

bool CSoundBase::selectDevice ( int iDriverIndex, bool bOpenDriverSetup, bool bTryAnyDriver )
{
    bool driverOk       = false;
    bool capabilitiesOk = false;
    bool aborted        = false;
    long lNumDevices    = strDeviceNames.count();
    long retries        = 0;
    int  startindex     = iDriverIndex;

    strErrorList.clear();

    if ( !soundProperties.bHasSetupDialog )
    {
        bOpenDriverSetup = false;
    }

    // find and load driver
    if ( ( iDriverIndex < 0 ) || ( iDriverIndex >= lNumDevices ) )
    {
        strErrorList.append ( htmlBold ( tr ( "The selected audio device is no longer present in the system." ) ) );
        strErrorList.append ( tr ( "Selecting The first usable device..." ) );
        startindex = iDriverIndex = 0;
        bTryAnyDriver             = true;
    }

    do
    {
        if ( !driverOk )
        {
            driverOk = checkDeviceChange ( tDeviceChangeCheck::CheckOpen, iDriverIndex );

            if ( !driverOk )
            {
                if ( bTryAnyDriver )
                {
                    strErrorList.append ( tr ( "Failed to open %1" ).arg ( strDeviceNames[iDriverIndex] ) );
                    iDriverIndex++;
                    if ( iDriverIndex >= lNumDevices )
                    {
                        iDriverIndex = 0;
                    }

                    if ( iDriverIndex == startindex )
                    {
                        aborted = true;
                    }
                    else
                    {
                        strErrorList.append ( "" );
                        strErrorList.append ( tr ( "Trying %1:" ).arg ( strDeviceNames[iDriverIndex] ) );
                        retries++;
                    }
                }
                else
                {
                    checkDeviceChange ( tDeviceChangeCheck::Abort, iDriverIndex );

                    if ( ( strErrorList.size() == 0 ) || ( iDriverIndex != startindex ) )
                    {
                        // Revert...
                        strErrorList.append ( htmlBold ( tr ( "Couldn't initialise the audio driver." ) ) );
                        strErrorList.append ( tr ( "Check if your audio hardware is plugged in and verify your driver settings." ) );
                    }

                    return false;
                }
            }
        }

        if ( driverOk )
        {
            // check device capabilities if it fulfills our requirements
            capabilitiesOk = checkDeviceChange ( tDeviceChangeCheck::CheckCapabilities, iDriverIndex ); // Checks if new device is usable

            // check if device is capable
            if ( !capabilitiesOk )
            {
                // if requested, open ASIO driver setup in case of an error
                if ( bOpenDriverSetup && !bTryAnyDriver )
                {
                    if ( openDeviceSetup() )
                    {
                        strErrorList.append ( htmlBold ( tr ( "Please check and correct your device settings..." ) ) );

                        QMessageBox confirm ( nullptr );
                        confirm.setIcon ( QMessageBox::Warning );
                        confirm.setWindowTitle ( QString ( APP_NAME ) + "-" + tr ( "Warning" ) );
                        confirm.setText ( getErrorString() );
                        confirm.addButton ( QMessageBox::Retry );
                        confirm.addButton ( QMessageBox::Abort );
                        confirm.exec();

                        strErrorList.clear();

                        switch ( confirm.result() )
                        {
                        case QMessageBox::Retry:
                            break;

                        case QMessageBox::Abort:
                        default:
                            aborted = true;
                        }
                    }
                }
                else
                {
                    if ( bTryAnyDriver )
                    {
                        checkDeviceChange ( tDeviceChangeCheck::Abort, iDriverIndex );
                        // Try next driver...
                        driverOk = false;
                        iDriverIndex++;
                        if ( iDriverIndex >= lNumDevices )
                        {
                            iDriverIndex = 0;
                        }

                        strErrorList.append ( "" );
                        if ( iDriverIndex == startindex )
                        {
                            strErrorList.append ( htmlBold ( tr ( "No usable audio devices found." ) ) );
                            aborted = true;
                        }
                        else
                        {
                            strErrorList.append ( tr ( "Trying %1:" ).arg ( strDeviceNames[iDriverIndex] ) );
                        }
                        retries++;
                    }
                    else
                    {
                        capabilitiesOk = true; // Ignore not ok...
                        strErrorList.append ( "" );
                        strErrorList.append ( htmlBold ( tr ( "Try to solve the above warnings." ) ) );
                        strErrorList.append ( tr ( "Then try selecting the device again or select another device.." ) );
                    }
                }
            }
        }

    } while ( !( driverOk && capabilitiesOk ) && ( iDriverIndex < lNumDevices ) && !aborted );

    if ( aborted && strErrorList.count() )
    {
        if ( bTryAnyDriver )
        {
            strErrorList.insert ( 0, htmlBold ( tr ( "No device could be opened due to the following errors(s):" ) ) + htmlNewLine() );
        }
        else
        {
            strErrorList.insert ( 0, htmlBold ( tr ( "The device could not be opened due to the following errors(s):" ) ) + htmlNewLine() );
        }
    }
    else if ( bTryAnyDriver && ( iDriverIndex >= lNumDevices ) )
    {
        strErrorList.append ( "" );
        strErrorList.append ( htmlBold ( tr ( "No usable audio devive available!" ) ) );
    }

    if ( driverOk && capabilitiesOk )
    {
        // We selected a valid driver....
        if ( strErrorList.count() )
        {
            if ( retries )
            {
                strErrorList.append ( htmlBold ( tr ( "Selecting %1" ).arg ( strDeviceNames[iDriverIndex] ) ) );
            }
            // If there is something in the errorlist, so these are warnings...
            // Show warnings now!
            CMsgBoxes::ShowWarning ( getErrorString() );
            strErrorList.clear();

            if ( checkDeviceChange ( tDeviceChangeCheck::Activate, iDriverIndex ) )
            {
                if ( soundProperties.bHasSetupDialog )
                {
                    OpenDeviceSetup();
                }
            }

            return true;
        }
        else
        {
            return checkDeviceChange ( tDeviceChangeCheck::Activate, iDriverIndex );
        }
    }

    checkDeviceChange ( tDeviceChangeCheck::Abort, iDriverIndex );

    return false;
}

bool CSoundBase::SetDevice ( const QString& strDevName, bool bOpenSetupOnError )
{
    QMutexLocker locker ( &mutexDeviceProperties );

    strErrorList.clear();
    createDeviceList( /* true */ ); // We should always re-create the devicelist when reading the ini file

    int iDriverIndex = strDevName.isEmpty() ? 0 : strDeviceNames.indexOf ( strDevName );

    if ( selectDevice ( iDriverIndex, bOpenSetupOnError, strDevName.isEmpty() ) ) // Calls openDevice and checkDeviceCapabilities
    {
        return true;
    }

    CMsgBoxes::ShowError ( getErrorString() );
    return false;
}

#ifdef OLD_SOUND_COMPATIBILITY
// Compatibilty function
QString CSoundBase::SetDev ( QString strDevName, bool bOpenSetupOnError )
{
    QMutexLocker locker ( &mutexDeviceProperties );

    strErrorList.clear();
    createDeviceList( /* true */ ); // We should always re-create the devicelist when reading the ini file

    int iDriverIndex = strDevName.isEmpty() ? 0 : strDeviceNames.indexOf ( strDevName );

    if ( selectDevice ( iDriverIndex, bOpenSetupOnError, strDevName.isEmpty() ) ) // Calls openDevice and checkDeviceCapabilities
    {
        return "";
    }

    return getErrorString();
}
#endif

bool CSoundBase::SetDevice ( int iDriverIndex, bool bOpenSetupOnError )
{
    QMutexLocker locker ( &mutexDeviceProperties );

    strErrorList.clear();
    createDeviceList();                                      // Only create devicelist if neccesary
    return selectDevice ( iDriverIndex, bOpenSetupOnError ); // Calls openDevice and checkDeviceCapabilities
}

bool CSoundBase::ReloadDevice ( bool bOpenSetupOnError )
{
    int  iWasDevice  = iCurrentDevice;
    bool bWasStarted = IsStarted();

    closeCurrentDevice();

    if ( iWasDevice >= 0 ) // Did we have a selected device ?
    {
        // can we re-select that device ?
        if ( selectDevice ( iWasDevice, bOpenSetupOnError ) )
        {
            // Re-start if it was started before...
            if ( bWasStarted )
            {
                Start();
            }

            return true;
        }
    }

    return false;
}

unsigned int CSoundBase::SetBufferSize ( unsigned int iDesiredBufferSize )
{
    CSoundStopper sound ( *this );

    iProtocolBufferSize = iDesiredBufferSize;
    iDeviceBufferSize   = getDeviceBufferSize ( iProtocolBufferSize );

    return iDeviceBufferSize;
}

bool CSoundBase::BufferSizeSupported ( unsigned int iDesiredBufferSize )
{
    unsigned int iActualBufferSize = getDeviceBufferSize ( iDesiredBufferSize );

    return ( iActualBufferSize == iDesiredBufferSize );
}

/******************************************************************************\
* MIDI handling                                                                *
\******************************************************************************/
void CSoundBase::parseMIDICommandLineArgument ( const QString& strMIDISetup )
{
    int iMIDIOffsetFader = 70; // Behringer X-TOUCH: offset of 0x46

    // parse the server info string according to definition: there is
    // the legacy definition with just one or two numbers that only
    // provides a definition for the controller offset of the level
    // controllers (default 70 for the sake of Behringer X-Touch)
    // [MIDI channel];[offset for level]
    //
    // The more verbose new form is a sequence of offsets for various
    // controllers: at the current point, 'f', 'p', 's', and 'm' are
    // parsed for fader, pan, solo, mute controllers respectively.
    // However, at the current point of time only 'f' and 'p'
    // controllers are actually implemented.  The syntax for a Korg
    // nanoKONTROL2 with 8 fader controllers starting at offset 0 and
    // 8 pan controllers starting at offset 16 would be
    //
    // [MIDI channel];f0*8;p16*8
    //
    // Namely a sequence of letters indicating the kind of controller,
    // followed by the offset of the first such controller, followed
    // by * and a count for number of controllers (if more than 1)
    if ( !strMIDISetup.isEmpty() )
    {
        // split the different parameter strings
        const QStringList slMIDIParams = strMIDISetup.split ( ";" );

        // [MIDI channel]
        if ( slMIDIParams.count() >= 1 )
        {
            iCtrlMIDIChannel = slMIDIParams[0].toUInt();
        }

        bool bSimple = true; // Indicates the legacy kind of specifying
                             // the fader controller offset without an
                             // indication of the count of controllers

        // [offset for level]
        if ( slMIDIParams.count() >= 2 )
        {
            int i = slMIDIParams[1].toUInt ( &bSimple );
            // if the second parameter can be parsed as a number, we
            // have the legacy specification of controllers.
            if ( bSimple )
                iMIDIOffsetFader = i;
        }

        if ( bSimple )
        {
            // For the legacy specification, we consider every controller
            // up to the maximum number of channels (or the maximum
            // controller number) a fader.
            for ( int i = 0; i < MAX_NUM_CHANNELS; i++ )
            {
                if ( i + iMIDIOffsetFader > 127 )
                {
                    break;
                }
                aMidiCtls[i + iMIDIOffsetFader] = { CMidiCtlEntry::tMidiCtlType::Fader, i };
            }
            return;
        }

        // We have named controllers

        for ( int i = 1; i < slMIDIParams.count(); i++ )
        {
            QString sParm = slMIDIParams[i].trimmed();
            if ( sParm.isEmpty() )
            {
                continue;
            }

            int iCtrl = QString ( sMidiCtlChar ).indexOf ( sParm[0] );
            if ( iCtrl < 0 )
                continue;
            CMidiCtlEntry::tMidiCtlType eTyp = static_cast<CMidiCtlEntry::tMidiCtlType> ( iCtrl );

            const QStringList slP    = sParm.mid ( 1 ).split ( '*' );
            int               iFirst = slP[0].toUInt();
            int               iNum   = ( slP.count() > 1 ) ? slP[1].toUInt() : 1;
            for ( int iOff = 0; iOff < iNum; iOff++ )
            {
                if ( iOff >= MAX_NUM_CHANNELS )
                    break;
                if ( iFirst + iOff >= 128 )
                    break;
                aMidiCtls[iFirst + iOff] = { eTyp, iOff };
            }
        }
    }
}

void CSoundBase::parseMIDIMessage ( const CVector<uint8_t>& vMIDIPaketBytes )
{
    if ( vMIDIPaketBytes.Size() > 0 )
    {
        const uint8_t iStatusByte = vMIDIPaketBytes[0];

        // check if status byte is correct
        if ( ( iStatusByte >= 0x80 ) && ( iStatusByte < 0xF0 ) )
        {
            // zero-based MIDI channel number (i.e. range 0-15)
            const int iMIDIChannelZB = iStatusByte & 0x0F;

            /*
            // debugging
            printf ( "%02X: ", iMIDIChannelZB );
            for ( int i = 0; i < vMIDIPaketBytes.Size(); i++ )
            {
                printf ( "%02X ", vMIDIPaketBytes[i] );
            }
            printf ( "\n" );
            */

            // per definition if MIDI channel is 0, we listen to all channels
            // note that iCtrlMIDIChannel is one-based channel number
            if ( ( iCtrlMIDIChannel == 0 ) || ( iCtrlMIDIChannel - 1 == iMIDIChannelZB ) )
            {
                // we only want to parse controller messages
                if ( ( iStatusByte >= 0xB0 ) && ( iStatusByte < 0xC0 ) )
                {
                    // make sure packet is long enough
                    if ( vMIDIPaketBytes.Size() > 2 && vMIDIPaketBytes[1] <= uint8_t ( 127 ) && vMIDIPaketBytes[2] <= uint8_t ( 127 ) )
                    {
                        const CMidiCtlEntry& cCtrl  = aMidiCtls[vMIDIPaketBytes[1]];
                        const int            iValue = vMIDIPaketBytes[2];
                        ;
                        switch ( cCtrl.eType )
                        {
                        case CMidiCtlEntry::tMidiCtlType::Fader:
                        {
                            // we are assuming that the controller number is the same
                            // as the audio fader index and the range is 0-127
                            const int iFaderLevel = static_cast<int> ( static_cast<double> ( iValue ) / 127 * AUD_MIX_FADER_MAX );

                            // consider offset for the faders

                            emit ControllerInFaderLevel ( cCtrl.iChannel, iFaderLevel );
                        }
                        break;
                        case CMidiCtlEntry::tMidiCtlType::Pan:
                        {
                            // Pan levels need to be symmetric between 1 and 127
                            const int iPanValue = static_cast<int> ( ( static_cast<double> ( qMax ( iValue, 1 ) ) - 1 ) / 126 * AUD_MIX_PAN_MAX );

                            emit ControllerInPanValue ( cCtrl.iChannel, iPanValue );
                        }
                        break;
                        case CMidiCtlEntry::tMidiCtlType::Solo:
                        {
                            // We depend on toggles reflecting the desired state
                            emit ControllerInFaderIsSolo ( cCtrl.iChannel, iValue >= 0x40 );
                        }
                        break;
                        case CMidiCtlEntry::tMidiCtlType::Mute:
                        {
                            // We depend on toggles reflecting the desired state
                            emit ControllerInFaderIsMute ( cCtrl.iChannel, iValue >= 0x40 );
                        }
                        break;
                        case CMidiCtlEntry::tMidiCtlType::MuteMyself:
                        {
                            // We depend on toggles reflecting the desired state to Mute Myself
                            emit ControllerInMuteMyself ( iValue >= 0x40 );
                        }
                        break;
                        default:
                            break;
                        }
                    }
                }
            }
        }
    }
}

//========================================================================
// CSoundStopper Used to temporarily stop CSound i.e. while changing settings.
// Stops CSound while in scope and Starts CSound again when going out of scope
// if it was started before.
// One can also query CSoundStopper to see if CSound was started and/or active.
//========================================================================

CSoundBase::CSoundStopper::CSoundStopper ( CSoundBase& sound ) : Sound ( sound ), bAborted ( false )
{
    bSoundWasActive  = Sound.IsActive();
    bSoundWasStarted = Sound.IsStarted();

    if ( bSoundWasStarted )
    {
        Sound.Stop();
    }
}

CSoundBase::CSoundStopper::~CSoundStopper()
{
    if ( bSoundWasStarted && !bAborted )
    {
        Sound.Start();
    }
}

bool CSoundBase::Start()
{
    if ( start() )
    {
        _onStarted();
        return true;
    }

    _onStopped();

    QString strError = getErrorString();
    if ( strError.size() )
    {
        CMsgBoxes::ShowError ( strError );
    }

    return false;
}

bool CSoundBase::Stop()
{
    if ( stop() )
    {
        _onStopped();
        return true;
    }

    // We didn't seem to be stopped
    // so restart active check
    _onStarted();

    QString strError = getErrorString();
    if ( strError.size() )
    {
        CMsgBoxes::ShowError ( strError );
    }

    return false;
}

bool CSoundBase::Restart()
{
    if ( IsStarted() )
    {
        Stop();
        return Start();
    }

    return false;
}
