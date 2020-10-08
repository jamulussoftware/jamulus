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

#include "sound.h"
#include "androiddebug.cpp"

/* Implementation *************************************************************/

CSound::CSound ( void           (*fpNewProcessCallback) ( CVector<float>& pfData, void* arg ),
                 void*          arg,
                 const int      iCtrlMIDIChannel,
                 const bool     ,
                 const QString& ) :
    CSoundBase ( "Oboe", fpNewProcessCallback, arg, iCtrlMIDIChannel )
{
#ifdef ANDROIDDEBUG
    qInstallMessageHandler(myMessageHandler);
#endif
}

void CSound::setupCommonStreamParams(oboe::AudioStreamBuilder *builder)
{
    // We request EXCLUSIVE mode since this will give us the lowest possible
    // latency. If EXCLUSIVE mode isn't available the builder will fall back to SHARED mode
    builder
            ->setFormat(oboe::AudioFormat::Float)
            ->setSharingMode(oboe::SharingMode::Exclusive)
            ->setChannelCount(oboe::ChannelCount::Stereo)
            ->setSampleRate(SYSTEM_SAMPLE_RATE_HZ)
            ->setFramesPerCallback(iOpenSLBufferSizeMono)
            ->setSampleRateConversionQuality(oboe::SampleRateConversionQuality::Medium)
            ->setPerformanceMode(oboe::PerformanceMode::LowLatency);

    return;
}

void CSound::closeStream ( oboe::ManagedStream& stream )
{
    if ( stream )
    {
        oboe::Result requestStopRes = stream->requestStop();
        oboe::Result result         = stream->close();

        if ( result != oboe::Result::OK )
        {
            throw CGenErr ( tr ( "Error closing stream: $s",
                                 oboe::convertToText ( result ) ) );
        }

        stream.reset();
    }
}


void CSound::openStreams()
{
    // Create callback
    mCallback = this;

    // Setup output stream
    oboe::AudioStreamBuilder inBuilder, outBuilder;

    outBuilder.setDirection ( oboe::Direction::Output );
    setupCommonStreamParams ( &outBuilder );

    oboe::Result result = outBuilder.openManagedStream ( mPlayStream );

    if ( result != oboe::Result::OK )
    {
        return;
    }
    mPlayStream->setBufferSizeInFrames ( iOpenSLBufferSizeStereo );

    warnIfNotLowLatency ( mPlayStream, "PlayStream" );
    printStreamDetails ( mPlayStream );

    // Setup input stream
    inBuilder.setDirection ( oboe::Direction::Input );

    // Only set callback for the input direction
    // the output will be handled writing directly on the stream
    inBuilder.setCallback(this);
    setupCommonStreamParams ( &inBuilder );

    result = inBuilder.openManagedStream ( mRecordingStream );

    if ( result != oboe::Result::OK )
    {
        closeStream ( mPlayStream );
        return;
    }

    mRecordingStream->setBufferSizeInFrames ( iOpenSLBufferSizeStereo );

    warnIfNotLowLatency ( mRecordingStream, "RecordStream" );
    printStreamDetails ( mRecordingStream );
}

void CSound::printStreamDetails ( oboe::ManagedStream& stream )
{
    QString sDirection              = ( stream->getDirection()==oboe::Direction::Input ? "Input" : "Output" );
    QString sFramesPerBurst         = QString::number ( stream->getFramesPerBurst() );
    QString sBufferSizeInFrames     = QString::number ( stream->getBufferSizeInFrames() );
    QString sBytesPerFrame          = QString::number ( stream->getBytesPerFrame() );
    QString sBytesPerSample         = QString::number ( stream->getBytesPerSample() );
    QString sBufferCapacityInFrames = QString::number ( stream->getBufferCapacityInFrames() );
    QString sPerformanceMode        = ( stream->getPerformanceMode() == oboe::PerformanceMode::LowLatency ? "LowLatency" : "NotLowLatency" );
    QString sSharingMode            = ( stream->getSharingMode() == oboe::SharingMode::Exclusive ? "Exclusive" : "Shared" );
    QString sDeviceID               = QString::number ( stream->getDeviceId() );
    QString sSampleRate             = QString::number ( stream->getSampleRate() );
    QString sAudioFormat            = ( stream->getFormat()==oboe::AudioFormat::I16 ? "I16" : "Float" );
    QString sFramesPerCallback      = QString::number ( stream->getFramesPerCallback() );
    qInfo() << "Stream details: [sDirection: " << sDirection <<
               ", FramesPerBurst: "            << sFramesPerBurst <<
               ", BufferSizeInFrames: "        << sBufferSizeInFrames <<
               ", BytesPerFrame: "             << sBytesPerFrame <<
               ", BytesPerSample: "            << sBytesPerSample <<
               ", BufferCapacityInFrames: "    << sBufferCapacityInFrames <<
               ", PerformanceMode: "           << sPerformanceMode <<
               ", SharingMode: "               << sSharingMode <<
               ", DeviceID: "                  << sDeviceID <<
               ", SampleRate: "                << sSampleRate <<
               ", AudioFormat: "               << sAudioFormat <<
               ", FramesPerCallback: "         << sFramesPerCallback << "]";
}

