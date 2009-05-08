/******************************************************************************\
 * Copyright (c) 2004-2009
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

#if !defined ( MIXERBOARD_H__FD6B49E1606C2AC__INCLUDED_ )
#define MIXERBOARD_H__FD6B49E1606C2AC__INCLUDED_

#include <qgroupbox.h>
#include <qlabel.h>
#include <qcheckbox.h>
#include <qlayout.h>
#include <qstring.h>
#include <qslider.h>
#include <qsizepolicy.h>
#include <qhostaddress.h>
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
    CChannelFader ( QWidget* pNW, QHBoxLayout* pParentLayout );
    ~CChannelFader()
    {
        pLabel->close();
        pcbMute->close();
        pcbSolo->close();
        pFader->close();

        // TODO get rid of pMainGrid
    }

    void SetText ( const QString sText );
    void Show() { pLabel->show(); pcbMute->show(); pcbSolo->show(); pFader->show(); }
    void Hide() { pLabel->hide(); pcbMute->hide(); pcbSolo->hide(); pFader->hide(); }
    bool IsVisible() { return pLabel->isVisible(); }

    void Reset();

protected:
    double CalcFaderGain ( const int value );

    QVBoxLayout*    pMainGrid;
    QSlider*        pFader;
    QCheckBox*      pcbMute;
    QCheckBox*      pcbSolo;
    QLabel*         pLabel;

public slots:
    void OnFaderValueChanged ( int value );
    void OnMuteStateChanged ( int value );

signals:
    void faderValueChanged ( double value );
};


class CAudioMixerBoard : public QGroupBox
{
    Q_OBJECT

public:
    CAudioMixerBoard ( QWidget* parent = 0, Qt::WindowFlags f = 0 );

    void HideAll();
    void ApplyNewConClientList ( CVector<CChannelShortInfo>& vecChanInfo );

protected:
    QString GenFaderText ( CChannelShortInfo& ChanInfo );

    CVector<CChannelFader*> vecpChanFader;
    QHBoxLayout*            pMainLayout;

public slots:
    // CODE TAG: MAX_NUM_CHANNELS_TAG
    // make sure we have MAX_NUM_CHANNELS connections!!!
    void OnFaderValueChangedCh0 ( double dValue ) { emit ChangeChanGain ( 0, dValue ); }
    void OnFaderValueChangedCh1 ( double dValue ) { emit ChangeChanGain ( 1, dValue ); }
    void OnFaderValueChangedCh2 ( double dValue ) { emit ChangeChanGain ( 2, dValue ); }
    void OnFaderValueChangedCh3 ( double dValue ) { emit ChangeChanGain ( 3, dValue ); }
    void OnFaderValueChangedCh4 ( double dValue ) { emit ChangeChanGain ( 4, dValue ); }
    void OnFaderValueChangedCh5 ( double dValue ) { emit ChangeChanGain ( 5, dValue ); }
    void OnFaderValueChangedCh6 ( double dValue ) { emit ChangeChanGain ( 6, dValue ); }
    void OnFaderValueChangedCh7 ( double dValue ) { emit ChangeChanGain ( 7, dValue ); }
    void OnFaderValueChangedCh8 ( double dValue ) { emit ChangeChanGain ( 8, dValue ); }
    void OnFaderValueChangedCh9 ( double dValue ) { emit ChangeChanGain ( 9, dValue ); }

signals:
    void ChangeChanGain ( int iId, double dGain );
};

#endif // MIXERBOARD_H__FD6B49E1606C2AC__INCLUDED_
