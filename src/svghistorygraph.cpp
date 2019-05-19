#include "svghistorygraph.h"

CSvgHistoryGraph::CSvgHistoryGraph() :
    sFileName               ( "" ),
    bDoHistory              ( false ),
    vHistoryDataFifo        ( NUM_ITEMS_HISTORY ),
    plotCanvasRectX         ( 0 ),
    plotCanvasRectY         ( 0 ),
    plotCanvasRectWidth     ( 600 ),
    plotCanvasRectHeight    ( 450 ),
    numDays                 ( 0 ),
    iYAxisStart             ( 0 ),
    iYAxisEnd               ( 24 ),
    iNumTicksY              ( 5 ),
    iGridFrameOffset        ( 10 ),
    iGridWidthWeekend       ( 3 ), // should be an odd value
    iTextOffsetToGrid       ( 3 ),
    iXAxisTextHeight        ( 22 ),
    iMarkerSizeNewCon       ( 11 ),
    iMarkerSizeServSt       ( 8 ),
    axisFontFamily          ( "sans-serif" ),
    axisFontWeight          ( "80" ),
    axisFontSize            ( 10 ),
    iTextOffsetX            ( 18 ),
    plotGridFrameX          ( plotCanvasRectX + iGridFrameOffset ),
    plotGridFrameY          ( plotCanvasRectY + iGridFrameOffset ),
    plotGridFrameWidth      ( plotCanvasRectWidth - 2 * iGridFrameOffset ),
    plotGridFrameHeight     ( plotCanvasRectHeight - 2 * iGridFrameOffset - iXAxisTextHeight ),
    plotGridFrameRight      ( plotGridFrameX + plotGridFrameWidth ),
    plotGridFrameBottom     ( plotGridFrameY + plotGridFrameHeight ),
    PlotBackgroundColor     ( "white" ),    // background
    PlotFrameColor          ( "black" ),    // frame
    PlotGridColor           ( "gray" ),     // grid
    PlotTextColor           ( "black" ),    // text
    PlotMarkerNewColor      ( "darkCyan" ), // marker for new connection
    PlotMarkerNewLocalColor ( "blue" ),     // marker for new local connection
    PlotMarkerStopColor     ( "red" )       // marker for server stop
{

    // set SVG veiwBox to correct size to ensure correct scaling
    svgRootAttributes.append("viewBox",
        QString("%1, %2, %3, %4")
            .arg(plotCanvasRectX)
            .arg(plotCanvasRectY)
            .arg(plotCanvasRectWidth)
            .arg(plotCanvasRectHeight)
    );

    svgRootAttributes.append("xmlns", "http://www.w3.org/2000/svg");
    svgRootAttributes.append("xmlns:xlink", "http://www.w3.org/1999/xlink");

    // generate plot grid frame rectangle
    gridFrameAttributes.append("stroke", QString("%1").arg(PlotFrameColor));
    gridFrameAttributes.append("stroke-width", QString("%1").arg(1));
    gridFrameAttributes.append("style", QString("fill: none;"));

    textAttributes.append("stroke", QString("%1").arg(PlotTextColor));
    textAttributes.append("font-family", axisFontFamily);
    textAttributes.append("font-weight", axisFontWeight);
    textAttributes.append("font-size", QString("%1").arg(axisFontSize));

    gridAttributes.append("stroke", QString("%1").arg(PlotGridColor));
    gridAttributes.append("stroke-width", QString("%1").arg(1));

    gridWeekendAttributes.append("stroke", QString("%1").arg(PlotGridColor));
    gridWeekendAttributes.append("stroke-width", QString("%1").arg(iGridWidthWeekend));

    // Connections -------------------------------------------------------------
    QObject::connect ( &TimerDailyUpdate, SIGNAL ( timeout() ),
        this, SLOT ( OnTimerDailyUpdate() ) );

}

void CSvgHistoryGraph::Start ( const QString& sNewFileName )
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

