/******************************************************************************\
 * Copyright (c) 2004-2020
 *
 * Author(s):
 *  Volker Fischer
 *
 * Note: We are assuming here that put and get operations are secured by a mutex
 *       and accessing does not occur at the same time.
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

#include "buffer.h"


/* Network buffer implementation **********************************************/
void CNetBuf::Init ( const int  iNewBlockSize,
                     const int  iNewNumBlocks,
                     const bool bPreserve )
{
    // store block size value
    iBlockSize = iNewBlockSize;

    // total size -> size of one block times the number of blocks
    CBufferBase<uint8_t>::Init ( iNewBlockSize * iNewNumBlocks,
                                 bPreserve );

    // clear buffer if not preserved
    if ( !bPreserve )
    {
        Clear();
    }
}

bool CNetBuf::Put ( const CVector<uint8_t>& vecbyData,
                    const int               iInSize )
{
    bool bPutOK = true;

    // check if there is not enough space available
    if ( GetAvailSpace() < iInSize )
    {
        return false;
    }

    // copy new data in internal buffer (implemented in base class)
    CBufferBase<uint8_t>::Put ( vecbyData, iInSize );

    return bPutOK;
}

bool CNetBuf::Get ( CVector<uint8_t>& vecbyData,
                    const int         iOutSize )
{
    bool bGetOK = true; // init return value

    // check size
    if ( ( iOutSize == 0 ) || ( iOutSize != iBlockSize ) )
    {
        return false;
    }

    // check if there is not enough data available
    if ( GetAvailData() < iOutSize )
    {
        return false;
    }

    // copy data from internal buffer in output buffer (implemented in base
    // class)
    CBufferBase<uint8_t>::Get ( vecbyData, iOutSize );

    return bGetOK;
}
