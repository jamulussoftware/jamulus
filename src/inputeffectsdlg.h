/******************************************************************************\
 * Copyright (c) 2026
 *
 * Author(s):
 *  uazure
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
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
\******************************************************************************/

#pragma once

#include <QCheckBox>
#include <QComboBox>
#include <QLabel>
#include <QLayout>
#include <QMenuBar>
#include <QMessageBox>
#include <QPushButton>
#include <QSlider>
#include <QWhatsThis>
#include "global.h"
#include "util.h"
#include "client.h"
#include "settings.h"
#include "inputeffectlane.h"
#include "ui_inputeffectsdlgbase.h"

/* Classes ********************************************************************/
// non-modal window with all local input effects, one column per input channel
class CInputEffectsDlg : public CBaseDlg, private Ui_CInputEffectsDlgBase
{
    Q_OBJECT

public:
    CInputEffectsDlg ( CClient* pNCliP, CClientSettings* pNSetP, QWidget* parent = nullptr );

    // shows one or two lanes depending on the audio channel configuration,
    // the linked setting and (on phones) the selected channel
    void UpdateChannelLayout();

    void UpdateMeters();
    void ResetMeters();
    void SetMeterStyle ( const EMeterStyle eNewMeterStyle );

    // the preview is only available while not connected
    void SetConnected ( const bool bNConnected );

    void UpdateAudioFaderSlider();

protected:
    virtual void hideEvent ( QHideEvent* ) override;

    void CopyLaneParams ( const int iFrom, const int iTo );
    void StopPreview();

    CClient*         pClient;
    CClientSettings* pSettings;
    bool             bConnected;
    bool             bMobile;
    int              iMobileLane; // lane shown on phones when not linked

public slots:
    void OnLinkedToggled ( bool bChecked );
    void OnChannelActivated ( int iIdx );
    void OnLaneParamChanged();
    void OnPreviewToggled ( bool bChecked );
    void OnLocalPreviewChanged ( bool bActive );
    void OnAudioPanValueChanged ( int value );
};
