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
CHistoryGraph::CHistoryGraph() :
    sFileName ( "" ),
    bDoHistory ( false ),
    vHistoryDataFifo ( NUM_ITEMS_HISTORY ),
    PlotCanvasRect    ( 0, 0, 600, 450 ), // defines total size of graph
    iNumTicksX        ( 0 ), // just an initialization value, will be overwritten
    iYAxisStart       ( 0 ),
    iYAxisEnd         ( 24 ),
    iNumTicksY        ( 5 ),
    iGridFrameOffset  ( 10 ),
    iTextOffsetToGrid ( 3 ),
    iXAxisTextHeight  ( 22 ),
    iMarkerSizeNewCon ( 11 ),
    iMarkerSizeServSt ( 9 ),
    AxisFont ( "Arial", 12 ),
    iTextOffsetX      ( 18 ),
    PlotBackgroundColor     ( Qt::white ), // background
    PlotFrameColor          ( Qt::black ), // frame
    PlotGridColor           ( Qt::gray ), // grid
    PlotTextColor           ( Qt::black ), // text
    PlotMarkerNewColor      ( Qt::darkCyan ), // marker for new connection
    PlotMarkerNewLocalColor ( Qt::blue ), // marker for new local connection
    PlotMarkerStopColor     ( Qt::red ), // marker for server stop
    PlotPixmap ( 1, 1, QImage::Format_RGB32 )
{
    // generate plot grid frame rectangle
    PlotGridFrame.setRect ( PlotCanvasRect.x() + iGridFrameOffset,
        PlotCanvasRect.y() + iGridFrameOffset,
        PlotCanvasRect.width() - 2 * iGridFrameOffset,
        PlotCanvasRect.height() - 2 * iGridFrameOffset - iXAxisTextHeight );

    // scale pixmap to correct size
    PlotPixmap = PlotPixmap.scaled (
        PlotCanvasRect.width(), PlotCanvasRect.height() );

    // connections
    QObject::connect ( &TimerDailyUpdate, SIGNAL ( timeout() ),
        this, SLOT ( OnTimerDailyUpdate() ) );
}

void CHistoryGraph::Start ( const QString& sNewFileName )
{
    if ( !sNewFileName.isEmpty() )
    {
        // save file name
        sFileName = sNewFileName;

        // set enable flag
        bDoHistory = true;

        // enable timer (update once a day)
        TimerDailyUpdate.start ( 3600000 * 24 );

        // initial update (empty graph)
        Update();
    }
}

void CHistoryGraph::DrawFrame ( const int iNewNumTicksX )
{
    int i;

    // store number of x-axis ticks (number of days we want to draw
    // the history for
    iNumTicksX = iNewNumTicksX;

    // clear base pixmap for new plotting
    PlotPixmap.fill ( PlotBackgroundColor.rgb() ); // fill background

    // create painter
    QPainter PlotPainter ( &PlotPixmap );


    // create actual plot region (grid frame) ----------------------------------
    PlotPainter.setPen ( PlotFrameColor );
    PlotPainter.drawRect ( PlotGridFrame );

    // calculate step for x-axis ticks so that we get the desired number of
    // ticks -> 5 ticks

// TODO the following equation does not work as expected but results are acceptable

    // we want to have "floor ( iNumTicksX / 5 )" which is the same as
    // "iNumTicksX / 5" since "iNumTicksX" is an integer variable
    const int iXAxisTickStep = iNumTicksX / 5 + 1;

    // grid (ticks) for x-axis
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
                PlotGridFrame.bottom() + iXAxisTextHeight + iTextOffsetToGrid ),
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

