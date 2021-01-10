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

const uint8_t CSound::RING_FACTOR = 20;

CSound::CSound ( void           (*fpNewProcessCallback) ( CVector<short>& psData, void* arg ),
                 void*          arg,
                 const QString& strMIDISetup,
                 const bool     ,
                 const QString& ) :
    CSoundBase ( "Oboe", fpNewProcessCallback, arg, strMIDISetup )
{
#ifdef ANDROIDDEBUG
    qInstallMessageHandler(myMessageHandler);
#endif
}

void CSound::setupCommonStreamParams ( oboe::AudioStreamBuilder* builder )
{
    // We request EXCLUSIVE mode since this will give us the lowest possible
    // latency. If EXCLUSIVE mode isn't available the builder will fall back to SHARED mode
    builder
            ->setFormat(oboe::AudioFormat::Float)
            ->setSharingMode(oboe::SharingMode::Exclusive)
            ->setChannelCount(oboe::ChannelCount::Stereo)
            ->setSampleRate(SYSTEM_SAMPLE_RATE_HZ)
            ->setFramesPerCallback(iOboeBufferSizeMono)
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
    // Setup output stream
    oboe::AudioStreamBuilder inBuilder, outBuilder;

    outBuilder.setDirection ( oboe::Direction::Output );
    outBuilder.setCallback(this);
    setupCommonStreamParams ( &outBuilder );

    oboe::Result result = outBuilder.openManagedStream ( mPlayStream );

    if ( result != oboe::Result::OK )
    {
        return;
    }
    mPlayStream->setBufferSizeInFrames ( iOboeBufferSizeStereo );

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

    mRecordingStream->setBufferSizeInFrames ( iOboeBufferSizeStereo );

    warnIfNotLowLatency ( mRecordingStream, "RecordStream" );
    printStreamDetails ( mRecordingStream );
    printStreamDetails ( mPlayStream );
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

void CSound::warnIfNotLowLatency ( oboe::ManagedStream& stream,
                                   QString              streamName )
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
    iOboeBufferSizeMono = iNewPrefMonoBufferSize; // 512

    // init base class
    CSoundBase::Init ( iOboeBufferSizeMono );

    // set internal buffer size value and calculate stereo buffer size
    iOboeBufferSizeStereo = 2 * iOboeBufferSizeMono;

    // create memory for intermediate audio buffer
    vecsTmpInputAudioSndCrdStereo.Init ( iOboeBufferSizeStereo );
    mOutBuffer.reset ( iOboeBufferSizeStereo * RING_FACTOR );

    return iOboeBufferSizeMono;
}

// This is the main callback method for when an audio stream is ready to publish data to an output stream
// or has received data on an input stream. As per manual much be very careful not to do anything in this back that
// can cause delays such as sleeping, file processing, allocate memory, etc.
oboe::DataCallbackResult CSound::onAudioReady ( oboe::AudioStream* oboeStream,
                                                void*              audioData,
                                                int32_t            numFrames )
{
    // only process if we are running
    if ( !bRun )
    {
        return oboe::DataCallbackResult::Continue;
    }

    if ( mStats.in_callback_calls % 1000 == 0 )
    {
        mStats.log();
    }

    if ( oboeStream == mRecordingStream.get() && audioData )
    {
        return onAudioInput(oboeStream, audioData, numFrames);
    }

    if ( oboeStream == mPlayStream.get() && audioData)
    {
        return onAudioOutput(oboeStream, audioData, numFrames);
    }

    return oboe::DataCallbackResult::Continue;
}

oboe::DataCallbackResult CSound::onAudioInput ( oboe::AudioStream* oboeStream, void* audioData, int32_t numFrames )
{
    mStats.in_callback_calls++;

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

    if ( numFrames != iOboeBufferSizeMono )
    {
        qDebug() << "Received " << numFrames << " expecting " << iOboeBufferSizeMono;
    }

    mStats.frames_in += numFrames;

    // Tell parent class that we've put some data ready to send to the server
    ProcessCallback ( vecsTmpInputAudioSndCrdStereo  );

    // The callback has placed in the vector the samples to play
    addOutputData(oboeStream->getChannelCount());

    return oboe::DataCallbackResult::Continue;
}

void CSound::addOutputData(int channel_count)
{
    QMutexLocker locker ( &MutexAudioProcessCallback );

    // Only copy data if we have data to copy, otherwise fill with silence
    if ( !vecsTmpInputAudioSndCrdStereo.empty() )
    {
        for ( int frmNum = 0; frmNum < iOboeBufferSizeMono; ++frmNum )
        {
            for ( int channelNum = 0; channelNum < channel_count; channelNum++ )
            {
                // copy sample received from server into output buffer

                // convert to 32 bit
                const int32_t iCurSam = static_cast<int32_t> (
                    vecsTmpInputAudioSndCrdStereo[frmNum * channel_count+ channelNum] );

                mOutBuffer.put ( ( static_cast<float> ( iCurSam ) ) / _MAXSHORT );
            }
        }
    }
    else
    {
        // prime output stream buffer with silence
        for ( int frmNum = 0; frmNum < iOboeBufferSizeMono; ++frmNum )
        {
            for ( int channelNum = 0; channelNum < channel_count; channelNum++ )
            {
                mOutBuffer.put ( 0 );
            }
        }
    }

    if ( mOutBuffer.isFull() )
    {
        mStats.ring_overrun++;
    }
}

oboe::DataCallbackResult CSound::onAudioOutput ( oboe::AudioStream* oboeStream,
                                                 void*              audioData,
                                                 int32_t            numFrames )
{
    mStats.frames_out += numFrames;
    mStats.out_callback_calls++;

    QMutexLocker locker ( &MutexAudioProcessCallback );

    std::size_t to_write = numFrames*oboeStream->getChannelCount();
    std::size_t count    = std::min ( mOutBuffer.size(), to_write );

    mOutBuffer.get ( (float*) audioData, count );

    if ( to_write > count )
    {
        mStats.frames_filled_out += ( to_write - count );
        memset ( ( (float*) audioData ) + count, 0, ( to_write - count ) * sizeof ( float ) );
    }

    return oboe::DataCallbackResult::Continue;
}

// TODO better handling of stream closing errors
void CSound::onErrorAfterClose ( oboe::AudioStream* oboeStream,
                                 oboe::Result       result )
{
    qDebug() << "CSound::onErrorAfterClose";
}

// TODO better handling of stream closing errors
void CSound::onErrorBeforeClose ( oboe::AudioStream* oboeStream,
                                  oboe::Result       result )
{
    qDebug() << "CSound::onErrorBeforeClose";
}

void CSound::Stats::reset() 
{
    frames_in          = 0;
    frames_out         = 0;
    frames_filled_out  = 0;
    in_callback_calls  = 0;
    out_callback_calls = 0;
    ring_overrun       = 0;
}

void CSound::Stats::log() const 
{
    qDebug() << "Stats: "
             << "frames_in: " << frames_in
             << ",frames_out: " << frames_out
             << ",frames_filled_out: " << frames_filled_out
             << ",in_callback_calls: " << in_callback_calls
             << ",out_callback_calls: " << out_callback_calls
             << ",ring_overrun: " << ring_overrun;
}
