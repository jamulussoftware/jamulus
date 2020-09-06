/******************************************************************************\
 * Copyright (c) 2004-2020
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
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
\******************************************************************************/

#include "sound.h"


/* Implementation *************************************************************/
CSound::CSound ( void           (*fpNewProcessCallback) ( CVector<short>& psData, void* arg ),
                 void*          arg,
                 const int      iCtrlMIDIChannel,
                 const bool     ,
                 const QString& ) :
    CSoundBase ( "OpenSL", true, fpNewProcessCallback, arg, iCtrlMIDIChannel )
{

}

void CSound::InitializeOpenSL()
{
    // set up stream formats for input and output
    SLDataFormat_PCM inStreamFormat;
    inStreamFormat.formatType    = SL_DATAFORMAT_PCM;
    inStreamFormat.numChannels   = 1;
    inStreamFormat.samplesPerSec = SL_SAMPLINGRATE_16;
    inStreamFormat.bitsPerSample = SL_PCMSAMPLEFORMAT_FIXED_16;
    inStreamFormat.containerSize = 16;
    inStreamFormat.channelMask   = SL_SPEAKER_FRONT_CENTER;
    inStreamFormat.endianness    = SL_BYTEORDER_LITTLEENDIAN;

    SLDataFormat_PCM outStreamFormat;
    outStreamFormat.formatType    = SL_DATAFORMAT_PCM;
    outStreamFormat.numChannels   = 2;
    outStreamFormat.samplesPerSec = SYSTEM_SAMPLE_RATE_HZ * 1000; // unit is mHz
    outStreamFormat.bitsPerSample = SL_PCMSAMPLEFORMAT_FIXED_16;
    outStreamFormat.containerSize = 16;
    outStreamFormat.channelMask   = SL_SPEAKER_FRONT_LEFT | SL_SPEAKER_FRONT_RIGHT;
    outStreamFormat.endianness    = SL_BYTEORDER_LITTLEENDIAN;

    // create the OpenSL root engine object
    slCreateEngine ( &engineObject,
                     0,
                     nullptr,
                     0,
                     nullptr,
                     nullptr );

    // realize the engine
    (*engineObject)->Realize ( engineObject,
                               SL_BOOLEAN_FALSE );

    // get the engine interface (required to create other objects)
    (*engineObject)->GetInterface ( engineObject,
                                    SL_IID_ENGINE,
                                    &engine );

    // create the main output mix
    (*engine)->CreateOutputMix ( engine,
                                 &outputMixObject,
                                 0,
                                 nullptr,
                                 nullptr );

    // realize the output mix
    (*outputMixObject)->Realize ( outputMixObject,
                                  SL_BOOLEAN_FALSE );

    // configure the audio (data) source for input
    SLDataLocator_IODevice micLocator;
    micLocator.locatorType = SL_DATALOCATOR_IODEVICE;
    micLocator.deviceType  = SL_IODEVICE_AUDIOINPUT;
    micLocator.deviceID    = SL_DEFAULTDEVICEID_AUDIOINPUT;
    micLocator.device      = nullptr;

    SLDataSource inDataSource;
    inDataSource.pLocator = &micLocator;
    inDataSource.pFormat  = nullptr;

    // configure the input buffer queue
    SLDataLocator_AndroidSimpleBufferQueue inBufferQueue;
    inBufferQueue.locatorType = SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE;
    inBufferQueue.numBuffers  = 2; // max number of buffers in queue

    // configure the audio (data) sink for input
    SLDataSink inDataSink;
    inDataSink.pLocator = &inBufferQueue;
    inDataSink.pFormat  = &inStreamFormat;

    // create the audio recorder
    const SLInterfaceID recorderIds[] = { SL_IID_ANDROIDSIMPLEBUFFERQUEUE };
    const SLboolean recorderReq[]     = { SL_BOOLEAN_TRUE };

    (*engine)->CreateAudioRecorder ( engine,
                                     &recorderObject,
                                     &inDataSource,
                                     &inDataSink,
                                     1,
                                     recorderIds,
                                     recorderReq );

    // realize the audio recorder
    (*recorderObject)->Realize ( recorderObject,
                                 SL_BOOLEAN_FALSE );

    // get the audio recorder interface
    (*recorderObject)->GetInterface ( recorderObject,
                                      SL_IID_RECORD,
                                      &recorder );

    // get the audio recorder simple buffer queue interface
    (*recorderObject)->GetInterface ( recorderObject,
                                      SL_IID_ANDROIDSIMPLEBUFFERQUEUE,
                                      &recorderSimpleBufQueue );

    // register the audio input callback
    (*recorderSimpleBufQueue)->RegisterCallback ( recorderSimpleBufQueue,
                                                  processInput,
                                                  this );

    // configure the output buffer queue
    SLDataLocator_AndroidSimpleBufferQueue outBufferQueue;
    outBufferQueue.locatorType = SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE;
    outBufferQueue.numBuffers  = 2; // max number of buffers in queue

    // configure the audio (data) source for output
    SLDataSource outDataSource;
    outDataSource.pLocator = &outBufferQueue;
    outDataSource.pFormat  = &outStreamFormat;

    // configure the output mix
    SLDataLocator_OutputMix outputMix;
    outputMix.locatorType = SL_DATALOCATOR_OUTPUTMIX;
    outputMix.outputMix   = outputMixObject;

    // configure the audio (data) sink for output
    SLDataSink outDataSink;
    outDataSink.pLocator = &outputMix;
    outDataSink.pFormat  = nullptr;

    // create the audio player
    const SLInterfaceID playerIds[] = { SL_IID_ANDROIDSIMPLEBUFFERQUEUE };
    const SLboolean playerReq[]     = { SL_BOOLEAN_TRUE };

    (*engine)->CreateAudioPlayer ( engine,
                                   &playerObject,
                                   &outDataSource,
                                   &outDataSink,
                                   1,
                                   playerIds,
                                   playerReq );

    // realize the audio player
    (*playerObject)->Realize ( playerObject,
                               SL_BOOLEAN_FALSE );

    // get the audio player interface
    (*playerObject)->GetInterface ( playerObject,
                                    SL_IID_PLAY,
                                    &player );

    // get the audio player simple buffer queue interface
    (*playerObject)->GetInterface ( playerObject,
                                    SL_IID_ANDROIDSIMPLEBUFFERQUEUE,
                                    &playerSimpleBufQueue );

    // register the audio output callback
    (*playerSimpleBufQueue)->RegisterCallback ( playerSimpleBufQueue,
                                                processOutput,
                                                this );
}

