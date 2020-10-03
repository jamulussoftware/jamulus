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


/* Classes ********************************************************************/
class CSound : public CSoundBase, public oboe::AudioStreamCallback//, public IRenderableAudio, public IRestartable
{
public:
    CSound ( void           (*fpNewProcessCallback) ( CVector<short>& psData, void* arg ),
             void*          arg,
             const int      iCtrlMIDIChannel,
             const bool     ,
             const QString& );
    virtual ~CSound() {}

    virtual int  Init ( const int iNewPrefMonoBufferSize );
    virtual void Start();
    virtual void Stop();

    // Call backs for Oboe
    virtual oboe::DataCallbackResult onAudioReady ( oboe::AudioStream* oboeStream, void* audioData, int32_t numFrames );
    virtual void onErrorAfterClose ( oboe::AudioStream* oboeStream, oboe::Result result );
    virtual void onErrorBeforeClose ( oboe::AudioStream* oboeStream, oboe::Result result );

    // these variables should be protected but cannot since we want
    // to access them from the callback function
    CVector<short> vecsTmpAudioSndCrdStereo;

    static void android_message_handler ( QtMsgType                 type,
                                          const QMessageLogContext& context,
                                          const QString&            message )
    {
        android_LogPriority priority = ANDROID_LOG_DEBUG;

        switch ( type )
        {
        case QtDebugMsg:    priority = ANDROID_LOG_DEBUG; break;
        case QtWarningMsg:  priority = ANDROID_LOG_WARN;  break;
        case QtCriticalMsg: priority = ANDROID_LOG_ERROR; break;
        case QtFatalMsg:    priority = ANDROID_LOG_FATAL; break;
        };

        __android_log_print ( priority, "Qt", "%s", qPrintable ( message ) );
    };

// TEST
CVector<short> vecsTmpAudioInSndCrd;
int            iModifiedInBufSize;

    int            iOpenSLBufferSizeMono;
    int            iOpenSLBufferSizeStereo;

private:
    void setupCommonStreamParams ( oboe::AudioStreamBuilder* builder );
    void printStreamDetails ( oboe::ManagedStream& stream );
    void openStreams();
    void closeStreams();
    void warnIfNotLowLatency ( oboe::ManagedStream& stream, QString streamName );
    void closeStream ( oboe::ManagedStream& stream );

    oboe::ManagedStream  mRecordingStream;
    oboe::ManagedStream  mPlayStream;
    AudioStreamCallback* mCallback;

    // used to reach a state where the input buffer is
    // empty and the garbage in the first 500ms or so is discarded
    static constexpr int32_t kNumCallbacksToDrain = 10;
    int32_t mCountCallbacksToDrain = kNumCallbacksToDrain;

    // Used to reference this instance of class from within the static callback
    CSound *pSound;

    QMutex Mutex;
};
