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
        // decrease number of invalid elements by one
        iNumInvalidElements -= 1;

        // return error flag, do not return function here since we have
        // to call the base Get function to pick one block out of the
        // buffer
        bGetOK = false;
    }

    // check if there is not enough data available -> correct
    if ( GetAvailData() < iInSize )
    {
        // in case we have a buffer underrun, invalidate the next 15 blocks
        // to avoid the unmusical noise resulting from a very short drop
        // out (note that if you want to change this value, also change
        // the value in celt_decode_lost in celt.c)
        iNumInvalidElements = 15;

        return false;
    }

    // copy data from internal buffer in output buffer (implemented in base
    // class)
    CBufferBase<uint8_t>::Get ( vecbyData );

    return bGetOK;
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
