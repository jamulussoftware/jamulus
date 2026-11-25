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

#pragma once

#include "inputeffect.h"

/* Definitions ****************************************************************/
#define FX_BOOST_DB_MAX     24
#define FX_BOOST_DB_DEFAULT 6

/* Classes ********************************************************************/
// Input boost: a plain gain in dB. The output is clamped to full scale.
class CInputBoost : public CInputEffect
{
public:
    CInputBoost() : iNumChannels ( 1 ), fGainDb ( FX_BOOST_DB_DEFAULT ), fGain ( 1.0f ) { SetGainDb ( FX_BOOST_DB_DEFAULT ); }

    void  SetGainDb ( const float fNewGainDb );
    float GetGainDb() const { return fGainDb; }

    virtual void Activate ( const int iNNumChannels, const int ) override { iNumChannels = iNNumChannels; }
    virtual void Reset() override {}
    virtual void Process ( float* const* ppfChannels, const int iNumFrames ) override;

protected:
    int                iNumChannels;
    std::atomic<float> fGainDb;
    std::atomic<float> fGain;
};
