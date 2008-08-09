/******************************************************************************\
 * Copyright (c) 2004-2008
 *
 * Author(s):
 *  Volker Fischer, Erik de Castro Lopo
 *
 * This code is based on the Open-Source implementation of IMA-ADPCM / MS-ADPCM
 * written by Erik de Castro Lopo <erikd[at-#]mega-nerd[dot*]com> in 1999-2004
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

    case CT_MSADPCM:
        return MsAdpcm.Init ( iNewAudioLen );

    default:
        return 0;
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
            vecbyOut[2 * i]     = vecsAudio[i] & 0xFF;
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

        case CT_MSADPCM:
            return MsAdpcm.Encode ( vecsAudio ); // MS-ADPCM

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

        case CT_MSADPCM:
            return MsAdpcm.Decode ( vecbyAdpcm ); // MS-ADPCM

        default:
            return CVector<short> ( 0 );
        }
    }
}



/******************************************************************************\
* IMA-ADPCM implementation                                                     *
\******************************************************************************/
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
    vecbyAdpcm.Init     ( iAdpcmSize );
    vecbyAdpcmTemp.Init ( iAudSize );


    /* Encode the block header ---------------------------------------------- */
    vecbyAdpcm[0] = vecsAudio[0] & 0xFF;
    vecbyAdpcm[1] = ( vecsAudio[0] >> 8 ) & 0xFF;
    vecbyAdpcm[2] = iStepindEnc;

    int iPrevAudio = vecsAudio[0];


    /* Encode the samples as 4 bit ------------------------------------------ */
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
        iPrevAudio  = CheckBounds ( iPrevAudio, _MINSHORT, _MAXSHORT );
        iStepindEnc = CheckBounds ( iStepindEnc, 0, IMA_STEP_SIZE_TAB_LEN - 1 );

        // use a temporary buffer as an intermediate result buffer
        vecbyAdpcmTemp[i] = bytecode;
    }


    /* Pack the 4 bit encoded samples --------------------------------------- */
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


    /* Read the block header ------------------------------------------------ */
    int current = vecbyAdpcm[0] | ( vecbyAdpcm[1] << 8 );
    if ( current & 0x8000 )
	{
        current -= 0x10000;
	}

    // get and bound step index
    int iStepindDec = CheckBounds ( vecbyAdpcm[2], 0, IMA_STEP_SIZE_TAB_LEN - 1 );

    // set first sample which was delivered in the header
    vecsAudio[0] = current;


    /* -------------------------------------------------------------------------
       pull apart the packed 4 bit samples and store them in their correct
       sample positions */
    // the first encoded audio sample is in header
    vecsAudio[1] = vecbyAdpcm[3] & 0x0F;

    for ( i = 4; i < iAdpcmSize; i++ )
    {
        const short bytecode = vecbyAdpcm[i];
        vecsAudio[2 * i - 6] = bytecode & 0x0F;
        vecsAudio[2 * i - 5] = ( bytecode >> 4 ) & 0x0F;
    }


    /* Decode the encoded 4 bit samples ------------------------------------- */
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



/******************************************************************************\
* MS-ADPCM implementation                                                      *
\******************************************************************************/
/*
MS ADPCM block layout:
byte	purpose
0		block predictor [0..6]
1,2		initial idelta (positive)
3,4		sample 1
5,6		sample 0
7..n	packed bytecodes
*/
int CMsAdpcm::Init ( const int iNewAudioLen )
{
    // set lengths for audio and compressed data
    iAudSize = iNewAudioLen;
    iAdpcmSize = 7 /* bytes header */ + (int) ceil (
        (double) ( iAudSize - 2 /* first two samples are in header */ ) / 2 );

    return iAdpcmSize;
}

CVector<unsigned char> CMsAdpcm::Encode ( const CVector<short>& vecsAudio )
{
    CVector<short>          vecsAudioTemp;
    CVector<unsigned char>  vecbyAdpcm;

    // init size
    vecsAudioTemp.Init ( iAudSize );
    vecbyAdpcm.Init    ( iAdpcmSize );

    // copy input vector (because we want to overwrite it)
    vecsAudioTemp = vecsAudio;

    // choose predictor
    int	bpred;
    int idelta;
    ChoosePredictor ( vecsAudio, bpred, idelta );


    /* Encode the block header ---------------------------------------------- */
    vecbyAdpcm[0] = bpred;
	vecbyAdpcm[1] = idelta & 0xFF;
	vecbyAdpcm[2] = ( idelta >> 8 ) & 0xFF;
	vecbyAdpcm[3] = vecsAudio[1] & 0xFF;
	vecbyAdpcm[4] = ( vecsAudio[1] >> 8 ) & 0xFF;
	vecbyAdpcm[5] = vecsAudio[0] & 0xFF;
	vecbyAdpcm[6] = ( vecsAudio[0] >> 8 ) & 0xFF;


    /* Encode the samples as 4 bit ------------------------------------------ */
	unsigned int  blockindx = 7;
	unsigned char byte      = 0;

    for ( int k = 2; k < iAudSize; k++ )
	{
        const int predict = ( vecsAudioTemp[k - 1] * ms_AdaptCoeff1[bpred] +
                              vecsAudioTemp[k - 2] * ms_AdaptCoeff2[bpred] ) >> 8;

		int errordelta = ( vecsAudio[k] - predict ) / idelta;

		if ( errordelta < -8 )
        {
			errordelta = -8 ;
        }
		else
        {
            if (errordelta > 7)
            {
			    errordelta = 7;
            }
        }
		int newsamp = predict + ( idelta * errordelta );

		if ( newsamp > 32767 )
        {
			newsamp = 32767;
        }
		else
        {
            if ( newsamp < -32768 )
            {
			    newsamp = -32768;
            }
        }
		if ( errordelta < 0 )
        {
			errordelta += 0x10;
        }

		byte = ( byte << 4 ) | ( errordelta & 0xF );

		if ( k % 2 )
		{
            vecbyAdpcm[blockindx++] = byte;
			byte = 0;
		}

		idelta = ( idelta * ms_AdaptationTable[errordelta] ) >> 8;

		if ( idelta < 16 )
        {
			idelta = 16;
        }
		vecsAudioTemp[k] = newsamp;
	}

    return vecbyAdpcm;
}

