/******************************************************************************\
 * Copyright (c) 2004-2022
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
void CNetBuf::Init ( const int iNewBlockSize, const int iNewNumBlocks, const bool bNUseSequenceNumber, const bool bPreserve )
{
    // store the sequence number activation flag
    bUseSequenceNumber = bNUseSequenceNumber;

    // in simulation mode the size is not changed during operation -> we do
    // not have to implement special code for this case
    // only enter the "preserve" branch, if object was already initialized
    // and the block sizes are the same
    if ( bPreserve && ( !bIsSimulation ) && bIsInitialized && ( iBlockSize == iNewBlockSize ) )
    {
        // extract all data from buffer in temporary storage
        CVector<CVector<uint8_t>> vecvecTempMemory = vecvecMemory; // allocate worst case memory by copying

        if ( !bNUseSequenceNumber )
        {
            int iPreviousDataCnt = 0;

            while ( Get ( vecvecTempMemory[iPreviousDataCnt], iBlockSize ) )
            {
                iPreviousDataCnt++;
            }

            // now resize the buffer to the new size (buffer is empty after this operation)
            Resize ( iNewNumBlocks, iNewBlockSize );

            // copy the previous data back in the buffer (make sure we only copy as much
            // data back as the new buffer size can hold)
            int iDataCnt = 0;

            while ( ( iDataCnt < iPreviousDataCnt ) && Put ( vecvecTempMemory[iDataCnt], iBlockSize ) )
            {
                iDataCnt++;
            }
        }
        else
        {
            // store current complete buffer state in temporary memory
            CVector<int>  veciTempBlockValid ( iNumBlocksMemory );
            const uint8_t iOldSequenceNumberAtGetPos = iSequenceNumberAtGetPos;
            const int     iOldNumBlocksMemory        = iNumBlocksMemory;
            const int     iOldBlockGetPos            = iBlockGetPos;
            int           iCurBlockPos               = 0;

            while ( iBlockGetPos < iNumBlocksMemory )
            {
                veciTempBlockValid[iCurBlockPos] = veciBlockValid[iBlockGetPos];
                vecvecTempMemory[iCurBlockPos++] = vecvecMemory[iBlockGetPos++];
            }

            for ( iBlockGetPos = 0; iBlockGetPos < iOldBlockGetPos; iBlockGetPos++ )
            {
                veciTempBlockValid[iCurBlockPos] = veciBlockValid[iBlockGetPos];
                vecvecTempMemory[iCurBlockPos++] = vecvecMemory[iBlockGetPos];
            }

            // now resize the buffer to the new size
            Resize ( iNewNumBlocks, iNewBlockSize );

            // write back the temporary data in new memory
            iSequenceNumberAtGetPos = iOldSequenceNumberAtGetPos;
            iBlockGetPos            = 0; // per definition

            for ( int iCurPos = 0; iCurPos < std::min ( iNewNumBlocks, iOldNumBlocksMemory ); iCurPos++ )
            {
                veciBlockValid[iCurPos] = veciTempBlockValid[iCurPos];
                vecvecMemory[iCurPos]   = vecvecTempMemory[iCurPos];
            }
        }
    }
    else
    {
        Resize ( iNewNumBlocks, iNewBlockSize );
    }

    // set initialized flag
    bIsInitialized = true;
}

void CNetBuf::Resize ( const int iNewNumBlocks, const int iNewBlockSize )
{
    // allocate memory for actual data buffer
    vecvecMemory.Init ( iNewNumBlocks );
    veciBlockValid.Init ( iNewNumBlocks, 0 ); // initialize with zeros = invalid

    if ( !bIsSimulation )
    {
        for ( int iBlock = 0; iBlock < iNewNumBlocks; iBlock++ )
        {
            vecvecMemory[iBlock].Init ( iNewBlockSize );
        }
    }

    // init buffer pointers and buffer state (empty buffer) and store buffer properties
    iBlockGetPos     = 0;
    iBlockPutPos     = 0;
    eBufState        = BS_EMPTY;
    iBlockSize       = iNewBlockSize;
    iNumBlocksMemory = iNewNumBlocks;
}

bool CNetBuf::Put ( const CVector<uint8_t>& vecbyData, int iInSize )
{
    // if the sequence number is used, we need a complete different way of applying
    // the new network packet
    if ( bUseSequenceNumber )
    {
        // check that the input size is a multiple of the block size
        if ( ( iInSize % ( iBlockSize + iNumBytesSeqNum ) ) != 0 )
        {
            return false;
        }

        // to get the number of input blocks we assume that the number of bytes for
        // the sequence number is much smaller than the number of coded audio bytes
        const int iNumBlocks = /* floor */ ( iInSize / iBlockSize );

        // copy new data in internal buffer
        for ( int iBlock = 0; iBlock < iNumBlocks; iBlock++ )
        {
            // calculate the block offset once per loop instead of repeated multiplying
            const int iBlockOffset = iBlock * ( iBlockSize + iNumBytesSeqNum );

            // extract sequence number of current received block (per definition
            // the sequence number is appended after the coded audio data)
            const int iCurrentSequenceNumber = vecbyData[iBlockOffset + iBlockSize];

            // calculate the sequence number difference and take care of wrap
            int iSeqNumDiff = iCurrentSequenceNumber - static_cast<int> ( iSequenceNumberAtGetPos );

            if ( iSeqNumDiff < -128 )
            {
                iSeqNumDiff += 256;
            }
            else if ( iSeqNumDiff >= 128 )
            {
                iSeqNumDiff -= 256;
            }

            // The 1-byte sequence number wraps around at a count of 256. So, if a packet is delayed
            // further than this we cannot detect it. But it does not matter since such a packet is
            // more than 100 ms delayed so we have a bad network situation anyway. Therefore we
            // assume that the sequence number difference between the received and local counter is
            // correct. The idea of the following code is that we always move our "buffer window" so
            // that the received packet fits into the buffer. By doing this we are robust against
            // sample rate offsets between client/server or buffer glitches in the audio driver since
            // we adjust the window. The downside is that we never throw away single packets which arrive
            // too late so we throw away valid packets when we move the "buffer window" to the delayed
            // packet and then back to the correct place when the next normal packet is received. But
            // tests showed that the new buffer strategy does not perform worse than the old jitter
            // buffer which did not use any sequence number at all.
            if ( iSeqNumDiff < 0 )
            {
                // the received packet comes too late so we shift the "buffer window" to the past
                // until the received packet is the very first packet in the buffer
                for ( int i = iSeqNumDiff; i < 0; i++ )
                {
                    // insert an invalid block at the shifted position
                    veciBlockValid[iBlockGetPos] = 0; // invalidate

                    // we decrease the local sequence number and get position and take care of wrap
                    iSequenceNumberAtGetPos--;
                    iBlockGetPos--;

                    if ( iBlockGetPos < 0 )
                    {
                        iBlockGetPos += iNumBlocksMemory;
                    }
                }

                // insert the new packet at the beginning of the buffer since it was delayed
                iBlockPutPos = iBlockGetPos;
            }
            else if ( iSeqNumDiff >= iNumBlocksMemory )
            {
                // the received packet comes too early so we move the "buffer window" in the
                // future until the received packet is the last packet in the buffer
                for ( int i = 0; i < iSeqNumDiff - iNumBlocksMemory + 1; i++ )
                {
                    // insert an invalid block at the shifted position
                    veciBlockValid[iBlockGetPos] = 0; // invalidate

                    // we increase the local sequence number and get position and take care of wrap
                    iSequenceNumberAtGetPos++;
                    iBlockGetPos++;

                    if ( iBlockGetPos >= iNumBlocksMemory )
                    {
                        iBlockGetPos -= iNumBlocksMemory;
                    }
                }

                // insert the new packet at the end of the buffer since it is too early (since
                // we add an offset to the get position, we have to take care of wrapping)
                iBlockPutPos = iBlockGetPos + iNumBlocksMemory - 1;

                if ( iBlockPutPos >= iNumBlocksMemory )
                {
                    iBlockPutPos -= iNumBlocksMemory;
                }
            }
            else
            {
                // this is the regular case: the received packet fits into the buffer so
                // we will write it at the correct position based on the sequence number
                iBlockPutPos = iBlockGetPos + iSeqNumDiff;

                if ( iBlockPutPos >= iNumBlocksMemory )
                {
                    iBlockPutPos -= iNumBlocksMemory;
                }
            }

            // for simulation buffer only update pointer, no data copying
            if ( !bIsSimulation )
            {
                // copy one block of data in buffer
                std::copy ( vecbyData.begin() + iBlockOffset, vecbyData.begin() + iBlockOffset + iBlockSize, vecvecMemory[iBlockPutPos].begin() );
            }

            // valid packet added, set flag
            veciBlockValid[iBlockPutPos] = 1;
        }
    }
    else
    {
        // check if there is not enough space available and that the input size is a
        // multiple of the block size
        if ( ( GetAvailSpace() < iInSize ) || ( ( iInSize % iBlockSize ) != 0 ) )
        {
            return false;
        }

        // copy new data in internal buffer
        const int iNumBlocks = iInSize / iBlockSize;

        for ( int iBlock = 0; iBlock < iNumBlocks; iBlock++ )
        {
            // for simultion buffer only update pointer, no data copying
            if ( !bIsSimulation )
            {
                // calculate the block offset once per loop instead of repeated multiplying
                const int iBlockOffset = iBlock * iBlockSize;

                // copy one block of data in buffer
                std::copy ( vecbyData.begin() + iBlockOffset, vecbyData.begin() + iBlockOffset + iBlockSize, vecvecMemory[iBlockPutPos].begin() );
            }

            // set the put position one block further
            iBlockPutPos++;

            // take care about wrap around of put pointer
            if ( iBlockPutPos == iNumBlocksMemory )
            {
                iBlockPutPos = 0;
            }
        }

        // set buffer state flag
        if ( iBlockPutPos == iBlockGetPos )
        {
            eBufState = BS_FULL;
        }
        else
        {
            eBufState = BS_OK;
        }
    }

    return true;
}

