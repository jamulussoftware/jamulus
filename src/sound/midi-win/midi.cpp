/******************************************************************************\
 * Copyright (c) 2024-2025
 *
 * Author(s):
 *  Tony Mountifield
 *
 * Description:
 * MIDI interface for Windows operating systems
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

#include "midi.h"

/* Implementation *************************************************************/

// pointer to the sound object (for passing received MIDI messages upwards)
extern CSound* pSound;

//---------------------------------------------------------------------------------------
// Windows Native MIDI support
//
// For API, see https://learn.microsoft.com/en-gb/windows/win32/multimedia/midi-reference

void CMidi::MidiStart()
{
    QString selMIDIDevice = pSound->GetMIDIDevice();

    /* Get the number of MIDI In devices in this computer */
    iMidiDevs = midiInGetNumDevs();

    qInfo() << qUtf8Printable ( QString ( "- MIDI devices found: %1" ).arg ( iMidiDevs ) );

    // open all connected MIDI devices and set the callback function to handle incoming messages
    for ( int i = 0; i < iMidiDevs; i++ )
    {
        HMIDIIN    hMidiIn; // windows handle
        MIDIINCAPS mic;     // device name and capabilities

        MMRESULT result = midiInGetDevCaps ( i, &mic, sizeof ( MIDIINCAPS ) );

        if ( result != MMSYSERR_NOERROR )
        {
            qWarning() << qUtf8Printable ( QString ( "! Failed to identify MIDI input device %1. Error code: %2" ).arg ( i ).arg ( result ) );
            continue; // try next device, if any
        }

        QString midiDev ( mic.szPname );

        if ( !selMIDIDevice.isEmpty() && selMIDIDevice != midiDev )
        {
            qInfo() << qUtf8Printable ( QString ( "  %1: %2 (ignored)" ).arg ( i ).arg ( midiDev ) );
            continue; // try next device, if any
        }

        qInfo() << qUtf8Printable ( QString ( "  %1: %2" ).arg ( i ).arg ( midiDev ) );

        result = midiInOpen ( &hMidiIn, i, (DWORD_PTR) MidiCallback, 0, CALLBACK_FUNCTION );

        if ( result != MMSYSERR_NOERROR )
        {
            qWarning() << qUtf8Printable ( QString ( "! Failed to open MIDI input device %1. Error code: %2" ).arg ( i ).arg ( result ) );
            continue; // try next device, if any
        }

        result = midiInStart ( hMidiIn );

        if ( result != MMSYSERR_NOERROR )
        {
            qWarning() << qUtf8Printable ( QString ( "! Failed to start MIDI input device %1. Error code: %2" ).arg ( i ).arg ( result ) );
            midiInClose ( hMidiIn );
            continue; // try next device, if any
        }

        // success, add it to list of open handles
        vecMidiInHandles.append ( hMidiIn );
    }
}

void CMidi::MidiStop()
{
    // stop MIDI if running
    for ( int i = 0; i < vecMidiInHandles.size(); i++ )
    {
        midiInStop ( vecMidiInHandles.at ( i ) );
        midiInClose ( vecMidiInHandles.at ( i ) );
    }
}

// See https://learn.microsoft.com/en-us/previous-versions//dd798460(v=vs.85)
// for the definition of the MIDI input callback function.
void CALLBACK CMidi::MidiCallback ( HMIDIIN hMidiIn, UINT wMsg, DWORD_PTR dwInstance, DWORD_PTR dwParam1, DWORD_PTR dwParam2 )
{
    Q_UNUSED ( hMidiIn );
    Q_UNUSED ( dwInstance );
    Q_UNUSED ( dwParam2 );

    if ( wMsg == MIM_DATA )
    {
        // See https://learn.microsoft.com/en-gb/windows/win32/multimedia/mim-data
        // The three bytes of a MIDI message are encoded into the 32-bit dwParam1 parameter.
        BYTE status = dwParam1 & 0xFF;
        BYTE data1  = ( dwParam1 >> 8 ) & 0xFF;
        BYTE data2  = ( dwParam1 >> 16 ) & 0xFF;

        // copy packet and send it to the MIDI parser
        CVector<uint8_t> vMIDIPaketBytes ( 3 );

        vMIDIPaketBytes[0] = static_cast<uint8_t> ( status );
        vMIDIPaketBytes[1] = static_cast<uint8_t> ( data1 );
        vMIDIPaketBytes[2] = static_cast<uint8_t> ( data2 );

        pSound->ParseMIDIMessage ( vMIDIPaketBytes );
    }
}
