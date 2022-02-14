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

#if !defined( LLCONVST_HOIHGE76G34528_3_434DFGUHF1912__INCLUDED_ )
#    define LLCONVST_HOIHGE76G34528_3_434DFGUHF1912__INCLUDED_

// copy the VST SDK in the llcon/windows directory: "llcon/windows/vstsdk2.4" to
// get it work
#    include "audioeffectx.h"
#    include <qtimer.h>
#    include "global.h"
#    include "client.h"

/* Definitions ****************************************************************/
// timeout after which the llcon client is stopped
#    define VST_STOP_TIMER_INTERVAL 1000

/* Classes ********************************************************************/
class CLlconVST : public QObject, public AudioEffectX
{
    Q_OBJECT

public:
    CLlconVST ( audioMasterCallback AudioMaster );

    virtual void processReplacing ( float** pvIn, float** pvOut, VstInt32 iNumSamples );

    virtual void setProgramName ( char* cName ) { vst_strncpy ( strProgName, cName, kVstMaxProgNameLen ); }
    virtual void getProgramName ( char* cName ) { vst_strncpy ( cName, strProgName, kVstMaxProgNameLen ); }

    virtual bool     getEffectName ( char* cString ) { return GetName ( cString ); }
    virtual bool     getVendorString ( char* cString ) { return GetName ( cString ); }
    virtual bool     getProductString ( char* cString ) { return GetName ( cString ); }
    virtual VstInt32 getVendorVersion() { return 1000; }

protected:
    bool GetName ( char* cName );
    char strProgName[kVstMaxProgNameLen + 1];

    CClient Client;
    QTimer  TimerOnOff;

protected slots:
    void OnTimerOnOff();
};

#endif /* !defined ( LLCONVST_HOIHGE76G34528_3_434DFGUHF1912__INCLUDED_ ) */
