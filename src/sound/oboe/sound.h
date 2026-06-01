/******************************************************************************\
 * Copyright (c) 2004-2026
 *
 * Author(s):
 *  Simon Tomlinson, Volker Fischer
 *
 * As of Jamulus 3.12.1dev (commit eb172d47): All new source code contributions must be licensed
 * under AGPL 3.0 or any later version.
 *
 * Existing code: Code contributed before 3.12.1dev (commit eb172d47) was licensed under GPL 2.0+.
 * This code will be licensed under GPL 3.0 (or any later version) from
 * 3.12.1dev (commit eb172d47).  When distributed as part of Jamulus, the AGPL 3.0 terms govern
 * the combined work, including network use provisions.
 *
 ******************************************************************************
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 * ---------------------------------------------------------------------------
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
\******************************************************************************/

#pragma once

#include <oboe/Oboe.h>
#include "../soundbase.h"
#include "../../global.h"
#include <QDebug>
#include <android/log.h>
#include "buffer.h"
#include <mutex>

/* Classes ********************************************************************/
class CSound : public CSoundBase, public oboe::AudioStreamCallback
{
    Q_OBJECT

public:
    static const uint8_t RING_FACTOR;
    CSound ( void ( *fpNewProcessCallback ) ( CVector<short>& psData, void* arg ), void* arg, const bool, const QString& );
    virtual ~CSound() {}

    virtual int  Init ( const int iNewPrefMonoBufferSize );
    virtual void Start();
    virtual void Stop();

    // Call backs for Oboe
    virtual oboe::DataCallbackResult onAudioReady ( oboe::AudioStream* oboeStream, void* audioData, int32_t numFrames );
    virtual void                     onErrorAfterClose ( oboe::AudioStream* oboeStream, oboe::Result result );
    virtual void                     onErrorBeforeClose ( oboe::AudioStream* oboeStream, oboe::Result result );

    struct Stats
    {
        Stats() { reset(); }
        void        reset();
        void        log() const;
        std::size_t frames_in;
        std::size_t frames_out;
        std::size_t frames_filled_out;
        std::size_t in_callback_calls;
        std::size_t out_callback_calls;
        std::size_t ring_overrun;
    };

protected:
    CVector<int16_t> vecsTmpInputAudioSndCrdStereo;
    CBuffer<int16_t> mOutBuffer;
    int              iOboeBufferSizeMono;
    int              iOboeBufferSizeStereo;

private:
    void setupCommonStreamParams ( oboe::AudioStreamBuilder* builder );
    void printStreamDetails ( oboe::ManagedStream& stream );
    void openStreams();
    void closeStreams();
    void warnIfNotLowLatency ( oboe::ManagedStream& stream, QString streamName );
    void closeStream ( oboe::ManagedStream& stream );

    oboe::DataCallbackResult onAudioInput ( oboe::AudioStream* oboeStream, void* audioData, int32_t numFrames );
    oboe::DataCallbackResult onAudioOutput ( oboe::AudioStream* oboeStream, void* audioData, int32_t numFrames );

    void addOutputData ( int channel_count );

    oboe::ManagedStream mRecordingStream;
    oboe::ManagedStream mPlayStream;

    // used to reach a state where the input buffer is
    // empty and the garbage in the first 500ms or so is discarded
    static constexpr int32_t kNumCallbacksToDrain   = 10;
    int32_t                  mCountCallbacksToDrain = kNumCallbacksToDrain;
    Stats                    mStats;
};
