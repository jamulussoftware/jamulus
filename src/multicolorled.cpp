/******************************************************************************\
 * Copyright (c) 2004-2020
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
    BitmCubeDisabled ( QString::fromUtf8 ( ":/png/LEDs/res/CLEDDisabledSmall.png" ) ),
    BitmCubeGrey     ( QString::fromUtf8 ( ":/png/LEDs/res/CLEDGreySmall.png" ) ),
    BitmCubeGreen    ( QString::fromUtf8 ( ":/png/LEDs/res/CLEDGreenSmall.png" ) ),
    BitmCubeYellow   ( QString::fromUtf8 ( ":/png/LEDs/res/CLEDYellowSmall.png" ) ),
    BitmCubeRed      ( QString::fromUtf8 ( ":/png/LEDs/res/CLEDRedSmall.png" ) )
{
    // init color flags
    Reset();

    // set init bitmap
    setPixmap ( BitmCubeGrey );
    eColorFlag = RL_GREY;
}

void CMultiColorLED::changeEvent ( QEvent* curEvent )
{
    // act on enabled changed state
    if ( curEvent->type() == QEvent::EnabledChange )
    {
        if ( isEnabled() )
        {
            setPixmap ( BitmCubeGrey );
            eColorFlag = RL_GREY;
        }
        else
        {
            setPixmap ( BitmCubeDisabled );
            eColorFlag = RL_DISABLED;
        }
    }
}

void CMultiColorLED::SetColor ( const ELightColor eNewColorFlag )
{
    switch ( eNewColorFlag )
    {
    case RL_RED:
        // red
        if ( eColorFlag != RL_RED )
        {
            setPixmap ( BitmCubeRed );
            eColorFlag = RL_RED;
        }
        break;

    case RL_YELLOW:
        // yellow
        if ( eColorFlag != RL_YELLOW )
        {
            setPixmap ( BitmCubeYellow );
            eColorFlag = RL_YELLOW;
        }
        break;

    case RL_GREEN:
        // green
        if ( eColorFlag != RL_GREEN )
        {
            setPixmap ( BitmCubeGreen );
            eColorFlag = RL_GREEN;
        }
        break;

    default:
        // if no color is active, set control to grey light
        if ( eColorFlag != RL_GREY )
        {
            setPixmap ( BitmCubeGrey );
            eColorFlag = RL_GREY;
        }
        break;
    }
}

void CMultiColorLED::Reset()
{
    if ( isEnabled() )
    {
        SetColor ( RL_GREY );
    }
}

void CMultiColorLED::SetLight ( const ELightColor eNewStatus )
{
    if ( isEnabled() )
    {
        SetColor ( eNewStatus );
    }
}
