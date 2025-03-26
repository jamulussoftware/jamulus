/******************************************************************************\
 * Copyright (c) 2004-2025
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

#include <QFrame>
#include <QPixmap>
#include <QTimer>
#include <QLayout>
#include <QProgressBar>
#include "util.h"
#include "global.h"

/* Definitions ****************************************************************/
#define NUM_LEDS_INCL_CLIP_LED ( NUM_STEPS_LED_BAR + 1 )
#define CLIP_IND_TIME_OUT_MS   20000

/* Classes ********************************************************************/
class CLevelMeter : public QWidget
{
    Q_OBJECT

public:
    enum ELevelMeterType
    {
        MT_BAR_NARROW,
        MT_BAR_WIDE,
        MT_LED_STRIPE,
        MT_LED_ROUND_SMALL,
        MT_LED_ROUND_BIG
    };

    CLevelMeter ( QWidget* parent = nullptr );
    virtual ~CLevelMeter();

    void SetValue ( const double dValue );
    void SetLevelMeterType ( const ELevelMeterType eNType );

protected:
    class cLED
    {
    public:
        enum ELightColor
        {
            RL_DISABLED,
            RL_BLACK,
            RL_GREEN,
            RL_YELLOW,
            RL_RED,
            RL_ROUND_SMALL_BLACK,
            RL_ROUND_SMALL_GREEN,
            RL_ROUND_SMALL_YELLOW,
            RL_ROUND_SMALL_RED,
            RL_ROUND_BIG_BLACK,
            RL_ROUND_BIG_GREEN,
            RL_ROUND_BIG_YELLOW,
            RL_ROUND_BIG_RED
        };

        cLED ( QWidget* parent );

        void        SetColor ( const ELightColor eNewColor );
        ELightColor GetColor() { return eCurLightColor; };
        QLabel*     GetLabelPointer() { return pLEDLabel; }

    protected:
        QPixmap BitmCubeLedBlack;
        QPixmap BitmCubeLedGreen;
        QPixmap BitmCubeLedYellow;
        QPixmap BitmCubeLedRed;
        QPixmap BitmCubeRoundSmallLedBlack;
        QPixmap BitmCubeRoundSmallLedGreen;
        QPixmap BitmCubeRoundSmallLedYellow;
        QPixmap BitmCubeRoundSmallLedRed;
        QPixmap BitmCubeRoundBigLedBlack;
        QPixmap BitmCubeRoundBigLedGreen;
        QPixmap BitmCubeRoundBigLedYellow;
        QPixmap BitmCubeRoundBigLedRed;

        ELightColor eCurLightColor;
        QLabel*     pLEDLabel;
    };

    virtual void mousePressEvent ( QMouseEvent* ) override { ClipReset(); }

    void SetBarMeterStyleAndClipStatus ( const ELevelMeterType eNType, const bool bIsClip );

    CMinimumStackedLayout* pMinStackedLayout;
    ELevelMeterType        eLevelMeterType;
    CVector<cLED*>         vecpLEDs;
    QProgressBar*          pBarMeter;

    QTimer TimerClip;

public slots:
    void ClipReset();
};
