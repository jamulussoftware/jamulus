/******************************************************************************\
 * Copyright (c) 2004-2010
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

#if !defined(_SOUND_H__9518A621345F78_363456876UZGSDF82CF549__INCLUDED_)
#define _SOUND_H__9518A621345F78_363456876UZGSDF82CF549__INCLUDED_

#include <CoreServices/CoreServices.h>
#include <AudioUnit/AudioUnit.h>
#include <qmutex.h>
#include "util.h"
#include "soundbase.h"
#include "global.h"


/* Classes ********************************************************************/
class CSound : public CSoundBase
{
public:
    CSound ( void (*fpNewProcessCallback) ( CVector<short>& psData, void* arg ), void* arg ) :
        CSoundBase ( true, fpNewProcessCallback, arg ) { OpenCoreAudio(); }
    virtual ~CSound() { CloseCoreAudio(); }

    virtual int  Init  ( const int iNewPrefMonoBufferSize );
    virtual void Start();
    virtual void Stop();

    // not implemented yet, always return one device and default string
    int     GetNumDev()                 { return 1; }
    QString GetDeviceName ( const int ) { return "CoreAudio"; }
    QString SetDev ( const int )        { return ""; } // dummy
    int     GetDev()                    { return 0; }

    // these variables should be protected but cannot since we want
    // to access them from the callback function
    CVector<short> vecsTmpAudioSndCrdStereo;
    int            iCoreAudioBufferSizeMono;
    int            iCoreAudioBufferSizeStero;

protected:
    void OpenCoreAudio();
    void CloseCoreAudio();
    UInt32  SetBufferSize ( AudioDeviceID& audioDeviceID, const bool bIsInput,
                            UInt32 iPrefBufferSize );

    // callbacks
    static OSStatus processInput ( void* inRefCon,AudioUnitRenderActionFlags* ioActionFlags,
        const AudioTimeStamp* inTimeStamp, UInt32 inBusNumber, UInt32 inNumberFrames,
        AudioBufferList* ioData );
		
    static OSStatus processOutput ( void* inRefCon,AudioUnitRenderActionFlags* ioActionFlags,
        const AudioTimeStamp* inTimeStamp, UInt32 inBusNumber, UInt32 inNumberFrames,
        AudioBufferList* ioData );		

    ComponentInstance audioInputUnit;
    AudioDeviceID     audioInputDevice;
    ComponentInstance audioOutputUnit;
    AudioDeviceID     audioOutputDevice;

    AudioBufferList*  pBufferList;

    QMutex            Mutex;
};

#endif // !defined(_SOUND_H__9518A621345F78_363456876UZGSDF82CF549__INCLUDED_)
