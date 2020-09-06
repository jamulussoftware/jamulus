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

#include <QFrame>
#include <QScrollArea>
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
#include "multicolorledbar.h"


/* Classes ********************************************************************/
class CChannelFader : public QObject
{
    Q_OBJECT

public:
    CChannelFader ( QWidget* pNW, QHBoxLayout* pParentLayout );

    void SetText ( const CChannelInfo& ChanInfo );
    QString GetReceivedName() { return strReceivedName; }
    void SetChannelInfos ( const CChannelInfo& cChanInfo );
    void Show() { pFrame->show(); }
    void Hide() { pFrame->hide(); }
    bool IsVisible() { return !pFrame->isHidden(); }
    bool IsSolo() { return pcbSolo->isChecked(); }
    bool IsMute() { return pcbMute->isChecked(); }
    void SetGUIDesign ( const EGUIDesign eNewDesign );
    void SetDisplayChannelLevel ( const bool eNDCL );
    bool GetDisplayChannelLevel();

    void UpdateSoloState ( const bool bNewOtherSoloState );
    void SetFaderLevel ( const int iLevel );
    void SetFaderIsSolo ( const bool bIsSolo );
    void SetFaderIsMute ( const bool bIsMute );
    int  GetFaderLevel() { return pFader->value(); }
    void Reset();
    void SetChannelLevel ( const uint16_t iLevel );

protected:
    double CalcFaderGain ( const int value );
    void   SetMute ( const bool bState );
    void   SendFaderLevelToServer ( const int iLevel );
    void   SetupFaderTag ( const ESkillLevel eSkillLevel );

    QFrame*            pFrame;

    QWidget*           pLevelsBox;
    QWidget*           pMuteSoloBox;
    CMultiColorLEDBar* plbrChannelLevel;
    QSlider*           pFader;

    QCheckBox*         pcbMute;
    QCheckBox*         pcbSolo;

    QGroupBox*         pLabelInstBox;
    QLabel*            plblLabel;
    QLabel*            plblInstrument;
    QLabel*            plblCountryFlag;

    QString            strReceivedName;

    bool               bOtherChannelIsSolo;

public slots:
    void OnLevelValueChanged ( int value ) { SendFaderLevelToServer ( value ); }
    void OnMuteStateChanged ( int value );

signals:
    void gainValueChanged ( double value );
    void soloStateChanged ( int value );
};


#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
template<unsigned int slotId>
class CAudioMixerBoardSlots : public CAudioMixerBoardSlots<slotId - 1>
{

public:
    void OnChGainValueChanged ( double dValue ) { UpdateGainValue ( slotId - 1, dValue ); }

protected:
    virtual void UpdateGainValue ( const int    iChannelIdx,
                                   const double dValue ) = 0;
};

template<>
class CAudioMixerBoardSlots<0> {};

#else
template<unsigned int slotId>
class CAudioMixerBoardSlots {};

#endif

