/******************************************************************************\
 * Copyright (c) 2021
 *
 * Author(s):
 *  Noam Postavsky
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

#include <cstdlib>
#include <cstdio>
#include <QThread>
#include <cstring>
#include "util.h"
#include "soundbase.h"
#include "global.h"

#ifdef WIN32
// copy the ASIO SDK in the llcon/windows directory: "llcon/windows/ASIOSDK2" to
// get it work
#    include "asiosys.h"
#    include "asio.h"
#    include "asiodrivers.h"
#endif // WIN32

#include <portaudio.h>

class CSound : public CSoundBase
{
public:
    CSound ( void ( *fpNewProcessCallback ) ( CVector<short>& psData, void* arg ),
             void*          arg,
             const QString& strMIDISetup,
             const bool,
             const QString& );
    virtual ~CSound();

    virtual int  Init ( const int iNewPrefMonoBufferSize );
    virtual void Start();
    virtual void Stop();

#ifdef WIN32
    // Portaudio's function for this takes a device index as a parameter.  So it
    // needs to reopen ASIO in order to get the right driver loaded.  Because of
    // that it also requires passing HWND of the main window.  This is fairly
    // awkward, so just call the ASIO function directly for the currently loaded
    // driver.
    virtual void OpenDriverSetup() { ASIOControlPanel(); }
#endif // WIN32

protected:
    virtual QString LoadAndInitializeDriver ( QString, bool );
    QString         ReinitializeDriver ( int devIndex );
    virtual void    UnloadCurrentDriver();

    PaDeviceIndex DeviceIndexFromName ( const QString& strDriverName );

    static int paStreamCallback ( const void*                     input,
                                  void*                           output,
                                  unsigned long                   frameCount,
                                  const PaStreamCallbackTimeInfo* timeInfo,
                                  PaStreamCallbackFlags           statusFlags,
                                  void*                           userData );

    PaHostApiIndex   asioIndex;
    PaDeviceIndex    deviceIndex;
    PaStream*        deviceStream;
    int              iPrefMonoBufferSize;
    CVector<int16_t> vecsAudioData;
};
