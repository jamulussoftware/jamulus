/******************************************************************************\
 * Copyright (c) 2026
 *
 * Author(s):
 *  Serhii Popov (uazure@gmail.com)
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
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
\******************************************************************************/

/*
    Interface of one local input effect.

    An effect processes one "lane": one channel (left or right input in the
    mono modes) or two channels (stereo mode). The audio is planar float with
    full scale 1.0, which is also the audio port format of CLAP, so a future
    plugin host can be plugged in as another CInputEffect.

    Parameters are set from the GUI thread and read from the audio thread,
    therefore they are stored in std::atomic members.
*/

#pragma once

#include <atomic>

/* Definitions ****************************************************************/
// maximum number of channels in one lane
#define MAX_FX_CHANNELS 2

/* Classes ********************************************************************/
class CInputEffect
{
public:
    CInputEffect() : bEnabled ( false ) {} // all effects are off by default
    virtual ~CInputEffect() {}

    // called with the sound card stopped, may allocate memory
    virtual void Activate ( const int iNumChannels, const int iMaxFrames ) = 0;

    // real-time safe: clear delay lines, envelopes etc., keep the parameters
    virtual void Reset() = 0;

    // real-time safe, in place processing: ppfChannels holds iNumChannels (as
    // given to Activate) pointers to iNumFrames samples each
    virtual void Process ( float* const* ppfChannels, const int iNumFrames ) = 0;

    // switched on/off by the user
    void SetEnabled ( const bool bNewEnabled ) { bEnabled = bNewEnabled; }
    bool IsEnabled() const { return bEnabled; }

protected:
    std::atomic<bool> bEnabled;
};