CVector<short> CMsAdpcm::Decode ( const CVector<unsigned char>& vecbyAdpcm )
{
    CVector<short> vecsAudio;
    short          bytecode;

    vecsAudio.Init ( iAudSize );


    /* Read the block header ------------------------------------------------ */
    short bpred = vecbyAdpcm[0];

    if ( bpred >= 7 )
    {
        // no valid MS ADPCM stream, do not decode
        return vecsAudio;
    }

	short chan_idelta = vecbyAdpcm[1] | ( vecbyAdpcm[2] << 8 );

	vecsAudio[1] = vecbyAdpcm[3] | ( vecbyAdpcm[4] << 8 );
	vecsAudio[0] = vecbyAdpcm[5] | ( vecbyAdpcm[6] << 8 );


    /* -------------------------------------------------------------------------
       pull apart the packed 4 bit samples and store them in their correct
       sample positions */
    for ( int i = 7; i < iAdpcmSize; i++ )
    {
        bytecode = vecbyAdpcm[i];
        vecsAudio[2 * i - 12] = ( bytecode >> 4 ) & 0x0F;
        vecsAudio[2 * i - 11] = bytecode & 0x0F;
    }


    /* Decode the encoded 4 bit samples ------------------------------------- */
	for ( int k = 2; k < iAudSize; k ++ )
	{
		bytecode = vecsAudio[k] & 0xF;

		// compute next Adaptive Scale Factor (ASF)
		int idelta = chan_idelta;

         // => / 256 => FIXED_POINT_ADAPTATION_BASE == 256
		chan_idelta = ( ms_AdaptationTable[bytecode] * idelta ) >> 8;

		if ( chan_idelta < 16 )
        {
			chan_idelta = 16;
        }

		if ( bytecode & 0x8 )
        {
			bytecode -= 0x10;
        }

        // => / 256 => FIXED_POINT_COEFF_BASE == 256
    	const int predict = ( ( vecsAudio[k - 1] * ms_AdaptCoeff1[bpred] ) +
                              ( vecsAudio[k - 2] * ms_AdaptCoeff2[bpred] ) ) >> 8;

		int current = ( bytecode * idelta ) + predict;

		if ( current > 32767 )
        {
			current = 32767 ;
        }
		else
        {
            if ( current < -32768 )
            {
			    current = -32768;
            }
        }

		vecsAudio[k] = current;
	}

	return vecsAudio;
}

void CMsAdpcm::ChoosePredictor ( const CVector<short>& vecsAudio,
                                 int& block_pred,
                                 int& idelta )
{
/*
    Choosing the block predictor:
    Each block requires a predictor and an idelta for each channel. The
    predictor is in the range [0..6] which is an index into the two AdaptCoeff
    tables. The predictor is chosen by trying all of the possible predictors on
    a small set of samples at the beginning of the block. The predictor with the
    smallest average abs (idelta) is chosen as the best predictor for this
    block. The value of idelta is chosen to to give a 4 bit code value of +/- 4
    (approx. half the max. code value). If the average abs (idelta) is zero, the
    sixth predictor is chosen. If the value of idelta is less then 16 it is set
    to 16.
*/
    unsigned int best_bpred  = 0;
    unsigned int best_idelta = 0;

    /* Microsoft uses an IDELTA_COUNT (number of sample pairs used to choose
       best predictor) value of 3. The best possible results would be obtained
       by using all the samples to choose the predictor. */
    unsigned int idelta_count = min ( MSADPCM_IDELTA_COUNT, vecsAudio.Size() - 1 );

	for ( unsigned int bpred = 0; bpred < MSADPCM_ADAPT_COEFF_COUNT; bpred++ )
	{
        unsigned int idelta_sum = 0 ;

		for ( unsigned int k = 2 ; k < 2 + idelta_count ; k++ )
        {
			idelta_sum += abs ( vecsAudio[k] - 
                ( ( vecsAudio[k - 1] * ms_AdaptCoeff1[bpred] +
                    vecsAudio[k - 2] * ms_AdaptCoeff2[bpred] ) >> 8 ) );
        }
		idelta_sum /= ( 4 * idelta_count );

		if ( bpred == 0 || idelta_sum < best_idelta )
		{
            best_bpred  = bpred;
			best_idelta = idelta_sum;
		}

		if ( !idelta_sum )
		{
            best_bpred  = bpred;
			best_idelta = 16;
			break;
		}
	}

	if ( best_idelta < 16 )
    {
		best_idelta = 16;
    }

	block_pred = best_bpred;
	idelta     = best_idelta;

	return;
}
