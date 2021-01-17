/******************************************************************************\
 * Copyright (c) 2021
 *
 * Author(s):
 *  Noam Postavsky
 *
 * Description:
 * Sound card interface using the portaudio library
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

#include "portaudiosound.h"
#include <pa_asio.h>

CSound::CSound ( void (*fpNewProcessCallback) ( CVector<short>& psData, void* arg ),
                 void*          arg,
                 const QString& strMIDISetup,
                 const bool,
                 const QString& ) :
    CSoundBase ( "portaudio", fpNewProcessCallback, arg, strMIDISetup ),
    deviceIndex ( -1 ),
    deviceStream ( NULL )
{
    PaError err = Pa_Initialize();
    if ( err != paNoError )
    {
        throw CGenErr ( tr ( "Failed to initialize PortAudio: %1" ).arg ( Pa_GetErrorText ( err ) ) );
    }

    // Find ASIO API index.  TODO: support non-ASIO APIs.
    PaHostApiIndex apiCount = Pa_GetHostApiCount();
    if ( apiCount < 0 )
    {
        throw CGenErr ( tr ( "Failed to get API count" ) );
    }
    PaHostApiIndex       apiIndex = -1;
    const PaHostApiInfo* apiInfo  = NULL;
    for ( PaHostApiIndex i = 0; i < apiCount; i++ )
    {
        apiInfo = Pa_GetHostApiInfo ( i );
        if ( apiInfo->type == paASIO )
        {
            apiIndex = i;
            break;
        }
    }
    if ( apiIndex < 0 || apiInfo->deviceCount == 0 )
    {
        throw CGenErr ( tr ( "No ASIO devices found" ) );
    }
    asioIndex = apiIndex;

    lNumDevs = std::min ( apiInfo->deviceCount, MAX_NUMBER_SOUND_CARDS );
    for ( int i = 0; i < lNumDevs; i++ )
    {
        PaDeviceIndex       devIndex = Pa_HostApiDeviceIndexToDeviceIndex ( asioIndex, i );
        const PaDeviceInfo* devInfo  = Pa_GetDeviceInfo ( devIndex );
        strDriverNames[i]            = devInfo->name;
    }
}

int CSound::Init ( const int iNewPrefMonoBufferSize )
{
    if ( deviceIndex >= 0 )
    {
        long minBufferSize, maxBufferSize, prefBufferSize, granularity;
        PaAsio_GetAvailableBufferSizes ( deviceIndex, &minBufferSize, &maxBufferSize, &prefBufferSize, &granularity );
        if ( granularity == 0 ) // no options, just take the preferred one.
        {
            iPrefMonoBufferSize = prefBufferSize;
        }
        else if ( iNewPrefMonoBufferSize > maxBufferSize )
        {
            iPrefMonoBufferSize = maxBufferSize;
        }
        else if ( iNewPrefMonoBufferSize < minBufferSize )
        {
            iPrefMonoBufferSize = minBufferSize;
        }
        else
        {
            // Requested size is within the range.
            int bufferSize = minBufferSize;
            while ( bufferSize < iNewPrefMonoBufferSize )
            {
                if ( granularity == -1 ) // available buffer size values are powers of two.
                {
                    bufferSize *= 2;
                }
                else
                {
                    bufferSize += granularity;
                }
            }
            iPrefMonoBufferSize = bufferSize;
        }
    }
    else
    {
        iPrefMonoBufferSize = iNewPrefMonoBufferSize;
    }

    vecsAudioData.Init ( iPrefMonoBufferSize * 2 );
    return CSoundBase::Init ( iPrefMonoBufferSize );
}

CSound::~CSound() { Pa_Terminate(); }

PaDeviceIndex CSound::DeviceIndexFromName ( const QString& strDriverName )
{
    for ( int i = 0; i < lNumDevs; i++ )
    {
        PaDeviceIndex devIndex = Pa_HostApiDeviceIndexToDeviceIndex ( asioIndex, i );
        const PaDeviceInfo* devInfo = Pa_GetDeviceInfo ( devIndex );
        if ( strDriverName.compare ( devInfo->name ) == 0 )
        {
            return devIndex;
        }
    }
    return -1;
}

QString CSound::LoadAndInitializeDriver ( QString strDriverName, bool bOpenDriverSetup )
{
    (void) bOpenDriverSetup; // FIXME: respect this

    int devIndex = DeviceIndexFromName ( strDriverName );
    if ( devIndex < 0 )
    {
        return tr ( "The current selected audio device is no longer present in the system." );
    }

    const PaDeviceInfo* deviceInfo = Pa_GetDeviceInfo ( devIndex );

    if ( deviceInfo->maxInputChannels < 2 || deviceInfo->maxOutputChannels < 2 )
    {
        // FIXME: handle mono devices.
        return tr ( "Less than 2 channels supported" );
    }

    if ( deviceStream != NULL )
    {
        Pa_CloseStream ( deviceStream );
        deviceStream = NULL;
    }
    deviceIndex = devIndex;

    PaStreamParameters paInputParams;
    paInputParams.device                    = deviceIndex;
    paInputParams.channelCount              = std::min ( 2, deviceInfo->maxInputChannels );
    paInputParams.sampleFormat              = paInt16;
    paInputParams.suggestedLatency          = deviceInfo->defaultLowInputLatency;
    paInputParams.hostApiSpecificStreamInfo = NULL;

    PaStreamParameters paOutputParams;
    paOutputParams.device                    = deviceIndex;
    paOutputParams.channelCount              = std::min ( 2, deviceInfo->maxOutputChannels );
    paOutputParams.sampleFormat              = paInt16;
    paOutputParams.suggestedLatency          = deviceInfo->defaultLowOutputLatency;
    paOutputParams.hostApiSpecificStreamInfo = NULL;

    PaError err = Pa_OpenStream ( &deviceStream,
                                  &paInputParams,
                                  &paOutputParams,
                                  SYSTEM_SAMPLE_RATE_HZ,
                                  iPrefMonoBufferSize,
                                  paNoFlag,
                                  &CSound::paStreamCallback,
                                  this );

    if ( err != paNoError )
    {
        return tr ( "Could not open Portaudio stream: %1, %2" ).arg ( Pa_GetErrorText ( err ) ).arg ( Pa_GetLastHostErrorInfo()->errorText );
    }

    return "";
}

void CSound::UnloadCurrentDriver()
{
    if ( deviceStream != NULL )
    {
        Pa_CloseStream ( deviceStream );
        deviceStream = NULL;
        deviceIndex  = -1;
    }
}

int CSound::paStreamCallback ( const void*                     input,
                               void*                           output,
                               unsigned long                   frameCount,
                               const PaStreamCallbackTimeInfo* timeInfo,
                               PaStreamCallbackFlags           statusFlags,
                               void*                           userData )
{
    (void) timeInfo;
    (void) statusFlags;

    CSound*           pSound        = static_cast<CSound*> ( userData );
    CVector<int16_t>& vecsAudioData = pSound->vecsAudioData;

    // CAPTURE ---------------------------------
    memcpy ( &vecsAudioData[0], input, sizeof ( int16_t ) * frameCount * 2 );

    pSound->ProcessCallback ( vecsAudioData );

    // PLAYBACK ------------------------------------------------------------
    memcpy ( output, &vecsAudioData[0], sizeof ( int16_t ) * frameCount * 2 );

    return paContinue;
}

void CSound::Start()
{
    // start audio
    Pa_StartStream ( deviceStream );

    // call base class
    CSoundBase::Start();
}

void CSound::Stop()
{
    // stop audio
    Pa_StopStream ( deviceStream );

    // call base class
    CSoundBase::Stop();
}
