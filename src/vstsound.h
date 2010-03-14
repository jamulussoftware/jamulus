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

#if !defined ( _VSTSOUND_H__9518A346345768_11D3_8C0D_EEBF182CF549__INCLUDED_ )
#define _VSTSOUND_H__9518A346345768_11D3_8C0D_EEBF182CF549__INCLUDED_

#include "../src/util.h"
#include "../src/global.h"
#include "../src/soundbase.h"


/* Classes ********************************************************************/
class CVSTSound : public CSoundBase
{
public:
    CSound ( void (*fpNewCallback) ( CVector<int16_t>& psData, void* arg ), void* arg );
    virtual ~CSound() {}

    // these functions are not actually used -> dummies
    virtual int  Init ( const int ) {}
    virtual void Start() {}
    virtual void Stop() {}
    virtual void OpenDriverSetup() {}
    int          GetNumDev() { return 1; }
    QString      GetDeviceName ( const int iDiD ) { return "VST"; }
    QString      SetDev ( const int iNewDev ) {}
    int          GetDev() { return 0; }

protected:
};

#endif // !defined ( _VSTSOUND_H__9518A346345768_11D3_8C0D_EEBF182CF549__INCLUDED_ )
