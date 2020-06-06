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
#include <QDial>
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
    void SetDisplayPans ( const bool eNDP );

    void UpdateSoloState ( const bool bNewOtherSoloState );
    void SetFaderLevel ( const int iLevel );
    void SetPanValue ( const int iPan );
    void SetFaderIsSolo ( const bool bIsSolo );
    void SetFaderIsMute ( const bool bIsMute );
    void SetRemoteFaderIsMute ( const bool bIsMute );
    int  GetFaderLevel() { return pFader->value(); }
    int  GetPanValue() { return pPan->value(); }
    void Reset();
    void SetChannelLevel ( const uint16_t iLevel );
    void SetIsMyOwnFader() { bIsMyOwnFader = true; }

protected:
    double CalcFaderGain ( const int value );
    void   SetMute ( const bool bState );
    void   SendFaderLevelToServer ( const int iLevel );
    void   SendPanValueToServer ( const int iPan );
    void   SetupFaderTag ( const ESkillLevel eSkillLevel );

    QFrame*            pFrame;

    QWidget*           pLevelsBox;
    QWidget*           pMuteSoloBox;
    CMultiColorLEDBar* plbrChannelLevel;
    QSlider*           pFader;
    QDial*             pPan;
    QLabel*            pPanLabel;
    QLabel*            pInfoLabel;

    QCheckBox*         pcbMute;
    QCheckBox*         pcbSolo;

    QGroupBox*         pLabelInstBox;
    QLabel*            plblLabel;
    QLabel*            plblInstrument;
    QLabel*            plblCountryFlag;

    QString            strReceivedName;

    bool               bOtherChannelIsSolo;
    bool               bIsMyOwnFader;

public slots:
    void OnLevelValueChanged ( int value ) { SendFaderLevelToServer ( value ); }
    void OnPanValueChanged ( int value ) { SendPanValueToServer ( value ); }
    void OnMuteStateChanged ( int value );

signals:
    void gainValueChanged ( double value, bool bIsMyOwnFader );
    void panValueChanged  ( double value );
    void soloStateChanged ( int value );
};

template<unsigned int slotId>
class CAudioMixerBoardSlots : public CAudioMixerBoardSlots<slotId - 1>
{
public:
    void OnChGainValueChanged ( double dValue, bool bIsMyOwnFader ) { UpdateGainValue ( slotId - 1, dValue, bIsMyOwnFader ); }
    void OnChPanValueChanged ( double dValue ) { UpdatePanValue ( slotId - 1, dValue ); }

protected:
    virtual void UpdateGainValue ( const int    iChannelIdx,
                                   const double dValue,
                                   const bool   bIsMyOwnFader ) = 0;
    virtual void UpdatePanValue ( const int    iChannelIdx,
                                  const double dValue ) = 0;
};

template<>
class CAudioMixerBoardSlots<0> {};


class CAudioMixerBoard :
    public QGroupBox,
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
    void SetDisplayPans ( const bool eNDP );
    void SetPanIsSupported();
    void SetServerRecordingState ( QString state );
    void SetRemoteFaderIsMute ( const int iChannelIdx, const bool bIsMute );
    void SetMyChannelID ( const int iChannelIdx ) { iMyChannelID = iChannelIdx; }

    void SetFaderLevel ( const int iChannelIdx,
                         const int iValue );

    void SetChannelLevels ( const CVector<uint16_t>& vecChannelLevel );

    // settings
    CVector<QString> vecStoredFaderTags;
    CVector<int>     vecStoredFaderLevels;
    CVector<int>     vecStoredPanValues;
    CVector<int>     vecStoredFaderIsSolo;
    CVector<int>     vecStoredFaderIsMute;
    int              iNewClientFaderLevel;

protected:
    class CMixerBoardScrollArea : public QScrollArea
    {
    public:
        CMixerBoardScrollArea ( QWidget* parent = nullptr ) : QScrollArea ( parent ) {}

    protected:
        virtual void resizeEvent ( QResizeEvent* event )
        {
            // if after a resize of the main window a vertical scroll bar is required, make
            // sure that the fader label is visible (scroll down completely)
            ensureVisible ( 0, 2000 ); // use a large value here
            QScrollArea::resizeEvent ( event );
        }
    };

    bool GetStoredFaderSettings ( const CChannelInfo& ChanInfo,
                                  int&                iStoredFaderLevel,
                                  int&                iStoredPanValue,
                                  bool&               bStoredFaderIsSolo,
                                  bool&               bStoredFaderIsMute );

    void StoreFaderSettings ( CChannelFader* pChanFader );
    void UpdateSoloStates();

    void OnGainValueChanged ( const int    iChannelIdx,
                              const double dValue );

    CVector<CChannelFader*> vecpChanFader;
    CMixerBoardScrollArea*  pScrollArea;
    QHBoxLayout*            pMainLayout;
    bool                    bDisplayChannelLevels;
    bool                    bDisplayPans;
    bool                    bIsPanSupported;
    bool                    bNoFaderVisible;
    int                     iMyChannelID;
    QString                 strServerName;
    QString                 strServerRecordingState;

    virtual void UpdateGainValue ( const int    iChannelIdx,
                                   const double dValue,
                                   const bool   bIsMyOwnFader );
    virtual void UpdatePanValue ( const int    iChannelIdx,
                                  const double dValue );

    template<unsigned int slotId>
    inline void connectFaderSignalsToMixerBoardSlots();

signals:
    void ChangeChanGain ( int iId, double dGain, bool bIsMyOwnFader );
    void ChangeChanPan ( int iId, double dPan );
    void NumClientsChanged ( int iNewNumClients );
};
