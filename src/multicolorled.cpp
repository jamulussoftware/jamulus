/******************************************************************************\
 * Copyright (c) 2004-2009
 *
 * Author(s):
 *  Volker Fischer
 *
 * Description:
 *  Implements a multi color LED
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
CMultiColorLED::CMultiColorLED ( QWidget* parent, Qt::WindowFlags f )
    : QLabel ( parent, f ),
    BitmCubeGreen  ( QString::fromUtf8 ( ":/png/LEDs/res/CLEDGreenSmall.png" ) ),
    BitmCubeRed    ( QString::fromUtf8 ( ":/png/LEDs/res/CLEDRedSmall.png" ) ),
    BitmCubeGrey   ( QString::fromUtf8 ( ":/png/LEDs/res/CLEDGreySmall.png" ) ),
    BitmCubeYellow ( QString::fromUtf8 ( ":/png/LEDs/res/CLEDYellowSmall.png" ) )
{
    // init color flags
    Reset();

    // set init bitmap
    setPixmap ( BitmCubeGrey );
    eColorFlag = RL_GREY;

    // init update time
    SetUpdateTime ( DEFAULT_UPDATE_TIME );

    // init timers -> we want to have single shot timers
    TimerRedLight.setSingleShot    ( true );
    TimerGreenLight.setSingleShot  ( true );
    TimerYellowLight.setSingleShot ( true );

    // connect timer events to the desired slots
    connect ( &TimerRedLight, SIGNAL ( timeout() ),
        this, SLOT ( OnTimerRedLight() ) );
    connect ( &TimerGreenLight, SIGNAL ( timeout() ),
        this, SLOT ( OnTimerGreenLight() ) );
    connect ( &TimerYellowLight, SIGNAL ( timeout() ),
        this, SLOT ( OnTimerYellowLight() ) );

    connect ( this, SIGNAL ( newPixmap ( const QPixmap& ) ),
        this, SLOT ( OnNewPixmap ( const QPixmap& ) ) );
}

void CMultiColorLED::Reset()
{
    // reset color flags
    bFlagRedLi    = false;
    bFlagGreenLi  = false;
    bFlagYellowLi = false;

    UpdateColor();
}

void CMultiColorLED::OnTimerRedLight() 
{
    bFlagRedLi = false;
    UpdateColor();
}

void CMultiColorLED::OnTimerGreenLight() 
{
    bFlagGreenLi = false;
    UpdateColor();
}

void CMultiColorLED::OnTimerYellowLight() 
{
    bFlagYellowLi = false;
    UpdateColor();
}

void CMultiColorLED::UpdateColor()
{
    /* Red light has highest priority, then comes yellow and at the end, we
       decide to set green light. Allways check the current color of the
       control before setting the color to prevent flicking */
    if ( bFlagRedLi )
    {
        if ( eColorFlag != RL_RED )
        {
            emit newPixmap ( BitmCubeRed );
            eColorFlag = RL_RED;
        }
        return;
    }

    if ( bFlagYellowLi )
    {
        if ( eColorFlag != RL_YELLOW )
        {
            emit newPixmap ( BitmCubeYellow );
            eColorFlag = RL_YELLOW;
        }
        return;
    }

    if ( bFlagGreenLi )
    {
        if ( eColorFlag != RL_GREEN )
        {
            emit newPixmap ( BitmCubeGreen );
            eColorFlag = RL_GREEN;
        }
        return;
    }

    // if no color is active, set control to grey light
    if ( eColorFlag != RL_GREY )
    {
        setPixmap ( BitmCubeGrey );
        eColorFlag = RL_GREY;
    }
}

void CMultiColorLED::SetLight ( const int iNewStatus )
{
    switch ( iNewStatus )
    {
    case MUL_COL_LED_GREEN:
        // green light
        bFlagGreenLi = true;
        TimerGreenLight.start();
        break;

    case MUL_COL_LED_YELLOW:
        // yellow light
        bFlagYellowLi = true;
        TimerYellowLight.start();
        break;

    case MUL_COL_LED_RED:
        // red light
        bFlagRedLi = true;
        TimerRedLight.start();
        break;
    }

    UpdateColor();
}

void CMultiColorLED::SetUpdateTime ( const int iNUTi )
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

    TimerGreenLight.setInterval  ( iUpdateTime );
    TimerYellowLight.setInterval ( iUpdateTime );
    TimerRedLight.setInterval    ( iUpdateTime );
}