bool CNetBuf::Get ( CVector<uint8_t>& vecbyData, const int iOutSize )
{
    bool bReturn = true;

    // check requested output size and available buffer data
    if ( ( iOutSize == 0 ) || ( iOutSize != iBlockSize ) || ( GetAvailData() < iOutSize ) )
    {
        return false;
    }

    // if using sequence numbers, we do not use the block put position
    // at all but only determine the state from the "valid block" indicator
    if ( bUseSequenceNumber )
    {
        bReturn = ( veciBlockValid[iBlockGetPos] > 0 );

        // invalidate the block we are now taking from the buffer
        veciBlockValid[iBlockGetPos] = 0; // zero means invalid
    }

    // for simultion buffer or invalid block only update pointer, no data copying
    if ( !bIsSimulation && bReturn )
    {
        // copy data from internal buffer in output buffer
        std::copy ( vecvecMemory[iBlockGetPos].begin(), vecvecMemory[iBlockGetPos].begin() + iBlockSize, vecbyData.begin() );
    }

    // set the get position and sequence number one block further
    iBlockGetPos++;
    iSequenceNumberAtGetPos++; // wraps around automatically

    // take care about wrap around of get pointer
    if ( iBlockGetPos == iNumBlocksMemory )
    {
        iBlockGetPos = 0;
    }

    // set buffer state flag
    if ( iBlockPutPos == iBlockGetPos )
    {
        eBufState = BS_EMPTY;
    }
    else
    {
        eBufState = BS_OK;
    }

    return bReturn;
}

