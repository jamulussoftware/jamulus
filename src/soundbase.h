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

#include <QThread>
#include <QString>
#include <QMessageBox>
#include "global.h"
#include "util.h"


// TODO better solution with enum definition
// problem: in signals it seems not to work to use CSoundBase::ESndCrdResetType
enum ESndCrdResetType
{
    RS_ONLY_RESTART = 1,
    RS_ONLY_RESTART_AND_INIT,
    RS_RELOAD_RESTART_AND_INIT
};


/* Classes ********************************************************************/
class CSoundBase : public QThread
{
    Q_OBJECT

public:
    CSoundBase ( const QString& strNewSystemDriverTechniqueName,
                 const bool     bNewIsCallbackAudioInterface,
                 void           (*fpNewProcessCallback) ( CVector<int16_t>& psData, void* pParg ),
                 void*          pParg,
                 const int      iNewCtrlMIDIChannel );

    virtual int  Init ( const int iNewPrefMonoBufferSize );
    virtual void Start();
    virtual void Stop();

    // device selection
    virtual int     GetNumDev() { return lNumDevs; }
    virtual QString GetDeviceName ( const int iDiD ) { return strDriverNames[iDiD]; }
    virtual QString SetDev ( const int );
    virtual int     GetDev() { return lCurDev; }

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

    virtual double  GetInOutLatencyMs() { return 0.0; } // "0.0" means no latency is available

    virtual void    OpenDriverSetup() {}

    bool IsRunning() const { return bRun; }

    // TODO this should be protected but since it is used
    // in a callback function it has to be public -> better solution
    void EmitReinitRequestSignal ( const ESndCrdResetType eSndCrdResetType )
        { emit ReinitRequest ( eSndCrdResetType ); }

    void EmitControllerInFaderLevel ( const int iChannelIdx,
                                      const int iValue ) { emit ControllerInFaderLevel ( iChannelIdx, iValue ); }

protected:
    // driver handling
    virtual QString  LoadAndInitializeDriver ( int, bool ) { return ""; }
    virtual void     UnloadCurrentDriver() {}
    QVector<QString> LoadAndInitializeFirstValidDriver ( const bool bOpenDriverSetup = false );

    static void GetSelCHAndAddCH ( const int iSelCH,    const int iNumInChan,
                                   int&      iSelCHOut, int&      iSelAddCHOut )
    {
        // we have a mixed channel setup, definitions:
        // - mixed channel setup only for 4 physical inputs:
        //   SelCH == 4: Ch 0 + Ch 2
        //   SelCh == 5: Ch 0 + Ch 3
        //   SelCh == 6: Ch 1 + Ch 2
        //   SelCh == 7: Ch 1 + Ch 3
        if ( iSelCH >= iNumInChan )
        {
            iSelAddCHOut = ( ( iSelCH - iNumInChan ) % 2 ) + 2;
            iSelCHOut    = ( iSelCH - iNumInChan ) / 2;
        }
        else
        {
            iSelAddCHOut = -1; // set it to an invalid number
            iSelCHOut    = iSelCH;
        }
    }

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

    void             ParseMIDIMessage ( const CVector<uint8_t>& vMIDIPaketBytes );

    bool             bIsCallbackAudioInterface;
    QString          strSystemDriverTechniqueName;
    int              iCtrlMIDIChannel;

    CVector<int16_t> vecsAudioSndCrdStereo;

    long             lNumDevs;
    long             lCurDev;
    QString          strDriverNames[MAX_NUMBER_SOUND_CARDS];

signals:
    void ReinitRequest ( int iSndCrdResetType );
    void ControllerInFaderLevel ( int iChannelIdx, int iValue );
};
