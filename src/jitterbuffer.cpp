/******************************************************************************\
 * Copyright (c) 2020
 *
 * Author(s):
 *  Hans Petter Selasky
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

#include <QCoreApplication>

#include "jitterbuffer.h"

static uint32_t JitterSeqno[1U << JITTER_SEQNO_LOG2];

static void __attribute__((__constructor__))
JitterSeqnoInit ()
{
    uint32_t seq;
    uint32_t r = 1;
    uint32_t value = 0;

    for ( seq = 0; seq != JITTER_SEQNO_LOG2; seq++ )
    {
        value *= 2;
        if ( r & 1 )
        {
            r += JITTER_SEQNO_PRIME;
            value |= 1;
        }
        r /= 2;
    }

    for ( seq = 0; seq != (JITTER_SEQNO_PRIME - 1); seq++ )
    {
        JitterSeqno[value & JITTER_SEQNO_MASK] = seq;

        value *= 2;
        if ( r & 1 )
        {
            r += JITTER_SEQNO_PRIME;
            value |= 1;
        }
        r /= 2;
    }
}

CSequenceNumber :: CSequenceNumber ()
{
    TxRemainder = 1;

    InitToggle();
}

void
CSequenceNumber :: InitToggle ()
{
    RxRemainder = 0;
    RxLastSeqno = -1U;
    RxValidSeqno = 0;
    RxNextSeqno = 0;
    RxTotalLocalSeqno = 0;
    RxTotalPeerSeqno = 0;
    RxTotalToggles = 0;
    ClockDrift = 0.0f;
    PacketDrops = 0.0f;
    PeerAdjust = 0.0f;
    LocalAdjustRemainder = 0.0f;
    LocalAdjustStep = 0.0f;
    AnyRxSeqnoReceived = false;
}

CSequenceNumber :: ~CSequenceNumber ()
{
}

void
CSequenceNumber :: RangeCheckCounters ()
{
    static constexpr uint32_t limit = 100000;

    if ( RxTotalLocalSeqno >= limit ||
         RxTotalPeerSeqno >= limit ||
         RxTotalToggles >= limit )
    {
        RxTotalLocalSeqno /= 2;
        RxTotalPeerSeqno /= 2;
        RxTotalToggles /= 2;
    }
}

void
CSequenceNumber :: PutToggle (const bool toggle)
{
    RxRemainder *= 2;
    RxRemainder |= toggle;

    const uint32_t seq = JitterSeqno[RxRemainder & JITTER_SEQNO_MASK];

    if ( seq == RxNextSeqno )
    {
        // need to wait for a couple of numbers
        // in sequence before we can trust the
        // sequence number
        if ( RxValidSeqno < JITTER_SEQNO_LOG2 )
        {
            RxValidSeqno++;
        }
        else
        {
            if (RxLastSeqno != -1U)
            {
                // check for sequence number wraparound
                if (seq < RxLastSeqno)
                {
                    RxTotalPeerSeqno += seq + JITTER_SEQNO_PRIME - 1 - RxLastSeqno;
                }
                else
                {
                    RxTotalPeerSeqno += seq - RxLastSeqno;
                }
            }
            else
            {
                // first time init
                RxTotalPeerSeqno = RxTotalLocalSeqno;
                RxTotalToggles = RxTotalLocalSeqno;
            }

            // update last sequence number
            RxLastSeqno = seq;
            // don't come here too often
            RxValidSeqno = 1;
            // record we received a valid sequence number
            AnyRxSeqnoReceived = true;

            // compute some statistics
            ClockDrift = (int32_t) (RxTotalLocalSeqno - RxTotalPeerSeqno) /
                (float) RxTotalLocalSeqno;
            PacketDrops = (int32_t) (RxTotalPeerSeqno - RxTotalToggles) /
                (float) RxTotalToggles;
            PeerAdjust = (int32_t) (RxTotalLocalSeqno - RxTotalToggles) /
                (float) RxTotalPeerSeqno;
        }
    }
    else
    {
        // out of sequence number detected
        // reset valid counter
        RxValidSeqno = 0;
    }

    // compute next sequence number
    RxNextSeqno = ( seq + 1 ) % ( JITTER_SEQNO_PRIME - 1 );
    // count number of calls
    RxTotalToggles ++;
    // range check the counters
    RangeCheckCounters();
}

// this function produce the toggle value for
// the transmitted stream
bool
CSequenceNumber :: GetToggle ()
{
    bool retval = ( TxRemainder & 1 );

    if ( retval )
        TxRemainder += JITTER_SEQNO_PRIME;
    TxRemainder /= 2;

    return (retval);
}

int
CSequenceNumber :: GetLocalAdjust ()
{
    LocalAdjustRemainder += LocalAdjustStep;

    if (LocalAdjustRemainder <= -1.0f)
    {
        // send less
        LocalAdjustRemainder += 1.0f;
        return (-1);
    }
    else if (LocalAdjustRemainder >= 1.0f)
    {
        // send more
        LocalAdjustRemainder -= 1.0f;
        return (1);
    }
    else
    {
        return (0);
    }
}

void
CSequenceNumber :: SetLocalAdjust (const float fValue)
{
    // limit the error rate so that we don't drop or add
    // packets more often than a valid sequence number can
    // be transmitted
    LocalAdjustStep = qBound(-1.0f / ( 3 * JITTER_SEQNO_LOG2 ),
        fValue, 1.0f / (3 * JITTER_SEQNO_LOG2) );
}

CJitterBuffer :: CJitterBuffer ()
{
    RxCurInUse = 0;
    RxMaxInUse = 0;
    RxAutoInUse = 0;
    RxCounter = 0;
    RxSequence = 0;
    RxOffset = 0;
    TxOffset = 0;
}

CJitterBuffer :: ~CJitterBuffer ()
{
}

void
CJitterBuffer :: SetMaxInUse ( const int _RxMaxInUse )
{
    QMutexLocker locker ( &RxMutex );
    RxMaxInUse = qBound ( 0, _RxMaxInUse, JITTER_MAX_FRAME_COUNT );
}

void
CJitterBuffer :: Init ( const size_t sMaxFrameSize,
                        const int _RxMaxInUse )
{
    QMutexLocker locker ( &RxMutex );

    for ( uint8_t x = 0; x != JITTER_MAX_FRAME_COUNT; x++ )
    {
        vecvecbyData[x].Init ( sMaxFrameSize, 0 );
    }

    memset ( vecfHistogram, 0, sizeof ( vecfHistogram ) );

    RxCurInUse = 0;
    RxMaxInUse = qBound ( 0, _RxMaxInUse, JITTER_MAX_FRAME_COUNT );
    RxAutoInUse = JITTER_MAX_FRAME_COUNT;
    RxCounter = 0;
    RxSequence = 0;
    RxOffset = 0;
    TxOffset = 0;

    InitToggle ();
}

bool
CJitterBuffer :: PutFrame ( const uint8_t *begin, const int len )
{
    QMutexLocker locker ( &RxMutex );
    bool toggle = begin[0] & 1;
    bool retval = false;

    // check if there is enough space in buffer
    if ( RxCurInUse < ( RxMaxInUse ? RxMaxInUse : RxAutoInUse ) &&
         len == vecvecbyData[0].Size() )
    {
        CVector<uint8_t> &vecbyDst = vecvecbyData[RxOffset];

        RxOffset++;
        RxOffset %= JITTER_MAX_FRAME_COUNT;

        RxCurInUse++;

	memcpy ( vecbyDst.data(), begin, len );

        retval = true;
    }

    PutToggle(toggle);

    const uint32_t index =
        ( JITTER_MAX_FRAME_COUNT +
          RxCounter -
          RxSequence ) % JITTER_MAX_FRAME_COUNT;

    RxCounter++;
    RxCounter %= JITTER_MAX_FRAME_COUNT;

    // do the statistics
    vecfHistogram[index] += 1.0f;

    // check if value reached the maximum
    if (vecfHistogram[index] >= JITTER_MAX_FRAME_COUNT)
    {
        uint8_t x, xmax;

        for (x = 0; x != JITTER_MAX_FRAME_COUNT; x++)
        {
            vecfHistogram[x] /= 2.0f;
        }

        // find width of peak
        for (x = xmax = 1; x != (JITTER_MAX_FRAME_COUNT - 1) / 2; x++)
        {
            if (vecfHistogram[(index + x) % JITTER_MAX_FRAME_COUNT] >= 0.5f ||
                vecfHistogram[(JITTER_MAX_FRAME_COUNT +
                    index - x) % JITTER_MAX_FRAME_COUNT] >= 0.5f)
            {
                xmax = x + 1;
            }
        }

        // update current jitter setting
        RxAutoInUse = 2 * xmax;
    }
    return retval;
}

bool
CJitterBuffer :: GetFrame ( uint8_t *begin, const int len )
{
    QMutexLocker locker ( &RxMutex );

    // update RX sequence number (s)
    RxSequence++;
    RxSequence %= JITTER_MAX_FRAME_COUNT;

    // update local RX sequence number w/o wrapping
    RxTotalLocalSeqno ++;

    // range check the counters
    RangeCheckCounters();

    // check if anything can be received
    if ( RxCurInUse != 0 &&
         len == vecvecbyData[0].Size() )
    {
        const CVector<uint8_t> &vecbySrc = vecvecbyData[TxOffset];

        TxOffset++;
        TxOffset %= JITTER_MAX_FRAME_COUNT;

        RxCurInUse--;

        memcpy ( begin, vecbySrc.data(), len );

        return true;
    }
    else
    {
        return false;
    }
}

void
CJitterBuffer :: GetStatistics ( float *pfStats )
{
    memcpy ( pfStats, vecfHistogram, sizeof ( vecfHistogram ) );
}

int
CJitterBuffer :: GetAutoSetting ()
{
    QMutexLocker locker ( &RxMutex );

    return qBound ( MIN_NET_BUF_SIZE_NUM_BL, (int) RxAutoInUse, MAX_NET_BUF_SIZE_NUM_BL );
}
