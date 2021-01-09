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
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 *
\******************************************************************************/

#pragma once

#include <QMutex>
#include <QMessageBox>
#include "../src/util.h"
#include "../src/global.h"
#include "../src/soundbase.h"

// copy the ASIO SDK in the llcon/windows directory: "llcon/windows/ASIOSDK2" to
// get it work
#include "asiosys.h"
#include "asio.h"
#include "asiodrivers.h"

#include <portaudio.h>


/* Definitions ****************************************************************/
// stereo for input and output
#define NUM_IN_OUT_CHANNELS         2


/* Classes ********************************************************************/
class CSound : public CSoundBase
{
public:
    CSound ( void           (*fpNewCallback) ( CVector<int16_t>& psData, void* arg ),
             void*          arg,
             const QString& strMIDISetup,
             const bool     ,
             const QString& );
    
    virtual ~CSound() { UnloadCurrentDriver(); }

    virtual int  Init ( const int iNewPrefMonoBufferSize );
    virtual void Start();
    virtual void Stop();

    virtual void OpenDriverSetup() { ASIOControlPanel(); }

    QStringList GetApiNames();
    void setApi(PaHostApiIndex newApi);
    int GetApi() { return selectedApi; }

    // device selection
    QStringList GetDevNames();
    QStringList GetInDevNames();
    QStringList GetOutDevNames();
    QString GetInDev() { return selectedInDev; }
    QString GetOutDev(){ return selectedOutDev; }
    virtual QString     SetDev ( const QString strDevName );
    virtual QString     SetInDev ( const QString strDevName );
    virtual QString     SetOutDev ( const QString strDevName );

    // // channel selection
    // virtual int     GetNumInputChannels() { return static_cast<int> ( lNumInChanPlusAddChan ); }
    // virtual QString GetInputChannelName ( const int iDiD ) { return channelInputName[iDiD]; }
    // virtual void    SetLeftInputChannel  ( const int iNewChan );
    // virtual void    SetRightInputChannel ( const int iNewChan );
    // virtual int     GetLeftInputChannel()  { return vSelectedInputChannels[0]; }
    // virtual int     GetRightInputChannel() { return vSelectedInputChannels[1]; }

    // virtual int     GetNumOutputChannels() { return static_cast<int> ( lNumOutChan ); }
    // virtual QString GetOutputChannelName ( const int iDiD ) { return channelInfosOutput[iDiD].name; }
    // virtual void    SetLeftOutputChannel  ( const int iNewChan );
    // virtual void    SetRightOutputChannel ( const int iNewChan );
    // virtual int     GetLeftOutputChannel()  { return vSelectedOutputChannels[0]; }
    // virtual int     GetRightOutputChannel() { return vSelectedOutputChannels[1]; }

    virtual float   GetInOutLatencyMs() { return fInOutLatencyMs; }

protected:
    virtual QString  LoadAndInitializeDriver ( QString inDevName,
                                               QString outDevName,
                                               bool    bOpenDriverSetup );
    virtual void     UnloadCurrentDriver();
    float            fInOutLatencyMs;

    CVector<int16_t> vecsMultChanAudioSndCrd;

    static int paStreamCallback(const void *input, void *output, unsigned long frameCount, const PaStreamCallbackTimeInfo *timeInfo, PaStreamCallbackFlags statusFlags, void *userData);

    PaHostApiIndex selectedApi;
    CVector<const PaDeviceInfo*> paDeviceInfos;
    PaStream* paStream;
    int paStreamInChannels, paStreamOutChannels;
    QString selectedInDev, selectedOutDev;
};
