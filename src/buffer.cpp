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
    int iMiddleOfBuffer = 0;

    if ( iBlockSize != 0 )
    {
        // with the following operation we set the new get pos to a block
        // boundary (one block below the middle of the buffer in case of odd
        // number of blocks, e.g.:
        // [buffer size]: [get pos]
        // 1: 0   /   2: 0   /   3: 1   /   4: 1   /   5: 2 ...
        iMiddleOfBuffer =
            ( ( ( iMemSize - iBlockSize) / 2 ) / iBlockSize ) * iBlockSize;
    }

    // different behaviour for get and put corrections
    if ( eClearType == CT_GET )
    {
        // clear buffer
        vecbyMemory.Reset ( 0 );

        // correct buffer so that after the current get operation the pointer
        // are at maximum distance
        iPutPos = 0;
        iGetPos = iMiddleOfBuffer;

        // The buffer was cleared, the next time blocks are read from the
        // buffer, these are invalid ones. Calculate the number of invalid
        // elements
        iNumInvalidElements = iMemSize - iMiddleOfBuffer;

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
        iPutPos = iMiddleOfBuffer;

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
