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
CServerLogging::CServerLogging()  :
    bDoLogging ( false ), File ( DEFAULT_LOG_FILE_NAME )
{


#if 0

    int i;

    // constants defining the plot properties
    const int iYAxisStart       = 0;
    const int iYAxisEnd         = 24;
    const int iNumTicksX        = 9;
    const int iNumTicksY        = 5;
    const int iPlotWidth        = 500;
    const int iPlotHeight       = 500;
    const int iGridFrameOffset  = 10;
    const int iTextOffsetToGrid = 3;
    const int iYAxisTextHeight  = 20;
    const QFont AxisFont ( "Arial", 10 );
    const QColor PlotBackgroundColor ( Qt::white ); // white background
    const QColor PlotFrameColor ( Qt::black ); // black frame
    const QColor PlotGridColor ( Qt::gray ); // gray grid
    const QColor PlotTextColor ( Qt::black ); // black text
    const QColor PlotMarkerColor ( Qt::red ); // red marker

    // get current date (this is the right edge of the x-axis)
    const QDate curDate = QDate::currentDate();

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
        PlotCanvasRect.height() - 2 * iGridFrameOffset - iYAxisTextHeight );

    PlotPainter.setPen ( PlotFrameColor );
    PlotPainter.drawRect ( PlotGridFrame );

    // grid (ticks) for x-axis
    const int iTextOffsetX = 20;
    const int iXSpace = PlotGridFrame.width() / ( iNumTicksX + 1 );
    for ( i = 0; i < iNumTicksX; i++ )
    {
        const int iCurX = PlotGridFrame.x() + iXSpace * ( i + 1 );

        // text (only every second tick)
        if ( !( i % 2 ) )
        {
            PlotPainter.setPen ( PlotTextColor );
            PlotPainter.setFont ( AxisFont );
            PlotPainter.drawText (
                QPoint ( iCurX - iTextOffsetX,
                PlotGridFrame.bottom() + iYAxisTextHeight + iTextOffsetToGrid ),
                curDate.addDays ( i - iNumTicksX + 1 ).toString ( "dd.MM." ) );
        }

        // grid
        PlotPainter.setPen ( PlotGridColor );
        PlotPainter.drawLine ( iCurX, PlotGridFrame.y(),
            iCurX, PlotGridFrame.bottom() );
    }

    // grid (ticks) for y-axis, draw iNumTicksY - 2 grid lines and
    // iNumTicksY - 1 text labels (the lowest grid line is the grid frame)
    const int iYSpace = PlotGridFrame.height() / ( iNumTicksY - 1 );
    for ( i = 0; i < ( iNumTicksY - 1 ); i++ )
    {
        const int iCurY = PlotGridFrame.y() + iYSpace * ( i + 1 );

        // text
        PlotPainter.setPen ( PlotTextColor );
        PlotPainter.setFont ( AxisFont );
        PlotPainter.drawText ( QPoint (
            PlotGridFrame.x() + iTextOffsetToGrid,
            iCurY - iTextOffsetToGrid ),
            QString ( "%1:00" ).arg (
            ( iYAxisEnd - iYAxisStart ) / ( iNumTicksY - 1 ) *
            ( ( iNumTicksY - 2 ) - i ) ) );

        // grid (do not overwrite frame)
        if ( i < ( iNumTicksY - 2 ) )
        {
            PlotPainter.setPen ( PlotGridColor );
            PlotPainter.drawLine ( PlotGridFrame.x(), iCurY,
                PlotGridFrame.right(), iCurY );
        }
    }




// TEST add some points in the graph
const QDate testDate = QDate::currentDate().addDays ( -3 );
const QTime testTime = QTime ( 18, 0, 0, 0 );

const int iXAxisOffs = curDate.daysTo ( testDate );
const int iYAxisOffs = 24 - testTime.hour();

const QPoint curPoint (
    PlotGridFrame.x() + PlotGridFrame.width() / iNumTicksX * ( iNumTicksX + iXAxisOffs ),
    PlotGridFrame.y() + PlotGridFrame.height() / ( iYAxisEnd - iYAxisStart ) * iYAxisOffs );

PlotPainter.setPen ( QPen ( QBrush ( PlotMarkerColor ), 9 ) );
PlotPainter.drawPoint ( curPoint );




 // save plot as a file
 PlotPixmap.save ( "test.jpg", "JPG", 90 );

#endif
}

CServerLogging::~CServerLogging()
{
    // close logging file of open
    if ( File.isOpen() )
    {
        File.close();
    }
}

void CServerLogging::Start ( const QString& strLoggingFileName )
{
    // open file
    File.setFileName ( strLoggingFileName );
    if ( File.open ( QIODevice::Append | QIODevice::Text ) )
    {
        bDoLogging = true;
    }
}

void CServerLogging::AddNewConnection ( const QHostAddress& ClientInetAddr )
{
    // logging of new connected channel
    const QString strLogStr = CurTimeDatetoLogString() + ", " +
        ClientInetAddr.toString() + ", connected";

#ifndef _WIN32
    QTextStream tsConsoloeStream ( stdout );
    tsConsoloeStream << strLogStr << endl; // on console
#endif
    *this << strLogStr; // in log file
}

void CServerLogging::AddServerStopped()
{
    const QString strLogStr = CurTimeDatetoLogString() + ",, server stopped "
        "-------------------------------------";

#ifndef _WIN32
    QTextStream tsConsoloeStream ( stdout );
    tsConsoloeStream << strLogStr << endl; // on console
#endif
    *this << strLogStr; // in log file
}

void CServerLogging::operator<< ( const QString& sNewStr )
{
    if ( bDoLogging )
    {
        // append new line in logging file
        QTextStream out ( &File );
        out << sNewStr << endl;
        File.flush();
    }
}

QString CServerLogging::CurTimeDatetoLogString()
{
    // time and date to string conversion
    const QDateTime curDateTime = QDateTime::currentDateTime();

    // format date and time output according to "3.9.2006, 11:38:08"
    return QString().setNum ( curDateTime.date().day() ) + "." +
        QString().setNum ( curDateTime.date().month() ) + "." +
        QString().setNum ( curDateTime.date().year() ) + ", " +
        curDateTime.time().toString();
}
