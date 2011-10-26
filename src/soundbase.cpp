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
CSoundBase::CSoundBase ( const QString& strNewSystemDriverTechniqueName,
                         const bool bNewIsCallbackAudioInterface,
                         void (*fpNewProcessCallback) ( CVector<int16_t>& psData, void* pParg ),
                         void* pParg ) :
    fpProcessCallback ( fpNewProcessCallback ),
    pProcessCallbackArg ( pParg ), bRun ( false ),
    bIsCallbackAudioInterface ( bNewIsCallbackAudioInterface ),
    strSystemDriverTechniqueName ( strNewSystemDriverTechniqueName )
{
    // initializations for the sound card names (default)
    lNumDevs          = 1;
    strDriverNames[0] = strSystemDriverTechniqueName;

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


/******************************************************************************\
* Device handling                                                              *
\******************************************************************************/
QString CSoundBase::SetDev ( const int iNewDev )
{
    QString strReturn         = ""; // init with no error
    bool    bTryLoadAnyDriver = false;

    // check if an ASIO driver was already initialized
    if ( lCurDev >= 0 )
    {
        // a device was already been initialized and is used, first clean up
        // driver
        UnloadCurrentDriver();

        const QString strErrorMessage = LoadAndInitializeDriver ( iNewDev );

        if ( !strErrorMessage.isEmpty() )
        {
            if ( iNewDev != lCurDev )
            {
                // loading and initializing the new driver failed, go back to
                // original driver and display error message
                LoadAndInitializeDriver ( lCurDev );
            }
            else
            {
                // the same driver is used but the driver properties seems to
                // have changed so that they are not compatible to our
                // software anymore
                QMessageBox::critical (
                    0, APP_NAME, QString ( tr ( "The audio driver properties "
                    "have changed to a state which is incompatible to this "
                    "software. The selected audio device could not be used "
                    "because of the following error: <b>" ) ) +
                    strErrorMessage +
                    QString ( tr ( "</b><br><br>Please restart the software." ) ),
                    "Close", 0 );

                _exit ( 0 );
            }

            // store error return message
            strReturn = strErrorMessage;
        }
    }
    else
    {
        if ( iNewDev != INVALID_SNC_CARD_DEVICE )
        {
            // This is the first time a driver is to be initialized, we first
            // try to load the selected driver, if this fails, we try to load
            // the first available driver in the system. If this fails, too, we
            // throw an error that no driver is available -> it does not make
            // sense to start the llcon software if no audio hardware is
            // available.
            if ( !LoadAndInitializeDriver ( iNewDev ).isEmpty() )
            {
                // loading and initializing the new driver failed, try to find
                // at least one usable driver
                bTryLoadAnyDriver = true;
            }
        }
        else
        {
            // try to find one usable driver (select the first valid driver)
            bTryLoadAnyDriver = true;
        }
    }

    if ( bTryLoadAnyDriver )
    {
        // try to load and initialize any valid driver
        QVector<QString> vsErrorList =
            LoadAndInitializeFirstValidDriver();

        if ( !vsErrorList.isEmpty() )
        {
            // create error message with all details
            QString sErrorMessage = tr ( "<b>No usable " ) +
                strSystemDriverTechniqueName + tr ( " audio device "
                "(driver) found.</b><br><br>"
                "In the following there is a list of all available drivers "
                "with the associated error message:<ul>" );

            for ( int i = 0; i < lNumDevs; i++ )
            {
                sErrorMessage += "<li><b>" + GetDeviceName ( i ) + "</b>: " +
                    vsErrorList[i] + "</li>";
            }
            sErrorMessage += "</ul>";

            throw CGenErr ( sErrorMessage );
        }
    }

    return strReturn;
}

QVector<QString> CSoundBase::LoadAndInitializeFirstValidDriver()
{
    QVector<QString> vsErrorList;

    // load and initialize first valid ASIO driver
    bool bValidDriverDetected = false;
    int  iCurDriverIdx = 0;

    // try all available drivers in the system ("lNumDevs" devices)
    while ( !bValidDriverDetected && ( iCurDriverIdx < lNumDevs ) )
    {
        // try to load and initialize current driver, store error message
        const QString strCurError =
            LoadAndInitializeDriver ( iCurDriverIdx );

        vsErrorList.append ( strCurError );

        if ( strCurError.isEmpty() )
        {
            // initialization was successful
            bValidDriverDetected = true;

            // store ID of selected driver
            lCurDev = iCurDriverIdx;

            // empty error list shows that init was successful
            vsErrorList.clear();
        }

        // try next driver
        iCurDriverIdx++;
    }

    return vsErrorList;
}
