/******************************************************************************\
 * Copyright (c) 2004-2025
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

#pragma once

#include <QDialog>
#include <QTabWidget>
#include <QLabel>
#include <QVBoxLayout>
#include <QImage>
#include <QPainter>
#include <QTimer>
#include "client.h"
#include "util.h"

/* Definitions ****************************************************************/
// defines the update time of the error rate graph
#define ERR_RATE_GRAPH_UPDATE_TIME_MS 200 // ms

/* Classes ********************************************************************/
class CAnalyzerConsole : public CBaseDlg
{
    Q_OBJECT

public:
    CAnalyzerConsole ( CClient* pNCliP, QWidget* parent = nullptr );

protected:
    virtual void showEvent ( QShowEvent* );
    virtual void hideEvent ( QHideEvent* );

    void DrawFrame();
    void DrawErrorRateTrace();
    int  CalcYPosInGraph ( const double dAxisMin, const double dAxisMax, const double dValue ) const;

    CClient* pClient;

    QTabWidget* pMainTabWidget;
    QWidget*    pTabWidgetBufErrRate;

    QLabel* pGraphErrRate;
    QImage  GraphImage;

    QRect GraphErrRateCanvasRect;
    QRect GraphGridFrame;

    int iGridFrameOffset;
    int iLineWidth;
    int iMarkerSize;
    int iXAxisTextHeight;

    QColor GraphBackgroundColor;
    QColor GraphFrameColor;
    QColor GraphGridColor;
    QColor LineColor;
    QColor LineLimitColor;
    QColor LineMaxUpLimitColor;

    QTimer TimerErrRateUpdate;

public slots:
    void OnTimerErrRateUpdate();
};
