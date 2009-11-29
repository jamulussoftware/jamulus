/******************************************************************************\
 * Copyright (c) 2004-2009
 *
 * Author(s):
 *  Volker Fischer
\******************************************************************************/

#if !defined(_SOUND_H__9518A621345F78_3634567_8C0D_EEBF182CF549__INCLUDED_)
#define _SOUND_H__9518A621345F78_3634567_8C0D_EEBF182CF549__INCLUDED_

#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <stdlib.h>
#include <stdio.h>
#include <qthread.h>
#include <string.h>
#include "util.h"
#include "soundbase.h"
#include "global.h"

#if WITH_SOUND
# include <jack/jack.h>
#endif


/* Definitions ****************************************************************/
#define NUM_IN_OUT_CHANNELS         2 // always stereo

// the number of periods is critical for latency
#define NUM_PERIOD_BLOCKS_IN        2
#define NUM_PERIOD_BLOCKS_OUT       1

#define MAX_SND_BUF_IN              200
#define MAX_SND_BUF_OUT             200


/* Classes ********************************************************************/
#if WITH_SOUND
class CSound : public CSoundBase
{
public:
    CSound ( void (*fpNewProcessCallback) ( CVector<short>& psData, void* arg ), void* arg ) :
        CSoundBase ( true, fpNewProcessCallback, arg ) { OpenJack(); }
    virtual ~CSound() { CloseJack(); }

    virtual int  Init  ( const int iNewPrefMonoBufferSize );
    virtual void Start();
    virtual void Stop();

    // not implemented yet, always return one device and default string
    int         GetNumDev() { return 1; }
    std::string GetDeviceName ( const int iDiD ) { return "wave mapper"; }
    std::string SetDev ( const int iNewDev ) { return ""; } // dummy
    int         GetDev() { return 0; }

    // these variables should be protected but cannot since we want
    // to access them from the callback function
    CVector<short> vecsTmpAudioSndCrdStereo;
    int            iJACKBufferSizeMono;
    int            iJACKBufferSizeStero;

    jack_port_t*   input_port_left;
    jack_port_t*   input_port_right;
    jack_port_t*   output_port_left;
    jack_port_t*   output_port_right;

protected:
    void OpenJack();
    void CloseJack();

    // callbacks
    static int      process ( jack_nframes_t nframes, void* arg );
    static int      bufferSizeCallback ( jack_nframes_t nframes, void *arg );
    static void     shutdownCallback ( void *arg );
    jack_client_t*  pJackClient;
};
#else
// no sound -> dummy class definition
class CSound : public CSoundBase
{
public:
    CSound ( void (*fpNewProcessCallback) ( CVector<short>& psData, void* pParg ), void* pParg ) :
        CSoundBase ( false, fpNewProcessCallback, pParg ) {}
    virtual ~CSound() { Close(); }

    // not used
    int         GetNumDev() { return 1; }
    std::string GetDeviceName ( const int iDiD ) { return "wave mapper"; }
    std::string SetDev ( const int iNewDev ) { return ""; } // dummy
    int         GetDev() { return 0; }

    // dummy definitions
    virtual int  Init  ( const int iNewPrefMonoBufferSize ) { CSoundBase::Init ( iNewPrefMonoBufferSize ); }
    virtual bool Read  ( CVector<short>& psData ) { printf ( "no sound!" ); return false; }
    virtual bool Write ( CVector<short>& psData ) { printf ( "no sound!" ); return false; }
};
#endif // WITH_SOUND

#endif // !defined(_SOUND_H__9518A621345F78_3634567_8C0D_EEBF182CF549__INCLUDED_)
