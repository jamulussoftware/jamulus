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
    void SetGUIDesign ( const EGUIDesign eNewDesign );

    void ResetSoloState();
    void SetOtherSoloState ( const bool bState );

    void Reset();

protected:
    double CalcFaderGain ( const int value );
    void   SetMute ( const bool bState );

    QVBoxLayout*    pMainGrid;
    QSlider*        pFader;
    QCheckBox*      pcbMute;
    QCheckBox*      pcbSolo;
    QLabel*         pLabel;

    bool            bOtherChannelIsSolo;

public slots:
    void OnGainValueChanged ( int value );
    void OnMuteStateChanged ( int value );

signals:
    void gainValueChanged ( double value );
    void soloStateChanged ( int value );
};


class CAudioMixerBoard : public QGroupBox
{
    Q_OBJECT

public:
    CAudioMixerBoard ( QWidget* parent = 0, Qt::WindowFlags f = 0 );

    void HideAll();
    void ApplyNewConClientList ( CVector<CChannelShortInfo>& vecChanInfo );
    void SetGUIDesign ( const EGUIDesign eNewDesign );

protected:
    QString GenFaderText ( CChannelShortInfo& ChanInfo );
    void OnChSoloStateChanged ( const int iChannelIdx, const int iValue );

    CVector<CChannelFader*> vecpChanFader;
    QHBoxLayout*            pMainLayout;

public slots:
    // CODE TAG: MAX_NUM_CHANNELS_TAG
    // make sure we have MAX_NUM_CHANNELS connections!!!
    void OnGainValueChangedCh0 ( double dValue ) { emit ChangeChanGain ( 0, dValue ); }
    void OnGainValueChangedCh1 ( double dValue ) { emit ChangeChanGain ( 1, dValue ); }
    void OnGainValueChangedCh2 ( double dValue ) { emit ChangeChanGain ( 2, dValue ); }
    void OnGainValueChangedCh3 ( double dValue ) { emit ChangeChanGain ( 3, dValue ); }
    void OnGainValueChangedCh4 ( double dValue ) { emit ChangeChanGain ( 4, dValue ); }
    void OnGainValueChangedCh5 ( double dValue ) { emit ChangeChanGain ( 5, dValue ); }
    void OnGainValueChangedCh6 ( double dValue ) { emit ChangeChanGain ( 6, dValue ); }
    void OnGainValueChangedCh7 ( double dValue ) { emit ChangeChanGain ( 7, dValue ); }
    void OnGainValueChangedCh8 ( double dValue ) { emit ChangeChanGain ( 8, dValue ); }
    void OnGainValueChangedCh9 ( double dValue ) { emit ChangeChanGain ( 9, dValue ); }

    void OnChSoloStateChangedCh0 ( int value ) { OnChSoloStateChanged ( 0, value ); }
    void OnChSoloStateChangedCh1 ( int value ) { OnChSoloStateChanged ( 1, value ); }
    void OnChSoloStateChangedCh2 ( int value ) { OnChSoloStateChanged ( 2, value ); }
    void OnChSoloStateChangedCh3 ( int value ) { OnChSoloStateChanged ( 3, value ); }
    void OnChSoloStateChangedCh4 ( int value ) { OnChSoloStateChanged ( 4, value ); }
    void OnChSoloStateChangedCh5 ( int value ) { OnChSoloStateChanged ( 5, value ); }
    void OnChSoloStateChangedCh6 ( int value ) { OnChSoloStateChanged ( 6, value ); }
    void OnChSoloStateChangedCh7 ( int value ) { OnChSoloStateChanged ( 7, value ); }
    void OnChSoloStateChangedCh8 ( int value ) { OnChSoloStateChanged ( 8, value ); }
    void OnChSoloStateChangedCh9 ( int value ) { OnChSoloStateChanged ( 9, value ); }

signals:
    void ChangeChanGain ( int iId, double dGain );
    void NumClientsChanged ( int iNewNumClients );
};

#endif // MIXERBOARD_H__FD6B49E1606C2AC__INCLUDED_
