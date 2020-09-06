/******************************************************************************\
 * Copyright (c) 2004-2020
 *
 * Author(s):
 *  Volker Fischer
 *  Peter L Jones <pljones@users.sf.net>
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

#include "historygraph.h"


/* Abstract class *************************************************************/
AHistoryGraph::AHistoryGraph ( const int iMaxDaysHistory ) :
    sFileName           ( "" ),
    bDoHistory          ( false ),
    vHistoryDataFifo    ( NUM_ITEMS_HISTORY ),
    iNumTicksX          ( 0 ), // number of days in history
    iHistMaxDays        ( iMaxDaysHistory ),

    BackgroundColor     ( "white" ), // background
    FrameColor          ( "black" ), // frame
    GridColor           ( "gray" ), // grid
    TextColor           ( "black" ), // text
    MarkerNewColor      ( "darkCyan" ), // marker for new connection
    MarkerNewLocalColor ( "blue" ), // marker for new local connection
    MarkerStopColor     ( "red" ), // marker for server stop

    canvasRectX         ( 0 ),
    canvasRectY         ( 0 ),
    canvasRectWidth     ( 640 ),
    canvasRectHeight    ( 450 ),

    iGridFrameOffset    ( 10 ),
    iGridWidthWeekend   ( 3 ), // should be an odd value
    iXAxisTextHeight    ( 22 ),
    gridFrameX          ( canvasRectX + iGridFrameOffset ),
    gridFrameY          ( canvasRectY + iGridFrameOffset ),
    gridFrameWidth      ( canvasRectWidth - 2 * iGridFrameOffset ),
    gridFrameHeight     ( canvasRectHeight - 2 * iGridFrameOffset - iXAxisTextHeight ),
    gridFrameRight      ( gridFrameX + gridFrameWidth - 1 ),
    gridFrameBottom     ( gridFrameY + gridFrameHeight - 1 ),

    axisFontFamily      ( "Arial" ),
    axisFontWeight      ( "100" ),
    axisFontSize        ( 12 ),

    iYAxisStart         ( 0 ),
    iYAxisEnd           ( 24 ),
    iNumTicksY          ( 5 ),

    iTextOffsetToGrid   ( 3 ),
    iTextOffsetX        ( 18 ),

    iMarkerSizeNewCon   ( 10 ),
    iMarkerSizeServSt   ( 6 )
{
}

