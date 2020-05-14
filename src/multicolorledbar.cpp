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

#include <QMouseEvent>

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

    // create clip LED
    pClipLED = new cLED ( parent );
    pLEDLayout->addWidget ( pClipLED->getLabelPointer() );
    pLEDLayout->addStretch(2); // Keep clip LED separated from the rest of the LED bar

    // create LEDs
    vecpLEDs.Init ( NUM_STEPS_LED_BAR );

    pPairedBar = NULL;

    for ( int iLEDIdx = NUM_STEPS_LED_BAR - 1; iLEDIdx >= 0; iLEDIdx-- )
    {
        // create LED object
        vecpLEDs[iLEDIdx] = new cLED ( parent );

        // add LED to layout with spacer (do not add spacer after last LED)
        pLEDLayout->addWidget ( vecpLEDs[iLEDIdx]->getLabelPointer() );
        if ( iLEDIdx > 0 )
        {
            pLEDLayout->addStretch(1);
        }
    }

    // initialize bar meter
    QWidget*  pBarMeter = new QWidget();
    pLevelBar = new CLevelBar ( pBarMeter, fClipLimitRatio );

    // setup stacked layout for meter type switching mechanism
    pStackedLayout = new QStackedLayout ( this );
    pStackedLayout->addWidget ( pLEDMeter );
    pStackedLayout->addWidget ( pBarMeter );

    // according to QScrollArea description: "When using a scroll area to display the
    // contents of a custom widget, it is important to ensure that the size hint of
    // the child widget is set to a suitable value."
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
    delete pClipLED;
    delete pLevelBar;
    delete pStackedLayout;
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
    if ( eLevelMeterType == MT_LED )
    {
        // update state of all LEDs
        for ( int iLEDIdx = 0; iLEDIdx < NUM_STEPS_LED_BAR; iLEDIdx++ )
        {
            // different reset behavior for enabled and disabled control
            if ( bEnabled )
            {
                vecpLEDs[iLEDIdx]->setColor ( cLED::RL_GREY );
            }
            else
            {
                vecpLEDs[iLEDIdx]->setColor ( cLED::RL_DISABLED );
            }
        }
        pClipLED->setColor ( cLED::RL_GREY );
    }
    else if (eLevelMeterType == MT_BAR )
    {
        pLevelBar->Reset();
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
    // Note: scale of input argument dValue is [0..NUM_STEPS_LED_BAR]
    if ( this->isEnabled() )
    {
        switch ( eLevelMeterType )
        {
        case MT_LED:
            // update state of all LEDs for current level value
            for ( int iLEDIdx = 0; iLEDIdx < NUM_STEPS_LED_BAR; iLEDIdx++ )
            {
                // set active LED color if value is above current LED index
                if ( iLEDIdx >= dValue )
                {
                    // we use grey LED for inactive state
                    vecpLEDs[iLEDIdx]->setColor ( cLED::RL_GREY );
                }
                // check which color we should use (green, yellow or red)
                else if ( iLEDIdx < YELLOW_BOUND_LED_BAR )
                {
                    // green region
                    vecpLEDs[iLEDIdx]->setColor ( cLED::RL_GREEN );
                }
                else if ( iLEDIdx < RED_BOUND_LED_BAR )
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
            if ( dValue > fClipLimitRatio * NUM_STEPS_LED_BAR)
            {
                // indicate clipping signal until user click
                pClipLED->setColor ( cLED::RL_RED );
            }

            break;

        case MT_BAR:
            pLevelBar->setValue ( 100 * dValue );
            break;
        }
    }
}

void CMultiColorLEDBar::mousePressEvent ( QMouseEvent* event )
{
    if ( event->button() == Qt::LeftButton )
    {
        // Mainly to reset the clip LED, but might be useful to reset the whole
        // color LED bar.
        Reset ( isEnabled() );
        if (pPairedBar != NULL )
        {
            pPairedBar->Reset ( pPairedBar->isEnabled() );
        }
    }
}

void CMultiColorLEDBar::setPairedBar ( CMultiColorLEDBar* pBar )
{
    this->pPairedBar = pBar;
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

CLevelBar::CLevelBar ( QWidget* pParent, float fClipRatio ):
    fClipLimitRatio ( fClipRatio )
{
    QVBoxLayout* pBarLayout = new QVBoxLayout ( pParent );

    pBarLayout->setAlignment ( Qt::AlignHCenter );
    pBarLayout->setMargin    ( 0 );
    pBarLayout->setSpacing   ( 0 );
    pBarLayout->setContentsMargins ( 0, 0, 0, 0 );

    // create clip LED
    pClipBar = new QProgressBar();
    pBarLayout->addWidget ( pClipBar );
    pBarLayout->addSpacing ( 5 ); // Keep clip bar separated from the rest of the bar

    pClipBar->setOrientation ( Qt::Vertical );
    pClipBar->setRange ( 0, 1 );
    pClipBar->setFormat ( "" ); // suppress percent numbers
    pClipBar->setMinimumSize ( QSize ( 19, 1 ) ); // 15px + 2 * 1px + 2 * 1px = 19px
    pClipBar->setStyleSheet (
        "QProgressBar        { margin:     1px;"
        "                      padding:    1px; "
        "                      width:      15px; "
        "                      max-height: 8px; } "
        "QProgressBar::chunk { background: red; }" );


    pBar = new QProgressBar();
    pBarLayout->addWidget ( pBar );

    pBar->setOrientation ( Qt::Vertical );
    pBar->setRange ( 0, 100 * NUM_STEPS_LED_BAR );
    pBar->setFormat ( "" ); // suppress percent numbers
    pBar->setMinimumSize ( QSize ( 19, 1 ) ); // 15px + 2 * 1px + 2 * 1px = 19px
    pBar->setStyleSheet (
        "QProgressBar        { margin:     1px;"
        "                      padding:    1px; "
        "                      width:      15px; }"
        "QProgressBar::chunk { background: green; }" );
}

CLevelBar::~CLevelBar()
{
    delete pClipBar;
}

void CLevelBar::setValue ( int dValue )
{
    pBar->setValue ( dValue );
    if ( dValue > fClipLimitRatio * 100 * NUM_STEPS_LED_BAR ) {
        pClipBar->setValue ( 1 );
    }
}

void CLevelBar::Reset()
{
    pBar->setValue ( 0 );
    pClipBar->setValue ( 0 );
}
