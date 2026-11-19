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

    return fGainReductionDb;
}

/******************************************************************************\
* Safety limiter                                                               *
\******************************************************************************/
void CSafetyLimiter::Init ( const int iSampleRate )
{
    fReleaseCoef = TimeConstToCoef ( FX_LIMITER_RELEASE_S, iSampleRate );
    Reset();
}

float CSafetyLimiter::Process ( const float fPeak )
{
    const float fTargetGain = ( fPeak > FX_LIMITER_CEILING ) ? FX_LIMITER_CEILING / fPeak : 1.0f;

    if ( fTargetGain < fGain )
    {
        // instant attack so that the ceiling is never exceeded
        fGain = fTargetGain;
    }
    else
    {
        fGain = fReleaseCoef * fGain + ( 1.0f - fReleaseCoef ) * fTargetGain;
    }

    return fGain;
}

/******************************************************************************\
* Input effects chain                                                          *
\******************************************************************************/
CInputEffects::CInputEffects() : eAudioChannelConf ( CC_MONO ), iMonoBlockSizeSam ( 0 ), fGainReductionDecay ( 0 )
{
    for ( int iCh = 0; iCh < NUM_FX_CHANNELS; iCh++ )
    {
        fGainReductionHoldDb[iCh] = 0.0f;
        fGainReductionDispDb[iCh] = 0.0f;
    }
}

void CInputEffects::Init ( const EAudChanConf eNAudioChannelConf, const int iNMonoBlockSizeSam, const int iSampleRate )
{
    eAudioChannelConf = eNAudioChannelConf;
    iMonoBlockSizeSam = iNMonoBlockSizeSam;

    vecsInputMeterBuf.Init ( 2 * iMonoBlockSizeSam );

    // peak hold decay of the gain reduction display, applied once per block
    fGainReductionDecay = expf ( -static_cast<float> ( iMonoBlockSizeSam ) / ( FX_GR_DISPLAY_DECAY_S * iSampleRate ) );

    for ( int iCh = 0; iCh < NUM_FX_CHANNELS; iCh++ )
    {
        Compressor[iCh].Init ( iSampleRate );
        Limiter[iCh].Init ( iSampleRate );
        fGainReductionHoldDb[iCh] = 0.0f;
        fGainReductionDispDb[iCh] = 0.0f;
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

void CInputEffects::Process ( CVector<int16_t>& vecsStereoInOut, const SInputEffectsParams& Params, CStereoSignalLevelMeter* pInputMeter )
{
    // In stereo mode the two channels are one source, therefore the settings
    // are always linked and the compressor and limiter use a common detector so
    // that the stereo image does not shift.
    const bool bStereo               = ( eAudioChannelConf == CC_STEREO );
    const int  iParCh1               = ( bStereo || Params.bLinked ) ? 0 : 1;
    const int  iPar[NUM_FX_CHANNELS] = { 0, iParCh1 };

    float fBoost[NUM_FX_CHANNELS];
    float fMakeup[NUM_FX_CHANNELS];
    bool  bCompOn[NUM_FX_CHANNELS];
    float fMaxGrDb[NUM_FX_CHANNELS];

    for ( int iCh = 0; iCh < NUM_FX_CHANNELS; iCh++ )
    {
        const int iP = iPar[iCh];

        // the signal is processed in floating point normalized to full scale
        fBoost[iCh]   = DbToLin ( Params.fBoostDb[iP] ) / 32768.0f;
        bCompOn[iCh]  = ( Params.fCompRatio[iP] > 1.0f );
        fMakeup[iCh]  = bCompOn[iCh] ? DbToLin ( CAudioCompressor::GetAutoMakeupDb ( Params.fCompThresholdDb[iP], Params.fCompRatio[iP] ) ) : 1.0f;
        fMaxGrDb[iCh] = 0.0f;

        if ( !bCompOn[iCh] )
        {
            Compressor[iCh].Reset();
        }
    }

    for ( int i = 0, j = 0; i < iMonoBlockSizeSam; i++, j += 2 )
    {
        float fSig[NUM_FX_CHANNELS] = { fBoost[0] * vecsStereoInOut[j], fBoost[1] * vecsStereoInOut[j + 1] };

        // the input level meter shows the boosted signal before compression
        if ( pInputMeter != nullptr )
        {
            vecsInputMeterBuf[j]     = Float2Short ( fSig[0] * 32768.0f );
            vecsInputMeterBuf[j + 1] = Float2Short ( fSig[1] * 32768.0f );
        }

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
                fMaxGrDb[0] = std::min ( fMaxGrDb[0], fGrDb );
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
                    fMaxGrDb[iCh] = std::min ( fMaxGrDb[iCh], fGrDb );
                }
            }
        }

        // safety limiter, always active, catches peaks which pass the
        // compressor during its attack phase or due to a high boost
        if ( bStereo )
        {
            const float fGain = Limiter[0].Process ( std::max ( fabsf ( fSig[0] ), fabsf ( fSig[1] ) ) );

            fSig[0] *= fGain;
            fSig[1] *= fGain;
        }
        else
        {
            fSig[0] *= Limiter[0].Process ( fabsf ( fSig[0] ) );
            fSig[1] *= Limiter[1].Process ( fabsf ( fSig[1] ) );
        }

        vecsStereoInOut[j]     = Float2Short ( fSig[0] * 32768.0f );
        vecsStereoInOut[j + 1] = Float2Short ( fSig[1] * 32768.0f );
    }

    if ( pInputMeter != nullptr )
    {
        pInputMeter->Update ( vecsInputMeterBuf, iMonoBlockSizeSam, true );
    }

    // gain reduction display with peak hold (in stereo mode both channels
    // show the common gain reduction)
    if ( bStereo )
    {
        fMaxGrDb[1] = fMaxGrDb[0];
    }

    for ( int iCh = 0; iCh < NUM_FX_CHANNELS; iCh++ )
    {
        fGainReductionHoldDb[iCh] = std::max ( -fMaxGrDb[iCh], fGainReductionHoldDb[iCh] * fGainReductionDecay );
        fGainReductionDispDb[iCh] = bCompOn[iCh] ? fGainReductionHoldDb[iCh] : 0.0f;
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
