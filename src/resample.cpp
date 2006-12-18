/******************************************************************************\
 * Copyright (c) 2004-2006
 *
 * Author(s):
 *  Volker Fischer
 *
 * Description:
 * Resample routine for arbitrary sample-rate conversions in a low range (for
 * frequency offset correction).
 * The algorithm is based on a polyphase structure. We upsample the input
 * signal with a factor INTERP_DECIM_I_D1 and calculate two successive samples
 * whereby we perform a linear interpolation between these two samples to get
 * an arbitraty sample grid.
 *
 * The polyphase filter is calculated with Matlab(TM), the associated file
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


/* Implementation *************************************************************/
int CResample::Resample ( CVector<double>& vecdInput,
                          CVector<double>& vecdOutput,
                          const double dRation )
{
    int i;

    /* move old data from the end to the history part of the buffer and
       add new data (shift register) */
    /* Shift old values */
    int iMovLen = iInputBlockSize;
    for ( i = 0; i < iHistorySize; i++ )
    {
        vecdIntBuff[i] = vecdIntBuff[iMovLen++];
    }

    /* Add new block of data */
    int iBlockEnd = iHistorySize;
    for ( i = 0; i < iInputBlockSize; i++ )
    {
        vecdIntBuff[iBlockEnd++] = vecdInput[i];
    }

    /* sample-interval of new sample frequency in relation to interpolated
       sample-interval */
    dTStep = (double) INTERP_DECIM_I_D1 / dRation;

    /* init output counter */
    int im = 0;

    /* main loop */
    do
    {
        /* quantize output-time to interpolated time-index */
        const int ik = (int) dtOut;


        /* calculate convolutions for the two interpolation-taps ------------ */
        /* phase for the linear interpolation-taps */
        const int ip1 = ik % INTERP_DECIM_I_D1;
        const int ip2 = ( ik + 1 ) % INTERP_DECIM_I_D1;

        /* sample positions in input vector */
        const int in1 = (int) ( ik / INTERP_DECIM_I_D1 );
        const int in2 = (int) ( ( ik + 1 ) / INTERP_DECIM_I_D1 );

        /* convolution */
        double dy1 = 0.0;
        double dy2 = 0.0;
        for (int i = 0; i < NUM_TAPS_PER_PHASE1; i++)
        {
            dy1 += fResTaps1[ip1 * INTERP_DECIM_I_D1 + i] * vecdIntBuff[in1 - i];
            dy2 += fResTaps1[ip2 * INTERP_DECIM_I_D1 + i] * vecdIntBuff[in2 - i];
        }


        /* linear interpolation --------------------------------------------- */
        /* get numbers after the comma */
        const double dxInt = dtOut - (int) dtOut;
        vecdOutput[im] = ( dy2 - dy1 ) * dxInt + dy1;


        /* increase output counter */
        im++;
            
        /* increase output-time and index one step */
        dtOut = dtOut + dTStep;
    } 
    while ( dtOut < dBlockDuration );

    /* set rtOut back */
    dtOut -= iInputBlockSize * INTERP_DECIM_I_D1;

    return im;
}

void CResample::Init ( const int iNewInputBlockSize )
{
    iInputBlockSize = iNewInputBlockSize;

    /* history size must be one sample larger, because we use always TWO
       convolutions */
    iHistorySize = NUM_TAPS_PER_PHASE1 + 1;

    /* calculate block duration */
    dBlockDuration = ( iInputBlockSize + iHistorySize - 1 ) * INTERP_DECIM_I_D1;

    /* allocate memory for internal buffer, clear sample history */
    vecdIntBuff.Init ( iInputBlockSize + iHistorySize, 0.0 );

    /* init absolute time for output stream (at the end of the history part */
    dtOut = (double) ( iHistorySize - 1 ) * INTERP_DECIM_I_D1;
}

void CAudioResample::Resample ( CVector<double>& vecdInput,
                                CVector<double>& vecdOutput )
{
    int j;

    if ( dRation == 1.0 )
    {
        /* if ratio is 1, no resampling is needed, just copy vector */
        for ( j = 0; j < iOutputBlockSize; j++ )
        {
            vecdOutput[j] = vecdInput[j];
        }
    }
    else
    {
        /* move old data from the end to the history part of the buffer and
           add new data (shift register) */
        /* Shift old values */
        int iMovLen = iInputBlockSize;
        for ( j = 0; j < iNumTaps; j++ )
        {
            vecdIntBuff[j] = vecdIntBuff[iMovLen++];
        }

        /* Add new block of data */
        int iBlockEnd = iNumTaps;
        for ( j = 0; j < iInputBlockSize; j++ )
        {
            vecdIntBuff[iBlockEnd++] = vecdInput[j];
        }

        /* main loop */
        for ( j = 0; j < iOutputBlockSize; j++ )
        {
            /* calculate filter phase */
            const int ip = (int) ( j * iI / dRation ) % iI;

            /* sample position in input vector */
            const int in = (int) ( j / dRation ) + iNumTaps;

            /* convolution */
            double dy = 0.0;
            for ( int i = 0; i < iNumTaps; i++ )
            {
                dy += pFiltTaps[ip + i * iI] * vecdIntBuff[in - i];
            }

            vecdOutput[j] = dy;
        }
    }
}

void CAudioResample::Init ( const int iNewInputBlockSize,
                            const int iFrom, const int iTo )
{
    dRation          = ( (double) iTo ) / iFrom;
    iInputBlockSize  = iNewInputBlockSize;
    iOutputBlockSize = (int) ( iInputBlockSize * dRation );

    // set correct parameters
    if ( iFrom == SND_CRD_SAMPLE_RATE ) // downsampling case
    {
        switch ( iFrom / iTo )
        {
        case 2: // 48 kHz to 24 kHz
            pFiltTaps        = fResTaps2;
            iNumTaps         = INTERP_I_2 * NUM_TAPS_PER_PHASE2;
            iI               = DECIM_D_2;
            break;

/* not yet supported
case ( 2 / 3 ): // 48 kHz to 32 kHz
    pFiltTaps        = fResTaps3_2;
    iNumTaps         = INTERP_I_3_2 * NUM_TAPS_PER_PHASE3_2;
    iI               = DECIM_D_3_2;
    break;
*/

        case 1: // 48 kHz to 48 kHz
            // no resampling needed
            break;

        default:
            // resample ratio not defined, throw error
            throw 0;
            break;
        }
    }
    else // upsampling case
    {
        switch ( iTo / iFrom )
        {
        case 2: // 24 kHz to 48 kHz
            pFiltTaps        = fResTaps2;
            iNumTaps         = DECIM_D_2 * NUM_TAPS_PER_PHASE2;
            iI               = INTERP_I_2;
            break;

/* not yet supported
case 1.5: // 32 kHz to 48 kHz
    pFiltTaps        = fResTaps3_2;
    iNumTaps         = DECIM_D_3_2 * NUM_TAPS_PER_PHASE3_2;
    iI               = INTERP_I_3_2;
    break;
*/

        default:
            // resample ratio not defined, throw error
            throw 0;
            break;
        }
    }

    // allocate memory for internal buffer, clear sample history
    vecdIntBuff.Init ( iInputBlockSize + iNumTaps, 0.0 );
}
