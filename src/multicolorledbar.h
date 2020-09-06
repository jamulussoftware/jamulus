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
#include <QPixmap>
#include <QTimer>
#include <QLayout>
#include <QProgressBar>
#include <QStackedLayout>
#include "util.h"
#include "global.h"


/* Definitions ****************************************************************/
// defines for LED level meter CMultiColorLEDBar
#define NUM_STEPS_LED_BAR                8
#define RED_BOUND_LED_BAR                7
#define YELLOW_BOUND_LED_BAR             5


/* Classes ********************************************************************/
class CMultiColorLEDBar : public QWidget
{
    Q_OBJECT

public:
    enum ELevelMeterType
    {
        MT_LED,
        MT_BAR
    };

    CMultiColorLEDBar ( QWidget* parent = nullptr, Qt::WindowFlags f = nullptr );
    virtual ~CMultiColorLEDBar();

    void setValue ( const double dValue );
    void SetLevelMeterType ( const ELevelMeterType eNType );

protected:
    class cLED
    {
    public:
        enum ELightColor
        {
            RL_DISABLED,
            RL_GREY,
            RL_GREEN,
            RL_YELLOW,
            RL_RED
        };

        cLED ( QWidget* parent );
        void    setColor ( const ELightColor eNewColor );
        QLabel* getLabelPointer() { return pLEDLabel; }

    protected:
        QPixmap     BitmCubeRoundDisabled;
        QPixmap     BitmCubeRoundGrey;
        QPixmap     BitmCubeRoundGreen;
        QPixmap     BitmCubeRoundYellow;
        QPixmap     BitmCubeRoundRed;

        ELightColor eCurLightColor;
        QLabel*     pLEDLabel;
    };

    void Reset ( const bool bEnabled );
    virtual void changeEvent ( QEvent* curEvent );

    QStackedLayout* pStackedLayout;
    ELevelMeterType eLevelMeterType;
    CVector<cLED*>  vecpLEDs;
    QProgressBar*   pProgressBar;
};
