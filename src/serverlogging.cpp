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
CHistoryGraph::CHistoryGraph ( const QString& sNewFileName ) :
    sFileName ( sNewFileName ),
    vDateTimeFifo ( NUM_ITEMS_HISTORY ),
    vItemTypeFifo ( NUM_ITEMS_HISTORY ),
    PlotCanvasRect ( 0, 0, 600, 450 ), // defines total size of graph
    iYAxisStart ( 0 ),
    iYAxisEnd ( 24 ),
    iNumTicksY ( 5 ),
    iNumTicksX ( 0 ), // just initialization value, will be overwritten
    iGridFrameOffset ( 10 ),
    iTextOffsetToGrid ( 3 ),
    iYAxisTextHeight ( 20 ),
    AxisFont ( "Arial", 10 ),
    iMarkerSize ( 9 ),
    PlotBackgroundColor ( Qt::white ), // white background
    PlotFrameColor ( Qt::black ), // black frame
    PlotGridColor ( Qt::gray ), // gray grid
    PlotTextColor ( Qt::black ), // black text
    PlotMarkerNewColor ( Qt::blue ), // blue marker for new connection
    PlotMarkerStopColor ( Qt::red ), // red marker server stop
    PlotPixmap ( 1, 1 )
{
    // generate plot grid frame rectangle
    PlotGridFrame.setCoords ( PlotCanvasRect.x() + iGridFrameOffset,
        PlotCanvasRect.y() + iGridFrameOffset,
        PlotCanvasRect.width() - 2 * iGridFrameOffset,
        PlotCanvasRect.height() - 2 * iGridFrameOffset - iYAxisTextHeight );

    // scale pixmap to correct size
    PlotPixmap = PlotPixmap.scaled (
        PlotCanvasRect.width(), PlotCanvasRect.height() ); 
}

void CHistoryGraph::DrawFrame ( const int iNewNumTicksX )
{
    int i;

    // store number of x-axis ticks (number of days we want to draw
    // the history for
    iNumTicksX = iNewNumTicksX;

    // clear base pixmap for new plotting
    PlotPixmap.fill ( PlotBackgroundColor ); // fill background

    // create painter
    QPainter PlotPainter ( &PlotPixmap );


    // create actual plot region (grid frame) ----------------------------------
    PlotPainter.setPen ( PlotFrameColor );
    PlotPainter.drawRect ( PlotGridFrame );

    // calculate step for x-axis ticks so that we get the desired number of
    // ticks -> 5 ticks

// TODO the following equation does not work as expected but results are exeptable

    // we want to have "floor ( iNumTicksX / 5 )" which is the same as
    // "iNumTicksX / 5" since "iNumTicksX" is an integer variable
    const int iXAxisTickStep = iNumTicksX / 5 + 1;

    // grid (ticks) for x-axis
    const int iTextOffsetX = 20;
    iXSpace = PlotGridFrame.width() / ( iNumTicksX + 1 );
    for ( i = 0; i < iNumTicksX; i++ )
    {
        int       iBottomExtraTickLen = 0;
        const int iCurX = PlotGridFrame.x() + iXSpace * ( i + 1 );

        // text (print only every "iXAxisTickStep" tick)
        if ( !( i % iXAxisTickStep ) )
        {
            PlotPainter.setPen ( PlotTextColor );
            PlotPainter.setFont ( AxisFont );
            PlotPainter.drawText (
                QPoint ( iCurX - iTextOffsetX,
                PlotGridFrame.bottom() + iYAxisTextHeight + iTextOffsetToGrid ),
                curDate.addDays ( i - iNumTicksX + 1 ).toString ( "dd.MM." ) );

            iBottomExtraTickLen = 5;
        }

        // grid
        PlotPainter.setPen ( PlotGridColor );
        PlotPainter.drawLine ( iCurX, PlotGridFrame.y(),
            iCurX, PlotGridFrame.bottom() + iBottomExtraTickLen );
    }

    // grid (ticks) for y-axis, draw iNumTicksY - 2 grid lines and
    // iNumTicksY - 1 text labels (the lowest grid line is the grid frame)
    iYSpace = PlotGridFrame.height() / ( iNumTicksY - 1 );
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
}

