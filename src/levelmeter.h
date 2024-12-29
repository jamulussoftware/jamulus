/******************************************************************************\
 * Copyright (c) 2004-2024
 *
 * Author(s):
 *  Volker Fischer
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

#pragma once

#include <QTimer>
#include "util.h"
#include "global.h"

/* Definitions ****************************************************************/
#define NUM_LEDS_INCL_CLIP_LED ( NUM_STEPS_LED_BAR + 1 )
#define CLIP_IND_TIME_OUT_MS   20000

/* Classes ********************************************************************/
class CLevelMeter : public QObject
{
    Q_OBJECT

    Q_PROPERTY(double doubleVal READ doubleVal WRITE setDoubleVal NOTIFY doubleValChanged)
    Q_PROPERTY(bool clipStatus READ clipStatus WRITE setClipStatus NOTIFY clipStatusChanged)

public:
    CLevelMeter ( QObject* parent = nullptr );
    virtual ~CLevelMeter();

    double doubleVal() const { return m_doubleVal; }
    bool clipStatus(); // const { return m_clipStatus; }

protected:
    // virtual void mousePressEvent ( QMouseEvent* ) override { ClipReset(); }
    QTimer TimerClip;

    double m_doubleVal; // level of this meter
    bool m_clipStatus;

public slots:
    void ClipReset();

    void setDoubleVal(double value);
    void setClipStatus( bool clipStatus );


signals:
    void doubleValChanged();
    void clipStatusChanged();
};
