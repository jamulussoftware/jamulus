/******************************************************************************\
 * Copyright (c) 2004-2006
 *
 * Author(s):
 *  Volker Fischer
 *
 * Description:
 *
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

#if !defined(MIXERBOARD_H__FD6B49E1606C2AC__INCLUDED_)
#define MIXERBOARD_H__FD6B49E1606C2AC__INCLUDED_

#include <qframe.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qstring.h>
#include <qslider.h>
#include <qsizepolicy.h>
#include "global.h"
#include "util.h"


/* Definitions ****************************************************************/
// audio mixer fader range
#define AUD_MIX_FADER_MAX           100


/* Classes ********************************************************************/
class CChannelFader
{
public:
    CChannelFader ( QWidget* pNW, QHBoxLayout* pNPtLy, QString sName );
    ~CChannelFader()
    {
        pLabel->close();
        pFader->close();

        // TODO get rid of pMainGrid
    }

    void SetText ( const std::string sText );
    void Show() { pLabel->show(); pFader->show(); }
    void Hide() { pLabel->hide(); pFader->hide(); }

protected:
    QGridLayout*    pMainGrid;
    QSlider*        pFader;
    QLabel*         pLabel;

    QHBoxLayout*    pParentLayout;
};


class CAudioMixerBoard : public QFrame
{
    Q_OBJECT

public:
    CAudioMixerBoard ( QWidget* parent = 0, const char* name = 0, WFlags f = 0 );

    void Clear();
    void ApplyNewConClientList ( CVector<CChannelShortInfo>& vecChanInfo );

protected:
    CVector<CChannelFader*> vecpChanFader;
    QHBoxLayout*            pMainLayout;

signals:
    void ChangeChanGain ( int iId, double dGain );
};


#endif // MIXERBOARD_H__FD6B49E1606C2AC__INCLUDED_
