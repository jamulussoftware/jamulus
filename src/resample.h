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

#if !defined ( RESAMPLE_H__3B0FEUFE7876F_FE8FE_CA63_4344_1912__INCLUDED_ )
#define RESAMPLE_H__3B0FEUFE7876F_FE8FE_CA63_4344_1912__INCLUDED_

#include "util.h"
#include "resamplefilter.h"
#include "global.h"


/* Classes ********************************************************************/
class CResample
{
public:
    CResample() {}
    virtual ~CResample() {}

    void Init ( const int iNewInputBlockSize );
    int Resample ( CVector<double>& vecdInput, CVector<double>& vecdOutput,
                   const double dRation );

protected:
    double          dTStep;
    double          dtOut;
    double          dBlockDuration;

    CVector<double> vecdIntBuff;
    int             iHistorySize;

    int             iInputBlockSize;
};

class CAudioResample
{
public:
    CAudioResample() {}
    virtual ~CAudioResample() {}

    void Init ( const int iNewInputBlockSize, const int iFrom, const int iTo );
    void Resample ( CVector<double>& vecdInput, CVector<double>& vecdOutput );

protected:
    double              dRation;

    CVector<double>     vecdIntBuff;
    int                 iHistorySize;

    int                 iInputBlockSize;
    int                 iOutputBlockSize;

    float*              pFiltTaps;
    int                 iNumTaps;
    int                 iI;
};


#endif // !defined ( RESAMPLE_H__3B0FEUFE7876F_FE8FE_CA63_4344_1912__INCLUDED_ )
