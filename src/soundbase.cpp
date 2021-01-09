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
                         const QString& strMIDISetup ) :
    fpProcessCallback            ( fpNewProcessCallback ),
    pProcessCallbackArg          ( pParg ),
    bRun                         ( false ),
    bCallbackEntered             ( false ),
    strSystemDriverTechniqueName ( strNewSystemDriverTechniqueName ),
    iCtrlMIDIChannel             ( INVALID_MIDI_CH ),
    iMIDIOffsetFader             ( 70 ) // Behringer X-TOUCH: offset of 0x46
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
* MIDI handling                                                                *
\******************************************************************************/
void CSoundBase::ParseCommandLineArgument ( const QString& strMIDISetup )
{
    // parse the server info string according to definition:
    // [MIDI channel];[offset for level]
    if ( !strMIDISetup.isEmpty() )
    {
        // split the different parameter strings
        const QStringList slMIDIParams = strMIDISetup.split ( ";" );

        // [MIDI channel]
        if ( slMIDIParams.count() >= 1 )
        {
            iCtrlMIDIChannel = slMIDIParams[0].toUInt();
        }

        // [offset for level]
        if ( slMIDIParams.count() >= 2 )
        {
            iMIDIOffsetFader = slMIDIParams[1].toUInt();
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
                    if ( vMIDIPaketBytes.Size() > 2 )
                    {
                        // we are assuming that the controller number is the same
                        // as the audio fader index and the range is 0-127
                        const int iFaderLevel = static_cast<int> ( static_cast<double> (
                            qMin ( vMIDIPaketBytes[2], uint8_t ( 127 ) ) ) / 127 * AUD_MIX_FADER_MAX );

                        // consider offset for the faders
                        const int iChID = vMIDIPaketBytes[1] - iMIDIOffsetFader;

                        emit ControllerInFaderLevel ( iChID, iFaderLevel );
                    }
                }
            }
        }
    }
}