void CHistoryGraph::AddMarker ( const QDateTime& curDateTime,
                                const bool bIsServerStop )
{
    // calculate x-axis offset (difference of days compared to
    // current date)
    const int iXAxisOffs = curDate.daysTo ( curDateTime.date() );

    // check range, if out of range, do not plot anything
    if ( -iXAxisOffs > ( iNumTicksX - 1 ) )
    {
        return;
    }

    // calculate y-axis offset (consider hours and minutes)
    const double dYAxisOffs = 24 - curDateTime.time().hour() -
        static_cast<double> ( curDateTime.time().minute() ) / 60;

    // calculate the actual point in the graph (in pixels)
    const QPoint curPoint (
        PlotGridFrame.x() + iXSpace * ( iNumTicksX + iXAxisOffs ),
        PlotGridFrame.y() + static_cast<int> ( static_cast<double> (
        PlotGridFrame.height() ) / ( iYAxisEnd - iYAxisStart ) * dYAxisOffs ) );

    // create painter for plot
    QPainter PlotPainter ( &PlotPixmap );

    // we use different markers for new connection and server stop items
    if ( bIsServerStop )
    {
        // filled circle marker
        PlotPainter.setPen ( QPen ( QBrush ( PlotMarkerStopColor ),
            iMarkerSize, Qt::SolidLine, Qt::RoundCap ) );
    }
    else
    {
        // filled square marker
        PlotPainter.setPen ( QPen ( QBrush ( PlotMarkerNewColor ),
            iMarkerSize ) );
    }
    PlotPainter.drawPoint ( curPoint );
}

void CHistoryGraph::Save ( const QString sFileName )
{
    // save plot as a file
    PlotPixmap.save ( sFileName, "JPG", 90 );
}

void CHistoryGraph::Add ( const QDateTime& newDateTime,
                          const bool newIsServerStop )
{
    // add new element in FIFOs
    vDateTimeFifo.Add ( newDateTime );
    vItemTypeFifo.Add ( static_cast<int> ( newIsServerStop ) );
}

void CHistoryGraph::Update()
{
    int i;

    // store current date for reference
    curDate = QDate::currentDate();

    // get oldest date in history
    QDate oldestDate = curDate.addDays ( 1 ); // one day in the future
    const int iNumItemsForHistory = vDateTimeFifo.Size();
    for ( i = 0; i < iNumItemsForHistory; i++ )
    {
        // only use valid dates
        if ( vDateTimeFifo[i].date().isValid() )
        {
            if ( vDateTimeFifo[i].date() < oldestDate )
            {
                oldestDate = vDateTimeFifo[i].date();
            }
        }
    }
    const int iNumDaysInHistory = -curDate.daysTo ( oldestDate ) + 1;

    // draw frame of the graph
    DrawFrame ( iNumDaysInHistory );

    // add markers
    for ( i = 0; i < iNumItemsForHistory; i++ )
    {
        AddMarker ( vDateTimeFifo[i], static_cast<bool> ( vItemTypeFifo[i] ) );
    }

    // save graph as picture in file
    Save ( sFileName );
}


CServerLogging::CServerLogging() :
    bDoLogging ( false ),
    File ( DEFAULT_LOG_FILE_NAME ),
    HistoryGraph ( "test.jpg" )
{


#if 1

// TEST add some elements
HistoryGraph.Add ( QDateTime ( QDate::currentDate().addDays ( -1 ),
    QTime ( 18, 0, 0, 0 ) ), false );

HistoryGraph.Add ( QDateTime ( QDate::currentDate().addDays ( -0 ),
    QTime ( 17, 50, 0, 0 ) ), true );

HistoryGraph.Update();

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
