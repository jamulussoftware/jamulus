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


/* Network buffer with statistic calculations implementation ******************/
CNetBufWithStats::CNetBufWithStats() :
    CNetBuf                   ( false ), // base class init: no simulation mode
    iMaxStatisticCount        ( MAX_STATISTIC_COUNT ),
    bUseDoubleSystemFrameSize ( false ),
    dAutoFilt_WightUpNormal   ( IIR_WEIGTH_UP_NORMAL ),
    dAutoFilt_WightDownNormal ( IIR_WEIGTH_DOWN_NORMAL ),
    dAutoFilt_WightUpFast     ( IIR_WEIGTH_UP_FAST ),
    dAutoFilt_WightDownFast   ( IIR_WEIGTH_DOWN_FAST ),
    dErrorRateBound           ( ERROR_RATE_BOUND ),
    dUpMaxErrorBound          ( UP_MAX_ERROR_BOUND )
{
    // Define the sizes of the simulation buffers,
    // must be NUM_STAT_SIMULATION_BUFFERS elements!
    // Avoid the buffer length 1 because we do not have a solution for a
    // sample rate offset correction. Caused by the jitter we usually get bad
    // performance with just one buffer.
    viBufSizesForSim[0] = 2;
    viBufSizesForSim[1] = 3;
    viBufSizesForSim[2] = 4;
    viBufSizesForSim[3] = 5;
    viBufSizesForSim[4] = 6;
    viBufSizesForSim[5] = 7;
    viBufSizesForSim[6] = 8;
    viBufSizesForSim[7] = 9;
    viBufSizesForSim[8] = 10;
    viBufSizesForSim[9] = 11;

    // set all simulation buffers in simulation mode
    for ( int i = 0; i < NUM_STAT_SIMULATION_BUFFERS; i++ )
    {
        SimulationBuffer[i].SetIsSimulation ( true );
    }
}

void CNetBufWithStats::GetErrorRates ( CVector<double>& vecErrRates,
                                       double&          dLimit,
                                       double&          dMaxUpLimit )
{
    // get all the averages of the error statistic
    vecErrRates.Init ( NUM_STAT_SIMULATION_BUFFERS );

    for ( int i = 0; i < NUM_STAT_SIMULATION_BUFFERS; i++ )
    {
        vecErrRates[i] = ErrorRateStatistic[i].GetAverage();
    }

    // get the limits for the decisions
    dLimit      = dErrorRateBound;
    dMaxUpLimit = dUpMaxErrorBound;
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
        // set the auto filter weights and max statistic count
        if ( bUseDoubleSystemFrameSize )
        {
            dAutoFilt_WightUpNormal   = IIR_WEIGTH_UP_NORMAL_DOUBLE_FRAME_SIZE;
            dAutoFilt_WightDownNormal = IIR_WEIGTH_DOWN_NORMAL_DOUBLE_FRAME_SIZE;
            dAutoFilt_WightUpFast     = IIR_WEIGTH_UP_FAST_DOUBLE_FRAME_SIZE;
            dAutoFilt_WightDownFast   = IIR_WEIGTH_DOWN_FAST_DOUBLE_FRAME_SIZE;
            iMaxStatisticCount        = MAX_STATISTIC_COUNT_DOUBLE_FRAME_SIZE;
            dErrorRateBound           = ERROR_RATE_BOUND_DOUBLE_FRAME_SIZE;
            dUpMaxErrorBound          = UP_MAX_ERROR_BOUND_DOUBLE_FRAME_SIZE;
        }
        else
        {
            dAutoFilt_WightUpNormal   = IIR_WEIGTH_UP_NORMAL;
            dAutoFilt_WightDownNormal = IIR_WEIGTH_DOWN_NORMAL;
            dAutoFilt_WightUpFast     = IIR_WEIGTH_UP_FAST;
            dAutoFilt_WightDownFast   = IIR_WEIGTH_DOWN_FAST;
            iMaxStatisticCount        = MAX_STATISTIC_COUNT;
            dErrorRateBound           = ERROR_RATE_BOUND;
            dUpMaxErrorBound          = UP_MAX_ERROR_BOUND;
        }

        for ( int i = 0; i < NUM_STAT_SIMULATION_BUFFERS; i++ )
        {
            // init simulation buffers with the correct size
            SimulationBuffer[i].Init ( iNewBlockSize, viBufSizesForSim[i] );

            // init statistics
            ErrorRateStatistic[i].Init ( iMaxStatisticCount, true );
        }

        // reset the initialization counter which controls the initialization
        // phase length
        ResetInitCounter();

        // init auto buffer setting with a meaningful value, also init the
        // IIR parameter with this value
        iCurAutoBufferSizeSetting = 6;
        dCurIIRFilterResult       = iCurAutoBufferSizeSetting;
        iCurDecidedResult         = iCurAutoBufferSizeSetting;
    }
}

