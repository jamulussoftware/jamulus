/******************************************************************************\
 * Copyright (c) 2004-2008
 *
 * Author(s):
 *  Volker Fischer, Erik de Castro Lopo
 *
 * This code is based on the Open-Source implementation of IMA-ADPCM written
 * by Erik de Castro Lopo <erikd[at-#]mega-nerd[dot*]com> in 1999-2004
 *
 * Changes:
 * - only support for one channel
 * - put 2 audio samples in header to get even number of audio samples encoded
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

#include "audiocompr.h"


/* Implementation *************************************************************/
int CAudioCompression::Init ( const int iNewAudioLen,
                              const EAudComprType eNewAuCoTy )
{
    eAudComprType = eNewAuCoTy;

    switch ( eNewAuCoTy )
    {
    case CT_NONE:
        return iCodeSize = 2 * iNewAudioLen; // short = 2 * byte

    case CT_IMAADPCM:
        return ImaAdpcm.Init ( iNewAudioLen );

    default: return 0;
    }
}

CVector<unsigned char> CAudioCompression::Encode ( const CVector<short>& vecsAudio )
{
    if ( eAudComprType == CT_NONE )
    {
        // no compression, simply ship pure samples
        CVector<unsigned char> vecbyOut ( iCodeSize );
        const int iAudSize = iCodeSize / 2;

        for ( int i = 0; i < iAudSize; i++ )
        {
            vecbyOut[2 * i] = vecsAudio[i] & 0xFF;
            vecbyOut[2 * i + 1] = ( vecsAudio[i] >> 8 ) & 0xFF;
        }
        return vecbyOut;
    }
    else
    {
        switch ( eAudComprType )
        {
        case CT_IMAADPCM:
            return ImaAdpcm.Encode ( vecsAudio ); // IMA-ADPCM

        default:
            return CVector<unsigned char> ( 0 );
        }
    }
}

CVector<short> CAudioCompression::Decode ( const CVector<unsigned char>& vecbyAdpcm )
{
    if ( eAudComprType == CT_NONE )
    {
        // no compression, reassemble pure samples
        const int iAudSize = iCodeSize / 2;
        CVector<short> vecsOut ( iAudSize );

        for ( int i = 0; i < iAudSize; i++ )
        {
            int current = vecbyAdpcm[2 * i] | ( vecbyAdpcm[2 * i + 1] << 8 );
            if ( current & 0x8000 )
			{
                current -= 0x10000;
			}

            vecsOut[i] = (short) current;
        }
        return vecsOut;
    }
    else
    {
        switch ( eAudComprType )
        {
        case CT_IMAADPCM:
            return ImaAdpcm.Decode ( vecbyAdpcm ); // IMA-ADPCM

        default:
            return CVector<short> ( 0 );
        }
    }
}


/* IMA-ADPCM implementation ------------------------------------------------- */
int CImaAdpcm::Init ( const int iNewAudioLen )
{
    // set lengths for audio and compressed data
    iAudSize = iNewAudioLen;
    iAdpcmSize = 4 /* bytes header */ + (int) ceil (
        (double) ( iAudSize - 2 /* first two samples are in header */ ) / 2 );

    iStepindEnc = 0;

    return iAdpcmSize;
}