void CSvgHistoryGraph::DrawFrame( QXmlStreamWriter &svgStreamWriter, const unsigned int _numDays )
{
    unsigned int i;

    // store number of x-axis ticks (number of days we want to draw
    // the history for
    numDays = _numDays;

    // Create actual plot region (grid frame) ----------------------------------
    rect(svgStreamWriter, plotGridFrameX, plotGridFrameY, plotGridFrameWidth, plotGridFrameHeight, gridFrameAttributes);

    // calculate step for x-axis ticks so that we get the desired number of
    // ticks -> 5 ticks

// TODO the following equation does not work as expected but results are acceptable

    // we want to have "floor ( numDays / 5 )" which is the same as
    // "numDays / 5" since "numDays" is an integer variable
    const unsigned int iXAxisTickStep = numDays / 5 + 1;

    // grid (ticks) for x-axis
    dayXSpace = static_cast<double> ( plotCanvasRectWidth / ( numDays + 1 ) );
    for ( i = 0; i < numDays; i++ )
    {
        unsigned int         iBottomExtraTickLen = 0;
        const unsigned int   iCurX = plotGridFrameX + static_cast<unsigned int> ( dayXSpace * ( i + 1 ) );
        const QDate curXAxisDate = curDate.addDays ( i - numDays + 1 );

        // text (print only every "iXAxisTickStep" tick)
        if ( !( i % iXAxisTickStep ) )
        {
            text(svgStreamWriter, static_cast<unsigned int>(std::max(0, static_cast<signed int>(iCurX - iTextOffsetX))), plotGridFrameBottom + iXAxisTextHeight + iTextOffsetToGrid, curXAxisDate.toString ( "dd.MM." ), textAttributes);

            iBottomExtraTickLen = 5;
        }

        // regular grid
        line(svgStreamWriter, iCurX, 1 + plotGridFrameY, iCurX, plotGridFrameBottom + iBottomExtraTickLen, gridAttributes);

        // different grid width for weekends (overwrite regular grid)
        if ( ( curXAxisDate.dayOfWeek() == 6 ) || // check for Saturday
             ( curXAxisDate.dayOfWeek() == 7 ) )  // check for Sunday
        {
            const unsigned int iGridWidthWeekendHalf = iGridWidthWeekend / 2;

            line(svgStreamWriter, iCurX, 1 + plotGridFrameY + iGridWidthWeekendHalf, iCurX, plotGridFrameBottom - iGridWidthWeekendHalf, gridWeekendAttributes);
        }
    }

    // grid (ticks) for y-axis, draw iNumTicksY - 2 grid lines and
    // iNumTicksY - 1 text labels (the lowest grid line is the grid frame)
    iYSpace = plotGridFrameHeight / ( iNumTicksY - 1 );
    for ( i = 0; i < ( iNumTicksY - 1 ); i++ )
    {
        const unsigned int iCurY = plotGridFrameY + iYSpace * ( i + 1 );

        // text
        text(svgStreamWriter, plotGridFrameX + iTextOffsetToGrid, iCurY - iTextOffsetToGrid,
                QString ( "%1:00" ).arg ( ( iYAxisEnd - iYAxisStart ) / ( iNumTicksY - 1 ) * ( ( iNumTicksY - 2 ) - i ) ),
                textAttributes );

        // grid (do not overwrite frame)
        if ( i < ( iNumTicksY - 2 ) )
        {
            line(svgStreamWriter, plotGridFrameX, iCurY, plotGridFrameRight, iCurY, gridAttributes);
        }
    }
}

void CSvgHistoryGraph::AddMarker ( QXmlStreamWriter &svgStreamWriter, const SHistoryData& curHistoryData )
{
    // calculate x-axis offset (difference of days compared to
    // current date)
    const unsigned int iXAxisOffs = static_cast<unsigned int>( curDate.daysTo ( curHistoryData.DateTime.date() ) );

    // check range, if out of range, do not plot anything
    if ( -iXAxisOffs > ( numDays - 1 ) )
    {
        return;
    }

    // calculate y-axis offset (consider hours and minutes)
    const double dYAxisOffs = 24 - curHistoryData.DateTime.time().hour() -
        static_cast<double> ( curHistoryData.DateTime.time().minute() ) / 60;

    // calculate the actual point in the graph (in pixels)
    unsigned int curPointX = plotGridFrameX + static_cast<unsigned int> ( dayXSpace * ( numDays + iXAxisOffs ) );
    unsigned int curPointY = plotGridFrameY + static_cast<unsigned int> ( static_cast<double> ( plotGridFrameHeight ) / ( iYAxisEnd - iYAxisStart ) * dYAxisOffs );
    unsigned int wh = 0;
    QXmlStreamAttributes curPointAttibutes;
    curPointAttibutes.append("stroke", "0");

    // we use different markers for new connection and server stop items
    switch ( curHistoryData.Type )
    {
    case CHistoryGraph::EHistoryItemType::HIT_SERVER_STOP:
        curPointAttibutes.append("fill", PlotMarkerStopColor);
        wh = iMarkerSizeServSt;
        break;

    case CHistoryGraph::EHistoryItemType::HIT_LOCAL_CONNECTION:
        curPointAttibutes.append("fill", PlotMarkerNewLocalColor);
        wh = iMarkerSizeNewCon;
        break;

    case CHistoryGraph::EHistoryItemType::HIT_REMOTE_CONNECTION:
        curPointAttibutes.append("fill", PlotMarkerNewColor);
        wh = iMarkerSizeNewCon;
        break;
    }
    rect(svgStreamWriter, curPointX, curPointY, wh, wh, curPointAttibutes);
}

