/******************************************************************************\
 * Copyright (c) 2004-2006
 *
 * Author(s):
 *  Volker Fischer
 *
 * Description:
 *  Implements a multi-color LED
 *  
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

#include "multicolorled.h"


/* Implementation *************************************************************/
CMultiColorLEDbase::CMultiColorLEDbase()
{
    /* Define size of the bitmaps */
    const int iXSize = 13;
    const int iYSize = 13;

    /* Create bitmaps */
    BitmCubeGreen.resize(iXSize, iYSize);
    BitmCubeGreen.fill(QColor(0, 255, 0));
    BitmCubeRed.resize(iXSize, iYSize);
    BitmCubeRed.fill(QColor(255, 0, 0));
    BitmCubeGrey.resize(iXSize, iYSize);
    BitmCubeGrey.fill(QColor(192, 192, 192));
    BitmCubeYellow.resize(iXSize, iYSize);
    BitmCubeYellow.fill(QColor(255, 255, 0));

    /* Init color flags */
    Reset();

    /* Set init-bitmap */
    SetPixmap(BitmCubeGrey);
    eColorFlag = RL_GREY;

    /* Init update time */
    iUpdateTime = DEFAULT_UPDATE_TIME;

    /* Connect timer events to the desired slots */
    connect(&TimerRedLight, SIGNAL(timeout()), 
        this, SLOT(OnTimerRedLight()));
    connect(&TimerGreenLight, SIGNAL(timeout()), 
        this, SLOT(OnTimerGreenLight()));
    connect(&TimerYellowLight, SIGNAL(timeout()), 
        this, SLOT(OnTimerYellowLight()));
}

void CMultiColorLEDbase::Reset()
{
    /* Reset color flags */
    bFlagRedLi = false;
    bFlagGreenLi = false;
    bFlagYellowLi = false;

    UpdateColor();
}

void CMultiColorLEDbase::OnTimerRedLight() 
{
    bFlagRedLi = false;
    UpdateColor();
}

void CMultiColorLEDbase::OnTimerGreenLight() 
{
    bFlagGreenLi = false;
    UpdateColor();
}

void CMultiColorLEDbase::OnTimerYellowLight() 
{
    bFlagYellowLi = false;
    UpdateColor();
}

void CMultiColorLEDbase::UpdateColor()
{
    /* Red light has highest priority, then comes yellow and at the end, we
       decide to set green light. Allways check the current color of the
       control before setting the color to prevent flicking */
    if (bFlagRedLi)
    {
        if (eColorFlag != RL_RED)
        {
            SetPixmap(BitmCubeRed);
            eColorFlag = RL_RED;
        }
        return;
    }

    if (bFlagYellowLi)
    {
        if (eColorFlag != RL_YELLOW)
        {
            SetPixmap(BitmCubeYellow);
            eColorFlag = RL_YELLOW;
        }
        return;
    }

    if (bFlagGreenLi)
    {
        if (eColorFlag != RL_GREEN)
        {
            SetPixmap(BitmCubeGreen);
            eColorFlag = RL_GREEN;
        }
        return;
    }

    /* If no color is active, set control to grey light */
    if (eColorFlag != RL_GREY)
    {
        SetPixmap(BitmCubeGrey);
        eColorFlag = RL_GREY;
    }
}

void CMultiColorLEDbase::SetLight(int iNewStatus)
{
    switch (iNewStatus)
    {
    case MUL_COL_LED_GREEN:
        /* Green light */
        bFlagGreenLi = true;
        TimerGreenLight.changeInterval(iUpdateTime);
        break;

    case MUL_COL_LED_YELLOW:
        /* Yellow light */
        bFlagYellowLi = true;
        TimerYellowLight.changeInterval(iUpdateTime);
        break;

    case MUL_COL_LED_RED:
        /* Red light */
        bFlagRedLi = true;
        TimerRedLight.changeInterval(iUpdateTime);
        break;
    }

    UpdateColor();
}

void CMultiColorLEDbase::SetUpdateTime(int iNUTi)
{
    /* Avoid too short intervals */
    if (iNUTi < MIN_TIME_FOR_RED_LIGHT)
        iUpdateTime = MIN_TIME_FOR_RED_LIGHT;
    else
        iUpdateTime = iNUTi;
}


CMultiColorLED::CMultiColorLED(QWidget* parent, const char* name, WFlags f) : 
    QLabel(parent, name, f)
{
    // set modified style
    setFrameShape(QFrame::Panel);
    setFrameShadow(QFrame::Sunken);
    setIndent(0);
}
