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

#include <algorithm>
#include <cmath>
#include "audiocompressor.h"
#include "global.h"

namespace
{
// 20 / ln(10) and ln(10) / 20, used for fast dB <-> linear conversion
const float fLinToDbFact = 8.685889638f;
const float fDbToLinFact = 0.1151292546f;

// smallest level considered by the detector (-120 dBFS)
const float fMinDetectorLevel = 1e-6f;

const float afRatios[FX_COMP_NUM_RATIOS] = { 2.0f, 3.0f, 4.0f };

inline float DbToLin ( const float fDb ) { return expf ( fDb * fDbToLinFact ); }

inline float TimeConstToCoef ( const float fTimeS ) { return expf ( -1.0f / ( fTimeS * SYSTEM_SAMPLE_RATE_HZ ) ); }
} // namespace

CAudioCompressor::CAudioCompressor() :
    iNumChannels ( 1 ),
    iRatioIdx ( FX_COMP_RATIO_IDX_DEFAULT ),
    fThresholdDb ( FX_COMP_THRESHOLD_DB_DEFAULT ),
    fAttackCoef ( 0.0f ),
    fReleaseCoef ( 0.0f ),
    fGainReductionDb ( 0.0f )
{}

float CAudioCompressor::GetRatio ( const int iIdx )
{
    if ( ( iIdx < 0 ) || ( iIdx >= FX_COMP_NUM_RATIOS ) )
    {
        return afRatios[FX_COMP_RATIO_IDX_DEFAULT];
    }

    return afRatios[iIdx];
}

void CAudioCompressor::SetRatioIdx ( const int iNewIdx )
{
    if ( ( iNewIdx >= 0 ) && ( iNewIdx < FX_COMP_NUM_RATIOS ) )
    {
        iRatioIdx = iNewIdx;
    }
}

void CAudioCompressor::SetThresholdDb ( const float fNewThresholdDb )
{
    fThresholdDb = std::min ( static_cast<float> ( FX_COMP_THRESHOLD_DB_MAX ), std::max ( static_cast<float> ( FX_COMP_THRESHOLD_DB_MIN ), fNewThresholdDb ) );
}

void CAudioCompressor::Activate ( const int iNNumChannels, const int /* iMaxFrames */ )
{
    iNumChannels = iNNumChannels;
    fAttackCoef  = TimeConstToCoef ( FX_COMP_ATTACK_S );
    fReleaseCoef = TimeConstToCoef ( FX_COMP_RELEASE_S );
    Reset();
}

void CAudioCompressor::Reset() { fGainReductionDb = 0.0f; }

void CAudioCompressor::Process ( float* const* ppfChannels, const int iNumFrames )
{
    // read the parameters once per block
    const float fRatio    = GetRatio ( iRatioIdx );
    const float fThreshDb = fThresholdDb;
    const float fSlope    = 1.0f / fRatio - 1.0f;

    // makeup gain which brings a full scale input back to full scale
    const float fMakeupDb = -fThreshDb * ( 1.0f - 1.0f / fRatio );

    for ( int i = 0; i < iNumFrames; i++ )
    {
        // peak detector over all channels of the lane
        float fLevel = fabsf ( ppfChannels[0][i] );

        for ( int c = 1; c < iNumChannels; c++ )
        {
            fLevel = std::max ( fLevel, fabsf ( ppfChannels[c][i] ) );
        }

        // level detection in the log domain
        const float fLevelDb     = fLinToDbFact * logf ( std::max ( fLevel, fMinDetectorLevel ) );
        const float fOvershootDb = fLevelDb - fThreshDb;

        // static characteristic with soft knee
        float fTargetGrDb;

        if ( 2.0f * fOvershootDb < -FX_COMP_KNEE_DB )
        {
            // below the knee: no compression
            fTargetGrDb = 0.0f;
        }
        else if ( 2.0f * fOvershootDb > FX_COMP_KNEE_DB )
        {
            // above the knee: full ratio
            fTargetGrDb = fOvershootDb * fSlope;
        }
        else
        {
            // inside the knee: quadratic transition
            const float fKneePos = fOvershootDb + FX_COMP_KNEE_DB / 2;

            fTargetGrDb = fSlope * fKneePos * fKneePos / ( 2.0f * FX_COMP_KNEE_DB );
        }

        // smoothing: attack if the gain reduction increases, release otherwise
        if ( fTargetGrDb < fGainReductionDb )
        {
            fGainReductionDb = fAttackCoef * fGainReductionDb + ( 1.0f - fAttackCoef ) * fTargetGrDb;
        }
        else
        {
            fGainReductionDb = fReleaseCoef * fGainReductionDb + ( 1.0f - fReleaseCoef ) * fTargetGrDb;
        }

        // flush to zero: the release decays towards zero and would end up in
        // denormal numbers, which are very slow on many CPUs (real-time safety)
        if ( fGainReductionDb > -1e-6f )
        {
            fGainReductionDb = 0.0f;
        }

        const float fGain = DbToLin ( fGainReductionDb + fMakeupDb );

        for ( int c = 0; c < iNumChannels; c++ )
        {
            ppfChannels[c][i] *= fGain;
        }
    }
}
