/******************************************************************************\
* Copyright (c) 2004-2009
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

#include "serverlogging.h"


/* Implementation *************************************************************/
CServerLogging::CServerLogging()
{
    int i;

    // constants defining the plot properties
    const int iYAxisStart = 0;
    const int iYAxisEnd   = 24;
    const int iNumTicksX = 10;
    const int iNumTicksY = 5;
    const int iPlotWidth  = 500;
    const int iPlotHeight = 500;
    const int iGridFrameOffset = 10;
    const QColor PlotBackgroundColor ( Qt::white ); // white background
    const QColor PlotFrameColor ( Qt::black ); // black frame
    const QColor PlotGridColor ( Qt::gray ); // gray grid
    const QColor PlotTextColor ( Qt::black ); // black text


    // create base pixmap for plot
    QRect PlotCanvasRect ( QPoint ( 0, 0 ), QPoint ( iPlotWidth, iPlotHeight ) );
    QPixmap PlotPixmap ( PlotCanvasRect.size() );
    PlotPixmap.fill ( PlotBackgroundColor ); // fill background

    // create painter for plot
    QPainter PlotPainter ( &PlotPixmap );


    // create actual plot region (grid frame) ----------------------------------
    QRect PlotGridFrame (
        PlotCanvasRect.x() + iGridFrameOffset,
        PlotCanvasRect.y() + iGridFrameOffset,
        PlotCanvasRect.width() - 2 * iGridFrameOffset,
        PlotCanvasRect.height() - 2 * iGridFrameOffset );

    PlotPainter.setPen ( PlotFrameColor );
    PlotPainter.drawRect ( PlotGridFrame );

    // grid (ticks) for x-axis
    const int iXSpace = PlotGridFrame.width() / ( iNumTicksX - 1 );
    for ( i = 0; i < ( iNumTicksX - 2 ); i++ )
    {
        const int iCurX = PlotGridFrame.x() + iXSpace * ( i + 1 );

        // text
        PlotPainter.setPen ( PlotTextColor );
// TODO

        // grid
        PlotPainter.setPen ( PlotGridColor );
        PlotPainter.drawLine ( iCurX, PlotGridFrame.y(),
            iCurX, PlotGridFrame.bottom() );
    }

    // grid (ticks) for y-axis
    const int iYSpace = PlotGridFrame.height() / ( iNumTicksY - 1 );
    for ( i = 0; i < ( iNumTicksY - 2 ); i++ )
    {
        const int iCurY = PlotGridFrame.y() + iYSpace * ( i + 1 );

        // text
        PlotPainter.setPen ( PlotTextColor );
        PlotPainter.setFont ( QFont ( "Arial", 10 ) );
        PlotPainter.drawText ( QPoint ( PlotGridFrame.x(), iCurY ),
            QString().setNum ( ( iYAxisEnd - iYAxisStart ) / iNumTicksY * i ) );


        // grid
        PlotPainter.setPen ( PlotGridColor );
        PlotPainter.drawLine ( PlotGridFrame.x(), iCurY,
            PlotGridFrame.right(), iCurY );
    }



    // save plot as a file
    PlotPixmap.save ( "test.jpg", "JPG", 90 );

}
