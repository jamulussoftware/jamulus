/******************************************************************************\
 * Copyright (c) 2004-2025
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
CLevelMeter::CLevelMeter ( QWidget* parent ) : QWidget ( parent ), eLevelMeterType ( MT_BAR_WIDE )
{
    // initialize LED meter
    QWidget*     pLEDMeter  = new QWidget();
    QVBoxLayout* pLEDLayout = new QVBoxLayout ( pLEDMeter );
    pLEDLayout->setAlignment ( Qt::AlignHCenter );
    pLEDLayout->setContentsMargins ( 0, 0, 0, 0 );
    pLEDLayout->setSpacing ( 0 );

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
    pBarMeter->setFormat ( "" );                        // suppress percent numbers

    // setup stacked layout for meter type switching mechanism
    pMinStackedLayout = new CMinimumStackedLayout ( this );
    pMinStackedLayout->addWidget ( pLEDMeter );
    pMinStackedLayout->addWidget ( pBarMeter );

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
    QObject::connect ( &TimerClip, &QTimer::timeout, this, &CLevelMeter::ClipReset );
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
    case MT_LED_STRIPE:
        // initialize all LEDs
        for ( int iLEDIdx = 0; iLEDIdx < NUM_LEDS_INCL_CLIP_LED; iLEDIdx++ )
        {
            vecpLEDs[iLEDIdx]->SetColor ( cLED::RL_BLACK );
        }
        pMinStackedLayout->setCurrentIndex ( 0 );
        break;

    case MT_LED_ROUND_BIG:
        // initialize all LEDs
        for ( int iLEDIdx = 0; iLEDIdx < NUM_LEDS_INCL_CLIP_LED; iLEDIdx++ )
        {
            vecpLEDs[iLEDIdx]->SetColor ( cLED::RL_ROUND_BIG_BLACK );
        }
        pMinStackedLayout->setCurrentIndex ( 0 );
        break;

    case MT_LED_ROUND_SMALL:
        // initialize all LEDs
        for ( int iLEDIdx = 0; iLEDIdx < NUM_LEDS_INCL_CLIP_LED; iLEDIdx++ )
        {
            vecpLEDs[iLEDIdx]->SetColor ( cLED::RL_ROUND_SMALL_BLACK );
        }
        pMinStackedLayout->setCurrentIndex ( 0 );
        break;

    case MT_BAR_WIDE:
    case MT_BAR_NARROW:
        pMinStackedLayout->setCurrentIndex ( 1 );
        break;
    }

    // update bar meter style and reset clip state
    SetBarMeterStyleAndClipStatus ( eNType, false );
}

void CLevelMeter::SetBarMeterStyleAndClipStatus ( const ELevelMeterType eNType, const bool bIsClip )
{
    switch ( eNType )
    {
    case MT_BAR_NARROW:
        if ( bIsClip )
        {
            pBarMeter->setStyleSheet ( "QProgressBar        { border:     0px solid red;"
                                       "                      margin:     0px;"
                                       "                      padding:    0px;"
                                       "                      width:      4px;"
                                       "                      background: red; }"
                                       "QProgressBar::chunk { background: green; }" );
        }
        else
        {
            pBarMeter->setStyleSheet ( "QProgressBar        { border:     0px;"
                                       "                      margin:     0px;"
                                       "                      padding:    0px;"
                                       "                      width:      4px; }"
                                       "QProgressBar::chunk { background: green; }" );
        }
        break;

    default: /* MT_BAR_WIDE */
        if ( bIsClip )
        {
            pBarMeter->setStyleSheet ( "QProgressBar        { border:     2px solid red;"
                                       "                      margin:     1px;"
                                       "                      padding:    1px;"
                                       "                      width:      15px;"
                                       "                      background: transparent; }"
                                       "QProgressBar::chunk { background: green; }" );
        }
        else
        {
            pBarMeter->setStyleSheet ( "QProgressBar        { margin:     1px;"
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
    case MT_LED_STRIPE:
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

    case MT_LED_ROUND_BIG:
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
                    vecpLEDs[iLEDIdx]->SetColor ( cLED::RL_ROUND_BIG_GREEN );
                }
                else
                {
                    if ( iLEDIdx < RED_BOUND_LED_BAR )
                    {
                        // yellow region
                        vecpLEDs[iLEDIdx]->SetColor ( cLED::RL_ROUND_BIG_YELLOW );
                    }
                    else
                    {
                        // red region
                        vecpLEDs[iLEDIdx]->SetColor ( cLED::RL_ROUND_BIG_RED );
                    }
                }
            }
            else
            {
                // we use black LED for inactive state
                vecpLEDs[iLEDIdx]->SetColor ( cLED::RL_ROUND_BIG_BLACK );
            }
        }
        break;

    case MT_LED_ROUND_SMALL:
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
                    vecpLEDs[iLEDIdx]->SetColor ( cLED::RL_ROUND_SMALL_GREEN );
                }
                else
                {
                    if ( iLEDIdx < RED_BOUND_LED_BAR )
                    {
                        // yellow region
                        vecpLEDs[iLEDIdx]->SetColor ( cLED::RL_ROUND_SMALL_YELLOW );
                    }
                    else
                    {
                        // red region
                        vecpLEDs[iLEDIdx]->SetColor ( cLED::RL_ROUND_SMALL_RED );
                    }
                }
            }
            else
            {
                // we use black LED for inactive state
                vecpLEDs[iLEDIdx]->SetColor ( cLED::RL_ROUND_SMALL_BLACK );
            }
        }
        break;

    case MT_BAR_WIDE:
    case MT_BAR_NARROW:
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
        case MT_LED_STRIPE:
            vecpLEDs[NUM_STEPS_LED_BAR]->SetColor ( cLED::RL_RED );
            break;

        case MT_LED_ROUND_BIG:
            vecpLEDs[NUM_STEPS_LED_BAR]->SetColor ( cLED::RL_ROUND_BIG_RED );
            break;

        case MT_LED_ROUND_SMALL:
            vecpLEDs[NUM_STEPS_LED_BAR]->SetColor ( cLED::RL_ROUND_SMALL_RED );
            break;

        case MT_BAR_WIDE:
        case MT_BAR_NARROW:
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
    case MT_LED_STRIPE:
        vecpLEDs[NUM_STEPS_LED_BAR]->SetColor ( cLED::RL_BLACK );
        break;

    case MT_LED_ROUND_BIG:
        vecpLEDs[NUM_STEPS_LED_BAR]->SetColor ( cLED::RL_ROUND_BIG_BLACK );
        break;

    case MT_LED_ROUND_SMALL:
        vecpLEDs[NUM_STEPS_LED_BAR]->SetColor ( cLED::RL_ROUND_SMALL_BLACK );
        break;

    case MT_BAR_WIDE:
    case MT_BAR_NARROW:
        SetBarMeterStyleAndClipStatus ( eLevelMeterType, false );
        break;
    }
}

