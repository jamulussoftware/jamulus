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


/* Implementation *************************************************************/
CSoundBase::CSoundBase ( const QString& strNewSystemDriverTechniqueName,
                         void           (*fpNewProcessCallback) ( CVector<int16_t>& psData, void* pParg ),
                         void*          pParg,
                         const int      iNewCtrlMIDIChannel ) :
    fpProcessCallback            ( fpNewProcessCallback ),
    pProcessCallbackArg          ( pParg ), bRun ( false ),
    strSystemDriverTechniqueName ( strNewSystemDriverTechniqueName ),
    iCtrlMIDIChannel             ( iNewCtrlMIDIChannel )
{
    // initializations for the sound card names (default)
    lNumDevs          = 1;
    strDriverNames[0] = strSystemDriverTechniqueName;

    // set current device
    lCurDev = 0; // default device
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
QString CSoundBase::SetDev ( const int iNewDev )
{
    // init return parameter with "no error"
    QString strReturn = "";

    // first check if valid input parameter
    if ( iNewDev >= lNumDevs )
    {
        // we should actually never get here...
        return tr ( "Invalid device selection." );
    }

    // check if an ASIO driver was already initialized
    if ( lCurDev != INVALID_INDEX )
    {
        // a device was already been initialized and is used, first clean up
        // driver
        UnloadCurrentDriver();

        const QString strErrorMessage = LoadAndInitializeDriver ( iNewDev, false );

        if ( !strErrorMessage.isEmpty() )
        {
            if ( iNewDev != lCurDev )
            {
                // loading and initializing the new driver failed, go back to
                // original driver and display error message
                LoadAndInitializeDriver ( lCurDev, false );
            }
            else
            {
                // the same driver is used but the driver properties seems to
                // have changed so that they are not compatible to our
                // software anymore
#ifndef HEADLESS
                QMessageBox::critical (
                    nullptr, APP_NAME, QString ( tr ( "The audio driver properties "
                    "have changed to a state which is incompatible with this "
                    "software. The selected audio device could not be used "
                    "because of the following error:" ) + " <b>" ) +
                    strErrorMessage +
                    QString ( "</b><br><br>" + tr ( "Please restart the software." ) ),
                    tr ( "Close" ), nullptr );
#endif

                _exit ( 0 );
            }

            // store error return message
            strReturn = strErrorMessage;
        }
    }
    else
    {
        // init flag for "load any driver"
        bool bTryLoadAnyDriver = false;

        if ( iNewDev != INVALID_INDEX )
        {
            // This is the first time a driver is to be initialized, we first
            // try to load the selected driver, if this fails, we try to load
            // the first available driver in the system. If this fails, too, we
            // throw an error that no driver is available -> it does not make
            // sense to start the software if no audio hardware is available.
            if ( !LoadAndInitializeDriver ( iNewDev, false ).isEmpty() )
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

        if ( bTryLoadAnyDriver )
        {
            // try to load and initialize any valid driver
            QVector<QString> vsErrorList = LoadAndInitializeFirstValidDriver();

            if ( !vsErrorList.isEmpty() )
            {
                // create error message with all details
                QString sErrorMessage = "<b>" + tr ( "No usable " ) +
                    strSystemDriverTechniqueName + tr ( " audio device "
                    "(driver) found." ) + "</b><br><br>" + tr (
                    "In the following there is a list of all available drivers "
                    "with the associated error message:" ) + "<ul>";

                for ( int i = 0; i < lNumDevs; i++ )
                {
                    sErrorMessage += "<li><b>" + GetDeviceName ( i ) + "</b>: " + vsErrorList[i] + "</li>";
                }
                sErrorMessage += "</ul>";

#ifdef _WIN32
                // to be able to access the ASIO driver setup for changing, e.g., the sample rate, we
                // offer the user under Windows that we open the driver setups of all registered
                // ASIO drivers
                sErrorMessage = sErrorMessage + "<br/>" + tr ( "Do you want to open the ASIO driver setups?" );

                if ( QMessageBox::Yes == QMessageBox::information ( nullptr, APP_NAME, sErrorMessage, QMessageBox::Yes|QMessageBox::No ) )
                {
                    LoadAndInitializeFirstValidDriver ( true );
                }

                sErrorMessage = APP_NAME + tr ( " could not be started because of audio interface issues." );
#endif

                throw CGenErr ( sErrorMessage );
            }
        }
    }

    return strReturn;
}

QVector<QString> CSoundBase::LoadAndInitializeFirstValidDriver ( const bool bOpenDriverSetup )
{
    QVector<QString> vsErrorList;

    // load and initialize first valid ASIO driver
    bool bValidDriverDetected = false;
    int  iCurDriverIdx        = 0;

    // try all available drivers in the system ("lNumDevs" devices)
    while ( !bValidDriverDetected && ( iCurDriverIdx < lNumDevs ) )
    {
        // try to load and initialize current driver, store error message
        const QString strCurError = LoadAndInitializeDriver ( iCurDriverIdx, bOpenDriverSetup );

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
                    if ( vMIDIPaketBytes.Size() > 2 )
                    {
                        // we are assuming that the controller number is the same
                        // as the audio fader index and the range is 0-127
                        const int iFaderLevel = static_cast<int> ( static_cast<double> (
                            qMin ( vMIDIPaketBytes[2], uint8_t ( 127 ) ) ) / 127 * AUD_MIX_FADER_MAX );

                        // Behringer X-TOUCH: offset of 0x46
                        const int iChID = vMIDIPaketBytes[1] - 70;

                        EmitControllerInFaderLevel ( iChID, iFaderLevel );
                    }
                }
            }
        }
    }
}
