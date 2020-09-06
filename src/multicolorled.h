/******************************************************************************\
 * Copyright (c) 2004-2020
 *
 * Author(s):
 *  Volker Fischer
 *
 * Description:
 *
 * SetLight():
 *  0: Green
 *  1: Yellow
 *  2: Red
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

#pragma once

#include <QLabel>
#include <QPixmap>
#include <QIcon>
#include "global.h"


/* Classes ********************************************************************/
class CMultiColorLED : public QLabel
{
public:
    enum ELightColor
    {
        RL_DISABLED,
        RL_GREY,
        RL_GREEN,
        RL_YELLOW,
        RL_RED
    };

    CMultiColorLED ( QWidget* parent = nullptr, Qt::WindowFlags f = nullptr );

    void Reset();
    void SetLight ( const ELightColor eNewStatus );

protected:
    ELightColor eColorFlag;

    virtual void changeEvent ( QEvent* curEvent );
    void SetColor ( const ELightColor eNewColorFlag );

    QPixmap BitmCubeDisabled;
    QPixmap BitmCubeGrey;
    QPixmap BitmCubeGreen;
    QPixmap BitmCubeYellow;
    QPixmap BitmCubeRed;

    int     iUpdateTime;

    bool    bFlagRedLi;
    bool    bFlagGreenLi;
    bool    bFlagYellowLi;
};
