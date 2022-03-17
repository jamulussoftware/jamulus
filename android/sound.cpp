/******************************************************************************\
 * Copyright (c) 2004-2022
 *
 * Author(s):
 *  Simon Tomlinson, Volker Fischer, maintained by pgScorpio
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

#define RING_FACTOR 20

/* Implementation *************************************************************/

CSound* CSound::pSound = NULL;

CSound::CSound ( void ( *theProcessCallback ) ( CVector<short>& psData, void* arg ), void* theProcessCallbackArg ) :
    CSoundBase ( "Oboe", theProcessCallback, theProcessCallbackArg )
{
    setObjectName ( "CSoundThread" );

#ifdef ANDROIDDEBUG
    qInstallMessageHandler ( myMessageHandler );
#endif

    soundProperties.bHasAudioDeviceSelection   = false;
    soundProperties.bHasInputChannelSelection  = false;
    soundProperties.bHasOutputChannelSelection = false;
    soundProperties.bHasInputGainSelection     = false;

    pSound = this;
}

void CSound::setupCommonStreamParams ( oboe::AudioStreamBuilder* builder )
{
    // We request EXCLUSIVE mode since this will give us the lowest possible
    // latency. If EXCLUSIVE mode isn't available the builder will fall back to SHARED mode

    // Setting format to be PCM 16 bits (int16_t)
    builder->setFormat ( oboe::AudioFormat::I16 )
        ->setSharingMode ( oboe::SharingMode::Exclusive )
        ->setChannelCount ( oboe::ChannelCount::Stereo )
        ->setSampleRate ( SYSTEM_SAMPLE_RATE_HZ )
        ->setFramesPerCallback ( iDeviceBufferSize )
        ->setSampleRateConversionQuality ( oboe::SampleRateConversionQuality::Medium )
        ->setPerformanceMode ( oboe::PerformanceMode::LowLatency );
}

void CSound::closeStream ( oboe::ManagedStream& stream )
{
    if ( stream )
    {
        oboe::Result result;
        result = stream->requestStop();
        if ( oboe::Result::OK != result )
        {
            throw CGenErr ( tr ( "Error requesting stream stop: $s", oboe::convertToText ( result ) ) );
        }

        result = stream->close();
        if ( oboe::Result::OK != result )
        {
            throw CGenErr ( tr ( "Error closing stream: $s", oboe::convertToText ( result ) ) );
        }

        stream.reset();
    }
}

bool CSound::openStreams()
{
    // Setup output stream
    oboe::AudioStreamBuilder inBuilder, outBuilder;

    outBuilder.setDirection ( oboe::Direction::Output );
    outBuilder.setCallback ( this );
    setupCommonStreamParams ( &outBuilder );

    oboe::Result result = outBuilder.openManagedStream ( mPlayStream );

    if ( result != oboe::Result::OK )
    {
        return false;
    }
    mPlayStream->setBufferSizeInFrames ( ( iDeviceBufferSize * 2 ) );

    warnIfNotLowLatency ( mPlayStream, "PlayStream" );
    printStreamDetails ( mPlayStream );

    // Setup input stream
    inBuilder.setDirection ( oboe::Direction::Input );

    // Only set callback for the input direction
    // the output will be handled writing directly on the stream
    inBuilder.setCallback ( this );
    setupCommonStreamParams ( &inBuilder );

    result = inBuilder.openManagedStream ( mRecordingStream );

    if ( result != oboe::Result::OK )
    {
        closeStream ( mPlayStream );
        return false;
    }

    mRecordingStream->setBufferSizeInFrames ( ( iDeviceBufferSize * 2 ) );

    warnIfNotLowLatency ( mRecordingStream, "RecordStream" );
    printStreamDetails ( mRecordingStream );
    printStreamDetails ( mPlayStream );

    return true;
}

void CSound::closeStreams()
{
    // clean up
    closeStream ( mRecordingStream );
    closeStream ( mPlayStream );
}

