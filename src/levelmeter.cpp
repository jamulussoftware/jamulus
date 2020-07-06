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
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 *
\******************************************************************************/

#include "levelmeter.h"


/* Implementation *************************************************************/
CLevelMeter::CLevelMeter ( QWidget* parent, Qt::WindowFlags f ) :
    QWidget ( parent, f ),
    eLevelMeterType ( MT_BAR )
{
    // initialize LED meter
    QWidget*     pLEDMeter  = new QWidget();
    QVBoxLayout* pLEDLayout = new QVBoxLayout ( pLEDMeter );
    pLEDLayout->setAlignment ( Qt::AlignHCenter );
    pLEDLayout->setMargin    ( 0 );
    pLEDLayout->setSpacing   ( 0 );

    // create LEDs plus the clip LED
    vecpLEDs.Init ( NUM_LEDS_INCL_CLIP_LED );

    for ( int iLEDIdx = NUM_LEDS_INCL_CLIP_LED - 1; iLEDIdx >= 0; iLEDIdx-- )
    {
        // create LED object
        vecpLEDs[iLEDIdx] = new cLED ( parent );

        // add LED to layout with spacer (do not add spacer on the bottom of the first LED)
        if ( iLEDIdx < NUM_LEDS_INCL_CLIP_LED - 1 )
        {
            pLEDLayout->addStretch();
        }

        pLEDLayout->addWidget ( vecpLEDs[iLEDIdx]->GetLabelPointer() );
    }

    // initialize bar meter
    pBarMeter = new QProgressBar();
    pBarMeter->setOrientation ( Qt::Vertical );
    pBarMeter->setRange ( 0, 100 * NUM_STEPS_LED_BAR ); // use factor 100 to reduce quantization (bar is continuous)
    pBarMeter->setFormat ( "" ); // suppress percent numbers

    // setup stacked layout for meter type switching mechanism
    pStackedLayout = new QStackedLayout ( this );
    pStackedLayout->addWidget ( pLEDMeter );
    pStackedLayout->addWidget ( pBarMeter );

    // according to QScrollArea description: "When using a scroll area to display the
    // contents of a custom widget, it is important to ensure that the size hint of
    // the child widget is set to a suitable value."
    pBarMeter->setMinimumSize ( QSize ( 1, 1 ) );
    pLEDMeter->setMinimumSize ( QSize ( 1, 1 ) );

    // update the meter type (using the default value of the meter type)
    SetLevelMeterType ( eLevelMeterType );

    // setup clip indicator timer
    TimerClip.setSingleShot ( true );
    TimerClip.setInterval ( CLIP_IND_TIME_OUT_MS );


    // Connections -------------------------------------------------------------
    QObject::connect ( &TimerClip, &QTimer::timeout,
        this, &CLevelMeter::ClipReset );
}

CLevelMeter::~CLevelMeter()
{
    // clean up the LED objects
    for ( int iLEDIdx = 0; iLEDIdx < NUM_LEDS_INCL_CLIP_LED; iLEDIdx++ )
    {
        delete vecpLEDs[iLEDIdx];
    }
}

void CLevelMeter::SetLevelMeterType ( const ELevelMeterType eNType )
{
    eLevelMeterType = eNType;

    switch ( eNType )
    {
    case MT_LED:
        // initialize all LEDs
        for ( int iLEDIdx = 0; iLEDIdx < NUM_LEDS_INCL_CLIP_LED; iLEDIdx++ )
        {
            vecpLEDs[iLEDIdx]->SetColor ( cLED::RL_BLACK );
        }
        pStackedLayout->setCurrentIndex ( 0 );
        break;

    case MT_BAR:
        pStackedLayout->setCurrentIndex ( 1 );
        break;

    case MT_SLIM_BAR:
        // set all LEDs to disabled, otherwise we would not get our desired small width
        for ( int iLEDIdx = 0; iLEDIdx < NUM_LEDS_INCL_CLIP_LED; iLEDIdx++ )
        {
            vecpLEDs[iLEDIdx]->SetColor ( cLED::RL_DISABLED );
        }

        pStackedLayout->setCurrentIndex ( 1 );
        break;
    }

    // update bar meter style and reset clip state
    SetBarMeterStyleAndClipStatus ( eNType, false );
}

