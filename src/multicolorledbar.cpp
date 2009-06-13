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
    : QFrame ( parent, f )
{
    // set total number of LEDs
    iNumLEDs = NUM_STEPS_INP_LEV_METER;

    // create layout and set spacing to zero
    pMainLayout = new QHBoxLayout ( this );
    pMainLayout->setAlignment ( Qt::AlignVCenter );
    pMainLayout->setMargin    ( 0 );
    pMainLayout->setSpacing   ( 0 );

    // create LEDs
    vecpLEDs.Init ( iNumLEDs );
    for ( int iLEDIdx = 0; iLEDIdx < iNumLEDs; iLEDIdx++ )
    {
        // create LED object
        vecpLEDs[iLEDIdx] = new cLED ( parent );

        // add LED to layout with spacer (do not add spacer on the left of the
        // first LED)
        if ( iLEDIdx > 0 )
        {
            pMainLayout->addStretch();
        }
        pMainLayout->addWidget ( vecpLEDs[iLEDIdx]->getLabelPointer() );
    }
}

CMultiColorLEDBar::~CMultiColorLEDBar()
{
    // clean up the LED objects
    for ( int iLEDIdx = 0; iLEDIdx < iNumLEDs; iLEDIdx++ )
    {
        delete vecpLEDs[iLEDIdx];
    }
}

void CMultiColorLEDBar::changeEvent ( QEvent* curEvent )
{
    // act on enabled changed state
    if ( (*curEvent).type() == QEvent::EnabledChange )
    {
        // reset all LEDs
        Reset ( this->isEnabled() );
    }
}

void CMultiColorLEDBar::Reset ( const bool bEnabled )
{
    // update state of all LEDs
    for ( int iLEDIdx = 0; iLEDIdx < iNumLEDs; iLEDIdx++ )
    {
        // different reset behavoiur for enabled and disabled control
        if ( bEnabled )
        {
            vecpLEDs[iLEDIdx]->setColor ( cLED::RL_GREY );
        }
        else
        {
            vecpLEDs[iLEDIdx]->setColor ( cLED::RL_DISABLED );
        }
    }
}

void CMultiColorLEDBar::setValue ( const int value )
{
    if ( this->isEnabled() )
    {
        // update state of all LEDs for current level value
        for ( int iLEDIdx = 0; iLEDIdx < iNumLEDs; iLEDIdx++ )
        {
            // set active LED color if value is above current LED index
            if ( iLEDIdx < value )
            {
                // check which color we should use (green, yellow or red)
                if ( iLEDIdx < YELLOW_BOUND_INP_LEV_METER )
                {
                    // green region
                    vecpLEDs[iLEDIdx]->setColor ( cLED::RL_GREEN );
                }
                else
                {
                    if ( iLEDIdx < RED_BOUND_INP_LEV_METER )
                    {
                        // yellow region
                        vecpLEDs[iLEDIdx]->setColor ( cLED::RL_YELLOW );
                    }
                    else
                    {
                        // red region
                        vecpLEDs[iLEDIdx]->setColor ( cLED::RL_RED );
                    }
                }
            }
            else
            {
                // we use grey LED for inactive state
                vecpLEDs[iLEDIdx]->setColor ( cLED::RL_GREY );
            }
        }
    }
}

CMultiColorLEDBar::cLED::cLED ( QWidget* parent ) :
    BitmCubeRoundDisabled ( QString::fromUtf8 ( ":/png/LEDs/res/VLEDDisabledSmall.png" ) ),
    BitmCubeRoundGrey     ( QString::fromUtf8 ( ":/png/LEDs/res/VLEDGreySmall.png" ) ),
    BitmCubeRoundGreen    ( QString::fromUtf8 ( ":/png/LEDs/res/VLEDGreenSmall.png" ) ),
    BitmCubeRoundYellow   ( QString::fromUtf8 ( ":/png/LEDs/res/VLEDYellowSmall.png" ) ),
    BitmCubeRoundRed      ( QString::fromUtf8 ( ":/png/LEDs/res/VLEDRedSmall.png" ) )
{
    // create LED label
    pLEDLabel = new QLabel ( "", parent );

    // bitmap defines minimum size of the label
    pLEDLabel->setMinimumSize (
        BitmCubeRoundGrey.width(), BitmCubeRoundGrey.height() );

    // set initial bitmap
    pLEDLabel->setPixmap ( BitmCubeRoundGrey );
    eCurLightColor = RL_GREY;
}

void CMultiColorLEDBar::cLED::setColor ( const ELightColor eNewColor )
{
    // only update LED if color has changed
    if ( eNewColor != eCurLightColor )
    {
        switch ( eNewColor )
        {
        case RL_DISABLED:
            pLEDLabel->setPixmap ( BitmCubeRoundDisabled );
            break;

        case RL_GREY:
            pLEDLabel->setPixmap ( BitmCubeRoundGrey );
            break;

        case RL_GREEN:
            pLEDLabel->setPixmap ( BitmCubeRoundGreen );
            break;

        case RL_YELLOW:
            pLEDLabel->setPixmap ( BitmCubeRoundYellow );
            break;

        case RL_RED:
            pLEDLabel->setPixmap ( BitmCubeRoundRed );
            break;
        }
        eCurLightColor = eNewColor;
    }
}
