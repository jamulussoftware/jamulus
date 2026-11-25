/******************************************************************************\
 * Copyright (c) 2026
 *
 * Author(s):
 *  uazure
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
    Feed-forward compressor with automatic makeup gain, following
    D. Giannoulis, M. Massberg, J. D. Reiss, "Digital Dynamic Range Compressor
    Design - A Tutorial and Analysis", J. Audio Eng. Soc., Vol. 60, No. 6, 2012:
    soft-knee static curve in the log domain, followed by a smoothed gain
    reduction with separate attack and release time constants. There is no
    look-ahead, so no latency is added. A two channel lane uses one common
    detector so that the stereo image does not shift.
*/

#pragma once

#include "inputeffect.h"

/* Definitions ****************************************************************/
// selectable ratios (index into the table)
#define FX_COMP_NUM_RATIOS       3
#define FX_COMP_RATIO_IDX_DEFAULT 0 // 2:1

#define FX_COMP_THRESHOLD_DB_MIN     ( -40 )
#define FX_COMP_THRESHOLD_DB_MAX     0
#define FX_COMP_THRESHOLD_DB_DEFAULT ( -20 )

// fixed (not configurable) dynamics parameters
#define FX_COMP_ATTACK_S  0.005f
#define FX_COMP_RELEASE_S 0.1f
#define FX_COMP_KNEE_DB   6.0f

/* Classes ********************************************************************/
class CAudioCompressor : public CInputEffect
{
public:
    CAudioCompressor();

    void  SetRatioIdx ( const int iNewIdx );
    int   GetRatioIdx() const { return iRatioIdx; }
    void  SetThresholdDb ( const float fNewThresholdDb );
    float GetThresholdDb() const { return fThresholdDb; }

    static float GetRatio ( const int iIdx );

    virtual void Activate ( const int iNNumChannels, const int ) override;
    virtual void Reset() override;
    virtual void Process ( float* const* ppfChannels, const int iNumFrames ) override;

protected:
    int                iNumChannels;
    std::atomic<int>   iRatioIdx;
    std::atomic<float> fThresholdDb;
    float              fAttackCoef;
    float              fReleaseCoef;
    float              fGainReductionDb; // smoothed envelope, audio thread only
};
