/******************************************************************************\
 * Copyright (c) 2004-2023
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

/* Definitions ****************************************************************/
// number of simulation network jitter buffers for evaluating the statistic
// NOTE If you want to change this number, the code has to modified, too!
#define NUM_STAT_SIMULATION_BUFFERS 10

// hysteresis for buffer size decision to avoid fast changes if close to the bound
#define FILTER_DECISION_HYSTERESIS 0.1

// definition of the upper error bound of the jitter buffers
#define ERROR_RATE_BOUND_DOUBLE_FRAME_SIZE 0.0005
#define ERROR_RATE_BOUND                   ( ERROR_RATE_BOUND_DOUBLE_FRAME_SIZE / 2 )

// definition of the upper jitter buffer error bound, if that one is reached we
// have to speed up the filtering to quickly get out of a incorrect buffer
// size state
#define UP_MAX_ERROR_BOUND_DOUBLE_FRAME_SIZE 0.01
#define UP_MAX_ERROR_BOUND                   ( UP_MAX_ERROR_BOUND_DOUBLE_FRAME_SIZE / 2 )

// each regular buffer access lead to a count for put and get, assuming 2.66 ms
// blocks we have 15 s / 2.66 ms * 2 = approx. 11000
#define MAX_STATISTIC_COUNT_DOUBLE_FRAME_SIZE 11000

// each regular buffer access lead to a count for put and get, assuming 1.33 ms
// blocks we have 15 s / 1.33 ms * 2 = approx. 22500
#define MAX_STATISTIC_COUNT 22500

// Note that the following definitions of the weigh constants assume a block
// size of 128 samples at a sampling rate of 48 kHz.
#define IIR_WEIGTH_UP_NORMAL_DOUBLE_FRAME_SIZE   0.999995
#define IIR_WEIGTH_DOWN_NORMAL_DOUBLE_FRAME_SIZE 0.9999
#define IIR_WEIGTH_UP_FAST_DOUBLE_FRAME_SIZE     0.9995
#define IIR_WEIGTH_DOWN_FAST_DOUBLE_FRAME_SIZE   0.999

// convert numbers from 128 samples case using http://www.tsdconseil.fr/tutos/tuto-iir1-en.pdf
// and https://octave-online.net:
// gamma = exp(-Ts/tau), after some calculations we get: x=0.999995;exp(64/128*log(x))
#define IIR_WEIGTH_UP_NORMAL   0.9999975
#define IIR_WEIGTH_DOWN_NORMAL 0.99994999875
#define IIR_WEIGTH_UP_FAST     0.9997499687422
#define IIR_WEIGTH_DOWN_FAST   0.999499875

/* Classes ********************************************************************/
// Buffer base class -----------------------------------------------------------
template<class TData>
class CBuffer
{
public:
    CBuffer() {}

    void Init ( const int iNewMemSize )
    {
        // allocate memory for actual data buffer
        vecMemory.Init ( iNewMemSize );

        // init buffer pointers and buffer state (empty buffer)
        iGetPos   = 0;
        iPutPos   = 0;
        eBufState = BS_EMPTY;

        // store total memory size value
        iMemSize = iNewMemSize;
    }

    virtual bool Put ( const CVector<TData>& vecData, const int iInSize )
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
            std::copy ( vecData.begin(), vecData.begin() + iInSize, vecMemory.begin() + iPutPos );

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
            eBufState = BS_FULL;
        }
        else
        {
            eBufState = BS_OK;
        }

        return true; // no error check in base class, always return ok
    }

    virtual bool Get ( CVector<TData>& vecData, const int iOutSize )
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
            std::copy ( vecMemory.begin() + iGetPos, vecMemory.begin() + iGetPos + iOutSize, vecData.begin() );

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
            eBufState = BS_EMPTY;
        }
        else
        {
            eBufState = BS_OK;
        }

        return true; // no error check in base class, always return ok
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

    bool isFull() const { return eBufState == BS_FULL; }
    bool isEmpty() const { return eBufState == BS_EMPTY; }

protected:
    enum EBufState
    {
        BS_OK,
        BS_FULL,
        BS_EMPTY
    };

    CVector<TData> vecMemory;
    int            iMemSize;
    int            iGetPos;
    int            iPutPos;
    EBufState      eBufState;
};

// Network buffer (jitter buffer) ----------------------------------------------
class CNetBuf
{
public:
    CNetBuf ( const bool bNIsSim = false ) : iSequenceNumberAtGetPos ( 0 ), bIsSimulation ( bNIsSim ), bIsInitialized ( false ) {}

    void Init ( const int iNewBlockSize, const int iNewNumBlocks, const bool bNUseSequenceNumber, const bool bPreserve = false );

    void SetIsSimulation ( const bool bNIsSim ) { bIsSimulation = bNIsSim; }

