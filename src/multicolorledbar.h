/******************************************************************************\
 * Copyright (c) 2004-2010
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
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
\******************************************************************************/

#if !defined ( _MULTCOLORLEDBAR_H__FD6B49B5_87DF_48DD_E1606C2AC__INCLUDED_ )
#define _MULTCOLORLEDBAR_H__FD6B49B5_87DF_48DD_E1606C2AC__INCLUDED_

#include <qframe.h>
#include <qpixmap.h>
#include <qtimer.h>
#include <qlayout.h>
#include "util.h"
#include "global.h"


/* Classes ********************************************************************/
class CMultiColorLEDBar : public QFrame
{
    Q_OBJECT

public:
    CMultiColorLEDBar ( QWidget* parent = 0, Qt::WindowFlags f = 0 );
    virtual ~CMultiColorLEDBar();

    void setValue ( const int value );

protected:
    class cLED
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

        cLED ( QWidget* parent );
        void setColor ( const ELightColor eNewColor );
        QLabel* getLabelPointer() { return pLEDLabel; }

    protected:
        QPixmap     BitmCubeRoundDisabled;
        QPixmap     BitmCubeRoundGrey;
        QPixmap     BitmCubeRoundGreen;
        QPixmap     BitmCubeRoundYellow;
        QPixmap     BitmCubeRoundRed;

        ELightColor eCurLightColor;
        QLabel*     pLEDLabel;
    };

    void Reset ( const bool bEnabled );
    virtual void changeEvent ( QEvent* curEvent );

    QHBoxLayout*     pMainLayout;

    int              iNumLEDs;
    CVector<cLED*>   vecpLEDs;
};

#endif // _MULTCOLORLEDBAR_H__FD6B49B5_87DF_48DD_E1606C2AC__INCLUDED_
