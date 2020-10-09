/******************************************************************************\
 * Copyright (c) 2004-2020
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
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 *
\******************************************************************************/

#pragma once

#include "util.h"
#include "global.h"

/* Classes ********************************************************************/
// Buffer base class -----------------------------------------------------------
template<class TData> class CBufferBase
{
public:
    CBufferBase () :
       bIsInitialized ( false ) {}

    void Init ( const int  iNewMemSize,
                const bool bPreserve = false )
    {
        // only enter the "preserve" branch, if object was already initialized
        if ( bPreserve && bIsInitialized )
        {
            // copy old data in new vector using get pointer as zero per
            // definition
            int iCurPos;

            // copy current data in temporary vector
            CVector<TData> vecTempMemory ( vecMemory );

            // resize actual buffer memory
            vecMemory.Init ( iNewMemSize );

            // get maximum number of data to be copied
            int iCopyLen = GetAvailData();

            if ( iCopyLen > iNewMemSize )
            {
                iCopyLen = iNewMemSize;
            }

            // set correct buffer state
            if ( iCopyLen == iNewMemSize )
            {
                eBufState = CBufferBase<TData>::BS_FULL;
            }
            else
            {
                if ( iCopyLen == 0 )
                {
                    eBufState = CBufferBase<TData>::BS_EMPTY;
                }
                else
                {
                    eBufState = CBufferBase<TData>::BS_OK;
                }
            }

            if ( iGetPos < iPutPos )
            {
                // "get" position is before "put" position -> no wrap around
                for ( iCurPos = 0; iCurPos < iCopyLen; iCurPos++ )
                {
                    vecMemory[iCurPos] = vecTempMemory[iGetPos + iCurPos];
                }
            }
            else
            {
                // "put" position is before "get" position -> wrap around
                bool bEnoughSpaceForSecondPart = true;
                int  iFirstPartLen             = iMemSize - iGetPos;

                // check that first copy length is not larger then new memory
                if ( iFirstPartLen >= iCopyLen )
                {
                    iFirstPartLen             = iCopyLen;
                    bEnoughSpaceForSecondPart = false;
                }

                for ( iCurPos = 0; iCurPos < iFirstPartLen; iCurPos++ )
                {
                    vecMemory[iCurPos] = vecTempMemory[iGetPos + iCurPos];
                }

                if ( bEnoughSpaceForSecondPart )
                {
                    // calculate remaining copy length
                    const int iRemainingCopyLen = iCopyLen - iFirstPartLen;

                    // perform copying of second part
                    for ( iCurPos = 0; iCurPos < iRemainingCopyLen; iCurPos++ )
                    {
                        vecMemory[iCurPos + iFirstPartLen] = vecTempMemory[iCurPos];
                    }
                }
            }

            // update put pointer
            if ( eBufState == CBufferBase<TData>::BS_FULL )
            {
                iPutPos = 0;
            }
            else
            {
                iPutPos = iCopyLen;
            }

            // update get position -> zero per definition
            iGetPos = 0;
        }
        else
        {
            // allocate memory for actual data buffer
            vecMemory.Init ( iNewMemSize );

            // init buffer pointers and buffer state (empty buffer)
            iGetPos   = 0;
            iPutPos   = 0;
            eBufState = CBufferBase<TData>::BS_EMPTY;
        }

        // store total memory size value
        iMemSize = iNewMemSize;

        // set initialized flag
        bIsInitialized = true;
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
            std::copy ( vecData.begin(),
                        vecData.begin() + iInSize,
                        vecMemory.begin() + iPutPos );

            // set the put position one block further (no wrap around needs
            // to be considered here)
            iPutPos += iInSize;
        }

        // take care about wrap around of put pointer
        if ( iPutPos == iMemSize )
        {
            iPutPos = 0;
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

        return true; // no error check in base class, always return ok
    }

    virtual bool Get ( CVector<TData>& vecData,
                       const int       iOutSize )
    {
        // copy data from internal buffer in output buffer
        int iCurPos = 0;

        if ( iGetPos + iOutSize > iMemSize )
        {
            // remaining data size for second block
            const int iRemData = iGetPos + iOutSize - iMemSize;

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
            std::copy ( vecMemory.begin() + iGetPos,
                        vecMemory.begin() + iGetPos + iOutSize,
                        vecData.begin() );

            // set the get position one block further (no wrap around needs
            // to be considered here)
            iGetPos += iOutSize;
        }

        // take care about wrap around of get pointer
        if ( iGetPos == iMemSize )
        {
            iGetPos = 0;
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

        return true; // no error check in base class, always return ok
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

    virtual void Clear()
    {
        // clear memory
        vecMemory.Reset ( 0 );

        // init buffer pointers and buffer state (empty buffer)
        iGetPos   = 0;
        iPutPos   = 0;
        eBufState = CBufferBase<TData>::BS_EMPTY;
    }

    CVector<TData> vecMemory;
    int            iMemSize;
    int            iGetPos;
    int            iPutPos;
    EBufState      eBufState;
    bool           bIsInitialized;
};


// Network buffer (jitter buffer) ----------------------------------------------
class CNetBuf : public CBufferBase<uint8_t>
{
public:
    void Init ( const int  iNewBlockSize,
                const int  iNewNumBlocks,
                const bool bPreserve = false );

    int GetSize() { return iMemSize / iBlockSize; }

    virtual bool Put ( const CVector<uint8_t>& vecbyData, const int iInSize );
    virtual bool Get ( CVector<uint8_t>& vecbyData, const int iOutSize );

protected:
    int iBlockSize;
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
        // allocate internal memory and reset read/write positions
        vecMemory.Init ( iNewMemSize );
        iMemSize    = iNewMemSize;
        iBufferSize = iNewMemSize;
        Reset();
    }

    void Reset()
    {
        iPutPos = 0;
        iGetPos = 0;
    }

    void SetBufferSize ( const int iNBSize )
    {
        // if buffer size has changed, apply new value and reset the buffer pointers
        if ( ( iNBSize != iBufferSize ) && ( iNBSize <= iMemSize ) )
        {
            iBufferSize = iNBSize;
            Reset();
        }
    }

    void PutAll ( const CVector<TData>& vecsData )
    {
        iGetPos = 0;

        std::copy ( vecsData.begin(),
                    vecsData.begin() + iBufferSize, // note that input vector might be larger then memory size
                    vecMemory.begin() );
    }

    bool Put ( const CVector<TData>& vecsData,
               const int             iVecSize )
    {
        // calculate the input size and the end position after copying
        const int iEnd = iPutPos + iVecSize;

        // first check for buffer overrun
        if ( iEnd <= iBufferSize )
        {
            // copy new data in internal buffer
            std::copy ( vecsData.begin(),
                        vecsData.begin() + iVecSize,
                        vecMemory.begin() + iPutPos );

            // set buffer pointer one block further
            iPutPos = iEnd;

            // return "buffer is ready for readout" flag
            return ( iEnd == iBufferSize );
        }

        // buffer overrun or not initialized, return "not ready"
        return false;
    }

    const CVector<TData>& GetAll()
    {
        iPutPos = 0;
        return vecMemory;
    }

    void GetAll ( CVector<TData>& vecsData,
                  const int       iVecSize )
    {
        iPutPos = 0;

        // copy data from internal buffer in given buffer
        std::copy ( vecMemory.begin(),
                    vecMemory.begin() + iVecSize,
                    vecsData.begin() );
    }

    bool Get ( CVector<TData>& vecsData,
               const int       iVecSize )
    {
        // calculate the input size and the end position after copying
        const int iEnd = iGetPos + iVecSize;

        // first check for buffer underrun
        if ( iEnd <= iBufferSize )
        {
            // copy new data from internal buffer
            std::copy ( vecMemory.begin() + iGetPos,
                        vecMemory.begin() + iGetPos + iVecSize,
                        vecsData.begin() );

            // set buffer pointer one block further
            iGetPos = iEnd;

            // return the memory could be read
            return true;
        }

        // return that no memory could be read
        return false;
    }

protected:
    CVector<TData> vecMemory;
    int            iMemSize;
    int            iBufferSize;
    int            iPutPos, iGetPos;
};