void CSound::printStreamDetails ( oboe::ManagedStream& stream )
{
    QString sDirection              = ( stream->getDirection() == oboe::Direction::Input ? "Input" : "Output" );
    QString sFramesPerBurst         = QString::number ( stream->getFramesPerBurst() );
    QString sBufferSizeInFrames     = QString::number ( stream->getBufferSizeInFrames() );
    QString sBytesPerFrame          = QString::number ( stream->getBytesPerFrame() );
    QString sBytesPerSample         = QString::number ( stream->getBytesPerSample() );
    QString sBufferCapacityInFrames = QString::number ( stream->getBufferCapacityInFrames() );
    QString sPerformanceMode        = ( stream->getPerformanceMode() == oboe::PerformanceMode::LowLatency ? "LowLatency" : "NotLowLatency" );
    QString sSharingMode            = ( stream->getSharingMode() == oboe::SharingMode::Exclusive ? "Exclusive" : "Shared" );
    QString sDeviceID               = QString::number ( stream->getDeviceId() );
    QString sSampleRate             = QString::number ( stream->getSampleRate() );
    QString sAudioFormat            = ( stream->getFormat() == oboe::AudioFormat::I16 ? "I16" : "Float" );
    QString sFramesPerCallback      = QString::number ( stream->getFramesPerCallback() );
    qInfo() << "Stream details: [sDirection: " << sDirection << ", FramesPerBurst: " << sFramesPerBurst
            << ", BufferSizeInFrames: " << sBufferSizeInFrames << ", BytesPerFrame: " << sBytesPerFrame << ", BytesPerSample: " << sBytesPerSample
            << ", BufferCapacityInFrames: " << sBufferCapacityInFrames << ", PerformanceMode: " << sPerformanceMode
            << ", SharingMode: " << sSharingMode << ", DeviceID: " << sDeviceID << ", SampleRate: " << sSampleRate
            << ", AudioFormat: " << sAudioFormat << ", FramesPerCallback: " << sFramesPerCallback << "]";
}

void CSound::warnIfNotLowLatency ( oboe::ManagedStream& stream, QString streamName )
{
    if ( stream->getPerformanceMode() != oboe::PerformanceMode::LowLatency )
    {
        QString latencyMode = ( stream->getPerformanceMode() == oboe::PerformanceMode::None ? "None" : "Power Saving" );
        Q_UNUSED ( latencyMode );
    }

    Q_UNUSED ( streamName );
}

bool CSound::start()
{
    if ( !IsStarted() )
    {
        if ( openStreams() )
        {
            audioBuffer.Init ( iDeviceBufferSize * 2 );
            mOutBuffer.Init ( iDeviceBufferSize * 2 * RING_FACTOR );

            if ( mRecordingStream->requestStart() == oboe::Result::OK )
            {
                if ( mPlayStream->requestStart() == oboe::Result::OK )
                {
                    return true;
                }

                mRecordingStream->requestStop();
            }
        }

        closeStreams();
        return false;
    }

    return true;
}

bool CSound::stop()
{
    if ( IsStarted() )
    {
        closeStreams();
    }

    return true;
}

