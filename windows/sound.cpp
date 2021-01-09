/******************************************************************************\
 * Copyright (c) 2004-2020
 *
 * Author(s):
 *  Volker Fischer
 *
 * Description:
 * Sound card interface for Windows operating systems
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


/* Implementation *************************************************************/
// external references
extern AsioDrivers* asioDrivers;
bool   loadAsioDriver ( char* name );

QStringList CSound::GetApiNames()
{
    QStringList names;
    PaHostApiIndex apiCount = Pa_GetHostApiCount();
    if (apiCount < 0) {
        throw CGenErr (tr ("Failed to get API count"));
    }
    // PaHostApiIndex defaultApiIndex = Pa_GetDefaultHostApi();
    for (PaHostApiIndex i = 0; i < apiCount; i++) {
        const PaHostApiInfo* info = Pa_GetHostApiInfo (i);
        names.append (info->name);
    }
    return names;
}

void CSound::setApi(PaHostApiIndex newApi)
{
    if (newApi == selectedApi)
        return;

    selectedApi = newApi;
    const PaHostApiInfo* apiInfo = Pa_GetHostApiInfo(selectedApi);
    lNumDevs = apiInfo->deviceCount;

    paDeviceInfos.Init(lNumDevs); // allocate vector
    for ( int i = 0; i < lNumDevs; i++ ) // then fill it.
    {
        PaDeviceIndex devIndex = Pa_HostApiDeviceIndexToDeviceIndex (selectedApi, i);
        paDeviceInfos[i] = Pa_GetDeviceInfo ( devIndex );
        strDriverNames[i] = paDeviceInfos[i]->name;
        if (strCurDevName.isEmpty() && apiInfo->defaultInputDevice == devIndex)
            strCurDevName = strDriverNames[i];
    }
}

// virtual QStringList GetDevNames();
// virtual QString     GetDev();

// QString CSound::SetDev ( const QString strDevName )
// {
//     strCurDevName = strDevName;
// }


QStringList CSound::GetDevNames()
{
    QStringList devices;
    int api = (selectedApi >= 0)? selectedApi : Pa_GetDefaultHostApi();
    const PaHostApiInfo* apiInfo = Pa_GetHostApiInfo(api);
    for (int i = 0; i < apiInfo->deviceCount; i++) {
        const PaDeviceInfo* devInfo = Pa_GetDeviceInfo (Pa_HostApiDeviceIndexToDeviceIndex(api, i));
        if (devInfo->maxInputChannels > 0 && devInfo->maxOutputChannels > 0)
            devices << devInfo->name;
    }
    return devices;
}
QStringList CSound::GetInDevNames()
{
    QStringList devices;
    int api = (selectedApi >= 0)? selectedApi : Pa_GetDefaultHostApi();
    const PaHostApiInfo* apiInfo = Pa_GetHostApiInfo(api);
    for (int i = 0; i < apiInfo->deviceCount; i++) {
        const PaDeviceInfo* devInfo = Pa_GetDeviceInfo (Pa_HostApiDeviceIndexToDeviceIndex(api, i));
        if (devInfo->maxInputChannels > 0 && devInfo->maxOutputChannels == 0)
            devices << devInfo->name;
    }
    return devices;
}
QStringList CSound::GetOutDevNames()
{
    QStringList devices;
    int api = (selectedApi >= 0)? selectedApi : Pa_GetDefaultHostApi();
    const PaHostApiInfo* apiInfo = Pa_GetHostApiInfo(api);
    for (int i = 0; i < apiInfo->deviceCount; i++) {
        const PaDeviceInfo* devInfo = Pa_GetDeviceInfo (Pa_HostApiDeviceIndexToDeviceIndex(api, i));
        if (devInfo->maxOutputChannels > 0 && devInfo->maxInputChannels == 0)
            devices << devInfo->name;
    }
    return devices;
}

QString CSound::SetDev(QString newDevName)
{
    selectedInDev = "";
    selectedOutDev = "";
    strCurDevName = newDevName;
    return LoadAndInitializeDriver(strCurDevName, strCurDevName, false);
}

QString CSound::SetInDev(QString newInDevName)
{
    strCurDevName = "";
    selectedInDev = newInDevName;
    return LoadAndInitializeDriver(selectedInDev, selectedOutDev, false);
}
QString CSound::SetOutDev(QString newOutDevName)
{
    strCurDevName = "";
    selectedOutDev = newOutDevName;
    return LoadAndInitializeDriver(selectedInDev, selectedOutDev, false);
}

/******************************************************************************\
* Common                                                                       *
\******************************************************************************/