void CNetBufWithStats::ResetInitCounter()
{
    // start initialization phase of IIR filtering, use a quarter the size
    // of the error rate statistic buffers which should be ok for a good
    // initialization value (initialization phase should be as short as
    // possible)
    iInitCounter = iMaxStatisticCount / 4;
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

bool CNetBufWithStats::Get ( CVector<uint8_t>& vecbyData,
                             const int         iOutSize )
{
    // call base class Get
    const bool bGetOK = CNetBuf::Get ( vecbyData, iOutSize );

    // update statistics calculations
    for ( int i = 0; i < NUM_STAT_SIMULATION_BUFFERS; i++ )
    {
        ErrorRateStatistic[i].Update (
            !SimulationBuffer[i].Get ( vecbyData, iOutSize ) );
    }

    // update auto setting
    UpdateAutoSetting();

    return bGetOK;
}

void CNetBufWithStats::UpdateAutoSetting()
{
    int  iCurDecision      = 0; // dummy initialization
    int  iCurMaxUpDecision = 0; // dummy initialization
    bool bDecisionFound;


    // Get regular error rate decision -----------------------------------------
    // Use a specified error bound to identify the best buffer size for the
    // current network situation. Start with the smallest buffer and
    // test for the error rate until the rate is below the bound.
    bDecisionFound = false;

    for ( int i = 0; i < NUM_STAT_SIMULATION_BUFFERS - 1; i++ )
    {
        if ( ( !bDecisionFound ) &&
             ( ErrorRateStatistic[i].GetAverage() <= dErrorRateBound ) )
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


    // Get maximum upper error rate decision -----------------------------------
    // Use a specified error bound to identify the maximum upper error rate
    // to identify if we have a too low buffer setting which gives a very
    // bad performance constantly. Start with the smallest buffer and
    // test for the error rate until the rate is below the bound.
    bDecisionFound = false;

    for ( int i = 0; i < NUM_STAT_SIMULATION_BUFFERS - 1; i++ )
    {
        if ( ( !bDecisionFound ) &&
             ( ErrorRateStatistic[i].GetAverage() <= dUpMaxErrorBound ) )
        {
            iCurMaxUpDecision = viBufSizesForSim[i];
            bDecisionFound    = true;
        }
    }

    if ( !bDecisionFound )
    {
        // in case no buffer is below bound, use largest buffer size
        iCurMaxUpDecision = viBufSizesForSim[NUM_STAT_SIMULATION_BUFFERS - 1];

        // This is a worst case, something very bad had happened. Hopefully
        // this was just temporary so that we initiate a new initialzation
        // phase to get quickly back to normal buffer sizes (hopefully).
        ResetInitCounter();
    }


    // Post calculation (filtering) --------------------------------------------
    // Define different weigths for up and down direction. Up direction
    // filtering shall be slower than for down direction since we assume
    // that the lower value is the actual value which can be used for
    // the current network condition. If the current error rate estimation
    // is higher, it may be a temporary problem which should not change
    // the current jitter buffer size significantly.
    // For the initialization phase, use lower weight values to get faster
    // adaptation.
    double       dWeightUp, dWeightDown;
    const double dHysteresisValue   = FILTER_DECISION_HYSTERESIS;
    bool         bUseFastAdaptation = false;

    // check for initialization phase
    if ( iInitCounter > 0 )
    {
        // decrease init counter
        iInitCounter--;

        // use the fast adaptation
        bUseFastAdaptation = true;
    }

    // if the current detected buffer setting is below the maximum upper bound
    // decision, then we enable a booster to go up to the minimum required
    // number of buffer blocks (i.e. we use weights for fast adaptation)
    if ( iCurAutoBufferSizeSetting < iCurMaxUpDecision )
    {
        bUseFastAdaptation = true;
    }

    if ( bUseFastAdaptation )
    {
        dWeightUp   = dAutoFilt_WightUpFast;
        dWeightDown = dAutoFilt_WightDownFast;
    }
    else
    {
        dWeightUp   = dAutoFilt_WightUpNormal;
        dWeightDown = dAutoFilt_WightDownNormal;
    }

    // apply non-linear IIR filter
    MathUtils().UpDownIIR1 ( dCurIIRFilterResult,
                             static_cast<double> ( iCurDecision ),
                             dWeightUp,
                             dWeightDown );

/*
// TEST store important detection parameters in file for debugging
static FILE* pFile = fopen ( "test.dat", "w" );
static int icnt = 0;
if ( icnt == 50 )
{
    fprintf ( pFile, "%d %e\n", iCurDecision, dCurIIRFilterResult );
    fflush ( pFile );
    icnt = 0;
}
else
{
    icnt++;
}
*/

    // apply a hysteresis
    iCurAutoBufferSizeSetting =
        MathUtils().DecideWithHysteresis ( dCurIIRFilterResult,
                                           iCurDecidedResult,
                                           dHysteresisValue );


    // Initialization phase check and correction -------------------------------
    // sometimes in the very first period after a connection we get a bad error
    // rate result -> delete this from the initialization phase
    if ( iInitCounter == iMaxStatisticCount / 8 )
    {
        // check error rate of the largest buffer as the indicator
        if ( ErrorRateStatistic[NUM_STAT_SIMULATION_BUFFERS - 1].
             GetAverage() > dErrorRateBound )
        {
            for ( int i = 0; i < NUM_STAT_SIMULATION_BUFFERS; i++ )
            {
                ErrorRateStatistic[i].Reset();
            }
        }
    }
}
