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