void CHistoryGraph::AddMarker ( const SHistoryData& curHistoryData )
{
    // calculate x-axis offset (difference of days compared to
    // current date)
    const int iXAxisOffs =
        curDate.daysTo ( curHistoryData.DateTime.date() );

    // check range, if out of range, do not plot anything
    if ( -iXAxisOffs > ( iNumTicksX - 1 ) )
    {
        return;
    }

    // calculate y-axis offset (consider hours and minutes)
    const double dYAxisOffs = 24 - curHistoryData.DateTime.time().hour() -
        static_cast<double> ( curHistoryData.DateTime.time().minute() ) / 60;

    // calculate the actual point in the graph (in pixels)
    const QPoint curPoint (
        PlotGridFrame.x() + iXSpace * ( iNumTicksX + iXAxisOffs ),
        PlotGridFrame.y() + static_cast<int> ( static_cast<double> (
        PlotGridFrame.height() ) / ( iYAxisEnd - iYAxisStart ) * dYAxisOffs ) );

    // create painter for plot
    QPainter PlotPainter ( &PlotPixmap );

    // we use different markers for new connection and server stop items
    switch ( curHistoryData.Type )
    {
    case HIT_SERVER_STOP:
        // filled circle marker
        PlotPainter.setPen ( QPen ( QBrush ( PlotMarkerStopColor ),
            iMarkerSizeServSt, Qt::SolidLine, Qt::RoundCap ) );
        break;

    case HIT_LOCAL_CONNECTION:
        // filled square marker
        PlotPainter.setPen ( QPen ( QBrush ( PlotMarkerNewLocalColor ),
            iMarkerSizeNewCon ) );
        break;

    case HIT_REMOTE_CONNECTION:
        // filled square marker
        PlotPainter.setPen ( QPen ( QBrush ( PlotMarkerNewColor ),
            iMarkerSizeNewCon ) );
        break;
    }
    PlotPainter.drawPoint ( curPoint );
}

void CHistoryGraph::Save ( const QString sFileName )
{
    // save plot as a file
    PlotPixmap.save ( sFileName, "JPG", 90 );
}

void CHistoryGraph::Add ( const QDateTime& newDateTime,
                          const EHistoryItemType curType )
{
    if ( bDoHistory )
    {
        // create and add new element in FIFO
        SHistoryData curHistoryData;
        curHistoryData.DateTime = newDateTime;
        curHistoryData.Type     = curType;

        vHistoryDataFifo.Add ( curHistoryData );
    }
}

void CHistoryGraph::Update()
{
    if ( bDoHistory )
    {
        int i;

        // store current date for reference
        curDate = QDate::currentDate();

        // get oldest date in history
        QDate oldestDate = curDate.addDays ( 1 ); // one day in the future
        const int iNumItemsForHistory = vHistoryDataFifo.Size();
        for ( i = 0; i < iNumItemsForHistory; i++ )
        {
            // only use valid dates
            if ( vHistoryDataFifo[i].DateTime.date().isValid() )
            {
                if ( vHistoryDataFifo[i].DateTime.date() < oldestDate )
                {
                    oldestDate = vHistoryDataFifo[i].DateTime.date();
                }
            }
        }
        const int iNumDaysInHistory = -curDate.daysTo ( oldestDate ) + 1;

        // draw frame of the graph
        DrawFrame ( iNumDaysInHistory );

        // add markers
        for ( i = 0; i < iNumItemsForHistory; i++ )
        {
            AddMarker ( vHistoryDataFifo[i] );
        }

        // save graph as picture in file
        Save ( sFileName );
    }
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

void CServerLogging::EnableHistory ( const QString& strHistoryFileName )
{
    HistoryGraph.Start ( strHistoryFileName );
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

    // add element to history, distinguish between a local connection
    // and a remote connection
    if ( ( ClientInetAddr == QHostAddress ( "127.0.0.1" ) ) ||
        ( ClientInetAddr.toString().left ( 7 ).compare ( "192.168" ) == 0 ) )
    {
        // local connection
        HistoryGraph.Add ( QDateTime::currentDateTime(),
            CHistoryGraph::HIT_LOCAL_CONNECTION );
    }
    else
    {
        // remote connection
        HistoryGraph.Add ( QDateTime::currentDateTime(),
            CHistoryGraph::HIT_REMOTE_CONNECTION );
    }
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

    // add element to history and update on server stop
    HistoryGraph.Add ( QDateTime::currentDateTime(),
        CHistoryGraph::HIT_SERVER_STOP );

    HistoryGraph.Update();
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
