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

#if !defined ( SOUNDBASE_HOIHGEH8_3_4344456456345634565KJIUHF1912__INCLUDED_ )
#define SOUNDBASE_HOIHGEH8_3_4344456456345634565KJIUHF1912__INCLUDED_

#include <qthread.h>
#include "global.h"
#include "util.h"


/* Classes ********************************************************************/
class CSoundBase : public QThread
{
    Q_OBJECT

public:
    CSoundBase ( const bool bNewIsCallbackAudioInterface,
        void (*fpNewProcessCallback) ( CVector<int16_t>& psData, void* pParg ),
        void* pParg ) : fpProcessCallback ( fpNewProcessCallback ),
        pProcessCallbackArg ( pParg ), bRun ( false ),
        bIsCallbackAudioInterface ( bNewIsCallbackAudioInterface ) {}
    virtual ~CSoundBase() {}

    virtual int  Init ( const int iNewPrefMonoBufferSize );
    virtual void Start();
    virtual void Stop();

    // dummy implementations in base class
    virtual int     GetNumDev()                 { return 1; }
    virtual QString GetDeviceName ( const int ) { return "Default"; }
    virtual QString SetDev ( const int )        { return ""; }
    virtual int     GetDev()                    { return 0; }

    virtual int     GetNumInputChannels() { return 2; }
    virtual QString GetInputChannelName ( const int ) { return "Default"; }
    virtual void    SetLeftInputChannel  ( const int ) {}
    virtual void    SetRightInputChannel ( const int ) {}
    virtual int     GetLeftInputChannel()  { return 0; }
    virtual int     GetRightInputChannel() { return 1; }

    virtual int     GetNumOutputChannels() { return 2; }
    virtual QString GetOutputChannelName ( const int ) { return "Default"; }
    virtual void    SetLeftOutputChannel  ( const int ) {}
    virtual void    SetRightOutputChannel ( const int ) {}
    virtual int     GetLeftOutputChannel()  { return 0; }
    virtual int     GetRightOutputChannel() { return 1; }

    virtual void    OpenDriverSetup() {}

    bool IsRunning() const { return bRun; }

    // TODO this should be protected but since it is used
    // in a callback function it has to be public -> better solution
    void EmitReinitRequestSignal() { emit ReinitRequest(); }

protected:
    // function pointer to callback function
    void (*fpProcessCallback) ( CVector<int16_t>& psData, void* arg );
    void* pProcessCallbackArg;

    // callback function call for derived classes
    void ProcessCallback ( CVector<int16_t>& psData )
    {
        (*fpProcessCallback) ( psData, pProcessCallbackArg );
    }

    // these functions should be overwritten by derived class for
    // non callback based audio interfaces
    virtual bool Read  ( CVector<int16_t>& ) { printf ( "no sound!" ); return false; }
    virtual bool Write ( CVector<int16_t>& ) { printf ( "no sound!" ); return false; }

    void run();
    bool bRun;
    bool bIsCallbackAudioInterface;

    CVector<int16_t> vecsAudioSndCrdStereo;

signals:
    void ReinitRequest();
};

#endif /* !defined ( SOUNDBASE_HOIHGEH8_3_4344456456345634565KJIUHF1912__INCLUDED_ ) */
