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

#include "util.h"
#include "soundbase.h"
#include "global.h"


/* Classes ********************************************************************/
class CSound : public CSoundBase
{
public:
    CSound ( void (*fpNewProcessCallback) ( CVector<short>& psData, void* arg ), void* arg ) :
        CSoundBase ( true, fpNewProcessCallback, arg ) { OpenJack(); }
    virtual ~CSound() {}

    virtual int  Init  ( const int iNewPrefMonoBufferSize );
    virtual void Start();
    virtual void Stop();

    // not implemented yet, always return one device and default string
    int     GetNumDev() { return 1; }
    QString GetDeviceName ( const int iDiD ) { return "CoreAudio"; }
    QString SetDev ( const int iNewDev ) { return ""; } // dummy
    int     GetDev() { return 0; }

protected:
};

#endif // !defined(_SOUND_H__9518A621345F78_363456876UZGSDF82CF549__INCLUDED_)
