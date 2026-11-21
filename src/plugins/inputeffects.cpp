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

#include <algorithm>
#include <cmath>
#include "inputeffects.h"

namespace
{
// 20 / ln(10) and ln(10) / 20, used for fast dB <-> linear conversion
const float fLinToDbFact = 8.685889638f;
const float fDbToLinFact = 0.1151292546f;

// smallest level considered by the compressor detector (-120 dBFS)
const float fMinDetectorLevel = 1e-6f;

inline float DbToLin ( const float fDb ) { return expf ( fDb * fDbToLinFact ); }

inline float TimeConstToCoef ( const float fTimeS, const int iSampleRate ) { return expf ( -1.0f / ( fTimeS * iSampleRate ) ); }
} // namespace

/******************************************************************************\
* Compressor                                                                   *
\******************************************************************************/
void CAudioCompressor::Init ( const int iSampleRate )
{
    fAttackCoef  = TimeConstToCoef ( FX_COMP_ATTACK_S, iSampleRate );
    fReleaseCoef = TimeConstToCoef ( FX_COMP_RELEASE_S, iSampleRate );
    Reset();
}

float CAudioCompressor::Process ( const float fLevel, const float fThresholdDb, const float fRatio )
{
    // level detection in the log domain
    const float fLevelDb     = fLinToDbFact * logf ( std::max ( fLevel, fMinDetectorLevel ) );
    const float fOvershootDb = fLevelDb - fThresholdDb;

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
        fTargetGrDb = fOvershootDb * ( 1.0f / fRatio - 1.0f );
    }
    else
    {
        // inside the knee: quadratic transition
        const float fKneePos = fOvershootDb + FX_COMP_KNEE_DB / 2;

        fTargetGrDb = ( 1.0f / fRatio - 1.0f ) * fKneePos * fKneePos / ( 2.0f * FX_COMP_KNEE_DB );
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

    return fGainReductionDb;
}

/******************************************************************************\
* Input effects chain                                                          *
\******************************************************************************/
CInputEffects::CInputEffects() : eAudioChannelConf ( CC_MONO ), iMonoBlockSizeSam ( 0 ) {}

void CInputEffects::Init ( const EAudChanConf eNAudioChannelConf, const int iNMonoBlockSizeSam, const int iSampleRate )
{
    eAudioChannelConf = eNAudioChannelConf;
    iMonoBlockSizeSam = iNMonoBlockSizeSam;

    for ( int iCh = 0; iCh < NUM_FX_CHANNELS; iCh++ )
    {
        Compressor[iCh].Init ( iSampleRate );
    }

    // In stereo mode a single reverb processes both channels (as before). In
    // the mono modes each channel has its own reverb, which only touches the
    // selected channel of the stereo buffer.
    if ( eAudioChannelConf == CC_STEREO )
    {
        Reverb[0].Init ( CC_STEREO, 2 * iMonoBlockSizeSam, iSampleRate );
    }
    else
    {
        Reverb[0].Init ( CC_MONO, 2 * iMonoBlockSizeSam, iSampleRate );
        Reverb[1].Init ( CC_MONO, 2 * iMonoBlockSizeSam, iSampleRate );
    }
}

void CInputEffects::Process ( CVector<int16_t>& vecsStereoInOut, const SInputEffectsParams& Params )
{
    // In stereo mode the two channels are one source, therefore the settings
    // are always linked and the compressor uses a common detector so that the
    // stereo image does not shift.
    const bool bStereo               = ( eAudioChannelConf == CC_STEREO );
    const int  iParCh1               = ( bStereo || Params.bLinked ) ? 0 : 1;
    const int  iPar[NUM_FX_CHANNELS] = { 0, iParCh1 };

    float fBoost[NUM_FX_CHANNELS];
    float fMakeup[NUM_FX_CHANNELS];
    bool  bCompOn[NUM_FX_CHANNELS];
    bool  bGainOn[NUM_FX_CHANNELS];

    for ( int iCh = 0; iCh < NUM_FX_CHANNELS; iCh++ )
    {
        const int iP = iPar[iCh];

        // the signal is processed in floating point normalized to full scale
        fBoost[iCh]  = DbToLin ( Params.fBoostDb[iP] ) / 32768.0f;
        bCompOn[iCh] = ( Params.fCompRatio[iP] > 1.0f );
        fMakeup[iCh] = bCompOn[iCh] ? DbToLin ( CAudioCompressor::GetAutoMakeupDb ( Params.fCompThresholdDb[iP], Params.fCompRatio[iP] ) ) : 1.0f;

        // without boost and compression the channel is passed through unchanged
        bGainOn[iCh] = bCompOn[iCh] || ( Params.fBoostDb[iP] > 0.0f );

        if ( !bCompOn[iCh] )
        {
            Compressor[iCh].Reset();
        }
    }

    // skip the sample loop if no channel needs gain processing
    for ( int i = 0, j = 0; ( bGainOn[0] || bGainOn[1] ) && ( i < iMonoBlockSizeSam ); i++, j += 2 )
    {
        float fSig[NUM_FX_CHANNELS] = { fBoost[0] * vecsStereoInOut[j], fBoost[1] * vecsStereoInOut[j + 1] };

        // compressor with automatic makeup gain
        if ( bStereo )
        {
            if ( bCompOn[0] )
            {
                const float fGrDb =
                    Compressor[0].Process ( std::max ( fabsf ( fSig[0] ), fabsf ( fSig[1] ) ), Params.fCompThresholdDb[0], Params.fCompRatio[0] );
                const float fGain = DbToLin ( fGrDb ) * fMakeup[0];

                fSig[0] *= fGain;
                fSig[1] *= fGain;
            }
        }
        else
        {
            for ( int iCh = 0; iCh < NUM_FX_CHANNELS; iCh++ )
            {
                if ( bCompOn[iCh] )
                {
                    const int   iP    = iPar[iCh];
                    const float fGrDb = Compressor[iCh].Process ( fabsf ( fSig[iCh] ), Params.fCompThresholdDb[iP], Params.fCompRatio[iP] );

                    fSig[iCh] *= DbToLin ( fGrDb ) * fMakeup[iCh];
                }
            }
        }

        // channels without gain processing keep their original samples
        if ( bGainOn[0] )
        {
            vecsStereoInOut[j] = Float2Short ( fSig[0] * 32768.0f );
        }

        if ( bGainOn[1] )
        {
            vecsStereoInOut[j + 1] = Float2Short ( fSig[1] * 32768.0f );
        }
    }

    // reverberation
    if ( bStereo )
    {
        if ( Params.fReverbAttenuation[0] != 0.0f )
        {
            Reverb[0].Process ( vecsStereoInOut, true, Params.fReverbAttenuation[0] );
        }
    }
    else
    {
        for ( int iCh = 0; iCh < NUM_FX_CHANNELS; iCh++ )
        {
            const float fAttenuation = Params.fReverbAttenuation[iPar[iCh]];

            if ( fAttenuation != 0.0f )
            {
                Reverb[iCh].Process ( vecsStereoInOut, iCh == 0, fAttenuation );
            }
        }
    }
}
