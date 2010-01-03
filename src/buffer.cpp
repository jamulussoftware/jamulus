/******************************************************************************\
 * Copyright (c) 2004-2009
 *
 * Author(s):
 *  Volker Fischer
 *
 * Note: we assuming here that put and get operations are secured by a mutex
 *       and do not take place at the same time
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

#include "buffer.h"


/* Implementation *************************************************************/
void CNetBuf::Init ( const int iNewBlockSize,
                     const int iNewNumBlocks )
{
    // total size -> size of one block times number of blocks
    iBlockSize = iNewBlockSize;
    iMemSize   = iNewBlockSize * iNewNumBlocks;

    // allocate and clear memory for actual data buffer
    vecbyMemory.Init ( iMemSize );

    // use the "get" flag to make sure the buffer is cleared
    Clear ( CT_GET );

    // init statistic
    ErrorRateStatistic.Init ( MAX_STATISTIC_COUNT );
}

bool CNetBuf::Put ( const CVector<uint8_t>& vecbyData,
                    const int iInSize )
{
    bool bPutOK = true;

    // Check if there is not enough space available -> correct
    if ( GetAvailSpace() < iInSize )
    {
        // not enough space in buffer for put operation, correct buffer to
        // prepare for new data
        Clear ( CT_PUT );

        bPutOK = false; // return error flag

        // check for special case: buffer memory is not sufficient
        if ( iInSize > iMemSize )
        {
            // do nothing here, just return error code
            return bPutOK;
        }
    }

    // copy new data in internal buffer
    int iCurPos = 0;
    if ( iPutPos + iInSize > iMemSize )
    {
        // remaining space size for second block
        const int iRemSpace = iPutPos + iInSize - iMemSize;

        // data must be written in two steps because of wrap around
        while ( iPutPos < iMemSize )
        {
            vecbyMemory[iPutPos++] = vecbyData[iCurPos++];
        }

        for ( iPutPos = 0; iPutPos < iRemSpace; iPutPos++ )
        {
            vecbyMemory[iPutPos] = vecbyData[iCurPos++];
        }
    }
    else
    {
        // data can be written in one step
        const int iEnd = iPutPos + iInSize;
        while ( iPutPos < iEnd )
        {
            vecbyMemory[iPutPos++] = vecbyData[iCurPos++];
        }
    }

    // set buffer state flag
    if ( iPutPos == iGetPos )
    {
        eBufState = CNetBuf::BS_FULL;
    }
    else
    {
        eBufState = CNetBuf::BS_OK;
    }

    // update statistic
    ErrorRateStatistic.Update ( !bPutOK );

    return bPutOK;
}

bool CNetBuf::Get ( CVector<uint8_t>& vecbyData )
{
    bool bGetOK = true; // init return value

    // get size of data to be get from the buffer
    const int iInSize = vecbyData.Size();

    // check size
    if ( ( iInSize == 0 ) || ( iInSize != iBlockSize ) )
    {
        return false;
    }

    // check for invalid data in buffer
    if ( iNumInvalidElements > 0 )
    {
        // decrease number of invalid elements by the queried number (input
        // size)
        iNumInvalidElements -= iInSize;

        bGetOK = false; // return error flag
    }

    // Check if there is not enough data available -> correct
    if ( GetAvailData() < iInSize )
    {
        // not enough data in buffer for get operation, correct buffer to
        // prepare for getting data
        Clear ( CT_GET );

        bGetOK = false; // return error flag

        // check for special case: buffer memory is not sufficient
        if ( iInSize > iMemSize )
        {
            // do nothing here, just return error code
            return bGetOK;
        }
    }

    // copy data from internal buffer in output buffer
    int iCurPos = 0;
    if ( iGetPos + iInSize > iMemSize )
    {
        // remaining data size for second block
        const int iRemData = iGetPos + iInSize - iMemSize;

        // data must be read in two steps because of wrap around
        while ( iGetPos < iMemSize )
        {
            vecbyData[iCurPos++] = vecbyMemory[iGetPos++];
        }

        for ( iGetPos = 0; iGetPos < iRemData; iGetPos++ )
        {
            vecbyData[iCurPos++] = vecbyMemory[iGetPos];
        }
    }
    else
    {
        // data can be read in one step
        const int iEnd = iGetPos + iInSize;
        while ( iGetPos < iEnd )
        {
            vecbyData[iCurPos++] = vecbyMemory[iGetPos++];
        }
    }

    // set buffer state flag
    if ( iPutPos == iGetPos )
    {
        eBufState = CNetBuf::BS_EMPTY;
    }
    else
    {
        eBufState = CNetBuf::BS_OK;
    }

    // update statistic
    ErrorRateStatistic.Update ( !bGetOK );

    return bGetOK;
}

