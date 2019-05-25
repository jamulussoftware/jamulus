#include "jpeghistorygraph.h"

CJpegHistoryGraph::CJpegHistoryGraph() :
    AHistoryGraph(),
    PlotPixmap ( 1, 1, QImage::Format_RGB32 ),
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
    if (!ok)
    {
        if (!QString("normal").compare(axisFontWeight, Qt::CaseSensitivity::CaseInsensitive)) { iAxisFontWeight = 50; }
        else if (!QString("bold").compare(axisFontWeight, Qt::CaseSensitivity::CaseInsensitive)) { weight = 75; }
    }
    else
    {
        if (weight <= 100) { iAxisFontWeight = 0; }
        else if (weight <= 200) { iAxisFontWeight = 12; }
        else if (weight <= 300) { iAxisFontWeight = 25; }
        else if (weight <= 400) { iAxisFontWeight = 50; }
        else if (weight <= 500) { iAxisFontWeight = 57; }
        else if (weight <= 600) { iAxisFontWeight = 63; }
        else if (weight <= 700) { iAxisFontWeight = 75; }
        else if (weight <= 800) { iAxisFontWeight = 81; }
        else if (weight <= 900) { iAxisFontWeight = 87; }
    }
    // if all else fails, it's left at -1

    QTextStream& tsConsoleStream = *( ( new ConsoleWriterFactory() )->get() );
    tsConsoleStream << "CJpegHistoryGraph" << endl; // on console

    // Connections -------------------------------------------------------------
    QObject::connect ( &TimerDailyUpdate, SIGNAL ( timeout() ),
        this, SLOT ( OnTimerDailyUpdate() ) );
}

// Override Update to create the fresh SVG stream each time
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
    PlotPainter.setFont ( QFont( axisFontFamily, static_cast<int>(axisFontSize), iAxisFontWeight ) );
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
