/******************************************************************************\
 * Copyright (c) 2004-2013
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

#if !defined ( ANALYZERCONSOLE_H__FD6BKUBVE723IUH06C2AC__INCLUDED_ )
#define ANALYZERCONSOLE_H__FD6BKUBVE723IUH06C2AC__INCLUDED_

#include <QDialog>
#include <QTabWidget>
#include <QLabel>
#include <QVBoxLayout>
#include <QImage>
#include <QPainter>
#include <QTimer>
#include "client.h"


/* Definitions ****************************************************************/
// defines the update time of the error rate graph
#define ERR_RATE_GRAPH_UPDATE_TIME_MS       200 // ms


/* Classes ********************************************************************/
class CAnalyzerConsole : public QDialog
{
    Q_OBJECT

public:
    CAnalyzerConsole ( CClient*        pNCliP,
                       QWidget*        parent = 0,
                       Qt::WindowFlags f = 0 );


protected:
    void DrawFrame();
    void DrawErrorRateTrace();
    int  CalcYPosInGraph ( const double dAxisMin,
                           const double dAxisMax,
                           const double dValue ) const;

    CClient*    pClient;

    QTabWidget* pMainTabWidget;
    QWidget*    pTabWidgetBufErrRate;

    QLabel*     pGraphErrRate;
    QImage      GraphImage;

    QRect       GraphErrRateCanvasRect;
    QRect       GraphGridFrame;

    int         iGridFrameOffset;
    int         iLineWidth;
    int         iMarkerSize;
    int         iXAxisTextHeight;

    QColor      GraphBackgroundColor;
    QColor      GraphFrameColor;
    QColor      GraphGridColor;
    QColor      LineColor;
    QColor      LineLimitColor;

    QTimer      TimerErrRateUpdate;


public slots:
    void OnTimerErrRateUpdate();
    void OnResizeEvent ( QResizeEvent* event );
};

#endif // ANALYZERCONSOLE_H__FD6BKUBVE723IUH06C2AC__INCLUDED_
