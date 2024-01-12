/******************************************************************************\
 * Copyright (c) 2004-2024
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
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 *
\******************************************************************************/

#include "multicolorled.h"

/* Implementation *************************************************************/
CMultiColorLED::CMultiColorLED ( QWidget* parent ) :
    QLabel ( parent ),
    BitmCubeDisabled ( QString::fromUtf8 ( ":/png/LEDs/res/CLEDDisabled.png" ) ),
    BitmCubeGrey ( QString::fromUtf8 ( ":/png/LEDs/res/CLEDGrey.png" ) ),
    BitmCubeGreen ( QString::fromUtf8 ( ":/png/LEDs/res/CLEDGreenBig.png" ) ),
    BitmCubeYellow ( QString::fromUtf8 ( ":/png/LEDs/res/IndicatorYellowFancy.png" ) ),
    BitmCubeRed ( QString::fromUtf8 ( ":/png/LEDs/res/IndicatorRedFancy.png" ) ),
    BitmIndicatorGreen ( QString::fromUtf8 ( ":/png/LEDs/res/IndicatorGreen.png" ) ),
    BitmIndicatorYellow ( QString::fromUtf8 ( ":/png/LEDs/res/IndicatorYellow.png" ) ),
    BitmIndicatorRed ( QString::fromUtf8 ( ":/png/LEDs/res/IndicatorRed.png" ) )
{
    // set init bitmap
    setPixmap ( BitmCubeGrey );
    eColorFlag = RL_GREY;

    // set default type and reset
    eType = MT_LED;
    Reset();
}

void CMultiColorLED::SetType ( const EType eNType )
{
    eType = eNType;
    Reset();
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
            if ( eType == MT_LED )
            {
                setPixmap ( BitmCubeRed );
            }
            else
            {
                setPixmap ( BitmIndicatorRed );
            }
            setAccessibleDescription ( tr ( "Red" ) );
            eColorFlag = RL_RED;
        }
        break;

    case RL_YELLOW:
        // yellow
        if ( eColorFlag != RL_YELLOW )
        {
            if ( eType == MT_LED )
            {
                setPixmap ( BitmCubeYellow );
            }
            else
            {
                setPixmap ( BitmIndicatorYellow );
            }
            setAccessibleDescription ( tr ( "Yellow" ) );
            eColorFlag = RL_YELLOW;
        }
        break;

    case RL_GREEN:
        // green
        if ( eColorFlag != RL_GREEN )
        {
            if ( eType == MT_LED )
            {
                setPixmap ( BitmCubeGreen );
            }
            else
            {
                setPixmap ( BitmIndicatorGreen );
            }
            setAccessibleDescription ( tr ( "Green" ) );
            eColorFlag = RL_GREEN;
        }
        break;

    default:
        // if no color is active, set control to grey light
        if ( eColorFlag != RL_GREY )
        {
            if ( eType == MT_LED )
            {
                setPixmap ( BitmCubeGrey );
            }
            else
            {
                // make it invisible if in indicator mode
                setPixmap ( QPixmap() );
            }
            eColorFlag = RL_GREY;
        }
        break;
    }
}

void CMultiColorLED::Reset()
{
    if ( isEnabled() )
    {
        // set color flag to disable to make sure the pixmap gets updated
        eColorFlag = RL_DISABLED;
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
