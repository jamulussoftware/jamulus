/******************************************************************************\
 * Copyright (c) 2004-2009
 *
 * Author(s):
 *  Volker Fischer
 *
 * The polyphase filter is calculated with Matlab, the associated file
 * is ResampleFilter.m.
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
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
\******************************************************************************/

#include "resample.h"


/******************************************************************************\
* Stereo Audio Resampler                                                       *
\******************************************************************************/
void CStereoAudioResample::ResampleStereo ( CVector<double>& vecdInput,
                                            CVector<double>& vecdOutput )
{
    int j;

    if ( dRation == 1.0 )
    {
        // if ratio is 1, no resampling is needed, just copy vector
        vecdOutput = vecdInput;
    }
    else
    {
        const int iTwoTimesNumTaps = 2 * iNumTaps;

        /* move old data from the end to the history part of the buffer and
           add new data (shift register) */
        // shift old values
        int iMovLen = iStereoInputBlockSize;
        for ( j = 0; j < iTwoTimesNumTaps; j++ )
        {
            vecdIntBuffStereo[j] = vecdIntBuffStereo[iMovLen++];
        }

        // add new block of data
        int iBlockEnd = iTwoTimesNumTaps;
        for ( j = 0; j < iStereoInputBlockSize; j++ )
        {
            vecdIntBuffStereo[iBlockEnd++] = vecdInput[j];
        }

        // main loop
        for ( j = 0; j < iMonoOutputBlockSize; j++ )
        {
            // calculate filter phase
            const int ip = (int) ( j * iI / dRation ) % iI;

            // sample position in stereo input vector
            const int in = 2 * ( (int) ( j / dRation ) + iNumTaps - 1 );

            // convolution
            double dyL = 0.0;
            double dyR = 0.0;
            for ( int i = 0; i < iNumTaps; i++ )
            {
                const double dCurFiltTap   = pFiltTaps[ip + i * iI];
                const int    iCurSamplePos = in - 2 * i;

                dyL += dCurFiltTap * vecdIntBuffStereo[iCurSamplePos];
                dyR += dCurFiltTap * vecdIntBuffStereo[iCurSamplePos + 1];
            }

            vecdOutput[2 * j]     = dyL;
            vecdOutput[2 * j + 1] = dyR;
        }
    }
}

void CStereoAudioResample::ResampleMono ( CVector<double>& vecdInput,
                                          CVector<double>& vecdOutput )
{
    int j;

    if ( dRation == 1.0 )
    {
        // if ratio is 1, no resampling is needed, just copy vector
        vecdOutput = vecdInput;
    }
    else
    {
        /* move old data from the end to the history part of the buffer and
           add new data (shift register) */
        // shift old values
        int iMovLen = iMonoInputBlockSize;
        for ( j = 0; j < iNumTaps; j++ )
        {
            vecdIntBuffMono[j] = vecdIntBuffMono[iMovLen++];
        }

        // add new block of data
        int iBlockEnd = iNumTaps;
        for ( j = 0; j < iMonoInputBlockSize; j++ )
        {
            vecdIntBuffMono[iBlockEnd++] = vecdInput[j];
        }

        // main loop
        for ( j = 0; j < iMonoOutputBlockSize; j++ )
        {
            // calculate filter phase
            const int ip = (int) ( j * iI / dRation ) % iI;

            // sample position in input vector
            const int in = (int) ( j / dRation ) + iNumTaps - 1;

            // convolution
            double dy = 0.0;
            for ( int i = 0; i < iNumTaps; i++ )
            {
                dy += pFiltTaps[ip + i * iI] * vecdIntBuffMono[in - i];
            }

            vecdOutput[j] = dy;
        }
    }
}

void CStereoAudioResample::Init ( const int iNewMonoInputBlockSize,
                                  const int iFrom,
                                  const int iTo )
{
    dRation               = ( (double) iTo ) / iFrom;
    iMonoInputBlockSize   = iNewMonoInputBlockSize;
    iStereoInputBlockSize = 2 * iNewMonoInputBlockSize;
    iMonoOutputBlockSize  = (int) ( iNewMonoInputBlockSize * dRation );

    // set correct parameters
    if ( iFrom >= iTo ) // downsampling case
    {
        switch ( iTo )
        {
        case ( SND_CRD_SAMPLE_RATE / 2 ): // 48 kHz to 24 kHz
            pFiltTaps    = fResTaps2;
            iNumTaps     = INTERP_I_2 * NUM_TAPS_PER_PHASE2;
            iI           = DECIM_D_2;
            break;

        case ( SND_CRD_SAMPLE_RATE * 7 / 12 ): // 48 kHz to 28 kHz
            pFiltTaps = fResTaps12_7;
            iNumTaps  = INTERP_I_12_7 * NUM_TAPS_PER_PHASE12_7;
            iI        = DECIM_D_12_7;
            break;

        case ( SND_CRD_SAMPLE_RATE * 2 / 3 ): // 48 kHz to 32 kHz
            pFiltTaps = fResTaps3_2;
            iNumTaps  = INTERP_I_3_2 * NUM_TAPS_PER_PHASE3_2;
            iI        = DECIM_D_3_2;
            break;

        case SND_CRD_SAMPLE_RATE: // 48 kHz to 48 kHz
            // no resampling needed
            pFiltTaps = NULL;
            iNumTaps  = 0;
            iI        = 1;
            break;

        default:
            // resample ratio not defined, throw error
            throw 0;
            break;
        }
    }
    else // upsampling case (assumption: iTo == SND_CRD_SAMPLE_RATE)
    {
        switch ( iFrom )
        {
        case ( SND_CRD_SAMPLE_RATE / 2 ): // 24 kHz to 48 kHz
            pFiltTaps = fResTaps2;
            iNumTaps  = DECIM_D_2 * NUM_TAPS_PER_PHASE2;
            iI        = INTERP_I_2;
            break;

        case ( SND_CRD_SAMPLE_RATE * 7 / 12 ): // 28 kHz to 48 kHz
            pFiltTaps = fResTaps12_7;
            iNumTaps  = DECIM_D_12_7 * NUM_TAPS_PER_PHASE12_7;
            iI        = INTERP_I_12_7;
            break;

        case ( SND_CRD_SAMPLE_RATE * 2 / 3 ): // 32 kHz to 48 kHz
            pFiltTaps = fResTaps3_2;
            iNumTaps  = DECIM_D_3_2 * NUM_TAPS_PER_PHASE3_2;
            iI        = INTERP_I_3_2;
            break;

        default:
            // resample ratio not defined, throw error
            throw 0;
            break;
        }
    }

    // allocate memory for internal buffer, clear sample history (for
    // the stereo case we have to consider that two times the number of taps of
    // additional memory is required)
    vecdIntBuffMono.Init   ( iMonoInputBlockSize + iNumTaps, 0.0 );
    vecdIntBuffStereo.Init ( iStereoInputBlockSize + 2 * iNumTaps, 0.0 );
}
