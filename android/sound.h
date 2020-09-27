/******************************************************************************\
 * Copyright (c) 2004-2020
 *
 * Author(s):
 *  Simon Tomlinson, Volker Fischer
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

#include <oboe/Oboe.h>
#include <QMutex>
#include "soundbase.h"
#include "global.h"
#include <QDebug>
#include <android/log.h>
#include "libs/oboe/samples/LiveEffect/src/main/cpp/FullDuplexStream.h"


/* Classes ********************************************************************/
class FullDuplexPass : public FullDuplexStream {
public:
    virtual oboe::DataCallbackResult
    onBothStreamsReady ( const void* inputData,
                         int         numInputFrames,
                         void*       outputData,
                         int         numOutputFrames )
    {
        size_t bytesPerFrame = this->getOutputStream()->getBytesPerFrame();
        size_t bytesToWrite  = numInputFrames * bytesPerFrame;
        size_t byteDiff      = ( numOutputFrames - numInputFrames ) * bytesPerFrame;
        size_t bytesToZero   = ( byteDiff > 0 ) ? byteDiff : 0;

qInfo() << "bytesToWrite: " << bytesToWrite;

        memcpy ( outputData, inputData, bytesToWrite );
        memset ( (u_char*) outputData + bytesToWrite, 0, bytesToZero );

        return oboe::DataCallbackResult::Continue;
    }
};


class CSound : public CSoundBase, public oboe::AudioStreamCallback
{
public:
    CSound ( void           (*fpNewProcessCallback) ( CVector<short>& psData, void* arg ),
             void*          arg,
             const int      iCtrlMIDIChannel,
             const bool     ,
             const QString& );
    virtual ~CSound() { closeStreams(); }

    virtual int  Init ( const int iNewPrefMonoBufferSize );
    virtual void Start();
    virtual void Stop();

    // Call backs for Oboe
    virtual oboe::DataCallbackResult onAudioReady ( oboe::AudioStream* oboeStream, void* audioData, int32_t numFrames );
    virtual void onErrorAfterClose ( oboe::AudioStream* oboeStream, oboe::Result result ) { qDebug() << "CSound::onErrorAfterClose"; }
    virtual void onErrorBeforeClose ( oboe::AudioStream* oboeStream, oboe::Result result ) { qDebug() << "CSound::onErrorBeforeClose"; };

    // these variables should be protected but cannot since we want
    // to access them from the callback function
    CVector<short> vecsTmpAudioSndCrdStereo;
    int            iOpenSLBufferSizeMono;

private:
    void setupCommonStreamParams ( oboe::AudioStreamBuilder* builder );
    void printStreamDetails ( oboe::ManagedStream& stream );
    void openStreams();
    void closeStreams();
    void closeStream ( oboe::ManagedStream& stream );

    FullDuplexPass           mFullDuplexPass;
    oboe::AudioStreamBuilder inBuilder;
    oboe::AudioStreamBuilder outBuilder;
    oboe::ManagedStream      mRecordingStream;
    oboe::ManagedStream      mPlayStream;
    AudioStreamCallback*     mCallback;

    // used to reach a state where the input buffer is
    // empty and the garbage in the first 500ms or so is discarded
    static constexpr int32_t kNumCallbacksToDrain   = 10;
    int32_t                  mCountCallbacksToDrain = kNumCallbacksToDrain;
};
