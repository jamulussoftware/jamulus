/******************************************************************************\
 * Copyright (c) 2004-2022
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

#ifndef JACK_ON_WINDOWS // these headers are not available in Windows OS
#    include <unistd.h>
#    include <sys/ioctl.h>
#endif
#include <fcntl.h>
#include <sys/types.h>
#include <stdlib.h>
#include <stdio.h>
#include <QThread>
#include <string.h>
#include "../../util.h"
#include "../soundbase.h"
#include "../../global.h"

#if WITH_JACK
#    include <jack/jack.h>
#    include <jack/midiport.h>
#endif

/* Definitions ****************************************************************/
#define NUM_IN_OUT_CHANNELS 2 // always stereo

// the number of periods is critical for latency
#define NUM_PERIOD_BLOCKS_IN  2
#define NUM_PERIOD_BLOCKS_OUT 1

#define MAX_SND_BUF_IN  200
#define MAX_SND_BUF_OUT 200

/* Classes ********************************************************************/
#if WITH_JACK
class CSound : public CSoundBase
{
    Q_OBJECT

public:
    CSound ( void ( *fpNewProcessCallback ) ( CVector<short>& psData, void* arg ),
             void*          arg,
             const QString& strMIDISetup,
             const bool     bNoAutoJackConnect,
             const QString& strJackClientName ) :
        CSoundBase ( "Jack", fpNewProcessCallback, arg, strMIDISetup ),
        iJACKBufferSizeMono ( 0 ),
        bJackWasShutDown ( false ),
        fInOutLatencyMs ( 0.0f )
    {
        QString strJackName = QString ( APP_NAME );

        if ( !strJackClientName.isEmpty() )
        {
            strJackName += " " + strJackClientName;
        }

        OpenJack ( bNoAutoJackConnect, strJackName.toLocal8Bit().data() );
    }

    virtual ~CSound() { CloseJack(); }

    virtual int  Init ( const int iNewPrefMonoBufferSize );
    virtual void Start();
    virtual void Stop();

    virtual float GetInOutLatencyMs() { return fInOutLatencyMs; }

    // these variables should be protected but cannot since we want
    // to access them from the callback function
    CVector<short> vecsTmpAudioSndCrdStereo;
    int            iJACKBufferSizeMono;
    int            iJACKBufferSizeStero;
    bool           bJackWasShutDown;

    jack_port_t* input_port_left;
    jack_port_t* input_port_right;
    jack_port_t* output_port_left;
    jack_port_t* output_port_right;
    jack_port_t* input_port_midi;

protected:
    void OpenJack ( const bool bNoAutoJackConnect, const char* jackClientName );

    void CloseJack();

    // callbacks
    static int     process ( jack_nframes_t nframes, void* arg );
    static int     bufferSizeCallback ( jack_nframes_t, void* arg );
    static void    shutdownCallback ( void* );
    jack_client_t* pJackClient;

    float fInOutLatencyMs;
};
#else
// no sound -> dummy class definition
class CSound : public CSoundBase
{
    Q_OBJECT

public:
    CSound ( void ( *fpNewProcessCallback ) ( CVector<short>& psData, void* pParg ),
             void*          pParg,
             const QString& strMIDISetup,
             const bool,
             const QString& ) :
        CSoundBase ( "nosound", fpNewProcessCallback, pParg, strMIDISetup ),
        HighPrecisionTimer ( true )
    {
        HighPrecisionTimer.Start();
        QObject::connect ( &HighPrecisionTimer, &CHighPrecisionTimer::timeout, this, &CSound::OnTimer );
    }
    virtual ~CSound() {}
    virtual int Init ( const int iNewPrefMonoBufferSize )
    {
        CSoundBase::Init ( iNewPrefMonoBufferSize );
        vecsTemp.Init ( 2 * iNewPrefMonoBufferSize );
        return iNewPrefMonoBufferSize;
    }
    CHighPrecisionTimer HighPrecisionTimer;
    CVector<short>      vecsTemp;

public slots:
    void OnTimer()
    {
        vecsTemp.Reset ( 0 );
        if ( IsRunning() )
        {
            ProcessCallback ( vecsTemp );
        }
    }
};
#endif // WITH_JACK
