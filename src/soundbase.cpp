/******************************************************************************\
 * Copyright (c) 2004-2020
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

// This is used as a lookup table for parsing option letters, mapping
// a single character to an EMidiCtlType
char const sMidiCtlChar[] = {
    // Has to follow order of EMidiCtlType
    /* [EMidiCtlType::Fader] = */ 'f',
    /* [EMidiCtlType::Pan]   = */ 'p',
    /* [EMidiCtlType::Solo]  = */ 's',
    /* [EMidiCtlType::Mute]  = */ 'm',
    /* [EMidiCtlType::None]  = */ '\0' };

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

    bool currentInList = false;

    // put all device names in the string list
    for ( int iDev = 0; iDev < lNumDevs; iDev++ )
    {
        currentInList |= ( strDriverNames[iDev] == strCurDevName );
        slDevNames << strDriverNames[iDev];
    }

    if ( !currentInList )
    {
        slDevNames << strCurDevName;
    }

    return slDevNames;
}

QString CSoundBase::SetDev ( const QString strDevName )
{
    QMutexLocker locker ( &MutexDevProperties );

    if ( !strCurDevName.isEmpty() )
    {
        // a device was already been initialized and is used, first clean up
        // driver
        UnloadCurrentDriver();
    }

    strCurDevName = strDevName; // Update current device, even if loading fails.
    return LoadAndInitializeDriver ( strDevName, false );
}

QString CSoundBase::LoadAndInitializeFirstValidDriver ( const bool bOpenDriverSetup )
{
    QVector<QString> vsErrorList;

    // try all available drivers in the system ("lNumDevs" devices)
    for ( int iDriverCnt = 0; iDriverCnt < lNumDevs; iDriverCnt++ )
    {
        if ( !strCurDevName.isEmpty() )
        {
            UnloadCurrentDriver();
        }

        // try to load and initialize current driver, store error message
        strCurDevName = GetDeviceName ( iDriverCnt );
        const QString strCurError = LoadAndInitializeDriver ( strCurDevName, bOpenDriverSetup );

        vsErrorList.append ( strCurError );

        if ( strCurError.isEmpty() )
        {
            // empty error list shows that init was successful
            vsErrorList.clear();
            break;
        }
    }

    if ( !vsErrorList.isEmpty() )
    {
        // create error message with all details
        QString sErrorMessage = "<b>" + tr ( "No usable " ) +
            strSystemDriverTechniqueName +
            tr ( " audio device (driver) found." ) + "</b><br><br>" +
            tr ("In the following there is a list of all available drivers "
                "with the associated error message:" ) +
            "<ul>";

        for ( int i = 0; i < lNumDevs; i++ )
        {
            sErrorMessage += "<li><b>" + GetDeviceName ( i ) + "</b>: " + vsErrorList[i] + "</li>";
        }
        sErrorMessage += "</ul>";

        strCurDevName = ""; // clear device selection.
        return sErrorMessage;
    }

    return "";
}

/******************************************************************************\
* MIDI handling                                                                *
\******************************************************************************/
void CSoundBase::ParseCommandLineArgument ( const QString& strMIDISetup )
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
                    break;
                aMidiCtls[i + iMIDIOffsetFader] = { EMidiCtlType::Fader, i };
            }
            return;
        }

        // We have named controllers

        for ( int i = 1; i < slMIDIParams.count(); i++ )
        {
            QString sParm = slMIDIParams[i].trimmed();
            if ( sParm.isEmpty() )
                continue;

            int iCtrl = QString ( sMidiCtlChar ).indexOf ( sParm[0] );
            if ( iCtrl < 0 )
                continue;
            EMidiCtlType eTyp = static_cast<EMidiCtlType> ( iCtrl );

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
                        const CMidiCtlEntry& cCtrl  = aMidiCtls[vMIDIPaketBytes[1]];
                        const int            iValue = vMIDIPaketBytes[2];
                        ;
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
                        default:
                            break;
                        }
                    }
                }
            }
        }
    }
}
