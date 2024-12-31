/******************************************************************************\
 * Copyright (c) 2004-2024
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
CLevelMeter::CLevelMeter ( QObject* parent )
    :   QObject(parent),
        m_doubleVal(0.0)
{
   // setup clip indicator timer
    TimerClip.setSingleShot ( true );
    TimerClip.setInterval ( CLIP_IND_TIME_OUT_MS );

    // Connections -------------------------------------------------------------
    QObject::connect ( &TimerClip, &QTimer::timeout, this, &CLevelMeter::ClipReset );
}

CLevelMeter::~CLevelMeter()
{
    // nothing for now
    ;
}

bool CLevelMeter::clipStatus()
{
    return m_clipStatus;
}

void CLevelMeter::setClipStatus( const bool bIsClip )
{
    m_clipStatus = bIsClip;

    emit clipStatusChanged();
}

void CLevelMeter::setDoubleVal ( const double value )
{
    m_doubleVal = value / NUM_STEPS_LED_BAR;
    emit doubleValChanged();
    // qDebug() << "Level doubleVal: " << m_doubleVal;

    // clip indicator management (note that in case of clipping, i.e. full
    // scale level, the value is above NUM_STEPS_LED_BAR since the minimum
    // value of int16 is -32768 but we normalize with 32767 -> therefore
    // we really only show the clipping indicator, if actually the largest
    // value of int16 is used)
    if ( value > NUM_STEPS_LED_BAR )
    {
        qDebug() << "Level value: " << value;
        setClipStatus(true);
        emit clipStatusChanged();
        TimerClip.start();
    }
}

void CLevelMeter::ClipReset()
{
    // we manually want to reset the clipping indicator: stop timer and reset
    // clipping indicator GUI element
    TimerClip.stop();
    setClipStatus(false);
    emit clipStatusChanged();
}
