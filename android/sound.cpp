/******************************************************************************\
 * Copyright (c) 2004-2014
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
CSound::CSound ( void (*fpNewProcessCallback) ( CVector<short>& psData, void* arg ), void* arg ) :
    CSoundBase ( "OpenSL", true, fpNewProcessCallback, arg )
{
    // set up stream format
    SLDataFormat_PCM streamFormat;
    streamFormat.formatType    = SL_DATAFORMAT_PCM;
    streamFormat.numChannels   = 2;
    streamFormat.samplesPerSec = SYSTEM_SAMPLE_RATE_HZ * 1000; // unit is mHz
    streamFormat.bitsPerSample = SL_PCMSAMPLEFORMAT_FIXED_16;
    streamFormat.containerSize = 16;
    streamFormat.channelMask   = SL_SPEAKER_FRONT_LEFT | SL_SPEAKER_FRONT_RIGHT;
    streamFormat.endianness    = SL_BYTEORDER_LITTLEENDIAN;

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
    inBufferQueue.numBuffers  = 1; // max number of buffers in queue

    // configure the audio (data) sink for input
    SLDataSink inDataSink;
    inDataSink.pLocator = &inBufferQueue;
    inDataSink.pFormat  = &streamFormat;

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
    outBufferQueue.numBuffers  = 1; // max number of buffers in queue

    // configure the audio (data) source for output
    SLDataSource outDataSource;
    outDataSource.pLocator = &outBufferQueue;
    outDataSource.pFormat  = &streamFormat;

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
    // start the rendering
    (*player)->SetPlayState ( player, SL_PLAYSTATE_PLAYING );

    // call base class
    CSoundBase::Start();
}

void CSound::Stop()
{
    // stop the audio stream
    (*player)->SetPlayState ( player, SL_PLAYSTATE_STOPPED );

    // clear the buffers
    (*playerSimpleBufQueue)->Clear ( playerSimpleBufQueue );

    // call base class
    CSoundBase::Stop();
}

int CSound::Init ( const int iNewPrefMonoBufferSize )
{
/*
    UInt32 iActualMonoBufferSize;

    // Error message string: in case buffer sizes on input and output cannot be
    // set to the same value
    const QString strErrBufSize = tr ( "The buffer sizes of the current "
        "input and output audio device cannot be set to a common value. Please "
        "choose other input/output audio devices in your system settings." );

    // try to set input buffer size
    iActualMonoBufferSize =
        SetBufferSize ( audioInputDevice[lCurDev], true, iNewPrefMonoBufferSize );

    if ( iActualMonoBufferSize != static_cast<UInt32> ( iNewPrefMonoBufferSize ) )
    {
        // try to set the input buffer size to the output so that we
        // have a matching pair
        if ( SetBufferSize ( audioOutputDevice[lCurDev], false, iActualMonoBufferSize ) !=
             iActualMonoBufferSize )
        {
            throw CGenErr ( strErrBufSize );
        }
    }
    else
    {
        // try to set output buffer size
        if ( SetBufferSize ( audioOutputDevice[lCurDev], false, iNewPrefMonoBufferSize ) !=
             static_cast<UInt32> ( iNewPrefMonoBufferSize ) )
        {
            throw CGenErr ( strErrBufSize );
        }
    }
*/

/*
// TEST
int iActualMonoBufferSize = iNewPrefMonoBufferSize;


    // store buffer size
    iOpenSLBufferSizeMono = iActualMonoBufferSize;

    // init base class
    CSoundBase::Init ( iOpenSLBufferSizeMono );

    // set internal buffer size value and calculate stereo buffer size
    iOpenSLBufferSizeStero = 2 * iOpenSLBufferSizeMono;

    // create memory for intermediate audio buffer
    vecsTmpAudioSndCrdStereo.Init ( iOpenSLBufferSizeStero );
*/

/*
    // fill audio unit buffer struct
    pBufferList->mNumberBuffers              = 1;
    pBufferList->mBuffers[0].mNumberChannels = 2; // stereo
    pBufferList->mBuffers[0].mDataByteSize   = iCoreAudioBufferSizeMono * 4; // 2 bytes, 2 channels
    pBufferList->mBuffers[0].mData           = &vecsTmpAudioSndCrdStereo[0];

    // initialize units
    if ( AudioUnitInitialize ( audioInputUnit ) )
    {
        throw CGenErr ( tr ( "Initialization of CoreAudio failed" ) );
    }

    if ( AudioUnitInitialize ( audioOutputUnit ) )
    {
        throw CGenErr ( tr ( "Initialization of CoreAudio failed" ) );
    }
*/

    return iOpenSLBufferSizeMono;
}

void CSound::processInput ( SLAndroidSimpleBufferQueueItf bufferQueue,
                            void*                         instance )
{
    CSound* pSound = reinterpret_cast<CSound*> ( instance );

    QMutexLocker locker ( &pSound->Mutex );

    // enqueue the buffer for record
    (*bufferQueue)->Enqueue ( bufferQueue,
                              &pSound->vecsTmpAudioSndCrdStereo[0],
                              pSound->iOpenSLBufferSizeStero );

    // call processing callback function
    pSound->ProcessCallback ( pSound->vecsTmpAudioSndCrdStereo );
}

void CSound::processOutput ( SLAndroidSimpleBufferQueueItf bufferQueue,
                             void*                         instance )
{
    CSound* pSound = reinterpret_cast<CSound*> ( instance );

    QMutexLocker locker ( &pSound->Mutex );

    // enqueue the buffer for playback
    (*bufferQueue)->Enqueue ( bufferQueue,
                              &pSound->vecsTmpAudioSndCrdStereo[0],
                              pSound->iOpenSLBufferSizeStero );
}
