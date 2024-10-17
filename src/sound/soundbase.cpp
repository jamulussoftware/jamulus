/******************************************************************************\
 * Copyright (c) 2004-2024
 *
 * Author(s):
 *  Volker Fischer
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

/* Implementation *************************************************************/
CSoundBase::CSoundBase ( const QString& strNewSystemDriverTechniqueName,
                         void ( *fpNewProcessCallback ) ( CVector<int16_t>& psData, void* pParg ),
                         void*          pParg,
                         const QString& strMIDISetup ) :
    fpProcessCallback ( fpNewProcessCallback ),
    pProcessCallbackArg ( pParg ),
    bRun ( false ),
    bCallbackEntered ( false ),
    strSystemDriverTechniqueName ( strNewSystemDriverTechniqueName ),
    iCtrlMIDIChannel ( INVALID_MIDI_CH ),
    aMidiCtls ( 128 )
{
    // parse the MIDI setup command line argument string
    ParseCommandLineArgument ( strMIDISetup );

    // initializations for the sound card names (default)
    lNumDevs          = 1;
    strDriverNames[0] = strSystemDriverTechniqueName;

    // set current device
    strCurDevName = ""; // default device
}

void CSoundBase::Stop()
{
    // set flag so that thread can leave the main loop
    bRun = false;

    // wait for draining the audio process callback
    QMutexLocker locker ( &MutexAudioProcessCallback );
}

/******************************************************************************\
* Device handling                                                              *
\******************************************************************************/
QStringList CSoundBase::GetDevNames()
{
    QMutexLocker locker ( &MutexDevProperties );

    QStringList slDevNames;

    // put all device names in the string list
    for ( int iDev = 0; iDev < lNumDevs; iDev++ )
    {
        slDevNames << strDriverNames[iDev];
    }

    return slDevNames;
}

