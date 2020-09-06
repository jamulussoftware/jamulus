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
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
\******************************************************************************/

#pragma once

#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>
#include <QMutex>
#include "soundbase.h"
#include "global.h"


/* Classes ********************************************************************/
class CSound : public CSoundBase
{
public:
    CSound ( void           (*fpNewProcessCallback) ( CVector<short>& psData, void* arg ),
             void*          arg,
             const int      iCtrlMIDIChannel,
             const bool     ,
             const QString& );
    virtual ~CSound() {}

    virtual int  Init ( const int iNewPrefMonoBufferSize );
    virtual void Start();
    virtual void Stop();

    // these variables should be protected but cannot since we want
    // to access them from the callback function
    CVector<short> vecsTmpAudioSndCrdStereo;

// TEST
CVector<short> vecsTmpAudioInSndCrd;
int            iModifiedInBufSize;

    int            iOpenSLBufferSizeMono;
    int            iOpenSLBufferSizeStereo;

protected:

    void InitializeOpenSL();
    void CloseOpenSL();

    // callbacks
    static void processInput ( SLAndroidSimpleBufferQueueItf bufferQueue,
                               void*                         instance );

    static void processOutput ( SLAndroidSimpleBufferQueueItf bufferQueue,
                                void*                         instance );

    SLObjectItf                   engineObject;
    SLEngineItf                   engine;
    SLObjectItf                   recorderObject;
    SLRecordItf                   recorder;
    SLAndroidSimpleBufferQueueItf recorderSimpleBufQueue;
    SLObjectItf                   outputMixObject;
    SLObjectItf                   playerObject;
    SLPlayItf                     player;
    SLAndroidSimpleBufferQueueItf playerSimpleBufQueue;

    QMutex                        Mutex;

};