    virtual bool Put ( const CVector<uint8_t>& vecbyData, int iInSize );
    virtual bool Get ( CVector<uint8_t>& vecbyData, const int iOutSize );

protected:
    enum EBufState
    {
        BS_OK,
        BS_FULL,
        BS_EMPTY
    };

    int  GetAvailSpace() const;
    int  GetAvailData() const;
    void Resize ( const int iNewNumBlocks, const int iNewBlockSize );

    CVector<CVector<uint8_t>> vecvecMemory;
    CVector<int>              veciBlockValid;
    int                       iNumBlocksMemory;
    int                       iBlockGetPos;
    int                       iBlockPutPos;
    int                       iBlockSize;
    uint8_t                   iSequenceNumberAtGetPos; // uint8_t so that it wraps automatically
    EBufState                 eBufState;
    bool                      bUseSequenceNumber;
    bool                      bIsSimulation;
    bool                      bIsInitialized;

    static constexpr int iNumBytesSeqNum = 1; // per definition 1 byte sequence counter
};

// Network buffer (jitter buffer) with statistic calculations ------------------
class CNetBufWithStats : public CNetBuf
{
public:
    CNetBufWithStats();

    void Init ( const int iNewBlockSize, const int iNewNumBlocks, const bool bNUseSequenceNumber, const bool bPreserve = false );

    void SetUseDoubleSystemFrameSize ( const bool bNDSFSize ) { bUseDoubleSystemFrameSize = bNDSFSize; }

    virtual bool Put ( const CVector<uint8_t>& vecbyData, const int iInSize );
    virtual bool Get ( CVector<uint8_t>& vecbyData, const int iOutSize );

    int  GetAutoSetting() { return iCurAutoBufferSizeSetting; }
    void GetErrorRates ( CVector<double>& vecErrRates, double& dLimit, double& dMaxUpLimit );

protected:
    void UpdateAutoSetting();
    void ResetInitCounter();

    // statistic (do not use the vector class since the classes do not have
    // appropriate copy constructor/operator)
    CErrorRate ErrorRateStatistic[NUM_STAT_SIMULATION_BUFFERS];
    CNetBuf    SimulationBuffer[NUM_STAT_SIMULATION_BUFFERS];
    int        viBufSizesForSim[NUM_STAT_SIMULATION_BUFFERS];

    double dCurIIRFilterResult;
    int    iCurDecidedResult;
    int    iInitCounter;
    int    iCurAutoBufferSizeSetting;
    int    iMaxStatisticCount;

    bool   bUseDoubleSystemFrameSize;
    double dAutoFilt_WightUpNormal;
    double dAutoFilt_WightDownNormal;
    double dAutoFilt_WightUpFast;
    double dAutoFilt_WightDownFast;
    double dErrorRateBound;
    double dUpMaxErrorBound;
};

// Conversion buffer (very simple buffer) --------------------------------------
// For this very simple buffer no wrap around mechanism is implemented. We
// assume here, that the applied buffers are an integer fraction of the total
// buffer size.
template<class TData>
class CConvBuf
{
public:
    CConvBuf() { Init ( 0 ); }

    void Init ( const int iNewMemSize, const bool bNUseSequenceNumber = false )
    {
        // allocate internal memory and reset read/write positions
        vecMemory.Init ( iNewMemSize );
        iMemSize           = iNewMemSize;
        iBufferSize        = iNewMemSize;
        bUseSequenceNumber = bNUseSequenceNumber;
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

    bool Put ( const CVector<TData>& vecData, const int iVecSize, const TData SequenceNumber = 0 )
    {
        // calculate the end position after copying
        int iEnd = iPutPos + iVecSize;

        // consider optional sequence number
        if ( bUseSequenceNumber )
        {
            iEnd++;
        }

        // first check for buffer overrun
        if ( iEnd <= iBufferSize )
        {
            // copy new data in internal buffer
            std::copy ( vecData.begin(), vecData.begin() + iVecSize, vecMemory.begin() + iPutPos );

            // add optional sequence number (NOTE that we currently
            // only support a single sequence number per packet)
            if ( bUseSequenceNumber )
            {
                // append the sequence number at the end
                vecMemory[iPutPos + iVecSize] = SequenceNumber;
            }

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

    void GetAll ( CVector<TData>& vecsData, const int iVecSize )
    {
        iPutPos = 0;

        // copy data from internal buffer in given buffer
        std::copy ( vecMemory.begin(), vecMemory.begin() + iVecSize, vecsData.begin() );
    }

    bool Get ( CVector<TData>& vecsData, const int iVecSize )
    {
        // calculate the input size and the end position after copying
        const int iEnd = iGetPos + iVecSize;

        // first check for buffer underrun
        if ( iEnd <= iBufferSize )
        {
            // copy new data from internal buffer
            std::copy ( vecMemory.begin() + iGetPos, vecMemory.begin() + iGetPos + iVecSize, vecsData.begin() );

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
    bool           bUseSequenceNumber;
    int            iPutPos, iGetPos;
};