QString CSoundBase::SetDev ( const QString strDevName )
{
    QMutexLocker locker ( &MutexDevProperties );

    // init return parameter with "no error"
    QString strReturn = "";

    // init flag for "load any driver"
    bool bTryLoadAnyDriver = false;

    // check if an ASIO driver was already initialized
    if ( !strCurDevName.isEmpty() )
    {
        // a device was already been initialized and is used, first clean up
        // driver
        UnloadCurrentDriver();

        const QString strErrorMessage = LoadAndInitializeDriver ( strDevName, false );

        if ( !strErrorMessage.isEmpty() )
        {
            if ( strDevName != strCurDevName )
            {
                // loading and initializing the new driver failed, go back to
                // original driver and create error message
                LoadAndInitializeDriver ( strCurDevName, false );

                // store error return message
                strReturn = QString ( tr ( "Can't use the selected audio device "
                                           "because of the following error: %1 "
                                           "The previous driver will be selected." )
                                          .arg ( strErrorMessage ) );
            }
            else
            {
                // loading and initializing the current driver failed, try to find
                // at least one usable driver
                bTryLoadAnyDriver = true;
            }
        }
    }
    else
    {
        if ( !strDevName.isEmpty() )
        {
            // This is the first time a driver is to be initialized, we first
            // try to load the selected driver, if this fails, we try to load
            // the first available driver in the system. If this fails, too, we
            // throw an error that no driver is available -> it does not make
            // sense to start the software if no audio hardware is available.
            if ( !LoadAndInitializeDriver ( strDevName, false ).isEmpty() )
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
        // if a driver was previously selected, show a warning message
        if ( !strDevName.isEmpty() )
        {
            strReturn = tr ( "The previously selected audio device "
                             "is no longer available or the driver has changed to an incompatible state. "
                             "We'll attempt to find a valid audio device, but this new audio device may cause feedback. "
                             "Before connecting to a server, please check your audio device settings." );
        }

        // try to load and initialize any valid driver
        QVector<QString> vsErrorList = LoadAndInitializeFirstValidDriver();

        if ( !vsErrorList.isEmpty() )
        {
            // create error message with all details
            QString sErrorMessage =
                tr ( "<b>%1 couldn't find a usable %2 audio device.</b><br><br>" ).arg ( APP_NAME ).arg ( strSystemDriverTechniqueName );

            for ( int i = 0; i < lNumDevs; i++ )
            {
                sErrorMessage += "<b>" + GetDeviceName ( i ) + "</b>: " + vsErrorList[i] + "<br><br>";
            }

#if defined( _WIN32 ) && !defined( WITH_JACK )
            // to be able to access the ASIO driver setup for changing, e.g., the sample rate, we
            // offer the user under Windows that we open the driver setups of all registered
            // ASIO drivers
            sErrorMessage += "<br>" + tr ( "You may be able to fix errors in the driver settings. Do you want to open these settings now?" );

            if ( QMessageBox::Yes == QMessageBox::information ( nullptr, APP_NAME, sErrorMessage, QMessageBox::Yes | QMessageBox::No ) )
            {
                LoadAndInitializeFirstValidDriver ( true );
            }

            sErrorMessage = QString ( tr ( "Can't start %1. Please restart %1 and check/reconfigure your audio settings." ) ).arg ( APP_NAME );
#endif

            throw CGenErr ( sErrorMessage );
        }
    }

    return strReturn;
}

QVector<QString> CSoundBase::LoadAndInitializeFirstValidDriver ( const bool bOpenDriverSetup )
{
    QVector<QString> vsErrorList;

    // load and initialize first valid ASIO driver
    bool bValidDriverDetected = false;
    int  iDriverCnt           = 0;

    // try all available drivers in the system ("lNumDevs" devices)
    while ( !bValidDriverDetected && ( iDriverCnt < lNumDevs ) )
    {
        // try to load and initialize current driver, store error message
        const QString strCurError = LoadAndInitializeDriver ( GetDeviceName ( iDriverCnt ), bOpenDriverSetup );

        vsErrorList.append ( strCurError );

        if ( strCurError.isEmpty() )
        {
            // initialization was successful
            bValidDriverDetected = true;

            // store ID of selected driver
            strCurDevName = GetDeviceName ( iDriverCnt );

            // empty error list shows that init was successful
            vsErrorList.clear();
        }

        // try next driver
        iDriverCnt++;
    }

    return vsErrorList;
}

/******************************************************************************\
* Command Line Handling                                                        *
\******************************************************************************/
void CSoundBase::ParseCommandLineArgument ( const QString& strMIDISetup )
{
    if ( strMIDISetup.isEmpty() )
    {
        // should be caught in main.cpp
        return;
    }

    // Parse the --ctrlmidich string.  There are two formats.
    // Default to the legacy kind of specifying the fader controller offset
    // without an indication of the count of controllers.
    bool bSimple = true;

    // split the different parameter strings
    const QStringList slMIDIParams = strMIDISetup.split ( ";" );
    int               iNumParams   = slMIDIParams.count();

    if ( iNumParams >= 1 )
    {
        bool bChOK = false;
        int  i     = slMIDIParams[0].toUInt ( &bChOK );
        if ( bChOK )
        {
            // [MIDI channel] supplied (else use default)
            iCtrlMIDIChannel = i;
        }
        else
        {
            // iCtrlMIDIChannel == INVALID_MIDI_CH, so no point continuing
            return;
        }
    }

    // Use Behringer X-TOUCH as default offset of 0x46
    int iMIDIOffsetFader = 70;
    if ( iNumParams >= 2 )
    {
        // if there is a second parameter that can be parsed as a number,
        // we have the legacy specification of controllers.
        int i = slMIDIParams[1].toUInt ( &bSimple );
        if ( bSimple )
        {
            // [offset for fader] supplied (else use default)
            iMIDIOffsetFader = i;
        }
    }

    if ( bSimple )
    {
        // For the legacy specification, we consider every controller
        // up to the maximum number of channels (or the maximum
        // controller number) a fader.
        for ( int i = 0; i + iMIDIOffsetFader <= 127 && i < MAX_NUM_CHANNELS; i++ )
        {
            // add a list entry for the CMidiCtlEntry
            aMidiCtls[i + iMIDIOffsetFader] = { EMidiCtlType::Fader, i };
        }
        return;
    }

    // We have named controllers
    // Validate and see whether "MyChannel" option is present

    bool        bMyChannel = false;
    QStringList slValid; // keep track of valid entries to make later processing simple

    for ( int i = 0; i < iNumParams; i++ )
    {
        QString sParm = slMIDIParams[i].trimmed();

        if ( sParm.isEmpty() )
        {
            // skip empty entries silently
            continue;
        }

        int iCtrl = sMidiCtl.indexOf ( sParm[0] );
        if ( iCtrl < 0 )
        {
            // skip unknown entries silently
            continue;
        }

        if ( static_cast<EMidiCtlType> ( iCtrl ) == EMidiCtlType::MyChannel )
        {
            // once seen, just remember this
            bMyChannel = true;
            continue;
        }

        const QStringList slP = sParm.mid ( 1 ).split ( '*' );

        if ( slP.count() > 2 )
        {
            // skip invalid entries silently
            continue;
        }

        bool bIsUInt = false;

        unsigned int u = slP[0].toUInt ( &bIsUInt );
        if ( !bIsUInt )
        {
            // skip invalid entries silently
            continue;
        }
        int iFirst = u;

        // silently default incoherent count to 1
        int iNum = 1;
        if ( static_cast<EMidiCtlType> ( iCtrl ) != EMidiCtlType::MuteMyself && slP.count() == 2 )
        {
            bIsUInt        = false;
            unsigned int u = slP[1].toUInt ( &bIsUInt );
            if ( bIsUInt )
            {
                iNum = u;
            }
        }

        // store the valid entry in a more splittable format
        slValid.append ( QString ( "%1*%2*%3" ).arg ( iCtrl ).arg ( iFirst ).arg ( iNum ) );
    }

    foreach ( QString sParm, slValid )
    {
        const QStringList  slP    = sParm.split ( '*' );
        const EMidiCtlType eTyp   = static_cast<EMidiCtlType> ( slP[0].toInt() );
        const int          iFirst = slP[1].toUInt();
        const int          iNum   = slP[2].toUInt();
        for ( int iOff = 0; iOff < iNum && iOff + iFirst <= 127 && iOff < MAX_NUM_CHANNELS; iOff++ )
        {
            // For MyChannel option, first offset is "MyChannel", then the rest are 0 to iNum-1 channels
            if ( bMyChannel )
            {
                aMidiCtls[iFirst + iOff] = { eTyp, iOff == 0 ? I_MY_CHANNEL : iOff - 1 };
            }
            else
            {
                aMidiCtls[iFirst + iOff] = { eTyp, iOff };
            }
        }
    }
}

/******************************************************************************\
* MIDI handling                                                                *
\******************************************************************************/
void CSoundBase::ParseMIDIMessage ( const CVector<uint8_t>& vMIDIPaketBytes )
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
                        // Where "MyChannel" is in effect, cCtrl.iChannel will be I_MY_CHANNEL
                        // for the first CC number in the range for a cCtrl.eType and then zero upwards.
                        const CMidiCtlEntry& cCtrl  = aMidiCtls[vMIDIPaketBytes[1]];
                        const int            iValue = vMIDIPaketBytes[2];

                        switch ( cCtrl.eType )
                        {
                        case Fader:
                        {
                            // we are assuming that the controller number is the same
                            // as the audio fader index and the range is 0-127
                            const int iFaderLevel = static_cast<int> ( static_cast<double> ( iValue ) / 127 * AUD_MIX_FADER_MAX );

                            // consider offset for the faders

                            emit ControllerInFaderLevel ( cCtrl.iChannel, iFaderLevel );
                        }
                        break;
                        case Pan:
                        {
                            // Pan levels need to be symmetric between 1 and 127
                            const int iPanValue = static_cast<int> ( static_cast<double> ( qMax ( iValue, 1 ) - 1 ) / 126 * AUD_MIX_PAN_MAX );

                            emit ControllerInPanValue ( cCtrl.iChannel, iPanValue );
                        }
                        break;
                        case Solo:
                        {
                            // We depend on toggles reflecting the desired state
                            emit ControllerInFaderIsSolo ( cCtrl.iChannel, iValue >= 0x40 );
                        }
                        break;
                        case Mute:
                        {
                            // We depend on toggles reflecting the desired state
                            emit ControllerInFaderIsMute ( cCtrl.iChannel, iValue >= 0x40 );
                        }
                        break;
                        case MuteMyself:
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
