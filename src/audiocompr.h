/******************************************************************************\
 * Copyright (c) 2004-2008
 *
 * Author(s):
 *  Volker Fischer, Erik de Castro Lopo
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

#if !defined ( AUDIOCOMPR_H_OIHGE76GEKJH3249_GEG98EG3_43441912__INCLUDED_ )
#define AUDIOCOMPR_H_OIHGE76GEKJH3249_GEG98EG3_43441912__INCLUDED_

#include "util.h"
#include "global.h"
#include "buffer.h"


/* Definitions ****************************************************************/
// tables IMA-ADPCM
#define IMA_INDX_ADJUST_TAB_LEN         16
static int ima_indx_adjust[IMA_INDX_ADJUST_TAB_LEN] =
{
    -1, -1, -1, -1, /* +0 - +3, decrease the step size */
     2,  4,  6,  8, /* +4 - +7, increase the step size */
    -1, -1, -1, -1, /* -0 - -3, decrease the step size */
     2,  4,  6,  8, /* -4 - -7, increase the step size */
};

#define IMA_STEP_SIZE_TAB_LEN           89
static int ima_step_size[IMA_STEP_SIZE_TAB_LEN] =
{
    7, 8, 9, 10, 11, 12, 13, 14, 16, 17, 19, 21, 23, 25, 28, 31, 34, 37, 41, 45,
    50, 55, 60, 66, 73, 80, 88, 97, 107, 118, 130, 143, 157, 173, 190, 209, 230,
    253, 279, 307, 337, 371, 408, 449, 494, 544, 598, 658, 724, 796, 876, 963,
    1060, 1166, 1282, 1411, 1552, 1707, 1878, 2066, 2272, 2499, 2749, 3024, 3327,
    3660, 4026, 4428, 4871, 5358, 5894, 6484, 7132, 7845, 8630, 9493, 10442,
    11487, 12635, 13899, 15289, 16818, 18500, 20350, 22385, 24623, 27086, 29794,
    32767
};

// tables MS-ADPCM
#define MSADPCM_IDELTA_COUNT	        20

#define MSADPCM_ADAPT_COEFF_COUNT       7
static int ms_AdaptCoeff1[MSADPCM_ADAPT_COEFF_COUNT] =
{
    256, 512, 0, 192, 240, 460, 392
};

static int ms_AdaptCoeff2[MSADPCM_ADAPT_COEFF_COUNT] =
{
    0, -256, 0, 64, 0, -208, -232
};

static int ms_AdaptationTable[] =
{
    230, 230, 230, 230, 307, 409, 512, 614,
	768, 614, 512, 409, 307, 230, 230, 230
};


/* Classes ********************************************************************/
/* IMA-ADPCM ---------------------------------------------------------------- */
class CImaAdpcm
{
public:
    CImaAdpcm() : iStepindEnc ( 0 ) {}
    virtual ~CImaAdpcm() {}

    int                     Init ( const int iNewAudioLen );
    CVector<unsigned char>  Encode ( const CVector<short>& vecsAudio );
    CVector<short>          Decode ( const CVector<unsigned char>& vecbyAdpcm );

protected:
    int iAudSize;
    int iAdpcmSize;
    int iStepindEnc;

    // inline functions must be declared in the header
    inline int CheckBounds ( const int iData, const int iMin, const int iMax )
    {
        if ( iData > iMax )
        {
            return iMax;
        }

        if ( iData < iMin )
        {
            return iMin;
        }

        return iData;
    }
};


/* MS-ADPCM ----------------------------------------------------------------- */
class CMsAdpcm
{
public:
    CMsAdpcm() {}
    virtual ~CMsAdpcm() {}

    int                     Init ( const int iNewAudioLen );
    CVector<unsigned char>  Encode ( const CVector<short>& vecsAudio );
    CVector<short>          Decode ( const CVector<unsigned char>& vecbyAdpcm );

protected:
    int iAudSize;
    int iAdpcmSize;

    void ChoosePredictor ( const CVector<short>& vecsAudio,
                           int& block_pred,
                           int& idelta );
};


/* Audio compression class -------------------------------------------------- */
class CAudioCompression
{
public:
    enum EAudComprType
    {
        CT_NONE     = 0,
        CT_IMAADPCM = 1,
        CT_MSADPCM  = 2
    };

    CAudioCompression() {}
    virtual ~CAudioCompression() {}

    int                     Init ( const int iNewAudioLen,
                                   const EAudComprType eNewAuCoTy );
    CVector<unsigned char>  Encode ( const CVector<short>& vecsAudio );
    CVector<short>          Decode ( const CVector<unsigned char>& vecbyAdpcm );

    EAudComprType           GetType() { return eAudComprType; }

protected:
    EAudComprType   eAudComprType;
    CImaAdpcm       ImaAdpcm;
    CMsAdpcm        MsAdpcm;
    int             iCodeSize;
};

#endif /* !defined ( AUDIOCOMPR_H_OIHGE76GEKJH3249_GEG98EG3_43441912__INCLUDED_ ) */
