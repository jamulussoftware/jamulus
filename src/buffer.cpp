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

    // check for invalid data in buffer (not applicable for simulation
    // buffers)
    if ( ( iNumInvalidElements > 0 ) && !bIsSimulation )
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
        iNumInvalidElements = 10;

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
    viBufSizesForSim[0]  = 2;
    viBufSizesForSim[1]  = 3;
    viBufSizesForSim[2]  = 4;
    viBufSizesForSim[3]  = 5;
    viBufSizesForSim[4]  = 6;
    viBufSizesForSim[5]  = 7;
    viBufSizesForSim[6]  = 8;
    viBufSizesForSim[7]  = 9;
    viBufSizesForSim[8]  = 10;
    viBufSizesForSim[9]  = 11;
    viBufSizesForSim[10] = 12;
    viBufSizesForSim[11] = 14;
    viBufSizesForSim[12] = 16;

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

        // start initialization phase of IIR filtering, use half the size of
        // the error rate statistic buffers which should be ok for a good
        // initialization value (initialization phase should be as short as
        // possible
        iInitCounter = MAX_STATISTIC_COUNT / 2;

        // init auto buffer setting with a meaningful value (should not be used
        // anyway)
        iCurAutoBufferSizeSetting = 5;
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

    // update auto setting
    UpdateAutoSetting();


// TEST
// sometimes in the very first period after a connection we get a bad error
// rate result -> delete this from the initialization phase
const double dInitState =
    ErrorRateStatistic[NUM_STAT_SIMULATION_BUFFERS - 1].InitializationState();

if ( dInitState < 0.1 )
{
    if ( ErrorRateStatistic[NUM_STAT_SIMULATION_BUFFERS - 1].GetAverage() > ERROR_RATE_BOUND )
    {
        for ( int i = 0; i < NUM_STAT_SIMULATION_BUFFERS; i++ )
        {
            ErrorRateStatistic[i].Reset();
        }
    }
}


    return bGetOK;
}

void CNetBufWithStats::UpdateAutoSetting()
{
    int  iCurDecision   = 0; // dummy initialization
    bool bDecisionFound = false;


    // Get error rate decision -------------------------------------------------
    // Use a specified error bound to identify the best buffer size for the
    // current network situation. Start with the smallest buffer and
    // test for the error rate until the rate is below the bound.
    for ( int i = 0; i < NUM_STAT_SIMULATION_BUFFERS - 1; i++ )
    {
        if ( ( !bDecisionFound ) &&
             ( ErrorRateStatistic[i].GetAverage() <= ERROR_RATE_BOUND ) )
        {
            iCurDecision   = viBufSizesForSim[i];
            bDecisionFound = true;
        }
    }

    if ( !bDecisionFound )
    {
        // in case no buffer is below bound, use largest buffer size
        iCurDecision = viBufSizesForSim[NUM_STAT_SIMULATION_BUFFERS - 1];
    }


    // Post calculation (filtering) --------------------------------------------
    if ( iInitCounter > 0 )
    {
        // for initialization phase, use current decision without applying
        // any filtering
        iCurAutoBufferSizeSetting = iCurDecision;

        // decrease init counter
        iInitCounter--;

        if ( iInitCounter == 0 )
        {
            // initialization phase is at the end now, init parameters for
            // regular estimation phase
            dCurIIRFilterResult = iCurDecision;
            iCurDecidedResult   = iCurDecision;
        }
    }
    else
    {
        // Define different weigths for up and down direction. Up direction
        // filtering shall be slower than for down direction since we assume
        // that the lower value is the actual value which can be used for
        // the current network condition. If the current error rate estimation
        // is higher, it may be a temporary problem which should not change
        // the current jitter buffer size significantly.
        const double dWeightUp        = 0.999995;
        const double dWeightDown      = 0.9999;
        const double dHysteresisValue = 0.1;

        // apply non-linear IIR filter
        LlconMath().UpDownIIR1( dCurIIRFilterResult,
                                static_cast<double> ( iCurDecision ),
                                dWeightUp,
                                dWeightDown);

        // apply a hysteresis
        iCurAutoBufferSizeSetting =
            LlconMath().DecideWithHysteresis ( dCurIIRFilterResult,
                                               iCurDecidedResult,
                                               dHysteresisValue );
    }

/*
#ifdef _WIN32
// TEST
static FILE* pFile = fopen ( "c:\\temp\\test.dat", "w" );
fprintf ( pFile, "%d %e %d\n", iCurDecision, dCurIIRFilterResult, GetAutoSetting() );
fflush ( pFile );
#endif
*/
}


// TEST for debugging
void CNetBufWithStats::StoreAllSimAverages()
{
    FILE* pFile = fopen ( "c:\\temp\\test1.dat", "w" );

    for ( int i = 0; i < NUM_STAT_SIMULATION_BUFFERS - 1; i++ )
    {
        fprintf ( pFile, "%e, ", ErrorRateStatistic[i].GetAverage() );
    }
    fprintf ( pFile, "%e", ErrorRateStatistic[NUM_STAT_SIMULATION_BUFFERS - 1].GetAverage() );
    fprintf ( pFile, "\n" );

    fclose ( pFile );

// scilab:
// close;x=read('c:/temp/test.dat',-1,13);plot2d([2,3,4,5,6,7,8,9,10,11,12,14,16], x, style=-1 , logflag = 'nl');plot2d([2 20],[1 1]*0.01);plot2d([2 20],[1 1]*0.005);x
}
