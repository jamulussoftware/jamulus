/******************************************************************************\
 * Copyright (c) 2004-2011
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
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
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
    CBufferBase<uint8_t>::Init ( iNewBlockSize * iNewNumBlocks, bPreserve );

    // use the "get" flag to make sure the buffer is cleared
    if ( !bPreserve )
    {
        Clear ( CT_GET );
    }
}

bool CNetBuf::Put ( const CVector<uint8_t>& vecbyData,
                    const int               iInSize )
{
    bool bPutOK = true;

    // Check if there is not enough space available -> correct
    if ( GetAvailSpace() < iInSize )
    {
/*
        // not enough space in buffer for put operation, correct buffer to
        // prepare for new data
        Clear ( CT_PUT );

        bPutOK = false; // return error flag
*/

/*
// TEST invalidate last written block for better PLC
// ATTENTION: We assume that mem size is factor of block size here!!!!
if ( !bIsSimulation )
{
    if ( iPutPos == 0 )
    {
        for ( int iPos = iMemSize - iInSize; iPos < iMemSize; iPos++ )
        {
            vecMemory[iPos] = 0;
        }
    }
    else
    {
        for ( int iPos = iPutPos - iInSize; iPos < iPutPos; iPos++ )
        {
            vecMemory[iPos] = 0;
        }
    }
}
*/

return false;

        // check for special case: buffer memory is not sufficient
        if ( iInSize > iMemSize )
        {
            // do nothing here, just return error code
            return bPutOK;
        }
    }

    // copy new data in internal buffer (implemented in base class)
    CBufferBase<uint8_t>::Put ( vecbyData, iInSize );

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
/*
    // check for invalid data in buffer
    if ( iNumInvalidElements > 0 )
    {
        // decrease number of invalid elements by the queried number (input
        // size)
        iNumInvalidElements -= iInSize;

        bGetOK = false; // return error flag
    }
*/

    // Check if there is not enough data available -> correct
    if ( GetAvailData() < iInSize )
    {
/*
        // not enough data in buffer for get operation, correct buffer to
        // prepare for getting data
        Clear ( CT_GET );

        bGetOK = false; // return error flag
*/
return false;

        // check for special case: buffer memory is not sufficient
        if ( iInSize > iMemSize )
        {
            // do nothing here, just return error code
            return bGetOK;
        }
    }

    // copy data from internal buffer in output buffer (implemented in base
    // class)
    CBufferBase<uint8_t>::Get ( vecbyData );

/*
// TEST check for all zero packet
bool bAllZeroPacket = true;
for ( int iPos = 0; iPos < iInSize; iPos++ )
{
    if ( vecbyData[iPos] != 0 )
    {
        bAllZeroPacket = false;
    }
}
if ( bAllZeroPacket )
{
    return false;
}
*/

    return bGetOK;
}

void CNetBuf::Clear ( const EClearType eClearType )
{

// TEST
CBufferBase<uint8_t>::Clear ( eClearType );


/*
    // Define the number of blocks bound for the "random offset" (1) algorithm.
    // If we are above the bound, we use the "middle of buffer" (2) algorithm.
    //
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
            // Random position algorithm.
            // overwrite fill level with random value, the range
            // is 0 to (iMemSize - iBlockSize)
            iNewFillLevel = static_cast<int> ( static_cast<double> ( rand() ) *
                iNumBlocks / RAND_MAX ) * iBlockSize;
        }
        else
        {
            // Middle of buffer algorithm.
            // with the following operation we set the fill level to a block
            // boundary (one block below the middle of the buffer in case of odd
            // number of blocks, e.g.:
            // [buffer size]: [get pos]
            // 1: 0   /   2: 0   /   3: 1   /   4: 1   /   5: 2 ...)
            iNewFillLevel =
                ( ( ( iMemSize - iBlockSize) / 2 ) / iBlockSize ) * iBlockSize;


// TEST
iNewFillLevel += static_cast<int> ( static_cast<double> ( rand() ) *
                iNumBlocksBoundInclForRandom / RAND_MAX ) * iBlockSize - 
                iNumBlocksBoundInclForRandom / 2 * iBlockSize;

        }
    }

    // different behaviour for get and put corrections
    if ( eClearType == CT_GET )
    {
        // clear buffer since we had a buffer underrun
        if ( !bIsSimulation )
        {
            vecMemory.Reset ( 0 );
        }

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

// TEST
//iNumInvalidElements = 8 * iMemSize;
*/
}


