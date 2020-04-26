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

#include <QLabel>
#include <QString>
#include <QLineEdit>
#include <QPushButton>
#include <QProgressBar>
#include <QWhatsThis>
#include <QTimer>
#include <QSlider>
#include <QRadioButton>
#include <QMenuBar>
#include <QLayout>
#include <QButtonGroup>
#include <QGroupBox>
#include "global.h"
#include "client.h"
#include "multicolorled.h"
#include "ui_clientsettingsdlgbase.h"


/* Definitions ****************************************************************/
// update time for GUI controls
#define DISPLAY_UPDATE_TIME         1000 // ms


/* Classes ********************************************************************/
// Sound card audio channel mixer dialog ---------------------------------------
class CSndCrdMixDlg : public QDialog
{
    Q_OBJECT

public:
    CSndCrdMixDlg ( CClient* pNCliP,
                    QWidget* parent = nullptr );

protected:
    double       CalcFaderGain ( const int value );
    virtual void showEvent ( QShowEvent* );

    QWidget* pInLFaderWidget[MAX_NUM_IN_OUT_CHANNELS];
    QSlider* pInLFader[MAX_NUM_IN_OUT_CHANNELS];
    QLabel*  pInLLabel[MAX_NUM_IN_OUT_CHANNELS];
    QWidget* pInRFaderWidget[MAX_NUM_IN_OUT_CHANNELS];
    QSlider* pInRFader[MAX_NUM_IN_OUT_CHANNELS];
    QLabel*  pInRLabel[MAX_NUM_IN_OUT_CHANNELS];
    QWidget* pOutLFaderWidget[MAX_NUM_IN_OUT_CHANNELS];
    QSlider* pOutLFader[MAX_NUM_IN_OUT_CHANNELS];
    QLabel*  pOutLLabel[MAX_NUM_IN_OUT_CHANNELS];
    QWidget* pOutRFaderWidget[MAX_NUM_IN_OUT_CHANNELS];
    QSlider* pOutRFader[MAX_NUM_IN_OUT_CHANNELS];
    QLabel*  pOutRLabel[MAX_NUM_IN_OUT_CHANNELS];

    CClient* pClient;

public slots:
    void OnValueChanged ( int );
};


// Client settings dialog ------------------------------------------------------
class CClientSettingsDlg : public QDialog, private Ui_CClientSettingsDlgBase
{
    Q_OBJECT

public:
    CClientSettingsDlg ( CClient* pNCliP,
                         QWidget* parent = nullptr,
                         Qt::WindowFlags f = nullptr );

    void SetStatus ( const CMultiColorLED::ELightColor eStatus ) { ledNetw->SetLight ( eStatus ); }

    void ResetStatusAndPingLED()
    {
        ledNetw->Reset();
        ledOverallDelay->Reset();
    }

    void SetPingTimeResult ( const int                         iPingTime,
                             const int                         iOverallDelayMs,
                             const CMultiColorLED::ELightColor eOverallDelayLEDColor );

    void UpdateDisplay();

protected:
    void    UpdateJitterBufferFrame();
    void    UpdateSoundCardFrame();
    void    UpdateSoundChannelSelectionFrame();
    void    UpdateCentralServerDependency();
    QString GenSndCrdBufferDelayString ( const int iFrameSize,
                                         const QString strAddText = "" );

    virtual void showEvent ( QShowEvent* ) { UpdateDisplay(); }

    CClient*      pClient;
    QTimer        TimerStatus;
    QButtonGroup  SndCrdBufferDelayButtonGroup;
    CSndCrdMixDlg SndCrdMixDlg;

 public slots:
    void OnTimerStatus() { UpdateDisplay(); }
    void OnNetBufValueChanged ( int value );
    void OnNetBufServerValueChanged ( int value );
    void OnAutoJitBufStateChanged ( int value );
    void OnGUIDesignFancyStateChanged ( int value );
    void OnDisplayChannelLevelsStateChanged ( int value );
    void OnEnableOPUS64StateChanged ( int value );
    void OnCentralServerAddressEditingFinished();
    void OnNewClientLevelEditingFinished();
    void OnSndCrdBufferDelayButtonGroupClicked ( QAbstractButton* button );
    void OnSoundcardActivated ( int iSndDevIdx );
    void OnLInChanActivated ( int iChanIdx );
    void OnRInChanActivated ( int iChanIdx );
    void OnLOutChanActivated ( int iChanIdx );
    void OnROutChanActivated ( int iChanIdx );
    void OnAudioChannelsActivated ( int iChanIdx );
    void OnAudioQualityActivated ( int iQualityIdx );
    void OnCentServAddrTypeActivated ( int iTypeIdx );
    void OnDriverSetupClicked();

signals:
    void GUIDesignChanged();
    void DisplayChannelLevelsChanged();
    void AudioChannelsChanged();
    void NewClientLevelChanged();
};