void CSound::warnIfNotLowLatency ( oboe::ManagedStream& stream, QString streamName )
{
    if ( stream->getPerformanceMode() != oboe::PerformanceMode::LowLatency )
    {
        QString latencyMode = ( stream->getPerformanceMode() == oboe::PerformanceMode::None ? "None" : "Power Saving" );
    }
}

void CSound::closeStreams()
{
    // clean up
    closeStream ( mRecordingStream );
    closeStream ( mPlayStream );
}

void CSound::Start()
{
    openStreams();

    // call base class
    CSoundBase::Start();

    // finally start the streams so the callback begins, start with inputstream first.
    mRecordingStream->requestStart();
    mPlayStream->requestStart();
}

void CSound::Stop()
{
    closeStreams();

    // call base class
    CSoundBase::Stop();
}

int CSound::Init ( const int iNewPrefMonoBufferSize )
{
    // store buffer size
    iOpenSLBufferSizeMono = iNewPrefMonoBufferSize; // 512

    // init base class
    CSoundBase::Init ( iOpenSLBufferSizeMono );

    // set internal buffer size value and calculate stereo buffer size
    iOpenSLBufferSizeStereo = 2 * iOpenSLBufferSizeMono;

    // create memory for intermediate audio buffer
    vecsTmpInputAudioSndCrdStereo.Init ( iOpenSLBufferSizeStereo );
    vecsTmpOutAudioSndCrdStereo.Init ( iOpenSLBufferSizeStereo );

    return iOpenSLBufferSizeMono;
}

// This is the main callback method for when an audio stream is ready to publish data to an output stream
// or has received data on an input stream. As per manual much be very careful not to do anything in this back that
// can cause delays such as sleeping, file processing, allocate memory, etc.
oboe::DataCallbackResult CSound::onAudioReady ( oboe::AudioStream* oboeStream,
                                                void*              audioData,
                                                int32_t            numFrames )
{
    // only process if we are running
    if ( ! bRun )
    {
        return oboe::DataCallbackResult::Continue;
    }

    if ( oboeStream == mRecordingStream.get() && audioData )
    {
        // First things first, we need to discard the input queue a little for 500ms or so
        if ( mCountCallbacksToDrain > 0 )
        {
            // discard the input buffer
            int32_t numBytes = numFrames * oboeStream->getBytesPerFrame();
            memset ( audioData, 0 /* value */, numBytes );
            mCountCallbacksToDrain--;
        }

        // We're good to start recording now
        // Take the data from the recording device output buffer and move
        // it to the vector ready to send up to the server
        float* floatData = static_cast<float*> ( audioData );

        // Copy recording data to internal vector
        for ( int frmNum = 0; frmNum < numFrames; ++frmNum )
        {
            for ( int channelNum = 0; channelNum < oboeStream->getChannelCount(); channelNum++ )
            {
                vecsTmpInputAudioSndCrdStereo[frmNum * oboeStream->getChannelCount() + channelNum] =
                    static_cast<int16_t>(floatData[frmNum * oboeStream->getChannelCount() + channelNum] * _MAXSHORT);
            }
        }

        // Tell parent class that we've put some data ready to send to the server
        ProcessCallback ( vecsTmpInputAudioSndCrdStereo  );

        // The callback has placed in the vector the samples to play

        // Only copy data if we have data to copy, otherwise fill with silence
        if ( !vecsTmpInputAudioSndCrdStereo.empty() )
        {
            for ( int frmNum = 0; frmNum < iOpenSLBufferSizeMono; ++frmNum )
            {
                for ( int channelNum = 0; channelNum < oboeStream->getChannelCount(); channelNum++ )
                {
                    // copy sample received from server into output buffer

                    // convert to 32 bit
                    const int32_t iCurSam = static_cast<int32_t> (
                        vecsTmpInputAudioSndCrdStereo[frmNum * oboeStream->getChannelCount() + channelNum] );

                    vecsTmpOutAudioSndCrdStereo[frmNum * oboeStream->getChannelCount() + channelNum] = ((float) iCurSam) / _MAXSHORT;
                }
            }
        }
        else
        {
            // prime output stream buffer with silence
            vecsTmpOutAudioSndCrdStereo.Reset(0);
        }

        // Write without blocking.
        oboe::ResultWithValue<int32_t> r = mPlayStream->write(vecsTmpOutAudioSndCrdStereo.data(),iOpenSLBufferSizeMono,0);
        if(r)
        {
            // Note: this could result in not writing the requested number of frames.
            // We could buffer them and wait for the next occasion, but for now we just discard them.
            if(r.value() < iOpenSLBufferSizeMono)
            {
                qDebug() << "Could only write " << r.value() << " frames of the " << iOpenSLBufferSizeMono << " requested.";
            }
        }
        else
        {
            qDebug() << "Could not write to output stream: " << oboe::convertToText(r.error());
        }
    }

    return oboe::DataCallbackResult::Continue;
}

// TODO better handling of stream closing errors
void CSound::onErrorAfterClose ( oboe::AudioStream* oboeStream, oboe::Result result )
{
    qDebug() << "CSound::onErrorAfterClose";
}

// TODO better handling of stream closing errors
void CSound::onErrorBeforeClose ( oboe::AudioStream* oboeStream, oboe::Result result )
{
    qDebug() << "CSound::onErrorBeforeClose";
}
