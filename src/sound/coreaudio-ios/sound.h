/******************************************************************************\
 * Copyright (c) 2004-2025
 *
 * Author(s):
 *  ann0see and ngocdh based on code from Volker Fischer
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
#include "../soundbase.h"
#include "../../global.h"

#import <AudioToolbox/AudioToolbox.h>

class CSound : public CSoundBase
{
    Q_OBJECT

public:
    CSound ( void ( *fpNewProcessCallback ) ( CVector<short>& psData, void* arg ),
             void*          arg,
             const QString& strMIDISetup,
             const bool,
             const QString& );
    ~CSound();

    virtual int  Init ( const int iNewPrefMonoBufferSize );
    virtual void Start();
    virtual void Stop();
    virtual void processBufferList ( AudioBufferList*, CSound* );

    AudioUnit audioUnit;

    // these variables/functions should be protected but cannot since we want
    // to access them from the callback function
    CVector<short> vecsTmpAudioSndCrdStereo;
    int            iCoreAudioBufferSizeMono;
    int            iCoreAudioBufferSizeStereo;
    bool           isInitialized;

protected:
    virtual QString LoadAndInitializeDriver ( QString strDriverName, bool );
    void            GetAvailableInOutDevices();
    void            SwitchDevice ( QString strDriverName );

    AudioBuffer     buffer;
    AudioBufferList bufferList;
    void            checkStatus ( int status );
    static OSStatus recordingCallback ( void*                       inRefCon,
                                        AudioUnitRenderActionFlags* ioActionFlags,
                                        const AudioTimeStamp*       inTimeStamp,
                                        UInt32                      inBusNumber,
                                        UInt32                      inNumberFrames,
                                        AudioBufferList*            ioData );

    QMutex Mutex;
};
