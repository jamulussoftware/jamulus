/******************************************************************************\
 * Copyright (c) 2004-2006
 *
 * Author(s):
 *  Volker Fischer
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

#if !defined ( BUFFER_H__3B123453_4344_BB23945IUHF1912__INCLUDED_ )
#define BUFFER_H__3B123453_4344_BB23945IUHF1912__INCLUDED_

#include "util.h"
#include "global.h"


/* Definitions ****************************************************************/
// time for fading effect for masking drop outs
#define FADE_IN_OUT_TIME            ( (double) 0.3 ) // ms
#define FADE_IN_OUT_NUM_SAM         ( (int) ( SYSTEM_SAMPLE_RATE * FADE_IN_OUT_TIME ) / 1000 )

// for extrapolation a shorter time for fading
#define FADE_IN_OUT_NUM_SAM_EXTRA   5 // samples


/* Classes ********************************************************************/
class CNetBuf
{
public:
    CNetBuf() {}
    virtual ~CNetBuf() {}

    void Init ( const int iNewBlockSize, const int iNewNumBlocks );
    int GetSize() { return iMemSize / iBlockSize; }

    bool Put ( CVector<double>& vecdData );
    bool Get ( CVector<double>& vecdData );

protected:
    enum EBufState { BS_OK, BS_FULL, BS_EMPTY };
    enum EClearType { CT_PUT, CT_GET };
    void Clear ( const EClearType eClearType );
    int GetAvailSpace() const;
    int GetAvailData() const;
    void FadeInAudioDataBlock ( CVector<double>& vecdData );
    void FadeOutExtrapolateAudioDataBlock ( CVector<double>& vecdData,
        const double dExPDiff, const double dExPLastV );

    CVector<double> vecdMemory;
    int             iMemSize;
    int             iBlockSize;
    int             iGetPos, iPutPos;
    EBufState       eBufState;
    bool            bFadeInNewPutData;
    int             iNumSamFading;
    int             iNumSamFadingExtra;

    // extrapolation parameters
    double          dExPDiff;
    double          dExPLastV;
};


// conversion buffer (very simple buffer)
class CConvBuf
{
public:
    CConvBuf() {}
    virtual ~CConvBuf() {}

    void Init ( const int iNewMemSize );
    int GetSize() { return iMemSize; }

    bool Put ( const CVector<short>& vecsData );
    CVector<short> Get();

protected:
    CVector<short>  vecsMemory;
    int             iMemSize;
    int             iPutPos;
};


#endif /* !defined ( BUFFER_H__3B123453_4344_BB23945IUHF1912__INCLUDED_ ) */
