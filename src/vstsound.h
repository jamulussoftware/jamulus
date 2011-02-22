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

#if !defined ( _VSTSOUND_H__9518A346345768_11D3_8C0D_EEBF182CF549__INCLUDED_ )
#define _VSTSOUND_H__9518A346345768_11D3_8C0D_EEBF182CF549__INCLUDED_

#include "../src/util.h"
#include "../src/global.h"
#include "../src/soundbase.h"


/* Classes ********************************************************************/
class CSound : public CSoundBase
{
public:
    CSound ( void (*fpNewCallback) ( CVector<int16_t>& psData, void* arg ), void* arg ) :
      CSoundBase ( true, fpNewCallback, arg ), iVSTMonoBufferSize ( 0 ) {}

    virtual ~CSound() {}

    // special VST functions
    void SetMonoBufferSize ( const int iNVBS ) { iVSTMonoBufferSize = iNVBS; }
    void VSTProcessCallback()
    {
        CSoundBase::ProcessCallback ( vecsTmpAudioSndCrdStereo );
    }

    virtual int Init ( const int )
    {
        // init base class
        CSoundBase::Init ( iVSTMonoBufferSize );
        vecsTmpAudioSndCrdStereo.Init ( 2 * iVSTMonoBufferSize /* stereo */);
        return iVSTMonoBufferSize;
    }

    // this vector must be accessible from the outside (quick hack solution)
    CVector<int16_t> vecsTmpAudioSndCrdStereo;

protected:
    int iVSTMonoBufferSize;
};

#endif // !defined ( _VSTSOUND_H__9518A346345768_11D3_8C0D_EEBF182CF549__INCLUDED_ )
