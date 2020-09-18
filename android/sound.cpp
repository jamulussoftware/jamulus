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

CSound::CSound ( void           (*fpNewProcessCallback) ( CVector<short>& psData, void* arg ),
                 void*          arg,
                 const int      iCtrlMIDIChannel,
                 const bool     ,
                 const QString& ) :
    CSoundBase ( "Oboe", fpNewProcessCallback, arg, iCtrlMIDIChannel )
{
#ifdef ANDROIDDEBUG
    qInstallMessageHandler ( myMessageHandler );
#endif

    // create callback
    mCallback = this;

    // setup input/output stream with basic stream parameters
    inBuilder.setDirection ( oboe::Direction::Input );
    setupCommonStreamParams ( &inBuilder );

    outBuilder.setDirection ( oboe::Direction::Output );
    setupCommonStreamParams ( &outBuilder );
}

void CSound::setupCommonStreamParams ( oboe::AudioStreamBuilder* builder )
{
    // We request EXCLUSIVE mode since this will give us the lowest possible
    // latency. If EXCLUSIVE mode isn't available the builder will fall back to SHARED mode.
    builder->setCallback ( this )
        ->setFormat                      ( oboe::AudioFormat::Float )
        ->setSharingMode                 ( oboe::SharingMode::Exclusive )
        ->setChannelCount                ( oboe::ChannelCount::Stereo )
        ->setSampleRate                  ( SYSTEM_SAMPLE_RATE_HZ )
        ->setSampleRateConversionQuality ( oboe::SampleRateConversionQuality::Medium )
        ->setPerformanceMode             ( oboe::PerformanceMode::LowLatency );

    return;
}

void CSound::closeStream ( oboe::ManagedStream& stream )
{
    if ( stream )
    {
        oboe::Result result = stream->close();

        if ( result != oboe::Result::OK )
        {
            throw CGenErr ( tr ( "Error closing stream: $s",
                                 oboe::convertToText ( result ) ) );
        }

        stream.reset();
    }
}

void CSound::closeStreams()
{
    // clean up
    closeStream ( mRecordingStream );
    closeStream ( mPlayStream );
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

void CSound::Start()
{
    // call base class
    CSoundBase::Start();

    // finally start the streams so the callback begins, start with inputstream first.
    mRecordingStream->requestStart();
    mPlayStream->requestStart();
}

void CSound::Stop()
{
    mPlayStream->requestStop();
    mRecordingStream->requestStop();

    // call base class
    CSoundBase::Stop();
}

int CSound::Init ( const int /* iNewPrefMonoBufferSize */ )
{
    // first close input/output streams
    closeStreams();


// TEST we need to open the streams to get the info about the BufferSizeInFrames
// -> TODO better solution
outBuilder.openManagedStream ( mPlayStream );
inBuilder.openManagedStream ( mRecordingStream );
const int iInFramesPerBurst  = mRecordingStream->getBufferSizeInFrames();
const int iOutFramesPerBurst = mPlayStream->getBufferSizeInFrames();
iOpenSLBufferSizeMono = std::max ( iInFramesPerBurst, iOutFramesPerBurst );
closeStreams();


    // we need to get the same number of samples for each call of the callback
    // (note that this must be done before the stream is opened)
    inBuilder.setFramesPerCallback ( iOpenSLBufferSizeMono );
    outBuilder.setFramesPerCallback ( iOpenSLBufferSizeMono );

    // open input/output streams
    inBuilder.openManagedStream ( mRecordingStream );
    outBuilder.openManagedStream ( mPlayStream );

    // apply the new frame size to the streams
    mRecordingStream->setBufferSizeInFrames ( iOpenSLBufferSizeMono );
    mPlayStream->setBufferSizeInFrames ( iOpenSLBufferSizeMono );


// TEST for debugging
qInfo() << "iOpenSLBufferSizeMono: " << iOpenSLBufferSizeMono;
printStreamDetails ( mRecordingStream );
printStreamDetails ( mPlayStream );


    // init base class
    CSoundBase::Init ( iOpenSLBufferSizeMono );

    // create memory for intermediate stereo audio buffer
    vecsTmpAudioSndCrdStereo.Init ( 2 * iOpenSLBufferSizeMono );

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

    // both, the input and output device use the same callback function
    QMutexLocker locker ( &MutexAudioProcessCallback );

if ( oboeStream == mRecordingStream.get() )
{
    qInfo() << "INPUT -------------";
}
else
{
    qInfo() << "OUTPUT ************";
}
qInfo() << "iOpenSLBufferSizeMono: " << iOpenSLBufferSizeMono << ", numFrames: " << numFrames;

    // perform the processing for input
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
                vecsTmpAudioSndCrdStereo[frmNum * oboeStream->getChannelCount() + channelNum] =
                    (short) floatData[frmNum * oboeStream->getChannelCount() + channelNum] * _MAXSHORT;
            }
        }

        // Tell parent class that we've put some data ready to send to the server
        ProcessCallback ( vecsTmpAudioSndCrdStereo  );
    }

    // perform the processing for output
    if ( oboeStream == mPlayStream.get() && audioData )
    {
        float* floatData = static_cast<float*> ( audioData );

//        // Zero out the incoming container array
//        memset ( audioData, 0, sizeof(float) * numFrames * oboeStream->getChannelCount() );

        // Only copy data if we have data to copy, otherwise fill with silence
        for ( int frmNum = 0; frmNum < numFrames; ++frmNum )
        {
            for ( int channelNum = 0; channelNum < oboeStream->getChannelCount(); channelNum++ )
            {
                // copy sample received from server into output buffer

                // convert to 32 bit
                const int32_t iCurSam = static_cast<int32_t> (
                    vecsTmpAudioSndCrdStereo[frmNum * oboeStream->getChannelCount() + channelNum] );

                floatData[frmNum * oboeStream->getChannelCount() + channelNum] = (float) iCurSam / _MAXSHORT;
            }
        }
    }

    return oboe::DataCallbackResult::Continue;
}
