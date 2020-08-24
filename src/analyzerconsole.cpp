/******************************************************************************\
 * Copyright (c) 2004-2020
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
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 *
\******************************************************************************/

#include "analyzerconsole.h"


// Analyzer console implementation *********************************************
CAnalyzerConsole::CAnalyzerConsole ( CClient* pNCliP,
                                     QWidget* parent,
                                     Qt::WindowFlags ) :
    QDialog                ( parent ),
    pClient                ( pNCliP ),
    GraphImage             ( 1, 1, QImage::Format_RGB32 ),
    GraphErrRateCanvasRect ( 0, 0, 600, 450 ), // defines total size of graph
    iGridFrameOffset       ( 10 ),
    iLineWidth             ( 2 ),
    iMarkerSize            ( 10 ),
    iXAxisTextHeight       ( 22 ),
    GraphBackgroundColor   ( Qt::white ), // background
    GraphFrameColor        ( Qt::black ), // frame
    GraphGridColor         ( Qt::gray ), // grid
    LineColor              ( Qt::blue ),
    LineLimitColor         ( Qt::green ),
    LineMaxUpLimitColor    ( Qt::red )
{
    // set the window icon and title text
    const QIcon icon = QIcon ( QString::fromUtf8 ( ":/png/main/res/fronticon.png" ) );
    setWindowIcon ( icon );
    setWindowTitle ( tr ( "Analyzer Console" ) );

    // create main layout
    QVBoxLayout* pMainLayout = new QVBoxLayout;

    // create and add main tab widget
    pMainTabWidget = new QTabWidget ( this );
    pMainLayout->addWidget ( pMainTabWidget );

    setLayout ( pMainLayout );

    // error rate gaph tab
    pTabWidgetBufErrRate = new QWidget();
    QVBoxLayout* pTabErrRateLayout = new QVBoxLayout ( pTabWidgetBufErrRate );

    pGraphErrRate = new QLabel ( this );
    pTabErrRateLayout->addWidget ( pGraphErrRate );

    pMainTabWidget->addTab ( pTabWidgetBufErrRate,
                             tr ( "Jitter statistics" ) );


    // Connections -------------------------------------------------------------
    // timers
    QObject::connect ( &TimerErrRateUpdate, &QTimer::timeout,
        this, &CAnalyzerConsole::OnTimerErrRateUpdate );
}

void CAnalyzerConsole::showEvent ( QShowEvent* )
{
    // start timer for error rate graph
    TimerErrRateUpdate.start ( ERR_RATE_GRAPH_UPDATE_TIME_MS );
}

void CAnalyzerConsole::hideEvent ( QHideEvent* )
{
    // if window is closed, stop timer
    TimerErrRateUpdate.stop();
}

void CAnalyzerConsole::OnTimerErrRateUpdate()
{
    // generate current graph image
    DrawFrame();
    DrawErrorRateTrace();

    // set new image to the label
    pGraphErrRate->setPixmap ( QPixmap().fromImage ( GraphImage ) );
}

void CAnalyzerConsole::DrawFrame()
{
    // scale image to correct size
    GraphImage = GraphImage.scaled (
        GraphErrRateCanvasRect.width(), GraphErrRateCanvasRect.height() );

    // generate plot grid frame rectangle
    GraphGridFrame.setRect ( GraphErrRateCanvasRect.x() + iGridFrameOffset,
        GraphErrRateCanvasRect.y() + iGridFrameOffset,
        GraphErrRateCanvasRect.width() - 2 * iGridFrameOffset,
        GraphErrRateCanvasRect.height() - 2 * iGridFrameOffset - iXAxisTextHeight );

    GraphImage.fill ( GraphBackgroundColor.rgb() ); // fill background

    // create painter
    QPainter GraphPainter ( &GraphImage );

    // create actual plot region (grid frame)
    GraphPainter.setPen ( GraphFrameColor );
    GraphPainter.drawRect ( GraphGridFrame );
}

void CAnalyzerConsole::DrawErrorRateTrace()
{
    float fStats[JITTER_MAX_FRAME_COUNT];
    float fMax;
    float fClockDrift;
    float fPacketDrops;
    float fPeerAdjust;
    float fLocalAdjust;
    int height = GraphGridFrame.height();
    int width = GraphGridFrame.width();
    int fsize = ( height + 31 ) / 32;
    int xmax;

    // make room for text
    height -= 2 * fsize;

    // create painter
    QPainter GraphPainter ( &GraphImage );

    // set font size
    QFont font ( GraphPainter.font() );
    font.setPixelSize ( fsize );
    GraphPainter.setFont ( font );

    // set font pen
    GraphPainter.setPen ( QPen ( QBrush ( GraphFrameColor ), 1 ) );

    // get jitter statistics
    pClient->GetJitterStatistics ( fStats, fClockDrift, fPacketDrops, fPeerAdjust, fLocalAdjust );

    // draw information about clock drift
    GraphPainter.drawText ( QPoint ( GraphGridFrame.x(), GraphGridFrame.y() + fsize ),
        QString("PeerClkDrift = %1   RxDrops = %2   PeerAdj = %3   LocalAdj = %4")
        .arg ( fClockDrift, 0, 'f', 5 )
        .arg ( fPacketDrops, 0, 'f', 5 )
        .arg ( fPeerAdjust, 0, 'f', 5 )
        .arg ( fLocalAdjust, 0, 'f', 5 ) );

    for ( unsigned i = xmax = 0; i != JITTER_MAX_FRAME_COUNT; i++ )
    {
        if ( fStats[xmax] < fStats[i] )
        {
            xmax = i;
        }
    }
    fMax = fStats[xmax];

    if ( fMax > 1.0f )
    {
        for ( unsigned i = 0; i != JITTER_MAX_FRAME_COUNT; i++ )
        {
            fStats[i] /= fMax;
        }
    }
    else
    {
        memset(fStats, 0, sizeof(fStats));
    }

    float dX = (float) width / (float) JITTER_MAX_FRAME_COUNT;

    QPoint lastPoint;

    // plot the first data
    for ( unsigned i = 0; i != JITTER_MAX_FRAME_COUNT; i++ )
    {
        // calculate the actual point in the graph (in pixels)
        const QPoint nextPoint (
            GraphGridFrame.x() + static_cast<int> ( dX * i + dX / 2.0f ),
            GraphGridFrame.y() + 2 * fsize + height - 1 - static_cast<int> ( fStats[i] * ( height - 1 ) ) );

        if (i == 0)
            lastPoint = nextPoint;

        // draw a marker and a solid line which goes from the bottom to the
        // marker (similar to Matlab stem() function)
        GraphPainter.setPen ( QPen ( QBrush ( LineColor ) ,
                                     iMarkerSize,
                                     Qt::SolidLine,
                                     Qt::RoundCap ) );

        GraphPainter.drawPoint ( nextPoint );

        GraphPainter.setPen ( QPen ( QBrush ( LineColor ), iLineWidth ) );

        GraphPainter.drawLine ( lastPoint, nextPoint );

        lastPoint = nextPoint;
    }
}
