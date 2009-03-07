/******************************************************************************\
 * Copyright (c) 2004-2009
 *
 * Author(s):
 *  Volker Fischer, Alexander Kurpiers
 *
 * This code is based on the Open-Source sound interface implementation of
 * the Dream DRM Receiver project.
 *
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
# if USE_JACK
#  include <jack/jack.h>
# else
#  define ALSA_PCM_NEW_HW_PARAMS_API
#  define ALSA_PCM_NEW_SW_PARAMS_API
#  include <alsa/asoundlib.h>
# endif
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
# if USE_JACK

// TODO, see http://jackit.sourceforge.net/cgi-bin/lxr/http/source/example-clients/simple_client.c

# else
class CSound : public CSoundBase
{
public:
    CSound ( void (*fpNewCallback) ( CVector<short>& psData, void* arg ), void* arg ) :
        CSoundBase ( false, fpNewCallback, arg ), rhandle ( NULL ),
        phandle ( NULL ), iCurPeriodSizeIn ( NUM_PERIOD_BLOCKS_IN ),
        iCurPeriodSizeOut ( NUM_PERIOD_BLOCKS_OUT ), bChangParamIn ( true ),
        bChangParamOut ( true ) {}
    virtual ~CSound() { Close(); }

    // not implemented yet, always return one device and default string
    int         GetNumDev() { return 1; }
    std::string GetDeviceName ( const int iDiD ) { return "wave mapper"; }
    int         SetDev ( const int iNewDev ) {} // dummy
    int         GetDev() { return 0; }

    virtual int Init ( const int iNewPrefMonoBufferSize )
    {
        // init base class
        CSoundBase::Init ( iNewPrefMonoBufferSize );

        // set internal buffer size for read and write
        iBufferSizeIn  = iNewPrefMonoBufferSize;
        iBufferSizeOut = iNewPrefMonoBufferSize;

        InitRecording();
        InitPlayback();

        return iNewPrefMonoBufferSize;
    }
    virtual bool Read  ( CVector<short>& psData );
    virtual bool Write ( CVector<short>& psData );

protected:
    void Close();
    void InitRecording();
    void InitPlayback();

    snd_pcm_t* rhandle;
    snd_pcm_t* phandle;

    bool SetHWParams ( snd_pcm_t* handle, const int iBufferSizeIn,
                       const int iNumPeriodBlocks );

    int iBufferSizeOut;
    int iBufferSizeIn;
    bool bChangParamIn;
    int iCurPeriodSizeIn;
    bool bChangParamOut;
    int iCurPeriodSizeOut;
};
# endif // USE_JACK
#else
// no sound -> dummy class definition
class CSound : public CSoundBase
{
public:
    CSound ( void (*fpNewCallback) ( CVector<short>& psData, void* arg ), void* arg ) :
        CSoundBase ( false, fpNewCallback, arg ) {}
    virtual ~CSound() { Close(); }

    // not used
    int         GetNumDev() { return 1; }
    std::string GetDeviceName ( const int iDiD ) { return "wave mapper"; }
    int         SetDev ( const int iNewDev ) {} // dummy
    int         GetDev() { return 0; }

    // dummy definitions
    virtual int  Init  ( const int iNewPrefMonoBufferSize ) { CSoundBase::Init ( iNewPrefMonoBufferSize ); }
    virtual bool Read  ( CVector<short>& psData ) { printf ( "no sound!" ); return false; }
    virtual bool Write ( CVector<short>& psData ) { printf ( "no sound!" ); return false; }
};
#endif // WITH_SOUND

#endif // !defined(_SOUND_H__9518A621345F78_3634567_8C0D_EEBF182CF549__INCLUDED_)
