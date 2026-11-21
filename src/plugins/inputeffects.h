/******************************************************************************\
 * Copyright (c) 2026
 *
 * Author(s):
 *  Serhii Popov
 *
 * As of Jamulus 3.12.1dev (commit eb172d47): All new source code contributions must be licensed
 * under AGPL 3.0 or any later version.
 *
 ******************************************************************************
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
\******************************************************************************/

/*
    Local input effects chain, applied per input channel (L/R) before the
    pan/mono mix: boost -> compressor (with automatic makeup gain) -> reverb.

    The compressor follows the feed-forward design described in
    D. Giannoulis, M. Massberg, J. D. Reiss, "Digital Dynamic Range Compressor
    Design - A Tutorial and Analysis", J. Audio Eng. Soc., Vol. 60, No. 6, 2012:
    soft-knee static curve in the log domain, followed by a smoothed gain
    reduction with separate attack and release time constants. There is no
    look-ahead, so no latency is added.
*/

#pragma once

#include "util.h"
#include "audioreverb.h"

/* Definitions ****************************************************************/
// number of local input channels which can be processed independently
#define NUM_FX_CHANNELS 2

// effect parameter ranges (as stored in the settings and shown in the GUI)
#define FX_BOOST_DB_MAX              24
#define FX_COMP_RATIO_IDX_MAX        3 // 0: off (1:1), 1: 2:1, 2: 3:1, 3: 4:1
#define FX_COMP_THRESHOLD_DB_MIN     ( -40 )
#define FX_COMP_THRESHOLD_DB_MAX     0
#define FX_COMP_THRESHOLD_DB_DEFAULT ( -20 )

// fixed (not configurable) dynamics parameters
#define FX_COMP_ATTACK_S  0.005f
#define FX_COMP_RELEASE_S 0.1f
#define FX_COMP_KNEE_DB   6.0f

/* Classes ********************************************************************/
// parameters for one processing block, prepared by the caller from the
// current settings
struct SInputEffectsParams
{
    bool  bLinked;                             // use channel 0 settings for both channels
    float fBoostDb[NUM_FX_CHANNELS];           // 0 .. FX_BOOST_DB_MAX
    float fCompRatio[NUM_FX_CHANNELS];         // 1 means compressor off
    float fCompThresholdDb[NUM_FX_CHANNELS];   // FX_COMP_THRESHOLD_DB_MIN .. 0
    float fReverbAttenuation[NUM_FX_CHANNELS]; // as used by CAudioReverb::Process, 0 means off
};

class CAudioCompressor
{
public:
    CAudioCompressor() : fAttackCoef ( 0 ), fReleaseCoef ( 0 ), fGainReductionDb ( 0 ) {}

    void Init ( const int iSampleRate );
    void Reset() { fGainReductionDb = 0; }

    // takes the detector level (linear, full scale = 1) and returns the
    // smoothed gain reduction in dB (always <= 0)
    float Process ( const float fLevel, const float fThresholdDb, const float fRatio );

    // makeup gain which brings a full scale input back to full scale
    static float GetAutoMakeupDb ( const float fThresholdDb, const float fRatio ) { return -fThresholdDb * ( 1.0f - 1.0f / fRatio ); }

protected:
    float fAttackCoef;
    float fReleaseCoef;
    float fGainReductionDb;
};

class CInputEffects
{
public:
    CInputEffects();

    // must not be called while Process() may run (allocates memory)
    void Init ( const EAudChanConf eNAudioChannelConf, const int iNMonoBlockSizeSam, const int iSampleRate );

    // in-place processing of an interleaved stereo block
    void Process ( CVector<int16_t>& vecsStereoInOut, const SInputEffectsParams& Params );

protected:
    EAudChanConf     eAudioChannelConf;
    int              iMonoBlockSizeSam;
    CAudioCompressor Compressor[NUM_FX_CHANNELS];
    CAudioReverb     Reverb[NUM_FX_CHANNELS];
};
