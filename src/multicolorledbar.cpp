/******************************************************************************\
 * Copyright (c) 2004-2009
 *
 * Author(s):
 *  Volker Fischer
 *
 * Description:
 *  Implements a multi color LED bar
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

#include "multicolorledbar.h"


/* Implementation *************************************************************/
CMultiColorLEDBar::CMultiColorLEDBar ( QWidget* parent, Qt::WindowFlags f )
    : QFrame ( parent, f ),
    BitmCubeRoundGrey   ( QString::fromUtf8 ( ":/png/LEDs/res/VLEDGreySmall.png" ) ),
    BitmCubeRoundGreen  ( QString::fromUtf8 ( ":/png/LEDs/res/VLEDGreenSmall.png" ) ),
    BitmCubeRoundYellow ( QString::fromUtf8 ( ":/png/LEDs/res/VLEDYellowSmall.png" ) ),
    BitmCubeRoundRed    ( QString::fromUtf8 ( ":/png/LEDs/res/VLEDRedSmall.png" ) )
{
    // set total number of LEDs
    iNumLEDs = NUM_STEPS_INP_LEV_METER;

    // create layout and set spacing to zero
    pMainLayout = new QHBoxLayout ( this );
    pMainLayout->setAlignment ( Qt::AlignVCenter );
    pMainLayout->setSpacing ( 0 );

    // create LEDs
    vecpLEDs.Init ( iNumLEDs );
    for ( int i = 0; i < iNumLEDs; i++ )
    {
        // create LED label
        vecpLEDs[i] = new QLabel ( "", parent );

        // add LED to layout
        pMainLayout->addWidget ( vecpLEDs[i] );

        // set initial bitmap
        vecpLEDs[i]->setPixmap ( BitmCubeRoundGrey );

        // bitmap defines minimum size of the label
        vecpLEDs[i]->setMinimumSize (
            BitmCubeRoundGrey.width(), BitmCubeRoundGrey.height() );
    }
}

void CMultiColorLEDBar::setValue ( const int value )
{

// TODO speed optimiation: only change bitmaps of LEDs which
// actually have to be changed

    // update state of all LEDs for current level value
    for ( int i = 0; i < iNumLEDs; i++ )
    {
        // set active if value is above current LED index
        SetLED ( i, i < value );
    }
}

void CMultiColorLEDBar::SetLED ( const int iLEDIdx, const bool bActive )
{
    if ( bActive )
    {
        // check which color we should use (green, yellow or red)
        if ( iLEDIdx < YELLOW_BOUND_INP_LEV_METER )
        {
            // green region
            vecpLEDs[iLEDIdx]->setPixmap ( BitmCubeRoundGreen );
        }
        else
        {
            if ( iLEDIdx < RED_BOUND_INP_LEV_METER )
            {
                // yellow region
                vecpLEDs[iLEDIdx]->setPixmap ( BitmCubeRoundYellow );
            }
            else
            {
                // red region
                vecpLEDs[iLEDIdx]->setPixmap ( BitmCubeRoundRed );
            }
        }
    }
    else
    {
        // we use grey LED for inactive state
        vecpLEDs[iLEDIdx]->setPixmap ( BitmCubeRoundGrey );
    }
}
