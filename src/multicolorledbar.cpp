/******************************************************************************\
 * Copyright (c) 2004-2020
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
CMultiColorLEDBar::CMultiColorLEDBar ( QWidget* parent, Qt::WindowFlags f ) :
    QWidget ( parent, f ),
    eLevelMeterType ( MT_BAR )
{
    // initialize LED meter
    QWidget*     pLEDMeter  = new QWidget();
    QVBoxLayout* pLEDLayout = new QVBoxLayout ( pLEDMeter );
    pLEDLayout->setAlignment ( Qt::AlignHCenter );
    pLEDLayout->setMargin    ( 0 );
    pLEDLayout->setSpacing   ( 0 );

    // create LEDs
    vecpLEDs.Init ( NUM_STEPS_LED_BAR );

    for ( int iLEDIdx = NUM_STEPS_LED_BAR - 1; iLEDIdx >= 0; iLEDIdx-- )
    {
        // create LED object
        vecpLEDs[iLEDIdx] = new cLED ( parent );

        // add LED to layout with spacer (do not add spacer on the bottom of the
        // first LED)
        if ( iLEDIdx < NUM_STEPS_LED_BAR - 1 )
        {
            pLEDLayout->addStretch();
        }

        pLEDLayout->addWidget ( vecpLEDs[iLEDIdx]->getLabelPointer() );
    }

    // initialize bar meter
    pProgressBar = new QProgressBar();
    pProgressBar->setOrientation ( Qt::Vertical );
    pProgressBar->setRange ( 0, 100 * NUM_STEPS_LED_BAR );
    pProgressBar->setFormat ( "" ); // suppress percent numbers
    pProgressBar->setStyleSheet (
        "QProgressBar        { margin:     1px;"
        "                      padding:    1px; "
        "                      width:      15px; }"
        "QProgressBar::chunk { background: green; }" );

    // setup stacked layout for meter type switching mechanism
    pStackedLayout = new QStackedLayout ( this );
    pStackedLayout->addWidget ( pLEDMeter );
    pStackedLayout->addWidget ( pProgressBar );

    // according to QScrollArea description: "When using a scroll area to display the
    // contents of a custom widget, it is important to ensure that the size hint of
    // the child widget is set to a suitable value."
    pProgressBar->setMinimumSize ( QSize ( 19, 1 ) );
    pLEDMeter->setMinimumSize    ( QSize ( 1, 1 ) );

    // update the meter type (using the default value of the meter type)
    SetLevelMeterType ( eLevelMeterType );
}

CMultiColorLEDBar::~CMultiColorLEDBar()
{
    // clean up the LED objects
    for ( int iLEDIdx = 0; iLEDIdx < NUM_STEPS_LED_BAR; iLEDIdx++ )
    {
        delete vecpLEDs[iLEDIdx];
    }
}

void CMultiColorLEDBar::changeEvent ( QEvent* curEvent )
{
    // act on enabled changed state
    if ( curEvent->type() == QEvent::EnabledChange )
    {
        // reset all LEDs
        Reset ( this->isEnabled() );
    }
}

void CMultiColorLEDBar::Reset ( const bool bEnabled )
{
    // update state of all LEDs
    for ( int iLEDIdx = 0; iLEDIdx < NUM_STEPS_LED_BAR; iLEDIdx++ )
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

void CMultiColorLEDBar::SetLevelMeterType ( const ELevelMeterType eNType )
{
    eLevelMeterType = eNType;

    switch ( eNType )
    {
    case MT_LED:
        pStackedLayout->setCurrentIndex ( 0 );
        break;

    case MT_BAR:
        pStackedLayout->setCurrentIndex ( 1 );
        break;
    }
}

void CMultiColorLEDBar::setValue ( const double dValue )
{
    if ( this->isEnabled() )
    {
        switch ( eLevelMeterType )
        {
        case MT_LED:
            // update state of all LEDs for current level value
            for ( int iLEDIdx = 0; iLEDIdx < NUM_STEPS_LED_BAR; iLEDIdx++ )
            {
                // set active LED color if value is above current LED index
                if ( iLEDIdx < dValue )
                {
                    // check which color we should use (green, yellow or red)
                    if ( iLEDIdx < YELLOW_BOUND_LED_BAR )
                    {
                        // green region
                        vecpLEDs[iLEDIdx]->setColor ( cLED::RL_GREEN );
                    }
                    else
                    {
                        if ( iLEDIdx < RED_BOUND_LED_BAR )
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
            break;

        case MT_BAR:
            pProgressBar->setValue ( 100 * dValue );
            break;
        }
    }
}

CMultiColorLEDBar::cLED::cLED ( QWidget* parent ) :
    BitmCubeRoundDisabled ( QString::fromUtf8 ( ":/png/LEDs/res/CLEDDisabledSmall.png" ) ),
    BitmCubeRoundGrey     ( QString::fromUtf8 ( ":/png/LEDs/res/HLEDGreySmall.png" ) ),
    BitmCubeRoundGreen    ( QString::fromUtf8 ( ":/png/LEDs/res/HLEDGreenSmall.png" ) ),
    BitmCubeRoundYellow   ( QString::fromUtf8 ( ":/png/LEDs/res/HLEDYellowSmall.png" ) ),
    BitmCubeRoundRed      ( QString::fromUtf8 ( ":/png/LEDs/res/HLEDRedSmall.png" ) )
{
    // create LED label
    pLEDLabel = new QLabel ( "", parent );

    // bitmap defines minimum size of the label
    pLEDLabel->setMinimumSize ( BitmCubeRoundGrey.width(), BitmCubeRoundGrey.height() );

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
