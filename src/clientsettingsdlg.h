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
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 *
\******************************************************************************/

#pragma once

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
#include <QMessageBox>
#include "global.h"
#include "util.h"
#include "client.h"
#include "settings.h"
#include "multicolorled.h"
#include "ui_clientsettingsdlgbase.h"


/* Definitions ****************************************************************/
// update time for GUI controls
#define DISPLAY_UPDATE_TIME         1000 // ms


/* Classes ********************************************************************/
class CClientSettingsDlg : public CBaseDlg, private Ui_CClientSettingsDlgBase
{
    Q_OBJECT

public:
    CClientSettingsDlg ( CClient*         pNCliP,
                         CClientSettings* pNSetP,
                         QWidget*         parent = nullptr );

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
    void UpdateSoundDeviceChannelSelectionFrame();

protected:
    void    UpdateJitterBufferFrame();
    void    UpdateSoundCardFrame();
    void    UpdateCustomCentralServerComboBox();
    QString GenSndCrdBufferDelayString ( const int     iFrameSize,
                                         const QString strAddText = "" );

    virtual void showEvent ( QShowEvent* );

    CClient*         pClient;
    CClientSettings* pSettings;
    QTimer           TimerStatus;
    QButtonGroup     SndCrdBufferDelayButtonGroup;

public slots:
    void OnTimerStatus() { UpdateDisplay(); }
    void OnNetBufValueChanged ( int value );
    void OnNetBufServerValueChanged ( int value );
    void OnAutoJitBufStateChanged ( int value );
    void OnEnableOPUS64StateChanged ( int value );
    void OnCentralServerAddressEditingFinished();
    void OnNewClientLevelEditingFinished() { pSettings->iNewClientFaderLevel = edtNewClientLevel->text().toInt(); }
    void OnSndCrdBufferDelayButtonGroupClicked ( QAbstractButton* button );
    void OnSoundcardActivated ( int iSndDevIdx );
    void OnLInChanActivated ( int iChanIdx );
    void OnRInChanActivated ( int iChanIdx );
    void OnLOutChanActivated ( int iChanIdx );
    void OnROutChanActivated ( int iChanIdx );
    void OnAudioChannelsActivated ( int iChanIdx );
    void OnAudioQualityActivated ( int iQualityIdx );
    void OnGUIDesignActivated ( int iDesignIdx );
    void OnDriverSetupClicked();
    void OnLanguageChanged ( QString strLanguage ) { pSettings->strLanguage = strLanguage; }

signals:
    void GUIDesignChanged();
    void AudioChannelsChanged();
    void CustomCentralServerAddrChanged();
};