void CSound::CloseOpenSL()
{
    // clean up
    (*recorderObject)->Destroy ( recorderObject );
    (*playerObject)->Destroy ( playerObject );
    (*outputMixObject)->Destroy ( outputMixObject );
    (*engineObject)->Destroy ( engineObject );
}

void CSound::Start()
{
    InitializeOpenSL();

// TEST We have to supply the interface with initial buffers, otherwise
// the rendering will not start.
// Note that the number of buffers enqueued here must match the maximum
// numbers of buffers configured in the constructor of this class.
vecsTmpAudioSndCrdStereo.Reset ( 0 );

    // enqueue initial buffers for record
    (*recorderSimpleBufQueue)->Enqueue ( recorderSimpleBufQueue,
                                         &vecsTmpAudioSndCrdStereo[0],
                                         iOpenSLBufferSizeStereo * 2 /* 2 bytes */ );

    (*recorderSimpleBufQueue)->Enqueue ( recorderSimpleBufQueue,
                                         &vecsTmpAudioSndCrdStereo[0],
                                         iOpenSLBufferSizeStereo * 2 /* 2 bytes */ );

    // enqueue initial buffers for playback
    (*playerSimpleBufQueue)->Enqueue ( playerSimpleBufQueue,
                                       &vecsTmpAudioSndCrdStereo[0],
                                       iOpenSLBufferSizeStereo * 2 /* 2 bytes */ );

    (*playerSimpleBufQueue)->Enqueue ( playerSimpleBufQueue,
                                       &vecsTmpAudioSndCrdStereo[0],
                                       iOpenSLBufferSizeStereo * 2 /* 2 bytes */ );

    // start the rendering
    (*recorder)->SetRecordState ( recorder, SL_RECORDSTATE_RECORDING );
    (*player)->SetPlayState ( player, SL_PLAYSTATE_PLAYING );

    // call base class
    CSoundBase::Start();
}

