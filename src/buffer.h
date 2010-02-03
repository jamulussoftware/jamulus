/******************************************************************************\
 * Copyright (c) 2004-2010
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
// each regular buffer access lead to a count for put and get, assuming 2.33 ms
// blocks we have 30 s / 2.33 ms * 2 = 25714
#define MAX_STATISTIC_COUNT                 25714


/* Classes ********************************************************************/
// Buffer base class -----------------------------------------------------------
template<class TData> class CBufferBase
{
public:
    virtual void Init ( const int iNewMemSize );

    virtual bool Put ( const CVector<TData>& vecData, const int iInSize );
    virtual bool Get ( CVector<TData>& vecData );

    virtual int GetAvailSpace() const;
    virtual int GetAvailData() const;

protected:
    enum EBufState { BS_OK, BS_FULL, BS_EMPTY };

    CVector<TData> vecMemory;
    int            iMemSize;
    int            iGetPos, iPutPos;
    EBufState      eBufState;
};


// Network buffer (jitter buffer) ----------------------------------------------
class CNetBuf : public CBufferBase<uint8_t>
{
public:
    void Init ( const int iNewBlockSize, const int iNewNumBlocks );
    int GetSize() { return iMemSize / iBlockSize; }

    bool Put ( const CVector<uint8_t>& vecbyData, const int iInSize );
    bool Get ( CVector<uint8_t>& vecbyData );

    double GetErrorRate() { return ErrorRateStatistic.GetAverage(); }

protected:
    enum EClearType { CT_PUT, CT_GET };

    void Clear ( const EClearType eClearType );

    int              iBlockSize;
    int              iNumInvalidElements;

    // statistic
    CErrorRate       ErrorRateStatistic;
};


// Conversion buffer (very simple buffer) --------------------------------------
// For this very simple buffer no wrap around mechanism is implemented. We
// assume here, that the applied buffers are an integer fraction of the total
// buffer size.
template<class TData> class CConvBuf
{
public:
    CConvBuf() { Init ( 0 ); }

    void Init ( const int iNewMemSize )
    {
        // set memory size
        iMemSize = iNewMemSize;

        // allocate and clear memory for actual data buffer
        vecsMemory.Init ( iMemSize );

        iPutPos = 0;
    }

    int GetSize() const { return iMemSize; }

    bool Put ( const CVector<TData>& vecsData )
    {
        const int iVecSize = vecsData.Size();

        // copy new data in internal buffer
        int iCurPos = 0;
        const int iEnd = iPutPos + iVecSize;

        // first check for buffer overrun
        if ( iEnd <= iMemSize )
        {
            // actual copy operation
            while ( iPutPos < iEnd )
            {
                vecsMemory[iPutPos++] = vecsData[iCurPos++];
            }

            // return "buffer is ready for readout" flag
            return ( iEnd == iMemSize );
        }
        else
        {
            // buffer overrun or not initialized, return "not ready"
            return false;
        }
    }

    CVector<TData> Get()
    {
        iPutPos = 0;
        return vecsMemory;
    }

protected:
    CVector<TData> vecsMemory;
    int            iMemSize;
    int            iPutPos;
};

#endif /* !defined ( BUFFER_H__3B123453_4344_BB23945IUHF1912__INCLUDED_ ) */
