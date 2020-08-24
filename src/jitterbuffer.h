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

#pragma once

#include <QMutex>
#include <vector>

#include "global.h"
#include "util.h"

#define	JITTER_SEQNO_PRIME 49139 // special prime number
#define	JITTER_SEQNO_LOG2 16 // bits
#define	JITTER_SEQNO_MASK ((1U << JITTER_SEQNO_LOG2) - 1U)

class CSequenceNumber {
public:
    CSequenceNumber ();
    ~CSequenceNumber ();

    void InitToggle ();
    void PutToggle ( const bool );
    bool GetToggle ();
    float GetClockDrift () const { return ClockDrift; };
    float GetPacketDrops () const { return PacketDrops; };
    float GetPeerAdjust () const { return PeerAdjust; };
    float GetLocalAdjustStep () const { return LocalAdjustStep; };
    int GetLocalAdjust ();
    void SetLocalAdjust ( const float );
    bool IsAdjustActive () { return AnyRxSeqnoReceived; };

protected:
    void RangeCheckCounters ();

    uint32_t RxTotalLocalSeqno;
    uint32_t RxTotalPeerSeqno;
    uint32_t RxTotalToggles;

    uint32_t TxRemainder;
    uint32_t RxRemainder;
    uint32_t RxValidSeqno;
    uint32_t RxNextSeqno;
    uint32_t RxLastSeqno;

    // current remainder for adjustment
    // in the range -1 .. +1
    float LocalAdjustRemainder;

    // cached and read only version of some numbers
    float ClockDrift;
    float PacketDrops;
    float PeerAdjust;
    float LocalAdjustStep;

    bool AnyRxSeqnoReceived;
};

#define	JITTER_MAX_FRAME_COUNT 32 // preferably a power of two value

class CJitterBuffer : public CSequenceNumber {
public:
    CJitterBuffer ();
    ~CJitterBuffer ();

    void Init ( const size_t sMaxFrameSize , const int RxMaxInUse );
    bool PutFrame ( const uint8_t *begin, const int len );
    bool GetFrame ( uint8_t *begin, const int len );
    void SetMaxInUse ( const int RxMaxInUse );
    void GetStatistics ( float *pfStats );
    int GetAutoSetting ();

protected:
    CVector<uint8_t> vecvecbyData[JITTER_MAX_FRAME_COUNT];
    float vecfHistogram[JITTER_MAX_FRAME_COUNT];

    QMutex RxMutex;

    uint32_t RxCurInUse;
    uint32_t RxMaxInUse;
    uint32_t RxAutoInUse;
    uint32_t RxCounter;
    uint32_t RxSequence;
    uint32_t RxOffset;
    uint32_t TxOffset;
};
