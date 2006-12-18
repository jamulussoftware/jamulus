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
class CChannelFader : public QObject
{
    Q_OBJECT

public:
    CChannelFader ( QWidget* pNW, QHBoxLayout* pParentLayout, QString sName );
    ~CChannelFader()
    {
        pLabel->close();
        pFader->close();

        // TODO get rid of pMainGrid
    }

    void SetText ( const std::string sText );
    void Show() { pLabel->show(); pFader->show(); }
    void Hide() { pLabel->hide(); pFader->hide(); }
    bool IsVisible() { return pLabel->isVisible(); }

     // init gain value -> maximum value as definition according to server
    void ResetGain() { pFader->setValue ( 0 ); }

protected:
    QGridLayout*    pMainGrid;
    QSlider*        pFader;
    QLabel*         pLabel;

public slots:
    void OnValueChanged ( int value );

signals:
    void valueChanged ( double value );
};


class CAudioMixerBoard : public QFrame
{
    Q_OBJECT

public:
    CAudioMixerBoard ( QWidget* parent = 0, const char* name = 0, WFlags f = 0 );

    void HideAll();
    void ApplyNewConClientList ( CVector<CChannelShortInfo>& vecChanInfo );

protected:
    std::string GenFaderText ( CChannelShortInfo& ChanInfo );

    CVector<CChannelFader*> vecpChanFader;
    QHBoxLayout*            pMainLayout;

public slots:
    // CODE TAG: MAX_NUM_CHANNELS_TAG
    // make sure we have MAX_NUM_CHANNELS connections!!!
    void OnValueChangedCh0 ( double dValue ) { emit ChangeChanGain ( 0, dValue ); }
    void OnValueChangedCh1 ( double dValue ) { emit ChangeChanGain ( 1, dValue ); }
    void OnValueChangedCh2 ( double dValue ) { emit ChangeChanGain ( 2, dValue ); }
    void OnValueChangedCh3 ( double dValue ) { emit ChangeChanGain ( 3, dValue ); }
    void OnValueChangedCh4 ( double dValue ) { emit ChangeChanGain ( 4, dValue ); }
    void OnValueChangedCh5 ( double dValue ) { emit ChangeChanGain ( 5, dValue ); }
    void OnValueChangedCh6 ( double dValue ) { emit ChangeChanGain ( 6, dValue ); }
    void OnValueChangedCh7 ( double dValue ) { emit ChangeChanGain ( 7, dValue ); }
    void OnValueChangedCh8 ( double dValue ) { emit ChangeChanGain ( 8, dValue ); }
    void OnValueChangedCh9 ( double dValue ) { emit ChangeChanGain ( 9, dValue ); }

signals:
    void ChangeChanGain ( int iId, double dGain );
};


#endif // MIXERBOARD_H__FD6B49E1606C2AC__INCLUDED_
