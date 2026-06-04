/******************************************************************************\
 * Copyright (c) 2004-2026
 *
 * Author(s):
 *  Volker Fischer
 *
 * As of Jamulus 3.12.1dev (commit eb172d47): All new source code contributions must be licensed
 * under AGPL 3.0 or any later version.
 *
 * Existing code: Code contributed before 3.12.1dev (commit eb172d47) was licensed under GPL 2.0+.
 * This code will be licensed under GPL 3.0 (or any later version) from
 * 3.12.1dev (commit eb172d47).  When distributed as part of Jamulus, the AGPL 3.0 terms govern
 * the combined work, including network use provisions.
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
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 * ---------------------------------------------------------------------------
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
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
    Q_OBJECT

public:
    enum ELightColor
    {
        RL_DISABLED,
        RL_GREY,
        RL_GREEN,
        RL_YELLOW,
        RL_RED
    };

    enum EType
    {
        MT_LED,
        MT_INDICATOR
    };

    CMultiColorLED ( QWidget* parent = nullptr );

    void Reset();
    void SetLight ( const ELightColor eNewStatus );
    void SetType ( const EType eNType );

protected:
    ELightColor eColorFlag;

    virtual void changeEvent ( QEvent* curEvent );
    void         SetColor ( const ELightColor eNewColorFlag );

    QPixmap BitmCubeDisabled;
    QPixmap BitmCubeGrey;
    QPixmap BitmCubeGreen;
    QPixmap BitmCubeYellow;
    QPixmap BitmCubeRed;
    QPixmap BitmIndicatorGreen;
    QPixmap BitmIndicatorYellow;
    QPixmap BitmIndicatorRed;

    int   iUpdateTime;
    EType eType;

    bool bFlagRedLi;
    bool bFlagGreenLi;
    bool bFlagYellowLi;
};