// This is the main callback method for when an audio stream is ready to publish data to an output stream
// or has received data on an input stream. As per manual much be very careful not to do anything in this back that
// can cause delays such as sleeping, file processing, allocate memory, etc.
oboe::DataCallbackResult CSound::onAudioReady ( oboe::AudioStream* oboeStream, void* audioData, int32_t numFrames )
{
    // only process if we are running
    if ( !IsStarted() )
    {
        return oboe::DataCallbackResult::Continue;
    }

    if ( mStats.in_callback_calls % 1000 == 0 )
    {
        mStats.log();
    }

    if ( oboeStream == mRecordingStream.get() && audioData )
    {
        return onAudioInput ( oboeStream, audioData, numFrames );
    }

    if ( oboeStream == mPlayStream.get() && audioData )
    {
        return onAudioOutput ( oboeStream, audioData, numFrames );
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
    //
    // According to the format that we've set on initialization, audioData
    // is an array of int16_t
    //
    int16_t* intData = static_cast<int16_t*> ( audioData );

    // Copy recording data to internal vector
    memcpy ( audioBuffer.data(), intData, sizeof ( int16_t ) * numFrames * oboeStream->getChannelCount() );

    if ( numFrames != (int32_t) iDeviceBufferSize )
    {
        qDebug() << "Received " << numFrames << " expecting " << iDeviceBufferSize;
    }

    mStats.frames_in += numFrames;

    // Tell parent class that we've put some data ready to send to the server
    processCallback ( audioBuffer );

    // The callback has placed in the vector the samples to play
    addOutputData ( oboeStream->getChannelCount() );

    return oboe::DataCallbackResult::Continue;
}

void CSound::addOutputData ( int channel_count )
{
    QMutexLocker locker ( &mutexAudioProcessCallback );

    // Only copy data if we have data to copy, otherwise fill with silence
    if ( audioBuffer.empty() )
    {
        // prime output stream buffer with silence
        audioBuffer.resize ( iDeviceBufferSize * channel_count, 0 );
    }

    mOutBuffer.Put ( audioBuffer, iDeviceBufferSize * channel_count );

    if ( mOutBuffer.isFull() )
    {
        mStats.ring_overrun++;
    }
}

oboe::DataCallbackResult CSound::onAudioOutput ( oboe::AudioStream* oboeStream, void* audioData, int32_t numFrames )
{
    mStats.frames_out += numFrames;
    mStats.out_callback_calls++;

    QMutexLocker locker ( &mutexAudioProcessCallback );

    std::size_t      to_write = (std::size_t) numFrames * oboeStream->getChannelCount();
    std::size_t      count    = std::min ( (std::size_t) mOutBuffer.GetAvailData(), to_write );
    CVector<int16_t> outBuffer ( count );

    mOutBuffer.Get ( outBuffer, count );

    //
    // According to the format that we've set on initialization, audioData
    // is an array of int16_t
    //

    memcpy ( audioData, outBuffer.data(), count * sizeof ( int16_t ) );

    if ( to_write > count )
    {
        mStats.frames_filled_out += ( to_write - count );
        memset ( ( (float*) audioData ) + count, 0, ( to_write - count ) * sizeof ( float ) );
    }

    return oboe::DataCallbackResult::Continue;
}

// TODO better handling of stream closing errors (use strErrorList ??)
void CSound::onErrorAfterClose ( oboe::AudioStream* oboeStream, oboe::Result result )
{
    qDebug() << "CSound::onErrorAfterClose";

    Q_UNUSED ( oboeStream );
    Q_UNUSED ( result );
}

// TODO better handling of stream closing errors (use strErrorList ??)
void CSound::onErrorBeforeClose ( oboe::AudioStream* oboeStream, oboe::Result result )
{
    qDebug() << "CSound::onErrorBeforeClose";

    Q_UNUSED ( oboeStream );
    Q_UNUSED ( result );
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
             << "frames_in: " << frames_in << ",frames_out: " << frames_out << ",frames_filled_out: " << frames_filled_out
             << ",in_callback_calls: " << in_callback_calls << ",out_callback_calls: " << out_callback_calls << ",ring_overrun: " << ring_overrun;
}

unsigned int CSound::getDeviceBufferSize ( unsigned int iDesiredBufferSize )
{
    unsigned int deviceBufferSize = iDesiredBufferSize;
    // Round up to a multiple of 2 !
    deviceBufferSize++;
    deviceBufferSize >>= 1;
    deviceBufferSize <<= 1;

    if ( deviceBufferSize < 64 )
    {
        deviceBufferSize = 64;
    }
    else if ( deviceBufferSize > 512 )
    {
        deviceBufferSize = 512;
    }

    return deviceBufferSize;
}

void CSound::closeCurrentDevice() // Closes the current driver and Clears Device Info
{
    // nothing to close on android
}

long CSound::createDeviceList ( bool bRescan )
{
    Q_UNUSED ( bRescan );

    strDeviceNames.clear();
    strDeviceNames.append ( SystemDriverTechniqueName() );

    // Just one device, so we force selection !
    iCurrentDevice = 0;

    return lNumDevices = 1;
}

bool CSound::setBaseValues()
{
    createDeviceList();

    clearDeviceInfo();

    // Set the input channel names:
    strInputChannelNames.append ( "input left" );
    strInputChannelNames.append ( "input right" );
    lNumInChan = 2;

    // Set added input channels: ( Always 0 for oboe )
    addAddedInputChannelNames();

    // Set the output channel names:
    strOutputChannelNames.append ( "output left" );
    strOutputChannelNames.append ( "output right" );
    lNumOutChan = 2;

    // Select input and output channels:
    resetChannelMapping();

    // Set the In/Out Latency:
    fInOutLatencyMs = 0.0;
    // TODO !!!

    return true;
}

bool CSound::checkCapabilities()
{
    // TODO ???
    //  For now anything is OK
    return true;
}

// TODO: ChannelMapping from inifile!
bool CSound::checkDeviceChange ( CSoundBase::tDeviceChangeCheck mode, int iDriverIndex ) // Open device sequence handling....
{
    // We have just one device !
    if ( iDriverIndex != 0 )
    {
        return false;
    }

    switch ( mode )
    {
    case CSoundBase::tDeviceChangeCheck::CheckOpen:
        Stop();
        strErrorList.clear();
        if ( !Start() )
        {
            return false;
        }
        return true;

    case CSoundBase::tDeviceChangeCheck::CheckCapabilities:
        return checkCapabilities();

    case CSoundBase::tDeviceChangeCheck::Activate:
        return setBaseValues();

    case CSoundBase::tDeviceChangeCheck::Abort:
        setBaseValues(); // Still set Base values, since there is no other device !
        return true;

    default:
        return false;
    }
}
