/******************************************************************************\
 * Copyright (c) 2004-2011
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
    virtual void Init ( const int iNewMemSize )
    {
        // store total memory size value
        iMemSize = iNewMemSize;

        // allocate memory for actual data buffer
        vecMemory.Init ( iNewMemSize );

        // init buffer pointers and buffer state (empty buffer)
        iGetPos   = 0;
        iPutPos   = 0;
        eBufState = CBufferBase<TData>::BS_EMPTY;
    }

    virtual bool Put ( const CVector<TData>& vecData,
                       const int             iInSize )
    {
        // copy new data in internal buffer
        int iCurPos = 0;
        if ( iPutPos + iInSize > iMemSize )
        {
            // remaining space size for second block
            const int iRemSpace = iPutPos + iInSize - iMemSize;

            // data must be written in two steps because of wrap around
            while ( iPutPos < iMemSize )
            {
                vecMemory[iPutPos++] = vecData[iCurPos++];
            }

            for ( iPutPos = 0; iPutPos < iRemSpace; iPutPos++ )
            {
                vecMemory[iPutPos] = vecData[iCurPos++];
            }
        }
        else
        {
            // data can be written in one step
            const int iEnd = iPutPos + iInSize;
            while ( iPutPos < iEnd )
            {
                vecMemory[iPutPos++] = vecData[iCurPos++];
            }
        }

        // set buffer state flag
        if ( iPutPos == iGetPos )
        {
            eBufState = CBufferBase<TData>::BS_FULL;
        }
        else
        {
            eBufState = CBufferBase<TData>::BS_OK;
        }

        return true; // no error check in base class, alyways return ok
    }

    virtual bool Get ( CVector<TData>& vecData )
    {
        // get size of data to be get from the buffer
        const int iInSize = vecData.Size();

        // copy data from internal buffer in output buffer
        int iCurPos = 0;
        if ( iGetPos + iInSize > iMemSize )
        {
            // remaining data size for second block
            const int iRemData = iGetPos + iInSize - iMemSize;

            // data must be read in two steps because of wrap around
            while ( iGetPos < iMemSize )
            {
                vecData[iCurPos++] = vecMemory[iGetPos++];
            }

            for ( iGetPos = 0; iGetPos < iRemData; iGetPos++ )
            {
                vecData[iCurPos++] = vecMemory[iGetPos];
            }
        }
        else
        {
            // data can be read in one step
            const int iEnd = iGetPos + iInSize;
            while ( iGetPos < iEnd )
            {
                vecData[iCurPos++] = vecMemory[iGetPos++];
            }
        }

        // set buffer state flag
        if ( iPutPos == iGetPos )
        {
            eBufState = CBufferBase<TData>::BS_EMPTY;
        }
        else
        {
            eBufState = CBufferBase<TData>::BS_OK;
        }

        return true; // no error check in base class, alyways return ok
    }

    virtual int GetAvailSpace() const
    {
        // calculate available space in buffer
        int iAvSpace = iGetPos - iPutPos;

        // check for special case and wrap around
        if ( iAvSpace < 0 )
        {
            iAvSpace += iMemSize; // wrap around
        }
        else
        {
            if ( ( iAvSpace == 0 ) && ( eBufState == BS_EMPTY ) )
            {
                iAvSpace = iMemSize;
            }
        }

        return iAvSpace;
    }

    virtual int GetAvailData() const
    {
        // calculate available data in buffer
        int iAvData = iPutPos - iGetPos;

        // check for special case and wrap around
        if ( iAvData < 0 )
        {
            iAvData += iMemSize; // wrap around
        }
        else
        {
            if ( ( iAvData == 0 ) && ( eBufState == BS_FULL ) )
            {
                iAvData = iMemSize;
            }
        }

        return iAvData;
    }

protected:
    enum EBufState { BS_OK, BS_FULL, BS_EMPTY };

    void PutSimulation ( const int iInSize )
    {
        // in this simulation only the buffer pointers and the buffer state
        // is updated, no actual data is transferred
        if ( iPutPos + iInSize > iMemSize )
        {
            // data must be written in two steps because of wrap around
            iPutPos += iInSize - iMemSize - 1;
        }
        else
        {
            // data can be written in one step
            iPutPos += iInSize - 1;
        }

        // set buffer state flag
        if ( iPutPos == iGetPos )
        {
            eBufState = CBufferBase<TData>::BS_FULL;
        }
        else
        {
            eBufState = CBufferBase<TData>::BS_OK;
        }
    }

    void GetSimulation ( const int iInSize )
    {
        // in this simulation only the buffer pointers and the buffer state
        // is updated, no actual data is transferred
        if ( iGetPos + iInSize > iMemSize )
        {
            // data must be read in two steps because of wrap around
            iGetPos += iInSize - iMemSize - 1;
        }
        else
        {
            // data can be read in one step
            iGetPos += iInSize - 1;
        }

        // set buffer state flag
        if ( iPutPos == iGetPos )
        {
            eBufState = CBufferBase<TData>::BS_EMPTY;
        }
        else
        {
            eBufState = CBufferBase<TData>::BS_OK;
        }
    }

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