int CNetBuf::GetAvailSpace() const
{
    // calculate available space in buffer
    int iAvBlocks = iBlockGetPos - iBlockPutPos;

    // check for special case and wrap around
    if ( iAvBlocks < 0 )
    {
        iAvBlocks += iNumBlocksMemory; // wrap around
    }
    else
    {
        if ( ( iAvBlocks == 0 ) && ( eBufState == BS_EMPTY ) )
        {
            iAvBlocks = iNumBlocksMemory;
        }
    }

    return iAvBlocks * iBlockSize;
}

int CNetBuf::GetAvailData() const
{
    // in case of using sequence numbers, we always return data from the
    // buffer per definition
    int iAvBlocks = iNumBlocksMemory;

    if ( !bUseSequenceNumber )
    {
        // calculate available data in buffer
        iAvBlocks = iBlockPutPos - iBlockGetPos;

        // check for special case and wrap around
        if ( iAvBlocks < 0 )
        {
            iAvBlocks += iNumBlocksMemory; // wrap around
        }
        else
        {
            if ( ( iAvBlocks == 0 ) && ( eBufState == BS_FULL ) )
            {
                iAvBlocks = iNumBlocksMemory;
            }
        }
    }

    return iAvBlocks * iBlockSize;
}

/* Network buffer with statistic calculations implementation ******************/
CNetBufWithStats::CNetBufWithStats() :
    CNetBuf ( false ), // base class init: no simulation mode
    iMaxStatisticCount ( MAX_STATISTIC_COUNT ),
    bUseDoubleSystemFrameSize ( false ),
    dAutoFilt_WightUpNormal ( IIR_WEIGTH_UP_NORMAL ),
    dAutoFilt_WightDownNormal ( IIR_WEIGTH_DOWN_NORMAL ),
    dAutoFilt_WightUpFast ( IIR_WEIGTH_UP_FAST ),
    dAutoFilt_WightDownFast ( IIR_WEIGTH_DOWN_FAST ),
    dErrorRateBound ( ERROR_RATE_BOUND ),
    dUpMaxErrorBound ( UP_MAX_ERROR_BOUND )
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