CVector<unsigned char> CImaAdpcm::Encode ( const CVector<short>& vecsAudio )
{
    int                     i;
    CVector<unsigned char>  vecbyAdpcm;
    CVector<unsigned char>  vecbyAdpcmTemp;

    // init size
    vecbyAdpcm.Init ( iAdpcmSize );
    vecbyAdpcmTemp.Init ( iAudSize );

    /* encode the block header ----------------------------------------------- */
    vecbyAdpcm[0] = vecsAudio[0] & 0xFF;
    vecbyAdpcm[1] = ( vecsAudio[0] >> 8 ) & 0xFF;
    vecbyAdpcm[2] = iStepindEnc;

    int iPrevAudio = vecsAudio[0];


    /* encode the samples as 4 bit ------------------------------------------- */
    for ( i = 1; i < iAudSize; i++ )
    {
        // init diff and step
        int diff = vecsAudio[i] - iPrevAudio;

        Q_ASSERT ( iStepindEnc < IMA_STEP_SIZE_TAB_LEN );
        int step = ima_step_size[iStepindEnc];

        short bytecode = 0;

        int vpdiff = step >> 3;
        if ( diff < 0 )
        {
            bytecode = 8;
            diff = -diff;
        }
        short mask = 4;
        while ( mask )
        {
            if ( diff >= step )
            {
                bytecode |= mask;
                diff -= step;
                vpdiff += step;
            }
            step >>= 1;
            mask >>= 1;
        }

        if ( bytecode & 8 )
		{
            iPrevAudio -= vpdiff;
		}
        else
		{
            iPrevAudio += vpdiff;
		}

        // adjust step size
        Q_ASSERT ( bytecode < IMA_INDX_ADJUST_TAB_LEN );
        iStepindEnc += ima_indx_adjust[bytecode];

        // check that values do not exceed the bounds
        iPrevAudio = CheckBounds ( iPrevAudio, _MINSHORT, _MAXSHORT );
        iStepindEnc = CheckBounds ( iStepindEnc, 0, IMA_STEP_SIZE_TAB_LEN - 1 );

        // use the input buffer as an intermediate result buffer
        vecbyAdpcmTemp[i] = bytecode;
    }


    /* pack the 4 bit encoded samples ---------------------------------------- */
    // The first encoded audio sample is in header
    vecbyAdpcm[3] = vecbyAdpcmTemp[1] & 0x0F;

    for ( i = 4; i < iAdpcmSize; i++ )
    {
        vecbyAdpcm[i] = vecbyAdpcmTemp[2 * i - 6] & 0x0F;
        vecbyAdpcm[i] |= ( vecbyAdpcmTemp[2 * i - 5] << 4 ) & 0xF0;
    }

    return vecbyAdpcm;
}

CVector<short> CImaAdpcm::Decode ( const CVector<unsigned char>& vecbyAdpcm )
{
    int             i;
    CVector<short>  vecsAudio;

    vecsAudio.Init ( iAudSize );


    /* read and check the block header --------------------------------------- */
    int current = vecbyAdpcm[0] | ( vecbyAdpcm[1] << 8 );
    if ( current & 0x8000 )
	{
        current -= 0x10000;
	}

    // get and bound step index
    int iStepindDec = CheckBounds ( vecbyAdpcm[2], 0, IMA_STEP_SIZE_TAB_LEN - 1 );

    // set first sample which was delivered in the header
    vecsAudio[0] = current;


    /* --------------------------------------------------------------------------
       pull apart the packed 4 bit samples and store them in their correct sample
       positions */
    // The first encoded audio sample is in header
    vecsAudio[1] = vecbyAdpcm[3] & 0x0F;

    for ( i = 4; i < iAdpcmSize; i++ )
    {
        const short bytecode = vecbyAdpcm[i];
        vecsAudio[2 * i - 6] = bytecode & 0x0F;
        vecsAudio[2 * i - 5] = ( bytecode >> 4 ) & 0x0F;
    }


    /* decode the encoded 4 bit samples -------------------------------------- */
    for ( i = 1; i < iAudSize; i++ )
    {
        const short bytecode = vecsAudio[i] & 0xF ;

        Q_ASSERT ( iStepindDec < IMA_STEP_SIZE_TAB_LEN );

        short step = ima_step_size[iStepindDec];
        int current = vecsAudio[i - 1];

        int diff = step >> 3;
        if ( bytecode & 1 )
		{
            diff += step >> 2;
		}
        if ( bytecode & 2 )
		{
            diff += step >> 1;
		}
        if ( bytecode & 4 )
		{
            diff += step;
		}
        if ( bytecode & 8 )
		{
            diff = -diff;
		}

        current += diff;

        Q_ASSERT ( bytecode < IMA_INDX_ADJUST_TAB_LEN );
        iStepindDec += ima_indx_adjust[bytecode];

        // check that values do not exceed the bounds
        current = CheckBounds ( current, _MINSHORT, _MAXSHORT );
        iStepindDec = CheckBounds ( iStepindDec, 0, IMA_STEP_SIZE_TAB_LEN - 1 );

        vecsAudio[i] = current;
    }

    return vecsAudio;
}