void CLevelMeter::SetBarMeterStyleAndClipStatus ( const ELevelMeterType eNType,
                                                  const bool            bIsClip )
{
    switch ( eNType )
    {
    case MT_SLIM_BAR:
        if ( bIsClip )
        {
            pBarMeter->setStyleSheet (
                "QProgressBar        { border:     0px solid red;"
                "                      margin:     0px;"
                "                      padding:    0px;"
                "                      width:      4px;"
                "                      background: red; }"
                "QProgressBar::chunk { background: green; }" );
        }
        else
        {
            pBarMeter->setStyleSheet (
                "QProgressBar        { border:     0px;"
                "                      margin:     0px;"
                "                      padding:    0px;"
                "                      width:      4px; }"
                "QProgressBar::chunk { background: green; }" );
        }
        break;

    default: /* MT_BAR */
        if ( bIsClip )
        {
            pBarMeter->setStyleSheet (
                "QProgressBar        { border:     2px solid red;"
                "                      margin:     1px;"
                "                      padding:    1px;"
                "                      width:      15px;"
                "                      background: transparent; }"
                "QProgressBar::chunk { background: green; }" );
        }
        else
        {
            pBarMeter->setStyleSheet (
                "QProgressBar        { margin:     1px;"
                "                      padding:    1px;"
                "                      width:      15px; }"
                "QProgressBar::chunk { background: green; }" );
        }
        break;
    }
}

void CLevelMeter::SetValue ( const double dValue )
{
    switch ( eLevelMeterType )
    {
    case MT_LED:
        // update state of all LEDs for current level value (except of the clip LED)
        for ( int iLEDIdx = 0; iLEDIdx < NUM_STEPS_LED_BAR; iLEDIdx++ )
        {
            // set active LED color if value is above current LED index
            if ( iLEDIdx < dValue )
            {
                // check which color we should use (green, yellow or red)
                if ( iLEDIdx < YELLOW_BOUND_LED_BAR )
                {
                    // green region
                    vecpLEDs[iLEDIdx]->SetColor ( cLED::RL_GREEN );
                }
                else
                {
                    if ( iLEDIdx < RED_BOUND_LED_BAR )
                    {
                        // yellow region
                        vecpLEDs[iLEDIdx]->SetColor ( cLED::RL_YELLOW );
                    }
                    else
                    {
                        // red region
                        vecpLEDs[iLEDIdx]->SetColor ( cLED::RL_RED );
                    }
                }
            }
            else
            {
                // we use black LED for inactive state
                vecpLEDs[iLEDIdx]->SetColor ( cLED::RL_BLACK );
            }
        }
        break;

    case MT_BAR:
    case MT_SLIM_BAR:
        pBarMeter->setValue ( 100 * dValue );
        break;
    }

    // clip indicator management (note that in case of clipping, i.e. full
    // scale level, the value is above NUM_STEPS_LED_BAR since the minimum
    // value of int16 is -32768 but we normalize with 32767 -> therefore
    // we really only show the clipping indicator, if actually the largest
    // value of int16 is used)
    if ( dValue > NUM_STEPS_LED_BAR )
    {
        switch ( eLevelMeterType )
        {
        case MT_LED:
            vecpLEDs[NUM_STEPS_LED_BAR]->SetColor ( cLED::RL_RED );
            break;

        case MT_BAR:
        case MT_SLIM_BAR:
            SetBarMeterStyleAndClipStatus ( eLevelMeterType, true );
            break;
        }

        TimerClip.start();
    }
}

void CLevelMeter::ClipReset()
{
    // we manually want to reset the clipping indicator: stop timer and reset
    // clipping indicator GUI element
    TimerClip.stop();

    switch ( eLevelMeterType )
    {
    case MT_LED:
        vecpLEDs[NUM_STEPS_LED_BAR]->SetColor ( cLED::RL_BLACK );
        break;

    case MT_BAR:
    case MT_SLIM_BAR:
        SetBarMeterStyleAndClipStatus ( eLevelMeterType, false );
        break;
    }
}


CLevelMeter::cLED::cLED ( QWidget* parent ) :
    BitmCubeRoundBlack  ( QString::fromUtf8 ( ":/png/LEDs/res/HLEDBlackSmall.png" ) ),
    BitmCubeRoundGreen  ( QString::fromUtf8 ( ":/png/LEDs/res/HLEDGreenSmall.png" ) ),
    BitmCubeRoundYellow ( QString::fromUtf8 ( ":/png/LEDs/res/HLEDYellowSmall.png" ) ),
    BitmCubeRoundRed    ( QString::fromUtf8 ( ":/png/LEDs/res/HLEDRedSmall.png" ) )
{
    // create LED label
    pLEDLabel = new QLabel ( "", parent );

    // set initial bitmap
    pLEDLabel->setPixmap ( BitmCubeRoundBlack );
    eCurLightColor = RL_BLACK;
}

void CLevelMeter::cLED::SetColor ( const ELightColor eNewColor )
{
    // only update LED if color has changed
    if ( eNewColor != eCurLightColor )
    {
        switch ( eNewColor )
        {
        case RL_DISABLED:
            // note that this is required for the compact channel mode
            pLEDLabel->setPixmap ( QPixmap() );
            break;

        case RL_BLACK:
            pLEDLabel->setPixmap ( BitmCubeRoundBlack );
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