void AHistoryGraph::Start ( const QString& sNewFileName )
{
    QTextStream& tsConsoleStream = *( ( new ConsoleWriterFactory() )->get() );
    tsConsoleStream << QString ( "AHistoryGraph::Start ( %1 )" ).arg ( sNewFileName ) << endl; // on console

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

void AHistoryGraph::Add ( const QDateTime& newDateTime, const EHistoryItemType curType )
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

void AHistoryGraph::Add ( const QDateTime& newDateTime, const QHostAddress ClientInetAddr )
{
    if ( bDoHistory )
    {
        // add element to history, distinguish between a local connection
        // and a remote connection
        if ( ( ClientInetAddr == QHostAddress ( "127.0.0.1" ) ) ||
             ( ClientInetAddr.toString().left ( 7 ).compare ( "192.168" ) == 0 ) )
        {
            // local connection
            Add ( newDateTime, HIT_LOCAL_CONNECTION );
        }
        else
        {
            // remote connection
            Add ( newDateTime, HIT_REMOTE_CONNECTION );
        }
    }
}

void AHistoryGraph::Update ( )
{
    if ( bDoHistory )
    {
        int i;

        // store current date for reference
        curDate = QDate::currentDate();

        // set oldest date to draw
        QDate minDate = curDate.addDays ( iHistMaxDays * -1 );

        // get oldest date in history
        QDate     oldestDate          = curDate.addDays ( 1 ); // one day in the future
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

        if ( oldestDate < minDate )
        {
            oldestDate = minDate;
        }

        const int iNumDaysInHistory = -curDate.daysTo ( oldestDate ) + 1;

        // draw frame of the graph
        DrawFrame ( iNumDaysInHistory );

        // add markers
        for ( i = 0; i < iNumItemsForHistory; i++ )
        {
            // only use valid dates
            if ( vHistoryDataFifo[i].DateTime.date().isValid() && ( oldestDate <= vHistoryDataFifo[i].DateTime.date() ) )
            {
                AddMarker ( vHistoryDataFifo[i] );
            }
        }

        // save graph as picture in file
        Save ( sFileName );
    }
}

void AHistoryGraph::DrawFrame ( const int iNewNumTicksX )
{
    int i;

    // store number of x-axis ticks (number of days we want to draw
    // the history for
    iNumTicksX = iNewNumTicksX;


    // Create actual plot region (grid frame) ----------------------------------
    rect( gridFrameX, gridFrameY, gridFrameWidth, gridFrameHeight );

    // calculate step for x-axis ticks so that we get the desired number of
    // ticks -> 5 ticks

// TODO the following equation does not work as expected but results are acceptable

    // we want to have "floor ( iNumTicksX / 5 )" which is the same as
    // "iNumTicksX / 5" since "iNumTicksX" is an integer variable
    const int iXAxisTickStep = iNumTicksX / 5 + 1;

    // grid (ticks) for x-axis
    dayXSpace = static_cast<double> ( gridFrameWidth ) / ( iNumTicksX + 1 );

    for ( i = 0; i < static_cast<int> ( iNumTicksX ); i++ )
    {
        int         iBottomExtraTickLen = 0;
        const int   iCurX = gridFrameX + static_cast<int> ( dayXSpace * ( i + 1 ) );
        const QDate curXAxisDate = curDate.addDays ( 0 - static_cast<signed> ( iNumTicksX ) + i + 1 );

        // text (print only every "iXAxisTickStep" tick)
        if ( !( i % iXAxisTickStep ) )
        {
            text ( iCurX - iTextOffsetX, gridFrameBottom + iXAxisTextHeight + iTextOffsetToGrid, curXAxisDate.toString ( "dd.MM." ) );

            iBottomExtraTickLen = 5;
        }

        // regular grid
        line ( iCurX, 1 + gridFrameY, iCurX, gridFrameBottom + iBottomExtraTickLen );

        // different grid width for weekends (overwrite regular grid)
        if ( ( curXAxisDate.dayOfWeek() == 6 ) || // check for Saturday
             ( curXAxisDate.dayOfWeek() == 7 ) )  // check for Sunday
        {
            const int iGridWidthWeekendHalf = iGridWidthWeekend / 2;

            line ( iCurX, 1 + gridFrameY + iGridWidthWeekendHalf, iCurX, gridFrameBottom - iGridWidthWeekendHalf, iGridWidthWeekend );
        }
    }

    // grid (ticks) for y-axis, draw iNumTicksY - 2 grid lines and
    // iNumTicksY - 1 text labels (the lowest grid line is the grid frame)
    iYSpace = gridFrameHeight / ( iNumTicksY - 1 );

    for ( i = 0; i < ( static_cast<int> ( iNumTicksY ) - 1 ); i++ )
    {
        const int iCurY = gridFrameY + iYSpace * ( i + 1 );

        // text
        text ( gridFrameX + iTextOffsetToGrid,
            iCurY - iTextOffsetToGrid,
            QString ( "%1:00" ).arg ( ( iYAxisEnd - iYAxisStart ) / ( iNumTicksY - 1 ) * ( ( iNumTicksY - 2 ) - i ) ) );

        // grid (do not overwrite frame)
        if ( i < ( static_cast<int> ( iNumTicksY ) - 2 ) )
        {
            line ( gridFrameX, iCurY, gridFrameRight, iCurY );
        }
    }
}

void AHistoryGraph::AddMarker ( const SHistoryData& curHistoryData )
{
    // calculate x-axis offset (difference of days compared to
    // current date)
    const int iXAxisOffs =
        curDate.daysTo ( curHistoryData.DateTime.date() );

    // check range, if out of range, do not plot anything
    if ( -iXAxisOffs > ( static_cast<signed> ( iNumTicksX ) - 1 ) )
    {
        return;
    }

    // calculate y-axis offset (consider hours and minutes)
    const double dYAxisOffs = 24 - curHistoryData.DateTime.time().hour() -
        static_cast<double> ( curHistoryData.DateTime.time().minute() ) / 60;

    // calculate the actual point in the graph (in pixels)
    int curPointX = gridFrameX + static_cast<int> ( dayXSpace * ( static_cast<signed> ( iNumTicksX ) + iXAxisOffs ) );
    int curPointY = gridFrameY + static_cast<int> ( static_cast<double> (
        gridFrameHeight ) / ( iYAxisEnd - iYAxisStart ) * dYAxisOffs );

    QString curPointColour = MarkerNewColor;
    int     curPointSize   = iMarkerSizeNewCon;

    // we use different markers for new connection and server stop items
    switch ( curHistoryData.Type )
    {
    case HIT_SERVER_STOP:
        curPointColour = MarkerStopColor;
        curPointSize   = iMarkerSizeServSt;
        break;

    case HIT_LOCAL_CONNECTION:
        curPointColour = MarkerNewLocalColor;
        break;

    case HIT_REMOTE_CONNECTION:
        curPointColour = MarkerNewColor;
        break;
    }

    point ( curPointX - curPointSize / 2, curPointY - curPointSize / 2, curPointSize, curPointColour );
}


/* JPEG History Graph implementation ******************************************/
CJpegHistoryGraph::CJpegHistoryGraph ( const int iMaxDaysHistory ) :
    AHistoryGraph   ( iMaxDaysHistory ),
    PlotPixmap      ( 1, 1, QImage::Format_RGB32 ),
    iAxisFontWeight ( -1 )
{
    // scale pixmap to correct size
    PlotPixmap = PlotPixmap.scaled ( canvasRectWidth, canvasRectHeight );

    // axisFontWeight is a string matching the CSS2 font-weight definition
    // Thin     = 0,    // 100
    // ExtraLight = 12, // 200
    // Light    = 25,   // 300
    // Normal   = 50,   // 400
    // Medium   = 57,   // 500
    // DemiBold = 63,   // 600
    // Bold     = 75,   // 700
    // ExtraBold = 81,  // 800
    // Black    = 87    // 900
    bool ok;
    int weight = axisFontWeight.toInt( &ok );

    if ( !ok )
    {
        if ( !QString ( "normal" ).compare ( axisFontWeight, Qt::CaseSensitivity::CaseInsensitive ) )
        {
            iAxisFontWeight = 50;
        }
        else if ( !QString ( "bold" ).compare ( axisFontWeight, Qt::CaseSensitivity::CaseInsensitive ) )
        {
            weight = 75;
        }
    }
    else
    {
        if ( weight <= 100 ) { iAxisFontWeight = 0; }
        else if ( weight <= 200 ) { iAxisFontWeight = 12; }
        else if ( weight <= 300 ) { iAxisFontWeight = 25; }
        else if ( weight <= 400 ) { iAxisFontWeight = 50; }
        else if ( weight <= 500 ) { iAxisFontWeight = 57; }
        else if ( weight <= 600 ) { iAxisFontWeight = 63; }
        else if ( weight <= 700 ) { iAxisFontWeight = 75; }
        else if ( weight <= 800 ) { iAxisFontWeight = 81; }
        else if ( weight <= 900 ) { iAxisFontWeight = 87; }
    }
    // if all else fails, it's left at -1

//    QTextStream& tsConsoleStream = *( ( new ConsoleWriterFactory() )->get() );
//    tsConsoleStream << "CJpegHistoryGraph" << endl; // on console


    // Connections -------------------------------------------------------------
    QObject::connect ( &TimerDailyUpdate, SIGNAL ( timeout() ),
        this, SLOT ( OnTimerDailyUpdate() ) );
}

// Override Update to blank out the plot area each time
void CJpegHistoryGraph::Update()
{
    if ( bDoHistory )
    {
        // create JPEG image
        PlotPixmap.fill ( BackgroundColor ); // fill background

        AHistoryGraph::Update();
    }
}

void CJpegHistoryGraph::Save ( const QString sFileName )
{
    // save plot as a file
    PlotPixmap.save ( sFileName, "JPG", 90 );
}

void CJpegHistoryGraph::rect ( const unsigned int x, const unsigned int y, const unsigned int width, const unsigned int height )
{
    QPainter PlotPainter ( &PlotPixmap );
    PlotPainter.setPen ( FrameColor );
    PlotPainter.drawRect ( x, y, width, height );
}

void CJpegHistoryGraph::text ( const unsigned int x, const unsigned int y, const QString& value )
{
    QPainter PlotPainter ( &PlotPixmap );
    PlotPainter.setPen ( TextColor );
    // QFont(const QString &family, int pointSize = -1, int weight = -1, bool italic = false);
    PlotPainter.setFont ( QFont( axisFontFamily, static_cast<int> ( axisFontSize ), iAxisFontWeight ) );
    PlotPainter.drawText ( QPoint ( x, y ), value );
}

void CJpegHistoryGraph::line ( const unsigned int x1, const unsigned int y1, const unsigned int x2, const unsigned int y2, const unsigned int strokeWidth )
{
    QPainter PlotPainter ( &PlotPixmap );
    PlotPainter.setPen ( QPen ( QBrush ( QColor ( GridColor ) ), strokeWidth ) );
    PlotPainter.drawLine ( x1, y1, x2, y2 );
}

void CJpegHistoryGraph::point ( const unsigned int x, const unsigned int y, const unsigned int size, const QString& colour )
{
    QPainter PlotPainter ( &PlotPixmap );
    PlotPainter.setPen ( QPen ( QBrush( QColor ( colour ) ), size ) );
    PlotPainter.drawPoint ( x, y );
}


/* SVG History Graph implementation *******************************************/
CSvgHistoryGraph::CSvgHistoryGraph ( const int iMaxDaysHistory ) :
    AHistoryGraph   ( iMaxDaysHistory ),
    svgImage        ( "" ),
    svgStreamWriter ( &svgImage )
{
    // set SVG veiwBox to correct size to ensure correct scaling
    svgRootAttributes.append ( "viewBox",
        QString ( "%1, %2, %3, %4" )
            .arg ( canvasRectX )
            .arg ( canvasRectY )
            .arg ( canvasRectWidth )
            .arg ( canvasRectHeight )
    );

    svgRootAttributes.append ( "xmlns", "http://www.w3.org/2000/svg" );
    svgRootAttributes.append ( "xmlns:xlink", "http://www.w3.org/1999/xlink" );

//    QTextStream& tsConsoleStream = *( ( new ConsoleWriterFactory() )->get() );
//    tsConsoleStream << "CSvgHistoryGraph" << endl; // on console


    // Connections -------------------------------------------------------------
    QObject::connect ( &TimerDailyUpdate, SIGNAL ( timeout() ),
        this, SLOT ( OnTimerDailyUpdate() ) );
}

// Override Update to create the fresh SVG stream each time
void CSvgHistoryGraph::Update()
{
    if ( bDoHistory )
    {
        // create SVG document
        svgImage = "";

        svgStreamWriter.setAutoFormatting ( true );
        svgStreamWriter.writeStartDocument();
        svgStreamWriter.writeStartElement ( "svg" );
        svgStreamWriter.writeAttributes ( svgRootAttributes );

        AHistoryGraph::Update();
    }
}

void CSvgHistoryGraph::Save ( const QString sFileName )
{
    svgStreamWriter.writeEndDocument();

    QFile outf ( sFileName );

    if ( !outf.open ( QFile::WriteOnly ) )
    {
        throw std::runtime_error ( ( sFileName + " could not be written. Aborting." ).toStdString() );
    }
    QTextStream out ( &outf );

    out << svgImage << endl;
}

void CSvgHistoryGraph::rect ( const unsigned int x, const unsigned int y, const unsigned int width, const unsigned int height )
{
    svgStreamWriter.writeEmptyElement ( "rect" );
    svgStreamWriter.writeAttribute ( "x", QString ( "%1" ).arg ( x ) );
    svgStreamWriter.writeAttribute ( "y", QString ( "%1" ).arg ( y ) );
    svgStreamWriter.writeAttribute ( "width", QString ( "%1" ).arg ( width ) );
    svgStreamWriter.writeAttribute ( "height", QString ( "%1" ).arg ( height ) );
    svgStreamWriter.writeAttribute ( "stroke", FrameColor );
    svgStreamWriter.writeAttribute ( "stroke-width", QString ( "1" ) );
    svgStreamWriter.writeAttribute ( "style", QString ( "fill: none;" ) );
}

void CSvgHistoryGraph::text ( const unsigned int x, const unsigned int y, const QString& value )
{
    svgStreamWriter.writeStartElement ( "text" );
    svgStreamWriter.writeAttribute ( "x", QString ( "%1" ).arg ( x ) );
    svgStreamWriter.writeAttribute ( "y", QString ( "%1" ).arg ( y ) );
    svgStreamWriter.writeAttribute ( "stroke", TextColor );
    svgStreamWriter.writeAttribute ( "font-family", axisFontFamily );
    svgStreamWriter.writeAttribute ( "font-weight", axisFontWeight );
    svgStreamWriter.writeAttribute ( "font-size", QString ( "%1" ).arg ( axisFontSize ) );

    svgStreamWriter.writeCharacters( value );
    svgStreamWriter.writeEndElement();
}

void CSvgHistoryGraph::line ( const unsigned int x1, const unsigned int y1, const unsigned int x2, const unsigned int y2, const unsigned int strokeWidth )
{
    svgStreamWriter.writeEmptyElement ( "line" );
    svgStreamWriter.writeAttribute ( "x1", QString ( "%1" ).arg ( x1 ) );
    svgStreamWriter.writeAttribute ( "y1", QString ( "%1" ).arg ( y1 ) );
    svgStreamWriter.writeAttribute ( "x2", QString ( "%1" ).arg ( x2 ) );
    svgStreamWriter.writeAttribute ( "y2", QString ( "%1" ).arg ( y2 ) );
    svgStreamWriter.writeAttribute ( "stroke", GridColor );
    svgStreamWriter.writeAttribute ( "stroke-width", QString ( "%1" ).arg ( strokeWidth ) );
}

void CSvgHistoryGraph::point ( const unsigned int x, const unsigned int y, const unsigned int size, const QString& colour )
{
    svgStreamWriter.writeEmptyElement ( "rect" );
    svgStreamWriter.writeAttribute ( "x", QString ( "%1" ).arg ( x ) );
    svgStreamWriter.writeAttribute ( "y", QString ( "%1" ).arg ( y ) );
    svgStreamWriter.writeAttribute ( "width", QString ( "%1" ).arg ( size ) );
    svgStreamWriter.writeAttribute ( "height", QString ( "%1" ).arg ( size ) );
    svgStreamWriter.writeAttribute ( "stroke-opacity", "0" );
    svgStreamWriter.writeAttribute ( "fill", colour );
}