void CSvgHistoryGraph::Save ( const QString sFileName, const QString& svgImage )
{
    QFile outf (sFileName);
    if (!outf.open(QFile::WriteOnly)) {
        throw std::runtime_error( (sFileName + " could not be written.  Aborting.").toStdString() );
    }
    QTextStream out(&outf);

    out << svgImage << endl;
}

void CSvgHistoryGraph::Add ( const QDateTime&   newDateTime,
                          const QHostAddress ClientInetAddr )
{
    if ( bDoHistory )
    {
        // add element to history, distinguish between a local connection
        // and a remote connection
        if ( ( ClientInetAddr == QHostAddress ( "127.0.0.1" ) ) ||
             ( ClientInetAddr.toString().left ( 7 ).compare ( "192.168" ) == 0 ) )
        {
            // local connection
            Add ( newDateTime, CHistoryGraph::HIT_LOCAL_CONNECTION );
        }
        else
        {
            // remote connection
            Add ( newDateTime, CHistoryGraph::HIT_REMOTE_CONNECTION );
        }
    }
}

void CSvgHistoryGraph::Add ( const QDateTime& newDateTime,
                          const CHistoryGraph::EHistoryItemType curType )
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

void CSvgHistoryGraph::Update()
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

        const unsigned int iNumDaysInHistory = static_cast<const unsigned>( -curDate.daysTo ( oldestDate ) + 1 );

        // create SVG document
        QString svgImage;
        QXmlStreamWriter svgStreamWriter( &svgImage );
        svgStreamWriter.setAutoFormatting(true);
        svgStreamWriter.writeStartDocument();
        svgStreamWriter.writeStartElement("svg");
        svgStreamWriter.writeAttributes(svgRootAttributes);

        // draw frame of the graph
        DrawFrame ( svgStreamWriter, iNumDaysInHistory );

        // add markers
        for ( i = 0; i < iNumItemsForHistory; i++ )
        {
            // only use valid dates
            if ( vHistoryDataFifo[i].DateTime.date().isValid() )
            {
                AddMarker ( svgStreamWriter, vHistoryDataFifo[i] );
            }
        }

        svgStreamWriter.writeEndDocument();

        // save graph as picture in file
        Save ( sFileName, svgImage );
    }
}

void CSvgHistoryGraph::rect ( QXmlStreamWriter &svgStreamWriter, const unsigned int x, const unsigned int y, const unsigned width, const unsigned height, const QXmlStreamAttributes &attrs )
{
    svgStreamWriter.writeEmptyElement("rect");
    svgStreamWriter.writeAttribute("x", QString("%1").arg(x));
    svgStreamWriter.writeAttribute("y", QString("%1").arg(y));
    svgStreamWriter.writeAttribute("width", QString("%1").arg(width));
    svgStreamWriter.writeAttribute("height", QString("%1").arg(height));
    if (attrs.size() > 0) {
        svgStreamWriter.writeAttributes(attrs);
    }
}

void CSvgHistoryGraph::line ( QXmlStreamWriter &svgStreamWriter, const unsigned int x1, const unsigned int y1, const unsigned int x2, const unsigned int y2, const QXmlStreamAttributes &attrs )
{
    svgStreamWriter.writeEmptyElement("line");
    svgStreamWriter.writeAttribute("x1", QString("%1").arg(x1));
    svgStreamWriter.writeAttribute("y1", QString("%1").arg(y1));
    svgStreamWriter.writeAttribute("x2", QString("%1").arg(x2));
    svgStreamWriter.writeAttribute("y2", QString("%1").arg(y2));
    if (attrs.size() > 0) {
        svgStreamWriter.writeAttributes(attrs);
    }
}

void CSvgHistoryGraph::text ( QXmlStreamWriter &svgStreamWriter, const unsigned int x, const unsigned int y, const QString &value, const QXmlStreamAttributes &attrs )
{
    svgStreamWriter.writeStartElement("text");
    svgStreamWriter.writeAttribute("x", QString("%1").arg(x));
    svgStreamWriter.writeAttribute("y", QString("%1").arg(y));
    if (attrs.size() > 0) {
        svgStreamWriter.writeAttributes(attrs);
    }
    svgStreamWriter.writeCharacters( value );
    svgStreamWriter.writeEndElement();
}