void CSound::Stop()
{
    // stop the audio stream
    (*recorder)->SetRecordState ( recorder, SL_RECORDSTATE_STOPPED );
    (*player)->SetPlayState ( player, SL_PLAYSTATE_STOPPED );

    // clear the buffers
    (*recorderSimpleBufQueue)->Clear ( recorderSimpleBufQueue );
    (*playerSimpleBufQueue)->Clear ( playerSimpleBufQueue );

    // call base class
    CSoundBase::Stop();

    CloseOpenSL();
}

int CSound::Init ( const int iNewPrefMonoBufferSize )
{


// TODO make use of the following:
// String sampleRate = am.getProperty(AudioManager.PROPERTY_OUTPUT_SAMPLE_RATE));
// String framesPerBuffer = am.getProperty(AudioManager.PROPERTY_OUTPUT_FRAMES_PER_BUFFER));
/*
// get the Audio IO DEVICE CAPABILITIES interface
SLAudioIODeviceCapabilitiesItf audioCapabilities;

(*engineObject)->GetInterface ( engineObject,
    SL_IID_AUDIOIODEVICECAPABILITIES,
    &audioCapabilities );

(*audioCapabilities)->QueryAudioInputCapabilities ( audioCapabilities,
                                                    inputDeviceIDs[i],
                                                    &audioInputDescriptor );
*/


    // store buffer size
    iOpenSLBufferSizeMono = iNewPrefMonoBufferSize;

    // init base class
    CSoundBase::Init ( iOpenSLBufferSizeMono );

    // set internal buffer size value and calculate stereo buffer size
    iOpenSLBufferSizeStereo = 2 * iOpenSLBufferSizeMono;

    // create memory for intermediate audio buffer
    vecsTmpAudioSndCrdStereo.Init ( iOpenSLBufferSizeStereo );


// TEST
#if ( SYSTEM_SAMPLE_RATE_HZ != 48000 )
# error "Only a system sample rate of 48 kHz is supported by this module"
#endif
// audio interface number of channels is 1 and the sample rate
// is 16 kHz -> just copy samples and perform no filtering as a
// first simple solution
// 48 kHz / 16 kHz = factor 3 (note that the buffer size mono might
// be divisible by three, therefore we will get a lot of drop outs)
iModifiedInBufSize = iOpenSLBufferSizeMono / 3;
vecsTmpAudioInSndCrd.Init ( iModifiedInBufSize );


    return iOpenSLBufferSizeMono;
}

void CSound::processInput ( SLAndroidSimpleBufferQueueItf bufferQueue,
                            void*                         instance )
{
    CSound* pSound = static_cast<CSound*> ( instance );

    // only process if we are running
    if ( !pSound->bRun )
    {
        return;
    }

    QMutexLocker locker ( &pSound->Mutex );

    // enqueue the buffer for record
    (*bufferQueue)->Enqueue ( bufferQueue,
                              &pSound->vecsTmpAudioInSndCrd[0],
                              pSound->iModifiedInBufSize * 2 /* 2 bytes */ );

// upsampling (without filtering) and channel management
pSound->vecsTmpAudioSndCrdStereo.Reset ( 0 );
for ( int i = 0; i < pSound->iModifiedInBufSize; i++ )
{
    pSound->vecsTmpAudioSndCrdStereo[6 * i] =
        pSound->vecsTmpAudioSndCrdStereo[6 * i + 1] =
            pSound->vecsTmpAudioInSndCrd[i];
}

}

void CSound::processOutput ( SLAndroidSimpleBufferQueueItf bufferQueue,
                             void*                         instance )
{
    CSound* pSound = static_cast<CSound*> ( instance );

    // only process if we are running
    if ( !pSound->bRun )
    {
        return;
    }

    QMutexLocker locker ( &pSound->Mutex );

    // call processing callback function
    pSound->ProcessCallback ( pSound->vecsTmpAudioSndCrdStereo );

    // enqueue the buffer for playback
    (*bufferQueue)->Enqueue ( bufferQueue,
                              &pSound->vecsTmpAudioSndCrdStereo[0],
                              pSound->iOpenSLBufferSizeStereo * 2 /* 2 bytes */ );
}
