/******************************************************************************\
 * Copyright (c) 2004-2020
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
#include <QMutex>
#include <QMessageBox>
#include "soundbase.h"
#include "global.h"

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

    virtual int  Init ( const int iNewPrefMonoBufferSize );
    virtual void Start();
    virtual void Stop();
    virtual void processBufferList( AudioBufferList*, CSound* );
    virtual void SetInputDeviceId( int deviceid );

    AudioUnit      audioUnit;

    // these variables/functions should be protected but cannot since we want
    // to access them from the callback function
    CVector<short> vecsTmpAudioSndCrdStereo;
    int            iCoreAudioBufferSizeMono;
    int            iCoreAudioBufferSizeStereo;
    bool isInitialized;
    

protected:
    MIDIPortRef    midiInPortRef;

    QMutex         Mutex;
};
