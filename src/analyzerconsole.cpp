/******************************************************************************\
 * Copyright (c) 2004-2024
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
CAnalyzerConsole::CAnalyzerConsole ( CClient* pNCliP, QWidget* parent ) :
    CBaseDlg ( parent, Qt::Window ), // use Qt::Window to get min/max window buttons
    pClient ( pNCliP ),
    GraphImage ( 1, 1, QImage::Format_RGB32 ),
    GraphErrRateCanvasRect ( 0, 0, 600, 450 ), // defines total size of graph
    iGridFrameOffset ( 10 ),
    iLineWidth ( 2 ),
    iMarkerSize ( 10 ),
    iXAxisTextHeight ( 22 ),
    GraphBackgroundColor ( Qt::white ), // background
    GraphFrameColor ( Qt::black ),      // frame
    GraphGridColor ( Qt::gray ),        // grid
    LineColor ( Qt::blue ),
    LineLimitColor ( Qt::green ),
    LineMaxUpLimitColor ( Qt::red )
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
    pTabWidgetBufErrRate           = new QWidget();
    QVBoxLayout* pTabErrRateLayout = new QVBoxLayout ( pTabWidgetBufErrRate );

    pGraphErrRate = new QLabel ( this );
    pTabErrRateLayout->addWidget ( pGraphErrRate );

    pMainTabWidget->addTab ( pTabWidgetBufErrRate, tr ( "Error Rate of Each Buffer Size" ) );

    // Connections -------------------------------------------------------------
    // timers
    QObject::connect ( &TimerErrRateUpdate, &QTimer::timeout, this, &CAnalyzerConsole::OnTimerErrRateUpdate );
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
    GraphImage = GraphImage.scaled ( GraphErrRateCanvasRect.width(), GraphErrRateCanvasRect.height() );

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
    // create painter
    QPainter GraphPainter ( &GraphImage );

    // get the network buffer error rates to be displayed
    CVector<double> vecButErrorRates;
    double          dLimit;
    double          dMaxUpLimit;

    pClient->GetBufErrorRates ( vecButErrorRates, dLimit, dMaxUpLimit );

    // get the number of data elements
    const int iNumBuffers = vecButErrorRates.Size();

    // convert the limits in the log domain
    const double dLogLimit      = log10 ( dLimit );
    const double dLogMaxUpLimit = log10 ( dMaxUpLimit );

    // use fixed y-axis scale where the limit line is in the middle of the graph
    const double dMax = 0;
    const double dMin = dLogLimit * 2;

    // calculate space between points on the x-axis
    const double dXSpace = static_cast<double> ( GraphGridFrame.width() ) / ( iNumBuffers - 1 );

    // plot the limit line as dashed line
    const double dYValLimitInGraph = CalcYPosInGraph ( dMin, dMax, dLogLimit );

    GraphPainter.setPen ( QPen ( QBrush ( LineLimitColor ), iLineWidth, Qt::DashLine ) );

    GraphPainter.drawLine ( QPoint ( GraphGridFrame.x(), dYValLimitInGraph ),
                            QPoint ( GraphGridFrame.x() + GraphGridFrame.width(), dYValLimitInGraph ) );

    // plot the maximum upper limit line as a dashed line
    const double dYValMaxUpLimitInGraph = CalcYPosInGraph ( dMin, dMax, dLogMaxUpLimit );

    GraphPainter.setPen ( QPen ( QBrush ( LineMaxUpLimitColor ), iLineWidth, Qt::DashLine ) );

    GraphPainter.drawLine ( QPoint ( GraphGridFrame.x(), dYValMaxUpLimitInGraph ),
                            QPoint ( GraphGridFrame.x() + GraphGridFrame.width(), dYValMaxUpLimitInGraph ) );

    // plot the data
    for ( int i = 0; i < iNumBuffers; i++ )
    {
        // data convert in log domain
        // check for special case if error rate is 0 (which would lead to -Inf
        // after the log operation)
        if ( vecButErrorRates[i] > 0 )
        {
            vecButErrorRates[i] = log10 ( vecButErrorRates[i] );
        }
        else
        {
            // definition: set it to lowest possible axis value
            vecButErrorRates[i] = dMin;
        }

        // calculate the actual point in the graph (in pixels)
        const QPoint curPoint ( GraphGridFrame.x() + static_cast<int> ( dXSpace * i ), CalcYPosInGraph ( dMin, dMax, vecButErrorRates[i] ) );

        // draw a marker and a solid line which goes from the bottom to the
        // marker (similar to Matlab stem() function)
        GraphPainter.setPen ( QPen ( QBrush ( LineColor ), iMarkerSize, Qt::SolidLine, Qt::RoundCap ) );

        GraphPainter.drawPoint ( curPoint );

        GraphPainter.setPen ( QPen ( QBrush ( LineColor ), iLineWidth ) );

        GraphPainter.drawLine ( QPoint ( curPoint.x(), GraphGridFrame.y() + GraphGridFrame.height() ), curPoint );
    }
}

int CAnalyzerConsole::CalcYPosInGraph ( const double dAxisMin, const double dAxisMax, const double dValue ) const
{
    // calculate value range
    const double dValRange = dAxisMax - dAxisMin;

    // calculate current normalized y-axis value
    const double dYValNorm = ( dValue - dAxisMin ) / dValRange;

    // consider the graph grid size to calculate the final y-axis value
    return GraphGridFrame.y() + static_cast<int> ( static_cast<double> ( GraphGridFrame.height() ) * ( 1 - dYValNorm ) );
}
