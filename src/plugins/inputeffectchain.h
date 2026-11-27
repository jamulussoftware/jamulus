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
    Chain of local input effects, applied to the sound card input before the
    signal is sent to the server.

    The interleaved stereo sound card block is converted once to planar float
    and back. In stereo mode there is one lane with two channels, in the mono
    modes there are two lanes (left and right input) with one channel each.
    Each lane runs the effects in this fixed order: boost, compressor, reverb.

    With NO_INTERNAL_EFFECTS defined (qmake CONFIG+=nointernaleffects) the
    chain is empty; the parameter accessors are not available.
*/

#pragma once

#include "util.h"
#include "inputeffect.h"
#include "inputboost.h"
#include "audiocompressor.h"
#include "audioreverb.h"

/* Definitions ****************************************************************/
#define MAX_FX_LANES        2
#define MAX_FX_EFFECTS_LANE 3

/* Classes ********************************************************************/
class CInputEffectChain
{
public:
    CInputEffectChain();

    // called with the sound card stopped: allocates the buffers and activates
    // the effects for the given channel configuration
    void Init ( const EAudChanConf eNAudioChannelConf, const int iNMonoBlockSizeSam );

    // real-time safe: in place processing of an interleaved stereo block of
    // iMonoBlockSizeSam frames
    void Process ( CVector<int16_t>& vecsStereoInOut );

    // real-time safe
    bool IsAnyEnabled() const;

#ifndef NO_INTERNAL_EFFECTS
    CInputBoost&      Boost ( const int iLane ) { return Boost_[iLane]; }
    CAudioCompressor& Compressor ( const int iLane ) { return Compressor_[iLane]; }
    CAudioReverb&     Reverb ( const int iLane ) { return Reverb_[iLane]; }
#endif

protected:
    int            iNumLanes;           // 1 for stereo, else 2
    int            iNumChannelsPerLane; // 2 for stereo, else 1
    int            iMonoBlockSizeSam;
    int            iNumEffectsPerLane;
    CVector<float> vecfChan[MAX_FX_CHANNELS]; // planar buffers: left, right
    CInputEffect*  apEffects[MAX_FX_LANES][MAX_FX_EFFECTS_LANE];
    bool           abWasEnabled[MAX_FX_LANES][MAX_FX_EFFECTS_LANE];

#ifndef NO_INTERNAL_EFFECTS
    CInputBoost      Boost_[MAX_FX_LANES];
    CAudioCompressor Compressor_[MAX_FX_LANES];
    CAudioReverb     Reverb_[MAX_FX_LANES];
#endif
};