/* Network buffer with statistic calculations implementation ******************/
CNetBufWithStats::CNetBufWithStats() :
    CNetBuf ( false ) // base class init: no simulation mode
{
    // define the sizes of the simulation buffers,
    // must be NUM_STAT_SIMULATION_BUFFERS elements!
    viBufSizesForSim[0] = 2;
    viBufSizesForSim[1] = 3;
    viBufSizesForSim[2] = 4;
    viBufSizesForSim[3] = 5;
    viBufSizesForSim[4] = 6;
    viBufSizesForSim[5] = 7;
    viBufSizesForSim[6] = 8;
    viBufSizesForSim[7] = 9;
    viBufSizesForSim[8] = 10;
    viBufSizesForSim[9] = 12;
    viBufSizesForSim[10] = 14;
    viBufSizesForSim[11] = 17;
    viBufSizesForSim[12] = 20;

    // set all simulation buffers in simulation mode
    for ( int i = 0; i < NUM_STAT_SIMULATION_BUFFERS; i++ )
    {
        SimulationBuffer[i].SetIsSimulation ( true );
    }
}

void CNetBufWithStats::Init ( const int  iNewBlockSize,
                              const int  iNewNumBlocks,
                              const bool bPreserve )
{
    // call base class Init
    CNetBuf::Init ( iNewBlockSize, iNewNumBlocks, bPreserve );

    // inits for statistics calculation
    if ( !bPreserve )
    {
        for ( int i = 0; i < NUM_STAT_SIMULATION_BUFFERS; i++ )
        {
            // init simulation buffers with the correct size
            SimulationBuffer[i].Init ( iNewBlockSize, viBufSizesForSim[i] );

            // init statistics
            ErrorRateStatistic[i].Init ( MAX_STATISTIC_COUNT, true );
        }
    }
}

bool CNetBufWithStats::Put ( const CVector<uint8_t>& vecbyData,
                             const int               iInSize )
{
    // call base class Put
    const bool bPutOK = CNetBuf::Put ( vecbyData, iInSize );

    // update statistics calculations
    for ( int i = 0; i < NUM_STAT_SIMULATION_BUFFERS; i++ )
    {
        ErrorRateStatistic[i].Update (
            !SimulationBuffer[i].Put ( vecbyData, iInSize ) );
    }

    return bPutOK;
}

bool CNetBufWithStats::Get ( CVector<uint8_t>& vecbyData )
{
    // call base class Get
    const bool bGetOK = CNetBuf::Get ( vecbyData );

    // update statistics calculations
    for ( int i = 0; i < NUM_STAT_SIMULATION_BUFFERS; i++ )
    {
        ErrorRateStatistic[i].Update (
            !SimulationBuffer[i].Get ( vecbyData ) );
    }

    return bGetOK;
}

int CNetBufWithStats::GetAutoSetting()
{
    // Use a specified error bound to identify the best buffer size for the
    // current network situation. Start with the smallest buffer and
    // test for the error rate until the rate is below the bound.
    for ( int i = 0; i < NUM_STAT_SIMULATION_BUFFERS - 1; i++ )
    {
        if ( ErrorRateStatistic[i].GetAverage() <= 0.005 )
        {
            return viBufSizesForSim[i];
        }
    }
    return viBufSizesForSim[NUM_STAT_SIMULATION_BUFFERS - 1];
}


// TEST for debugging
void CNetBufWithStats::StoreAllSimAverages()
{

    FILE* pFile = fopen ( "c:\\temp\\test.dat", "w" );


    for ( int i = 0; i < NUM_STAT_SIMULATION_BUFFERS - 1; i++ )
    {
        fprintf ( pFile, "%e, ", ErrorRateStatistic[i].GetAverage() );
    }
    fprintf ( pFile, "%e", ErrorRateStatistic[NUM_STAT_SIMULATION_BUFFERS - 1].GetAverage() );
    fprintf ( pFile, "\n" );


/*
const int iLen = ErrorRateStatistic[4].ErrorsMovAvBuf.Size();
for ( int i = 0; i < iLen; i++ )
{
    fprintf ( pFile, "%e\n", ErrorRateStatistic[4].ErrorsMovAvBuf[i] );
}
*/
    fclose ( pFile );

// scilab:
// close;x=read('c:/temp/test.dat',-1,13);plot2d([2,3,4,5,6,7,8,9,10,12,14,17,20], x, style=-1 , logflag = 'nl');plot2d([2 20],[1 1]*0.01);plot2d([2 20],[1 1]*0.005);x
}
