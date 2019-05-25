#include "historygraph.h"

AHistoryGraph::AHistoryGraph() :
    sFileName        ( "" ),
    bDoHistory       ( false ),
    vHistoryDataFifo ( NUM_ITEMS_HISTORY ),
    iNumTicksX       ( 0 ), // number of days in history

    BackgroundColor     ( "white" ), // background
    FrameColor          ( "black" ), // frame
    GridColor           ( "gray" ), // grid
    TextColor           ( "black" ), // text
    MarkerNewColor      ( "darkCyan" ), // marker for new connection
    MarkerNewLocalColor ( "blue" ), // marker for new local connection
    MarkerStopColor     ( "red" ), // marker for server stop

    canvasRectX             ( 0 ),
    canvasRectY             ( 0 ),
    canvasRectWidth         ( 640 ),
    canvasRectHeight        ( 450 ),

    iGridFrameOffset        ( 10 ),
    iGridWidthWeekend       ( 3 ), // should be an odd value
    iXAxisTextHeight        ( 22 ),
    gridFrameX              ( canvasRectX + iGridFrameOffset ),
    gridFrameY              ( canvasRectY + iGridFrameOffset ),
    gridFrameWidth          ( canvasRectWidth - 2 * iGridFrameOffset ),
    gridFrameHeight         ( canvasRectHeight - 2 * iGridFrameOffset - iXAxisTextHeight ),
    gridFrameRight          ( gridFrameX + gridFrameWidth - 1 ),
    gridFrameBottom         ( gridFrameY + gridFrameHeight - 1 ),

    axisFontFamily          ( "Arial" ),
    axisFontWeight          ( "100" ),
    axisFontSize            ( 12 ),

    iYAxisStart             ( 0 ),
    iYAxisEnd               ( 24 ),
    iNumTicksY              ( 5 ),

    iTextOffsetToGrid       ( 3 ),
    iTextOffsetX            ( 18 ),

    iMarkerSizeNewCon       ( 11 ),
    iMarkerSizeServSt       ( 8 )
{
}

void AHistoryGraph::Start ( const QString& sNewFileName )
{
    QTextStream& tsConsoleStream = *( ( new ConsoleWriterFactory() )->get() );
    tsConsoleStream << QString("AHistoryGraph::Start ( %1 )").arg(sNewFileName) << endl; // on console
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
            // only use valid dates
            if ( vHistoryDataFifo[i].DateTime.date().isValid() )
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
    for ( i = 0; i < static_cast<int>(iNumTicksX); i++ )
    {
        int         iBottomExtraTickLen = 0;
        const int   iCurX = gridFrameX + static_cast<int> ( dayXSpace * ( i + 1 ) );
        const QDate curXAxisDate = curDate.addDays ( i - iNumTicksX + 1 );

        // text (print only every "iXAxisTickStep" tick)
        if ( !( i % iXAxisTickStep ) )
        {
            text( iCurX - iTextOffsetX, gridFrameBottom + iXAxisTextHeight + iTextOffsetToGrid, curXAxisDate.toString ( "dd.MM." ) );

            iBottomExtraTickLen = 5;
        }

        // regular grid
        line( iCurX, 1 + gridFrameY, iCurX, gridFrameBottom + iBottomExtraTickLen );

        // different grid width for weekends (overwrite regular grid)
        if ( ( curXAxisDate.dayOfWeek() == 6 ) || // check for Saturday
             ( curXAxisDate.dayOfWeek() == 7 ) )  // check for Sunday
        {
            const int iGridWidthWeekendHalf = iGridWidthWeekend / 2;

            line( iCurX, 1 + gridFrameY + iGridWidthWeekendHalf, iCurX, gridFrameBottom - iGridWidthWeekendHalf, iGridWidthWeekend );
        }
    }

    // grid (ticks) for y-axis, draw iNumTicksY - 2 grid lines and
    // iNumTicksY - 1 text labels (the lowest grid line is the grid frame)
    iYSpace = gridFrameHeight / ( iNumTicksY - 1 );
    for ( i = 0; i < ( static_cast<int>(iNumTicksY) - 1 ); i++ )
    {
        const int iCurY = gridFrameY + iYSpace * ( i + 1 );

        // text
        text( gridFrameX + iTextOffsetToGrid,
            iCurY - iTextOffsetToGrid,
            QString ( "%1:00" ).arg ( ( iYAxisEnd - iYAxisStart ) / ( iNumTicksY - 1 ) * ( ( iNumTicksY - 2 ) - i ) ) );

        // grid (do not overwrite frame)
        if ( i < ( static_cast<int>(iNumTicksY) - 2 ) )
        {
            line( gridFrameX, iCurY, gridFrameRight, iCurY );
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
    if ( -iXAxisOffs > ( static_cast<int>(iNumTicksX) - 1 ) )
    {
        return;
    }

    // calculate y-axis offset (consider hours and minutes)
    const double dYAxisOffs = 24 - curHistoryData.DateTime.time().hour() -
        static_cast<double> ( curHistoryData.DateTime.time().minute() ) / 60;

    // calculate the actual point in the graph (in pixels)
    int curPointX = gridFrameX + static_cast<int> ( dayXSpace * ( iNumTicksX + iXAxisOffs ) );
    int curPointY = gridFrameY + static_cast<int> ( static_cast<double> (
        gridFrameHeight ) / ( iYAxisEnd - iYAxisStart ) * dYAxisOffs );
    QString curPointColour = MarkerNewColor;
    int curPointSize = iMarkerSizeNewCon;

    // we use different markers for new connection and server stop items
    switch ( curHistoryData.Type )
    {
    case HIT_SERVER_STOP:
        curPointColour = MarkerStopColor;
        curPointSize = iMarkerSizeServSt;
        break;

    case HIT_LOCAL_CONNECTION:
        curPointColour = MarkerNewLocalColor;
        break;

    case HIT_REMOTE_CONNECTION:
        curPointColour = MarkerNewColor;
        break;
    }

    point( curPointX, curPointY, curPointSize, curPointColour );
}