QString CSound::LoadAndInitializeDriver ( QString inDevName,
                                          QString outDevName,
                                          bool    bOpenDriverSetup )
{
    int api = selectedApi > 0? selectedApi : Pa_GetDefaultHostApi();
    if (api < 0)
    {
        return (tr("Failed to get default PortAudio API: %1").arg(Pa_GetErrorText(selectedApi)));
    }
    setApi(api);

    int devIn = INVALID_INDEX, devOut = INVALID_INDEX;
    const PaDeviceInfo* inDeviceInfo = NULL;
    const PaDeviceInfo* outDeviceInfo = NULL;
    for ( int i = 0; i < paDeviceInfos.Size(); i++ )
    {
        if (inDevName.compare (paDeviceInfos[i]->name) == 0 )
        {
            inDeviceInfo = paDeviceInfos[i];
            devIn = Pa_HostApiDeviceIndexToDeviceIndex (selectedApi, i);
        }
        if (outDevName.compare (paDeviceInfos[i]->name) == 0 )
        {
            outDeviceInfo = paDeviceInfos[i];
            devOut = Pa_HostApiDeviceIndexToDeviceIndex (selectedApi, i);
        }
    }
    if ( devIn == INVALID_INDEX && devOut == INVALID_INDEX )
    {
        return tr ( "The current selected audio devices are no longer present in the system." );
    }

    PaStreamParameters paInputParams;
    if (devIn != INVALID_INDEX) {
        paInputParams.device = devIn;
        paInputParams.channelCount = std::min (2, inDeviceInfo->maxInputChannels);
        paInputParams.sampleFormat = paInt16;
        paInputParams.suggestedLatency = inDeviceInfo->defaultLowInputLatency;
        paInputParams.hostApiSpecificStreamInfo = NULL;
    } else {
        paInputParams.channelCount = 0;
    }

    PaStreamParameters paOutputParams;
    if (devOut != INVALID_INDEX) {
        paOutputParams.device = devOut;
        paOutputParams.channelCount = std::min (2, outDeviceInfo->maxOutputChannels);
        paOutputParams.sampleFormat = paInt16;
        paOutputParams.suggestedLatency =  outDeviceInfo->defaultLowOutputLatency;
        paOutputParams.hostApiSpecificStreamInfo = NULL;
    } else {
        paOutputParams.channelCount = 0;
    }

    PaError err = Pa_OpenStream ( &paStream,
                                  devIn != INVALID_INDEX? &paInputParams : NULL,
                                  devOut != INVALID_INDEX? &paOutputParams: NULL,
                                  SYSTEM_SAMPLE_RATE_HZ,
                                  vecsMultChanAudioSndCrd.Size()/2,// paFramesPerBufferUnspecified
                                  paNoFlag,
                                  &CSound::paStreamCallback,
                                  this );

    if (err != paNoError)
    {
        paStreamInChannels = paStreamOutChannels = 0;
        return tr ("Could not open Portaudio stream: %1, %2").arg
            (Pa_GetErrorText (err)).arg
            (Pa_GetLastHostErrorInfo()->errorText);
    }
    paStreamInChannels = paInputParams.channelCount;
    paStreamOutChannels = paOutputParams.channelCount;

    // TODO: check this
    (void) bOpenDriverSetup;

    return "";
}

void CSound::UnloadCurrentDriver()
{
    if (paStream) Pa_CloseStream (paStream);
    paStream = NULL;
}

int CSound::Init ( const int iNewPrefMonoBufferSize )
{
    // init base class
    CSoundBase::Init ( iNewPrefMonoBufferSize );

    // create memory for intermediate audio buffer
    vecsMultChanAudioSndCrd.Init ( iNewPrefMonoBufferSize*2 );

    return iNewPrefMonoBufferSize;
}

void CSound::Start()
{
    // start audio
    Pa_StartStream (paStream);

    // call base class
    CSoundBase::Start();
}

void CSound::Stop()
{
    // stop audio
    Pa_StopStream (paStream);

    // call base class
    CSoundBase::Stop();
}

CSound::CSound ( void           (*fpNewCallback) ( CVector<int16_t>& psData, void* arg ),
                 void*          arg,
                 const QString& strMIDISetup,
                 const bool     ,
                 const QString& ) :
    CSoundBase              ( "ASIO", fpNewCallback, arg, strMIDISetup ),
    fInOutLatencyMs         ( 0.0f ), // "0.0" means that no latency value is available
    selectedApi (-1),
    paStream (NULL)
{
    PaError err = Pa_Initialize();
    if ( err != paNoError )
    {
        throw CGenErr( tr("Failed to initialize PortAudio: %1").arg (Pa_GetErrorText (err)));
    }
}

int CSound::paStreamCallback(const void *input, void *output, unsigned long frameCount,
                             const PaStreamCallbackTimeInfo *timeInfo, PaStreamCallbackFlags statusFlags,
                             void *userData)
{
    (void) timeInfo;
    (void) statusFlags;

    CSound* pSound = static_cast<CSound*>(userData);
    // get references to class members
    //int&              iASIOBufferSizeMono     = pSound->iASIOBufferSizeMono;
    CVector<int16_t>& vecsMultChanAudioSndCrd = pSound->vecsMultChanAudioSndCrd;

    // CAPTURE ---------------------------------
    // TODO: handle GetSelCHAndAddCH() setting?
    unsigned long sampleCount = std::min(frameCount * pSound->paStreamInChannels, static_cast<unsigned long>(vecsMultChanAudioSndCrd.size()));
    memcpy (&vecsMultChanAudioSndCrd[0], input, sizeof (int16_t) * sampleCount);

    pSound->ProcessCallback ( vecsMultChanAudioSndCrd );

    // PLAYBACK ------------------------------------------------------------
    sampleCount = std::min(frameCount * pSound->paStreamOutChannels, static_cast<unsigned long>(vecsMultChanAudioSndCrd.size()));
    memcpy (output, &vecsMultChanAudioSndCrd[0], sizeof (int16_t) * sampleCount);

    return paContinue;
}
