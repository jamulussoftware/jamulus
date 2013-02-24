/******************************************************************************\
 * Copyright (c) 2004-2013
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

#include <QFrame>
#include <QGroupBox>
#include <QLabel>
#include <QCheckBox>
#include <QLayout>
#include <QString>
#include <QSlider>
#include <QSizePolicy>
#include <QHostAddress>
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

    void SetText ( const QString sText );
    void SetInstrumentPicture ( const int iInstrument );
    void Show() { pFrame->show(); }
    void Hide() { pFrame->hide(); }
    bool IsVisible() { return pLabel->isVisible(); }
    void SetGUIDesign ( const EGUIDesign eNewDesign );

    void ResetSoloState();
    void SetOtherSoloState ( const bool bState );

    int  GetFaderLevel() { return pFader->value(); }
    bool IsDefaultFaderLevel() { return GetFaderLevel() == AUD_MIX_FADER_MAX; }
    void Reset ( const int iLevelValue = AUD_MIX_FADER_MAX );

protected:
    double CalcFaderGain ( const int value );
    void   SetMute ( const bool bState );

    QFrame*    pFrame;
    QSlider*   pFader;
    QCheckBox* pcbMute;
    QCheckBox* pcbSolo;
    QLabel*    pLabel;
    QLabel*    pInstrument;

    bool       bOtherChannelIsSolo;

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
    void ApplyNewConClientList ( CVector<CChannelInfo>& vecChanInfo );
    void SetServerName ( const QString& strNewServerName );
    void SetGUIDesign ( const EGUIDesign eNewDesign );

protected:
    QString GenFaderText ( const CChannelInfo& ChanInfo );

    int GetStoredFaderLevel ( const CChannelInfo& ChanInfo );
    void StoreFaderLevel ( CChannelFader* pChanFader );

    void OnChSoloStateChanged ( const int iChannelIdx, const int iValue );
    void OnGainValueChanged ( const int iChannelIdx, const double dValue );

    CVector<CChannelFader*> vecpChanFader;
    QHBoxLayout*            pMainLayout;

    CVector<QString>        vecStoredFaderTags;
    CVector<int>            vecStoredFaderGains;

public slots:
    // CODE TAG: MAX_NUM_CHANNELS_TAG
    // make sure we have MAX_NUM_CHANNELS connections!!!
    void OnGainValueChangedCh0  ( double dValue ) { OnGainValueChanged ( 0,  dValue ); }
    void OnGainValueChangedCh1  ( double dValue ) { OnGainValueChanged ( 1,  dValue ); }
    void OnGainValueChangedCh2  ( double dValue ) { OnGainValueChanged ( 2,  dValue ); }
    void OnGainValueChangedCh3  ( double dValue ) { OnGainValueChanged ( 3,  dValue ); }
    void OnGainValueChangedCh4  ( double dValue ) { OnGainValueChanged ( 4,  dValue ); }
    void OnGainValueChangedCh5  ( double dValue ) { OnGainValueChanged ( 5,  dValue ); }
    void OnGainValueChangedCh6  ( double dValue ) { OnGainValueChanged ( 6,  dValue ); }
    void OnGainValueChangedCh7  ( double dValue ) { OnGainValueChanged ( 7,  dValue ); }
    void OnGainValueChangedCh8  ( double dValue ) { OnGainValueChanged ( 8,  dValue ); }
    void OnGainValueChangedCh9  ( double dValue ) { OnGainValueChanged ( 9,  dValue ); }
    void OnGainValueChangedCh10 ( double dValue ) { OnGainValueChanged ( 10, dValue ); }
    void OnGainValueChangedCh11 ( double dValue ) { OnGainValueChanged ( 11, dValue ); }

    void OnChSoloStateChangedCh0  ( int value ) { OnChSoloStateChanged ( 0,  value ); }
    void OnChSoloStateChangedCh1  ( int value ) { OnChSoloStateChanged ( 1,  value ); }
    void OnChSoloStateChangedCh2  ( int value ) { OnChSoloStateChanged ( 2,  value ); }
    void OnChSoloStateChangedCh3  ( int value ) { OnChSoloStateChanged ( 3,  value ); }
    void OnChSoloStateChangedCh4  ( int value ) { OnChSoloStateChanged ( 4,  value ); }
    void OnChSoloStateChangedCh5  ( int value ) { OnChSoloStateChanged ( 5,  value ); }
    void OnChSoloStateChangedCh6  ( int value ) { OnChSoloStateChanged ( 6,  value ); }
    void OnChSoloStateChangedCh7  ( int value ) { OnChSoloStateChanged ( 7,  value ); }
    void OnChSoloStateChangedCh8  ( int value ) { OnChSoloStateChanged ( 8,  value ); }
    void OnChSoloStateChangedCh9  ( int value ) { OnChSoloStateChanged ( 9,  value ); }
    void OnChSoloStateChangedCh10 ( int value ) { OnChSoloStateChanged ( 10, value ); }
    void OnChSoloStateChangedCh11 ( int value ) { OnChSoloStateChanged ( 11, value ); }

signals:
    void ChangeChanGain ( int iId, double dGain );
    void NumClientsChanged ( int iNewNumClients );
};

#endif // MIXERBOARD_H__FD6B49E1606C2AC__INCLUDED_