void CNetBufWithStats::GetErrorRates ( CVector<double>& vecErrRates, double& dLimit, double& dMaxUpLimit )
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

void CNetBufWithStats::Init ( const int iNewBlockSize, const int iNewNumBlocks, const bool bNUseSequenceNumber, const bool bPreserve )
{
    // call base class Init
    CNetBuf::Init ( iNewBlockSize, iNewNumBlocks, bNUseSequenceNumber, bPreserve );

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
            SimulationBuffer[i].Init ( iNewBlockSize, viBufSizesForSim[i], bNUseSequenceNumber );

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

bool CNetBufWithStats::Put ( const CVector<uint8_t>& vecbyData, const int iInSize )
{
    // call base class Put
    const bool bPutOK = CNetBuf::Put ( vecbyData, iInSize );

    // update statistics calculations
    for ( int i = 0; i < NUM_STAT_SIMULATION_BUFFERS; i++ )
    {
        ErrorRateStatistic[i].Update ( !SimulationBuffer[i].Put ( vecbyData, iInSize ) );
    }

    return bPutOK;
}

bool CNetBufWithStats::Get ( CVector<uint8_t>& vecbyData, const int iOutSize )
{
    // call base class Get
    const bool bGetOK = CNetBuf::Get ( vecbyData, iOutSize );

    // update statistics calculations
    for ( int i = 0; i < NUM_STAT_SIMULATION_BUFFERS; i++ )
    {
        ErrorRateStatistic[i].Update ( !SimulationBuffer[i].Get ( vecbyData, iOutSize ) );
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
        if ( ( !bDecisionFound ) && ( ErrorRateStatistic[i].GetAverage() <= dErrorRateBound ) )
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
        if ( ( !bDecisionFound ) && ( ErrorRateStatistic[i].GetAverage() <= dUpMaxErrorBound ) )
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
        // this was just temporary so that we initiate a new initialization
        // phase to get quickly back to normal buffer sizes (hopefully).
        ResetInitCounter();
    }

    // Post calculation (filtering) --------------------------------------------
    // Define different weights for up and down direction. Up direction
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
    MathUtils().UpDownIIR1 ( dCurIIRFilterResult, static_cast<double> ( iCurDecision ), dWeightUp, dWeightDown );

    //### TEST: BEGIN ###//
    // TEST store important detection parameters in file for debugging
    /*
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
    //### TEST: END ###//

    // apply a hysteresis
    iCurAutoBufferSizeSetting = MathUtils().DecideWithHysteresis ( dCurIIRFilterResult, iCurDecidedResult, dHysteresisValue );

    // Initialization phase check and correction -------------------------------
    // sometimes in the very first period after a connection we get a bad error
    // rate result -> delete this from the initialization phase
    if ( iInitCounter == iMaxStatisticCount / 8 )
    {
        // check error rate of the largest buffer as the indicator
        if ( ErrorRateStatistic[NUM_STAT_SIMULATION_BUFFERS - 1].GetAverage() > dErrorRateBound )
        {
            for ( int i = 0; i < NUM_STAT_SIMULATION_BUFFERS; i++ )
            {
                ErrorRateStatistic[i].Reset();
            }
        }
    }
}
