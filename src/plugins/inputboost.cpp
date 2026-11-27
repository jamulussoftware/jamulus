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

#include <cmath>
#include "inputboost.h"

void CInputBoost::SetGainDb ( const float fNewGainDb )
{
    float fClipped = fNewGainDb;

    if ( fClipped < 0.0f )
    {
        fClipped = 0.0f;
    }

    if ( fClipped > FX_BOOST_DB_MAX )
    {
        fClipped = FX_BOOST_DB_MAX;
    }

    fGainDb = fClipped;
    fGain   = powf ( 10.0f, fClipped / 20.0f );
}

void CInputBoost::Process ( float* const* ppfChannels, const int iNumFrames )
{
    const float fCurGain = fGain;

    if ( fCurGain == 1.0f )
    {
        return;
    }

    for ( int c = 0; c < iNumChannels; c++ )
    {
        float* pfChan = ppfChannels[c];

        for ( int i = 0; i < iNumFrames; i++ )
        {
            // clamp to full scale (the same saturation as Float2Short does)
            float fSample = fCurGain * pfChan[i];

            if ( fSample > 1.0f )
            {
                fSample = 1.0f;
            }
            else if ( fSample < -1.0f )
            {
                fSample = -1.0f;
            }

            pfChan[i] = fSample;
        }
    }
}
