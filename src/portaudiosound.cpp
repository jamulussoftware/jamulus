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

// Needed for reset request callback
static CSound* pThisSound;

static void resetRequestCallback() { pThisSound->EmitReinitRequestSignal ( RS_RELOAD_RESTART_AND_INIT ); }


CSound::CSound ( void ( *fpNewProcessCallback ) ( CVector<short>& psData, void* arg ),
                 void*          arg,
                 const QString& strMIDISetup,
                 const bool,
                 const QString& strApiName ) :
    CSoundBase ( "portaudio", fpNewProcessCallback, arg, strMIDISetup ),
    strSelectApiName ( strApiName.isEmpty() ? "ASIO" : strApiName ),
    inDeviceIndex ( -1 ),
    outDeviceIndex ( -1 ),
    deviceStream ( NULL ),
    vSelectedInputChannels ( NUM_IN_OUT_CHANNELS ),
    vSelectedOutputChannels ( NUM_IN_OUT_CHANNELS )
{
    pThisSound = this;

    int numPortAudioDevices = std::min ( InitPa(), MAX_NUMBER_SOUND_CARDS );
    lNumDevs                = 0;
    for ( int i = 0; i < numPortAudioDevices; i++ )
    {
        PaDeviceIndex       devIndex = Pa_HostApiDeviceIndexToDeviceIndex ( selectedApiIndex, i );
        const PaDeviceInfo* devInfo  = Pa_GetDeviceInfo ( devIndex );
        if ( devInfo->maxInputChannels > 0 && devInfo->maxOutputChannels > 0 )
        {
            // Duplex device.
            paInputDeviceIndices[lNumDevs]  = devIndex;
            paOutputDeviceIndices[lNumDevs] = devIndex;
            strDriverNames[lNumDevs++]      = devInfo->name;
        }
        else if ( devInfo->maxInputChannels > 0)
        {
            // For each input device, construct virtual duplex devices by
            // combining it with every output device (i.e., Cartesian product of
            // input and output devices).
            for ( int j = 0; j < numPortAudioDevices; j++ )
            {
                PaDeviceIndex       outDevIndex = Pa_HostApiDeviceIndexToDeviceIndex ( selectedApiIndex, j );
                const PaDeviceInfo* outDevInfo  = Pa_GetDeviceInfo ( outDevIndex );
                if ( outDevInfo->maxOutputChannels > 0 && outDevInfo->maxInputChannels == 0 )
                {
                    paInputDeviceIndices[lNumDevs]  = devIndex;
                    paOutputDeviceIndices[lNumDevs] = outDevIndex;
                    strDriverNames[lNumDevs++]      = QString ( "in: " ) + devInfo->name + "/out: " + outDevInfo->name;
                }
            }
        }
    }
}

QString CSound::GetPaApiNames()
{
    // NOTE: It is okay to initialize PortAudio even if it already has been
    // initialized, so long as every Pa_Initialize call is balanced with a
    // Pa_Terminate.
    class PaInitInScope
    {
    public:
        PaError err;
        PaInitInScope() : err ( Pa_Initialize() ) {}
        ~PaInitInScope() { Pa_Terminate(); }
    } paInScope;

    if ( paInScope.err != paNoError )
    {
        return "";
    }

    PaHostApiIndex apiCount = Pa_GetHostApiCount();
    if ( apiCount < 0 )
    {
        return "";
    }

    QStringList apiNames;
    for ( PaHostApiIndex i = 0; i < apiCount; i++ )
    {
        const PaHostApiInfo* apiInfo = Pa_GetHostApiInfo ( i );
        QString name                 = apiInfo->name;
        name                         = name.section ( ' ', -1 ); // Drop "Windows " from "Windows <apiname>" names.
        apiNames << name;
    }

    return apiNames.join ( ", " );
}

int CSound::InitPa()
{
    PaError err = Pa_Initialize();
    if ( err != paNoError )
    {
        throw CGenErr ( tr ( "Failed to initialize PortAudio: %1" ).arg ( Pa_GetErrorText ( err ) ) );
    }

    PaHostApiIndex apiCount = Pa_GetHostApiCount();
    if ( apiCount < 0 )
    {
        throw CGenErr ( tr ( "Failed to get API count" ) );
    }

    PaHostApiIndex       apiIndex = -1;
    const PaHostApiInfo* apiInfo  = NULL;
    for ( PaHostApiIndex i = 0; i < apiCount; i++ )
    {
        apiInfo      = Pa_GetHostApiInfo ( i );
        QString name = apiInfo->name;
        name         = name.section ( ' ', -1 ); // Drop "Windows " from "Windows <apiname>" names.
        if ( strSelectApiName.compare ( name, Qt::CaseInsensitive ) == 0 )
        {
#ifdef _WIN32
            if ( apiInfo->type == paASIO )
            {
                PaAsio_RegisterResetRequestCallback ( &resetRequestCallback );
            }
#endif // _WIN32

            apiIndex = i;
            break;
        }
    }
    if ( apiIndex < 0 || apiInfo->deviceCount == 0 )
    {
        throw CGenErr ( tr ( "No %1 devices found" ).arg ( strSelectApiName ) );
    }
    selectedApiIndex = apiIndex;

    return apiInfo->deviceCount;
}

