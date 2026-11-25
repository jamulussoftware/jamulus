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

#include "inputeffectchain.h"

CInputEffectChain::CInputEffectChain() : iNumLanes ( 2 ), iNumChannelsPerLane ( 1 ), iMonoBlockSizeSam ( 0 ), iNumEffectsPerLane ( 0 )
{
    for ( int iLane = 0; iLane < MAX_FX_LANES; iLane++ )
    {
        for ( int iEff = 0; iEff < MAX_FX_EFFECTS_LANE; iEff++ )
        {
            apEffects[iLane][iEff]    = nullptr;
            abWasEnabled[iLane][iEff] = false;
        }
    }
}

void CInputEffectChain::Init ( const EAudChanConf eNAudioChannelConf, const int iNMonoBlockSizeSam )
{
    // in stereo mode both channels form one source which is processed by one
    // lane, in the mono modes each input channel has its own lane
    iNumLanes           = ( eNAudioChannelConf == CC_STEREO ) ? 1 : 2;
    iNumChannelsPerLane = MAX_FX_CHANNELS + 1 - iNumLanes;
    iMonoBlockSizeSam   = iNMonoBlockSizeSam;

    for ( int c = 0; c < MAX_FX_CHANNELS; c++ )
    {
        vecfChan[c].Init ( iMonoBlockSizeSam, 0.0f );
    }

#ifndef NO_INTERNAL_EFFECTS
    iNumEffectsPerLane = 3;

    for ( int iLane = 0; iLane < MAX_FX_LANES; iLane++ )
    {
        apEffects[iLane][0] = &Boost_[iLane];
        apEffects[iLane][1] = &Compressor_[iLane];
        apEffects[iLane][2] = &Reverb_[iLane];
    }
#else
    iNumEffectsPerLane = 0;
#endif

    for ( int iLane = 0; iLane < iNumLanes; iLane++ )
    {
        for ( int iEff = 0; iEff < iNumEffectsPerLane; iEff++ )
        {
            apEffects[iLane][iEff]->Activate ( iNumChannelsPerLane, iMonoBlockSizeSam );
            apEffects[iLane][iEff]->Reset();
            abWasEnabled[iLane][iEff] = false;
        }
    }
}

bool CInputEffectChain::IsAnyEnabled() const
{
    for ( int iLane = 0; iLane < iNumLanes; iLane++ )
    {
        for ( int iEff = 0; iEff < iNumEffectsPerLane; iEff++ )
        {
            if ( apEffects[iLane][iEff]->IsEnabled() || abWasEnabled[iLane][iEff] )
            {
                return true;
            }
        }
    }

    return false;
}

void CInputEffectChain::Process ( CVector<int16_t>& vecsStereoInOut )
{
    // nothing to do if no effect is (or just was) enabled: keep the fast path
    if ( !IsAnyEnabled() )
    {
        return;
    }

    const float fToFloat = 1.0f / 32768.0f;

    // de-interleave to planar float, full scale 1.0
    for ( int i = 0, j = 0; i < iMonoBlockSizeSam; i++, j += 2 )
    {
        vecfChan[0][i] = fToFloat * vecsStereoInOut[j];
        vecfChan[1][i] = fToFloat * vecsStereoInOut[j + 1];
    }

    for ( int iLane = 0; iLane < iNumLanes; iLane++ )
    {
        // a stereo lane sees both channels, a mono lane sees its own one
        float* apfLane[MAX_FX_CHANNELS] = { &vecfChan[iLane][0], &vecfChan[1][0] };

        for ( int iEff = 0; iEff < iNumEffectsPerLane; iEff++ )
        {
            CInputEffect* pEffect  = apEffects[iLane][iEff];
            const bool    bEnabled = pEffect->IsEnabled();

            if ( bEnabled )
            {
                pEffect->Process ( apfLane, iMonoBlockSizeSam );
            }
            else if ( abWasEnabled[iLane][iEff] )
            {
                // just switched off: clear the state so that nothing stale is
                // heard when it is switched on again
                pEffect->Reset();
            }

            abWasEnabled[iLane][iEff] = bEnabled;
        }
    }

    // back to interleaved int16
    for ( int i = 0, j = 0; i < iMonoBlockSizeSam; i++, j += 2 )
    {
        vecsStereoInOut[j]     = Float2Short ( vecfChan[0][i] * 32768.0f );
        vecsStereoInOut[j + 1] = Float2Short ( vecfChan[1][i] * 32768.0f );
    }
}
