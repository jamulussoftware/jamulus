/******************************************************************************\
 * Copyright (c) 2021
 *
 * Author(s):
 *  Noam Postavsky
 *
 * Description:
 * Sound card interface using the portaudio library
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

#include "portaudiosound.h"

#include <portaudio.h>

CSound::CSound ( void (*fpNewProcessCallback) ( CVector<short>& psData, void* arg ),
                 void*          arg,
                 const QString& strMIDISetup,
                 const bool,
                 const QString& ) :
    CSoundBase ( "portaudio", fpNewProcessCallback, arg, strMIDISetup )
{
    PaError err = Pa_Initialize();
    if ( err != paNoError )
    {
        throw CGenErr ( tr ( "Failed to initialize PortAudio: %1" ).arg ( Pa_GetErrorText ( err ) ) );
    }
}