CLevelMeter::cLED::cLED ( QWidget* parent ) :
    BitmCubeLedBlack ( QString::fromUtf8 ( ":/png/LEDs/res/HLEDBlack.png" ) ),
    BitmCubeLedGreen ( QString::fromUtf8 ( ":/png/LEDs/res/HLEDGreen.png" ) ),
    BitmCubeLedYellow ( QString::fromUtf8 ( ":/png/LEDs/res/HLEDYellow.png" ) ),
    BitmCubeLedRed ( QString::fromUtf8 ( ":/png/LEDs/res/HLEDRed.png" ) ),
    BitmCubeRoundSmallLedBlack ( QString::fromUtf8 ( ":/png/LEDs/res/CLEDBlackSmall.png" ) ),
    BitmCubeRoundSmallLedGreen ( QString::fromUtf8 ( ":/png/LEDs/res/CLEDGreenSmall.png" ) ),
    BitmCubeRoundSmallLedYellow ( QString::fromUtf8 ( ":/png/LEDs/res/CLEDYellowSmall.png" ) ),
    BitmCubeRoundSmallLedRed ( QString::fromUtf8 ( ":/png/LEDs/res/CLEDRedSmall.png" ) ),
    BitmCubeRoundBigLedBlack ( QString::fromUtf8 ( ":/png/LEDs/res/CLEDBlackBig.png" ) ),
    BitmCubeRoundBigLedGreen ( QString::fromUtf8 ( ":/png/LEDs/res/CLEDGreenBig.png" ) ),
    BitmCubeRoundBigLedYellow ( QString::fromUtf8 ( ":/png/LEDs/res/CLEDYellowBig.png" ) ),
    BitmCubeRoundBigLedRed ( QString::fromUtf8 ( ":/png/LEDs/res/CLEDRedBig.png" ) )
{
    // create LED label
    pLEDLabel = new QLabel ( "", parent );

    // set initial bitmap
    pLEDLabel->setPixmap ( BitmCubeLedBlack );
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
            pLEDLabel->setPixmap ( BitmCubeLedBlack );
            break;

        case RL_GREEN:
            pLEDLabel->setPixmap ( BitmCubeLedGreen );
            break;

        case RL_YELLOW:
            pLEDLabel->setPixmap ( BitmCubeLedYellow );
            break;

        case RL_RED:
            pLEDLabel->setPixmap ( BitmCubeLedRed );
            break;

        case RL_ROUND_SMALL_BLACK:
            pLEDLabel->setPixmap ( BitmCubeRoundSmallLedBlack );
            break;

        case RL_ROUND_SMALL_GREEN:
            pLEDLabel->setPixmap ( BitmCubeRoundSmallLedGreen );
            break;

        case RL_ROUND_SMALL_YELLOW:
            pLEDLabel->setPixmap ( BitmCubeRoundSmallLedYellow );
            break;

        case RL_ROUND_SMALL_RED:
            pLEDLabel->setPixmap ( BitmCubeRoundSmallLedRed );
            break;

        case RL_ROUND_BIG_BLACK:
            pLEDLabel->setPixmap ( BitmCubeRoundBigLedBlack );
            break;

        case RL_ROUND_BIG_GREEN:
            pLEDLabel->setPixmap ( BitmCubeRoundBigLedGreen );
            break;

        case RL_ROUND_BIG_YELLOW:
            pLEDLabel->setPixmap ( BitmCubeRoundBigLedYellow );
            break;

        case RL_ROUND_BIG_RED:
            pLEDLabel->setPixmap ( BitmCubeRoundBigLedRed );
            break;
        }
        eCurLightColor = eNewColor;
    }
}