int CNetBuf::GetAvailSpace() const
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

int CNetBuf::GetAvailData() const
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

void CNetBuf::Clear ( const EClearType eClearType )
{
    // Define the number of blocks bound for the "random offset" (1) algorithm.
    // If we are above the bound, we use the "middle of buffer" (2) algorithm.
    // Test results (with different jitter buffer sizes), given is the error
    // probability of jitter buffer (probability of corrections in the buffer):
    //  kX, 128 samples, WLAN:
    //      2: (1) 5 %,    (2) 12.3 %
    //      3: (1) 18.3 %, (2) 17.1 %
    //      5: (1) 0.9 %,  (2) 0.8 %
    //  kX, 128 samples, localhost:
    //      2: (1) 2.5 %,  (2) 13 %
    //      3: (1) 0.9 %,  (2) 1.1 %
    //      5: (1) 0.7 %,  (2) 0.6 %
    //  Behringer, 128 samples, WLAN:
    //      2: (1) 5.8 %,  (2) 9.4 %
    //      3: (1) 0.9 %,  (2) 0.8 %
    //      5: (1) 0.4 %,  (2) 0.3 %
    //  Behringer, 128 samples, localhost:
    //      2: (1) 1 %,    (2) 9.8 %
    //      3: (1) 0.57 %, (2) 0.6 %
    //      5: (1) 0.6 %,  (2) 0.56 %
    //  kX, 256 samples, WLAN:
    //      3: (1) 24.2 %, (2) 18.4 %
    //      4: (1) 1.5 %,  (2) 2.5 %
    //      5: (1) 1 %,    (2) 1 %
    //  ASIO4All, 256 samples, WLAN:
    //      3: (1) 14.9 %, (2) 11.9 %
    //      4: (1) 1.5 %,  (2) 7 %
    //      5: (1) 1.2 %,  (2) 1.3 %
    const int  iNumBlocksBoundInclForRandom = 4; // by extensive testing: 4

    int iNewFillLevel = 0;

    if ( iBlockSize != 0 )
    {
        const int iNumBlocks = iMemSize / iBlockSize;
        if ( iNumBlocks <= iNumBlocksBoundInclForRandom ) // just for small buffers
        {
            // random position algorithm
            // overwrite fill level with random value, the range
            // is 0 to (iMemSize - iBlockSize)
            iNewFillLevel = static_cast<int> ( static_cast<double> ( rand() ) *
                iNumBlocks / RAND_MAX ) * iBlockSize;
        }
        else
        {
            // middle of buffer algorithm
            // with the following operation we set the fill level to a block
            // boundary (one block below the middle of the buffer in case of odd
            // number of blocks, e.g.:
            // [buffer size]: [get pos]
            // 1: 0   /   2: 0   /   3: 1   /   4: 1   /   5: 2 ...)
            iNewFillLevel =
                ( ( ( iMemSize - iBlockSize) / 2 ) / iBlockSize ) * iBlockSize;
        }
    }

    // different behaviour for get and put corrections
    if ( eClearType == CT_GET )
    {
        // clear buffer since we had a buffer underrun
        vecbyMemory.Reset ( 0 );

        // reset buffer pointers so that they are at maximum distance after
        // the get operation (assign new fill level value to the get pointer)
        iPutPos = 0;
        iGetPos = iNewFillLevel;

        // The buffer was cleared, the next time blocks are read from the
        // buffer, these are invalid ones. Calculate the number of invalid
        // elements
        iNumInvalidElements = iMemSize - iNewFillLevel;

        // check for special case
        if ( iPutPos == iGetPos )
        {
            eBufState = CNetBuf::BS_FULL;
        }
        else
        {
            eBufState = CNetBuf::BS_OK;
        }
    }
    else
    {
        // in case of "put" correction, do not delete old data but only shift
        // the pointers
        iPutPos = iNewFillLevel;

        // adjust put pointer relative to current get pointer, take care of
        // wrap around
        iPutPos += iGetPos;
        if ( iPutPos > iMemSize )
        {
            iPutPos -= iMemSize;
        }

        // in case of put correction, no invalid blocks are inserted
        iNumInvalidElements = 0;

        // check for special case
        if ( iPutPos == iGetPos )
        {
            eBufState = CNetBuf::BS_EMPTY;
        }
        else
        {
            eBufState = CNetBuf::BS_OK;
        }
    }
}