int CSound::Init ( const int iNewPrefMonoBufferSize )
{
    const PaHostApiInfo* apiInfo = Pa_GetHostApiInfo ( selectedApiIndex );
    if ( inDeviceIndex >= 0 && apiInfo->type == paASIO )
    {
        long minBufferSize, maxBufferSize, prefBufferSize, granularity;
        PaAsio_GetAvailableBufferSizes ( inDeviceIndex, &minBufferSize, &maxBufferSize, &prefBufferSize, &granularity );
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

    vecsAudioData.Init ( iPrefMonoBufferSize * NUM_IN_OUT_CHANNELS );
    if ( deviceStream && inDeviceIndex >= 0 )
    {
        ReinitializeDriver ( inDeviceIndex, outDeviceIndex );
    }

    return CSoundBase::Init ( iPrefMonoBufferSize );
}

CSound::~CSound() { Pa_Terminate(); }

bool CSound::DeviceIndexFromName ( const QString& strDriverName, PaDeviceIndex& inIndex, PaDeviceIndex& outIndex )
{
    inIndex  = -1;
    outIndex = -1;

    if ( selectedApiIndex < 0 )
    {
        InitPa();
    }

    // find driver index from given driver name
    int iDriverIdx = INVALID_INDEX; // initialize with an invalid index
    for ( int i = 0; i < MAX_NUMBER_SOUND_CARDS; i++ )
    {
        if ( strDriverName.compare ( strDriverNames[i] ) == 0 )
        {
            iDriverIdx = i;
        }
    }

    if ( iDriverIdx == INVALID_INDEX )
    {
        return false;
    }

    inIndex  = paInputDeviceIndices[iDriverIdx];
    outIndex = paOutputDeviceIndices[iDriverIdx];
    return true;
}

int CSound::GetNumInputChannels()
{
    if ( inDeviceIndex >= 0 )
    {
        const PaDeviceInfo* deviceInfo = Pa_GetDeviceInfo ( inDeviceIndex );
        return deviceInfo->maxInputChannels;
    }
    return CSoundBase::GetNumInputChannels();
}
int CSound::GetNumOutputChannels()
{
    if ( outDeviceIndex >= 0 )
    {
        const PaDeviceInfo* deviceInfo = Pa_GetDeviceInfo ( outDeviceIndex );
        return deviceInfo->maxOutputChannels;
    }
    return CSoundBase::GetNumOutputChannels();
}

QString CSound::GetInputChannelName ( const int channel )
{
    const PaHostApiInfo* apiInfo = Pa_GetHostApiInfo ( selectedApiIndex );
    if ( inDeviceIndex >= 0 && apiInfo->type == paASIO )
    {
        const char* channelName;
        PaError err = PaAsio_GetInputChannelName ( inDeviceIndex, channel, &channelName );
        if ( err == paNoError )
        {
            return QString ( channelName );
        }
    }
    return CSoundBase::GetInputChannelName ( channel );
}
QString CSound::GetOutputChannelName ( const int channel )
{
    const PaHostApiInfo* apiInfo = Pa_GetHostApiInfo ( selectedApiIndex );
    if ( outDeviceIndex >= 0 && apiInfo->type == paASIO )
    {
        const char* channelName;
        PaError err = PaAsio_GetOutputChannelName ( outDeviceIndex, channel, &channelName );
        if ( err == paNoError )
        {
            return QString ( channelName );
        }
    }
    return CSoundBase::GetOutputChannelName ( channel );
}

void CSound::SetLeftInputChannel ( const int channel )
{
    if ( channel < GetNumInputChannels() )
    {
        vSelectedInputChannels[0] = channel;
    }
}
void CSound::SetRightInputChannel ( const int channel )
{
    if ( channel < GetNumInputChannels() )
    {
        vSelectedInputChannels[1] = channel;
    }
}

void CSound::SetLeftOutputChannel ( const int channel )
{
    if ( channel < GetNumOutputChannels() )
    {
        vSelectedOutputChannels[0] = channel;
    }
}
void CSound::SetRightOutputChannel ( const int channel )
{
    if ( channel < GetNumOutputChannels() )
    {
        vSelectedOutputChannels[1] = channel;
    }
}

QString CSound::LoadAndInitializeDriver ( QString strDriverName, bool bOpenDriverSetup )
{
    (void) bOpenDriverSetup; // FIXME: respect this

    PaDeviceIndex inIndex, outIndex;
    bool          haveDevice = DeviceIndexFromName ( strDriverName, inIndex, outIndex );
    if ( !haveDevice )
    {
        return tr ( "The current selected audio device is no longer present in the system." );
    }
    QString err = ReinitializeDriver ( inIndex, outIndex );
    if ( err.isEmpty() )
    {
        strCurDevName = strDriverName;
    }
    return err;
}

QString CSound::ReinitializeDriver ( PaDeviceIndex inIndex, PaDeviceIndex outIndex )
{
    const PaDeviceInfo* inDeviceInfo  = Pa_GetDeviceInfo ( inIndex );
    const PaDeviceInfo* outDeviceInfo = Pa_GetDeviceInfo ( outIndex );

    if ( inDeviceInfo->maxInputChannels < NUM_IN_OUT_CHANNELS ||
         outDeviceInfo->maxOutputChannels < NUM_IN_OUT_CHANNELS )
    {
        // FIXME: handle mono devices.
        return tr ( "Less than 2 channels not supported" );
    }

    if ( deviceStream != NULL )
    {
        Pa_CloseStream ( deviceStream );
        deviceStream   = NULL;
        inDeviceIndex  = -1;
        outDeviceIndex = -1;
    }

    const PaHostApiInfo* apiInfo = Pa_GetHostApiInfo ( selectedApiIndex );

    PaStreamParameters paInputParams;
    PaAsioStreamInfo   asioInputInfo;
    paInputParams.device       = inIndex;
    paInputParams.channelCount = std::min ( NUM_IN_OUT_CHANNELS, inDeviceInfo->maxInputChannels );
    paInputParams.sampleFormat = paInt16;
    // NOTE: Setting latency to deviceInfo->defaultLowInputLatency seems like it
    // would be sensible, but gives an overly large buffer size at least in the
    // case of ASIO4ALL.  On the other hand, putting 0 causes an error with
    // WDM-KS devices.  Put a latency value that corresponds to our intended.
    // buffer size.
    paInputParams.suggestedLatency = (PaTime) iPrefMonoBufferSize / SYSTEM_SAMPLE_RATE_HZ;
    if ( apiInfo->type == paASIO )
    {
        paInputParams.hostApiSpecificStreamInfo = &asioInputInfo;
        asioInputInfo.size                      = sizeof asioInputInfo;
        asioInputInfo.hostApiType               = paASIO;
        asioInputInfo.version                   = 1;
        asioInputInfo.flags                     = paAsioUseChannelSelectors;
        asioInputInfo.channelSelectors          = &vSelectedInputChannels[0];
    }
    else
    {
        paInputParams.hostApiSpecificStreamInfo = NULL;
    }

    PaStreamParameters paOutputParams;
    PaAsioStreamInfo   asioOutputInfo;
    paOutputParams.device           = outIndex;
    paOutputParams.channelCount     = std::min ( NUM_IN_OUT_CHANNELS, outDeviceInfo->maxOutputChannels );
    paOutputParams.sampleFormat     = paInt16;
    paOutputParams.suggestedLatency = paInputParams.suggestedLatency;
    if ( apiInfo->type == paASIO )
    {
        paOutputParams.hostApiSpecificStreamInfo = &asioOutputInfo;
        asioOutputInfo.size                      = sizeof asioOutputInfo;
        asioOutputInfo.hostApiType               = paASIO;
        asioOutputInfo.version                   = 1;
        asioOutputInfo.flags                     = paAsioUseChannelSelectors;
        asioOutputInfo.channelSelectors          = &vSelectedOutputChannels[0];
    }
    else
    {
        paOutputParams.hostApiSpecificStreamInfo = NULL;
    }

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

    inDeviceIndex  = inIndex;
    outDeviceIndex = outIndex;
    return "";
}

void CSound::UnloadCurrentDriver()
{
    if ( deviceStream != NULL )
    {
        Pa_CloseStream ( deviceStream );
        deviceStream   = NULL;
        inDeviceIndex  = -1;
        outDeviceIndex = -1;
    }
    if ( selectedApiIndex >= 0 )
    {
        Pa_Terminate();
        selectedApiIndex = -1;
    }
}

#ifdef WIN32
void CSound::OpenDriverSetup()
{
    if ( deviceStream )
    {
        // PaAsio_ShowControlPanel() doesn't work when the device is opened.
        ASIOControlPanel();
    }
    else
    {
        PaDeviceIndex inIndex, outIndex;
        DeviceIndexFromName ( strCurDevName, inIndex, outIndex );
        // pass NULL (?) for system specific ptr.
        PaAsio_ShowControlPanel ( inIndex, NULL );
    }
}
#endif // WIN32

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
    memcpy ( &vecsAudioData[0], input, sizeof ( int16_t ) * frameCount * NUM_IN_OUT_CHANNELS );

    pSound->ProcessCallback ( vecsAudioData );

    // PLAYBACK ------------------------------------------------------------
    memcpy ( output, &vecsAudioData[0], sizeof ( int16_t ) * frameCount * NUM_IN_OUT_CHANNELS );

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