class CAudioMixerBoard :
    public QScrollArea,
    public CAudioMixerBoardSlots<MAX_NUM_CHANNELS>
{
    Q_OBJECT

public:
    CAudioMixerBoard ( QWidget* parent = nullptr, Qt::WindowFlags f = nullptr );

    void HideAll();
    void ApplyNewConClientList ( CVector<CChannelInfo>& vecChanInfo );
    void SetServerName ( const QString& strNewServerName );
    void SetGUIDesign ( const EGUIDesign eNewDesign );
    void SetDisplayChannelLevels ( const bool eNDCL );

    void SetFaderLevel ( const int iChannelIdx,
                         const int iValue );

    void SetChannelLevels ( const CVector<uint16_t>& vecChannelLevel );

    // settings
    CVector<QString> vecStoredFaderTags;
    CVector<int>     vecStoredFaderLevels;
    CVector<int>     vecStoredFaderIsSolo;
    CVector<int>     vecStoredFaderIsMute;
    int              iNewClientFaderLevel;

protected:
    bool GetStoredFaderSettings ( const CChannelInfo& ChanInfo,
                                  int&                iStoredFaderLevel,
                                  bool&               bStoredFaderIsSolo,
                                  bool&               bStoredFaderIsMute );

    void StoreFaderSettings ( CChannelFader* pChanFader );
    void UpdateSoloStates();

    CVector<CChannelFader*> vecpChanFader;
    QGroupBox*              pGroupBox;
    QHBoxLayout*            pMainLayout;
    bool                    bDisplayChannelLevels;
    bool                    bNoFaderVisible;
    QString                 strServerName;

#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
    virtual void UpdateGainValue ( const int    iChannelIdx,
                                   const double dValue );

    template<unsigned int slotId>
    inline void connectFaderSignalsToMixerBoardSlots();

#else
    void UpdateGainValue ( const int    iChannelIdx,
                           const double dValue );

#endif

#if QT_VERSION < 0x50000  // MOC does not expand macros in Qt 4, so we cannot use QT_VERSION_CHECK(5, 0, 0)
public slots:
    // CODE TAG: MAX_NUM_CHANNELS_TAG
    // make sure we have MAX_NUM_CHANNELS connections!!!
    void OnGainValueChangedCh0  ( double dValue ) { UpdateGainValue ( 0,  dValue ); }
    void OnGainValueChangedCh1  ( double dValue ) { UpdateGainValue ( 1,  dValue ); }
    void OnGainValueChangedCh2  ( double dValue ) { UpdateGainValue ( 2,  dValue ); }
    void OnGainValueChangedCh3  ( double dValue ) { UpdateGainValue ( 3,  dValue ); }
    void OnGainValueChangedCh4  ( double dValue ) { UpdateGainValue ( 4,  dValue ); }
    void OnGainValueChangedCh5  ( double dValue ) { UpdateGainValue ( 5,  dValue ); }
    void OnGainValueChangedCh6  ( double dValue ) { UpdateGainValue ( 6,  dValue ); }
    void OnGainValueChangedCh7  ( double dValue ) { UpdateGainValue ( 7,  dValue ); }
    void OnGainValueChangedCh8  ( double dValue ) { UpdateGainValue ( 8,  dValue ); }
    void OnGainValueChangedCh9  ( double dValue ) { UpdateGainValue ( 9,  dValue ); }
    void OnGainValueChangedCh10 ( double dValue ) { UpdateGainValue ( 10, dValue ); }
    void OnGainValueChangedCh11 ( double dValue ) { UpdateGainValue ( 11, dValue ); }
    void OnGainValueChangedCh12 ( double dValue ) { UpdateGainValue ( 12, dValue ); }
    void OnGainValueChangedCh13 ( double dValue ) { UpdateGainValue ( 13, dValue ); }
    void OnGainValueChangedCh14 ( double dValue ) { UpdateGainValue ( 14, dValue ); }
    void OnGainValueChangedCh15 ( double dValue ) { UpdateGainValue ( 15, dValue ); }
    void OnGainValueChangedCh16 ( double dValue ) { UpdateGainValue ( 16, dValue ); }
    void OnGainValueChangedCh17 ( double dValue ) { UpdateGainValue ( 17, dValue ); }
    void OnGainValueChangedCh18 ( double dValue ) { UpdateGainValue ( 18, dValue ); }
    void OnGainValueChangedCh19 ( double dValue ) { UpdateGainValue ( 19, dValue ); }
    void OnGainValueChangedCh20 ( double dValue ) { UpdateGainValue ( 20, dValue ); }
    void OnGainValueChangedCh21 ( double dValue ) { UpdateGainValue ( 21, dValue ); }
    void OnGainValueChangedCh22 ( double dValue ) { UpdateGainValue ( 22, dValue ); }
    void OnGainValueChangedCh23 ( double dValue ) { UpdateGainValue ( 23, dValue ); }
    void OnGainValueChangedCh24 ( double dValue ) { UpdateGainValue ( 24, dValue ); }
    void OnGainValueChangedCh25 ( double dValue ) { UpdateGainValue ( 25, dValue ); }
    void OnGainValueChangedCh26 ( double dValue ) { UpdateGainValue ( 26, dValue ); }
    void OnGainValueChangedCh27 ( double dValue ) { UpdateGainValue ( 27, dValue ); }
    void OnGainValueChangedCh28 ( double dValue ) { UpdateGainValue ( 28, dValue ); }
    void OnGainValueChangedCh29 ( double dValue ) { UpdateGainValue ( 29, dValue ); }
    void OnGainValueChangedCh30 ( double dValue ) { UpdateGainValue ( 30, dValue ); }
    void OnGainValueChangedCh31 ( double dValue ) { UpdateGainValue ( 31, dValue ); }
    void OnGainValueChangedCh32 ( double dValue ) { UpdateGainValue ( 32, dValue ); }
    void OnGainValueChangedCh33 ( double dValue ) { UpdateGainValue ( 33, dValue ); }
    void OnGainValueChangedCh34 ( double dValue ) { UpdateGainValue ( 34, dValue ); }
    void OnGainValueChangedCh35 ( double dValue ) { UpdateGainValue ( 35, dValue ); }
    void OnGainValueChangedCh36 ( double dValue ) { UpdateGainValue ( 36, dValue ); }
    void OnGainValueChangedCh37 ( double dValue ) { UpdateGainValue ( 37, dValue ); }
    void OnGainValueChangedCh38 ( double dValue ) { UpdateGainValue ( 38, dValue ); }
    void OnGainValueChangedCh39 ( double dValue ) { UpdateGainValue ( 39, dValue ); }
    void OnGainValueChangedCh40 ( double dValue ) { UpdateGainValue ( 40, dValue ); }
    void OnGainValueChangedCh41 ( double dValue ) { UpdateGainValue ( 41, dValue ); }
    void OnGainValueChangedCh42 ( double dValue ) { UpdateGainValue ( 42, dValue ); }
    void OnGainValueChangedCh43 ( double dValue ) { UpdateGainValue ( 43, dValue ); }
    void OnGainValueChangedCh44 ( double dValue ) { UpdateGainValue ( 44, dValue ); }
    void OnGainValueChangedCh45 ( double dValue ) { UpdateGainValue ( 45, dValue ); }
    void OnGainValueChangedCh46 ( double dValue ) { UpdateGainValue ( 46, dValue ); }
    void OnGainValueChangedCh47 ( double dValue ) { UpdateGainValue ( 47, dValue ); }
    void OnGainValueChangedCh48 ( double dValue ) { UpdateGainValue ( 48, dValue ); }
    void OnGainValueChangedCh49 ( double dValue ) { UpdateGainValue ( 49, dValue ); }

    void OnChSoloStateChanged() { UpdateSoloStates(); }

#endif

signals:
    void ChangeChanGain ( int iId, double dGain );
    void NumClientsChanged ( int iNewNumClients );
};
