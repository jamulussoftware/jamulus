/******************************************************************************\
 * Copyright (c) 2004-2008
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
CMultiColorLEDbase::CMultiColorLEDbase ( QWidget* parent, Qt::WindowFlags f )
    : QLabel ( parent, f ),
    BitmCubeGreen ( LED_WIDTH_HEIGHT_SIZE_PIXEL, LED_WIDTH_HEIGHT_SIZE_PIXEL ),
    BitmCubeRed ( LED_WIDTH_HEIGHT_SIZE_PIXEL, LED_WIDTH_HEIGHT_SIZE_PIXEL ),
    BitmCubeGrey ( LED_WIDTH_HEIGHT_SIZE_PIXEL, LED_WIDTH_HEIGHT_SIZE_PIXEL ),
    BitmCubeYellow ( LED_WIDTH_HEIGHT_SIZE_PIXEL, LED_WIDTH_HEIGHT_SIZE_PIXEL )
{
    // create bitmaps
    BitmCubeGreen.fill  ( QColor ( 0, 255, 0 ) );
    BitmCubeRed.fill    ( QColor ( 255, 0, 0 ) );
    BitmCubeGrey.fill   ( QColor ( 192, 192, 192 ) );
    BitmCubeYellow.fill ( QColor ( 255, 255, 0 ) );

    // init color flags
    Reset();

    // set init bitmap
    SetPixmap ( BitmCubeGrey );
    eColorFlag = RL_GREY;

    // init update time
    iUpdateTime = DEFAULT_UPDATE_TIME;

    // connect timer events to the desired slots
    connect ( &TimerRedLight, SIGNAL ( timeout() ), 
        this, SLOT ( OnTimerRedLight() ) );
    connect ( &TimerGreenLight, SIGNAL ( timeout() ), 
        this, SLOT ( OnTimerGreenLight() ) );
    connect ( &TimerYellowLight, SIGNAL ( timeout() ), 
        this, SLOT ( OnTimerYellowLight() ) );
}

void CMultiColorLEDbase::Reset()
{
    // reset color flags
    bFlagRedLi    = false;
    bFlagGreenLi  = false;
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
    if ( bFlagRedLi )
    {
        if ( eColorFlag != RL_RED )
        {
            SetPixmap ( BitmCubeRed );
            eColorFlag = RL_RED;
        }
        return;
    }

    if ( bFlagYellowLi )
    {
        if ( eColorFlag != RL_YELLOW )
        {
            SetPixmap ( BitmCubeYellow );
            eColorFlag = RL_YELLOW;
        }
        return;
    }

    if ( bFlagGreenLi )
    {
        if ( eColorFlag != RL_GREEN )
        {
            SetPixmap ( BitmCubeGreen );
            eColorFlag = RL_GREEN;
        }
        return;
    }

    // if no color is active, set control to grey light
    if ( eColorFlag != RL_GREY )
    {
        SetPixmap ( BitmCubeGrey );
        eColorFlag = RL_GREY;
    }
}

void CMultiColorLEDbase::SetLight ( const int iNewStatus )
{
    switch ( iNewStatus )
    {
    case MUL_COL_LED_GREEN:
        // green light
        bFlagGreenLi = true;
        TimerGreenLight.setInterval ( iUpdateTime );
        break;

    case MUL_COL_LED_YELLOW:
        // yellow light
        bFlagYellowLi = true;
        TimerYellowLight.setInterval ( iUpdateTime );
        break;

    case MUL_COL_LED_RED:
        // red light
        bFlagRedLi = true;
        TimerRedLight.setInterval ( iUpdateTime );
        break;
    }

    UpdateColor();
}

void CMultiColorLEDbase::SetUpdateTime ( const int iNUTi )
{
    // avoid too short intervals
    if ( iNUTi < MIN_TIME_FOR_RED_LIGHT )
	{
        iUpdateTime = MIN_TIME_FOR_RED_LIGHT;
	}
    else
	{
        iUpdateTime = iNUTi;
	}
}


CMultiColorLED::CMultiColorLED ( QWidget* parent, Qt::WindowFlags f )
    : CMultiColorLEDbase ( parent, f )
{
    // set modified style
    setFrameShape  ( QFrame::Panel );
    setFrameShadow ( QFrame::Sunken );
    setIndent      ( 0 );
}
