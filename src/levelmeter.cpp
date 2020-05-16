/******************************************************************************\
 * Copyright (c) 2004-2020
 *
 * Author(s):
 *  Volker Fischer
 *
 * Description:
 *  Implements a Level meter, displaying as LED or progress bar.
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

#include "levelmeter.h"


/* Implementation *************************************************************/
CLevelMeter::CLevelMeter ( QWidget* parent, Qt::WindowFlags f ) :
    QWidget ( parent, f )
{
    pPairedBar = NULL;

    // initialize LED meter
    QWidget*     pwLEDMeter  = new QWidget ( parent );
    pLevelLED = new CLevelMeterLED ( pwLEDMeter );

    // initialize bar meter
    QWidget*  pwBarMeter = new QWidget ( parent );
    pLevelBar = new CLevelMeterBar ( pwBarMeter );

    // setup stacked layout for meter type switching mechanism
    pStackedLayout = new QStackedLayout ( this );
    pStackedLayout->addWidget ( pwLEDMeter );
    pStackedLayout->addWidget ( pwBarMeter );

    // according to QScrollArea description: "When using a scroll area to display the
    // contents of a custom widget, it is important to ensure that the size hint of
    // the child widget is set to a suitable value."
    pwLEDMeter->setMinimumSize    ( QSize ( 1, 1 ) );
    // 15px + 2 * 1px + 2 * 1px = 19px
    pwBarMeter->setMinimumSize    ( QSize ( 19, 1 ) );

    // initialize the meter type to default
    SetLevelMeterType ( MT_BAR );
}

CLevelMeter::~CLevelMeter()
{
    delete pStackedLayout;
}

void CLevelMeter::changeEvent ( QEvent* curEvent )
{
    // act on enabled changed state
    if ( curEvent->type() == QEvent::EnabledChange )
    {
        // reset all LEDs
        Reset();
    }
}

void CLevelMeter::Reset()
{
    pCurrentLevelMeter->Reset();
}
}

void CLevelMeter::SetLevelMeterType ( const ELevelMeterType eNType )
{
    switch ( eNType )
    {
    case MT_LED:
        pStackedLayout->setCurrentIndex ( 0 );
        pCurrentLevelMeter = pLevelLED;
        break;

    case MT_BAR:
        pStackedLayout->setCurrentIndex ( 1 );
        pCurrentLevelMeter = pLevelBar;
        break;
    }
}

void CLevelMeter::SetValue ( const double dValue )
{
    // Scale of input argument dValue is [0..NUM_STEPS_LED_BAR]
    if ( this->isEnabled() )
    {
        pCurrentLevelMeter->SetValue ( dValue );
    }
}

void CLevelMeter::mousePressEvent ( QMouseEvent* event )
{
    if ( event->button() == Qt::LeftButton )
    {
        // Mainly to reset the clip LED, but might be useful to reset the whole
        // color LED bar.
        Reset();
        if (pPairedBar != NULL )
        {
            pPairedBar->Reset();
        }
    }
}

void CLevelMeter::SetPairedBar ( CLevelMeter* pBar )
{
    this->pPairedBar = pBar;
}


/* Internal classes ***********************************************************/
CLevelMeterBar::CLevelMeterBar ( QWidget* pParent )
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

CLevelMeterBar::~CLevelMeterBar()
{
    delete pClipBar;
    delete pBar;
}

void CLevelMeterBar::SetValue ( double dValue )
{
    // Scale of input argument dValue is [0..NUM_STEPS_LED_BAR]
    // Range of the progress bar is [0..100 * NUM_STEPS_LED_BAR]
    pBar->setValue ( 100 * dValue );
    if ( dValue > fClipLimitRatio * NUM_STEPS_LED_BAR ) {
        pClipBar->setValue ( 1 );
    }
}

void CLevelMeterBar::Reset()
{
    pBar->setValue ( 0 );
    pClipBar->setValue ( 0 );
}


CLevelMeterLED::CLevelMeterLED ( QWidget* pParent )
{
    QVBoxLayout* pLEDLayout = new QVBoxLayout ( pParent );
    pLEDLayout->setAlignment ( Qt::AlignHCenter );
    pLEDLayout->setMargin    ( 0 );
    pLEDLayout->setSpacing   ( 0 );

    // create clip LED
    pClipLED = new cLED ( pParent );
    pLEDLayout->addWidget ( pClipLED->getLabelPointer() );
    pLEDLayout->addStretch(2); // Keep clip LED separated from the rest of the LED bar

    // create LEDs
    vecpLEDs.Init ( NUM_STEPS_LED_BAR );

    for ( int iLEDIdx = NUM_STEPS_LED_BAR - 1; iLEDIdx >= 0; iLEDIdx-- )
    {
        // create LED object
        vecpLEDs[iLEDIdx] = new cLED ( pParent );

        // add LED to layout with spacer (do not add spacer after last LED)
        pLEDLayout->addWidget ( vecpLEDs[iLEDIdx]->getLabelPointer() );
        if ( iLEDIdx > 0 )
        {
            pLEDLayout->addStretch(1);
        }
    }
}

CLevelMeterLED::~CLevelMeterLED()
{
    // clean up the LED objects
    for ( int iLEDIdx = 0; iLEDIdx < NUM_STEPS_LED_BAR; iLEDIdx++ )
    {
        delete vecpLEDs[iLEDIdx];
    }
    delete pClipLED;
}

void CLevelMeterLED::SetValue ( double dValue )
{
    // Scale of input argument dValue is [0..NUM_STEPS_LED_BAR]

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
        // indicate clipping signal
        pClipLED->setColor ( cLED::RL_RED );
    }
}

void CLevelMeterLED::Reset()
{
    // update state of all LEDs
    for ( int iLEDIdx = 0; iLEDIdx < NUM_STEPS_LED_BAR; iLEDIdx++ )
    {
        vecpLEDs[iLEDIdx]->setColor ( cLED::RL_GREY );
    }
    pClipLED->setColor ( cLED::RL_GREY );
}

CLevelMeterLED::cLED::cLED ( QWidget* parent ) :
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

void CLevelMeterLED::cLED::setColor ( const ELightColor eNewColor )
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
