/******************************************************************************\
 * Copyright (c) 2004-2009
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
CMultiColorLEDBar::CMultiColorLEDBar ( QWidget* parent, Qt::WindowFlags f )
    : QLabel ( parent, f ),
    BitmCubeRoundGrey   ( QString::fromUtf8 ( ":/png/LEDs/res/VRLEDGreySmall.png" ) ),
    BitmCubeRoundGreen  ( QString::fromUtf8 ( ":/png/LEDs/res/VRLEDGreySmall.png" ) ),
    BitmCubeRoundYellow ( QString::fromUtf8 ( ":/png/LEDs/res/VRLEDGreySmall.png" ) ),
    BitmCubeRoundRed    ( QString::fromUtf8 ( ":/png/LEDs/res/VRLEDGreySmall.png" ) ),
    BitmCubeEdgeGrey    ( QString::fromUtf8 ( ":/png/LEDs/res/VLEDGreySmall.png" ) ),
    BitmCubeEdgeGreen   ( QString::fromUtf8 ( ":/png/LEDs/res/VLEDGreySmall.png" ) ),
    BitmCubeEdgeYellow  ( QString::fromUtf8 ( ":/png/LEDs/res/VLEDGreySmall.png" ) ),
    BitmCubeEdgeRed     ( QString::fromUtf8 ( ":/png/LEDs/res/VLEDGreySmall.png" ) )
{

}
