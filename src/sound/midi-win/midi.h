/******************************************************************************\
 * Copyright (c) 2024-2024
 *
 * Author(s):
 *  Tony Mountifield
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

#pragma once

#include "../../util.h"
#include "../../global.h"

/* Classes ********************************************************************/
class CMidi
{
public:
    CMidi() {}

    virtual ~CMidi() {}

    void MidiStart();
    void MidiStop();

protected:
    int              iMidiDevs;
    QVector<HMIDIIN> vecMidiInHandles; // windows handles

    static void CALLBACK MidiCallback ( HMIDIIN hMidiIn, UINT wMsg, DWORD_PTR dwInstance, DWORD_PTR dwParam1, DWORD_PTR dwParam2 );
};

// Provide the definition of CSound for the MIDI callback
// This must be after the above definition of CMidi
#if defined( JACK_ON_WINDOWS )
#    include "sound/jack/sound.h"
#else
#    include "sound/asio/sound.h"
#endif
