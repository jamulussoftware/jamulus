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

#include <QGroupBox>
#include <QComboBox>
#include <QLabel>
#include <QSlider>
#include <QWhatsThis>
#include "global.h"
#include "util.h"
#include "client.h"
#include "levelmeter.h"
#include "ui_inputeffectlanebase.h"

/* Classes ********************************************************************/
// one column of the Input Effects window: the controls of all effects of one
// lane (input channel) plus the level meters before and after the effects
class CInputEffectLane : public QGroupBox, private Ui_CInputEffectLaneBase
{
    Q_OBJECT

public:
    CInputEffectLane ( QWidget* parent = nullptr );

    // binds the controls to the effects of the given lane
    void SetClientAndLane ( CClient* pNCliP, const int iNLane );

    // controls <- effect parameters
    void UpdateFromClient();

    void UpdateMeters ( const double dRawDb, const double dOutDb );
    void ResetMeters();
    void SetMeterStyle ( const EMeterStyle eNewMeterStyle );

protected:
    void UpdateValueLabels();

    CClient* pClient;
    int      iLane;

public slots:
    void OnBoostToggled ( bool bChecked );
    void OnBoostSliderChanged ( int iValue );
    void OnCompToggled ( bool bChecked );
    void OnCompAmountActivated ( int iIdx );
    void OnCompThresholdSliderChanged ( int iValue );
    void OnReverbToggled ( bool bChecked );
    void OnReverbSliderChanged ( int iValue );

signals:
    // a parameter was changed by the user
    void ParamChanged();
};
