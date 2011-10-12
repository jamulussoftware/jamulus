/******************************************************************************\
 * Copyright (c) 2004-2011
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

#include "soundbase.h"


/* Implementation *************************************************************/
CSoundBase::CSoundBase ( const bool bNewIsCallbackAudioInterface,
                         void (*fpNewProcessCallback) ( CVector<int16_t>& psData, void* pParg ),
                         void* pParg ) :
    fpProcessCallback ( fpNewProcessCallback ),
    pProcessCallbackArg ( pParg ), bRun ( false ),
    bIsCallbackAudioInterface ( bNewIsCallbackAudioInterface )
{
    // initializations for the sound card names (default)
    lNumDevs          = 1;
    strDriverNames[0] = "Default";

    // set current device
    lCurDev = 0; // default device
}

int CSoundBase::Init ( const int iNewPrefMonoBufferSize )
{
    // init audio sound card buffer
    if ( !bIsCallbackAudioInterface )
    {
        vecsAudioSndCrdStereo.Init ( 2 * iNewPrefMonoBufferSize /* stereo */ );
    }

    return iNewPrefMonoBufferSize;
}

void CSoundBase::Start()
{
    bRun = true;

// TODO start audio interface

    // start the audio thread in case we do not have an callback
    // based audio interface
    if ( !bIsCallbackAudioInterface )
    {
        start();
    }
}

void CSoundBase::Stop()
{
    // set flag so that thread can leave the main loop
    bRun = false;

    // give thread some time to terminate
    if ( !bIsCallbackAudioInterface )
    {
        wait ( 5000 );
    }
}

void CSoundBase::run()
{
    // Set thread priority (The working thread should have a higher
    // priority than the GUI)
#ifdef _WIN32
    SetThreadPriority ( GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL );
#else
/*
    // set the process to realtime privs, taken from
    // "http://www.gardena.net/benno/linux/audio" but does not seem to work,
    // maybe a problem with user rights
    struct sched_param schp;
    memset ( &schp, 0, sizeof ( schp ) );
    schp.sched_priority = sched_get_priority_max ( SCHED_FIFO );
    sched_setscheduler ( 0, SCHED_FIFO, &schp );
*/
#endif

    // main loop of working thread
    while ( bRun )
    {
        // get audio from sound card (blocking function)
        if ( Read ( vecsAudioSndCrdStereo ) )
        {
            PostWinMessage ( MS_SOUND_IN, MUL_COL_LED_RED );
        }
        else
        {
            PostWinMessage ( MS_SOUND_IN, MUL_COL_LED_GREEN );
        }

        // process audio data
        (*fpProcessCallback) ( vecsAudioSndCrdStereo, pProcessCallbackArg );

        // play the new block
        if ( Write ( vecsAudioSndCrdStereo ) )
        {
            PostWinMessage ( MS_SOUND_OUT, MUL_COL_LED_RED );
        }
        else
        {
            PostWinMessage ( MS_SOUND_OUT, MUL_COL_LED_GREEN );
        }
    }
}
