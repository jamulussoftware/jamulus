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

#pragma once

#include <QDateTime>
#include <QHostAddress>
#include <QFile>
#include <QString>
#include <QTimer>
#include "global.h"
#include "util.h"

// for CJpegHistoryGraph
#include <QImage>
#include <QPainter>

// for CSvgHistoryGraph
#include <QXmlStreamWriter>
#include <QXmlStreamAttributes>


/* Definitions ****************************************************************/
// number of history items to store
#define NUM_ITEMS_HISTORY           20000


/* Interface ******************************************************************/
class AHistoryGraph
{
public:
    enum EHistoryItemType
    {
        HIT_LOCAL_CONNECTION,
        HIT_REMOTE_CONNECTION,
        HIT_SERVER_STOP
    };

    AHistoryGraph ( const int iMaxDaysHistory );
    ~AHistoryGraph() { }

    void Start ( const QString& sNewFileName );
    void Add ( const QDateTime& newDateTime, const EHistoryItemType curType );
    void Add ( const QDateTime& newDateTime, const QHostAddress ClientInetAddr );
    virtual void Update ( );

protected:
    struct SHistoryData
    {
        QDateTime        DateTime;
        EHistoryItemType Type;
    };
    void DrawFrame ( const int iNewNumTicksX );
    void AddMarker ( const SHistoryData& curHistoryData );
    virtual void Save ( const QString sFileName ) = 0;

    virtual void rect ( const unsigned int x, const unsigned int y, const unsigned int width, const unsigned int height ) = 0;
    virtual void text ( const unsigned int x, const unsigned int y, const QString& value ) = 0;
    virtual void line ( const unsigned int x1, const unsigned int y1, const unsigned int x2, const unsigned int y2, const unsigned int strokeWidth = 1 ) = 0;
    virtual void point ( const unsigned int x, const unsigned int y, const unsigned int size, const QString& colour ) = 0;

    // Constructor sets these
    QString             sFileName;
    bool                bDoHistory;
    CFIFO<SHistoryData> vHistoryDataFifo;
    unsigned int        iNumTicksX; // Class global, not sure why
    int                 iHistMaxDays;

    QString             BackgroundColor;
    QString             FrameColor;
    QString             GridColor;
    QString             TextColor;
    QString             MarkerNewColor;
    QString             MarkerNewLocalColor;
    QString             MarkerStopColor;

    const unsigned int  canvasRectX;
    const unsigned int  canvasRectY;
    const unsigned int  canvasRectWidth;
    const unsigned int  canvasRectHeight;

    const unsigned int  iGridFrameOffset;
    const unsigned int  iGridWidthWeekend;
    const unsigned int  iXAxisTextHeight;
    const unsigned int  gridFrameX;
    const unsigned int  gridFrameY;
    const unsigned int  gridFrameWidth;
    const unsigned int  gridFrameHeight;
    const unsigned int  gridFrameRight;
    const unsigned int  gridFrameBottom;

    const QString       axisFontFamily;
    const QString       axisFontWeight;
    const unsigned int  axisFontSize;

    const unsigned int  iYAxisStart;
    const unsigned int  iYAxisEnd;
    const unsigned int  iNumTicksY;

    const unsigned int  iTextOffsetToGrid;
    const unsigned int  iTextOffsetX;

    const unsigned int  iMarkerSizeNewCon;
    const unsigned int  iMarkerSizeServSt;

    // others
    double       dayXSpace;
    unsigned int iYSpace;
    QDate        curDate;
    QTimer       TimerDailyUpdate;
};


/* Implementations ************************************************************/
class CJpegHistoryGraph : public QObject, virtual public AHistoryGraph
{
    Q_OBJECT

public:
    CJpegHistoryGraph ( const int iMaxDaysHistory );
    virtual void Update ( );

protected:
    virtual void Save ( const QString sFileName );

    virtual void rect ( const unsigned int x, const unsigned int y, const unsigned int width, const unsigned int height );
    virtual void text ( const unsigned int x, const unsigned int y, const QString& value );
    virtual void line ( const unsigned int x1, const unsigned int y1, const unsigned int x2, const unsigned int y2, const unsigned int strokeWidth = 1 );
    virtual void point ( const unsigned int x, const unsigned int y, const unsigned int size, const QString& colour );

private:
    QImage PlotPixmap;
    int iAxisFontWeight;

public slots:
    void OnTimerDailyUpdate() { Update(); }
};

class CSvgHistoryGraph : public QObject, virtual public AHistoryGraph
{
    Q_OBJECT

public:
    CSvgHistoryGraph ( const int iMaxDaysHistory );
    virtual void Update();

protected:
    virtual void Save ( const QString sFileName );

    virtual void rect ( const unsigned int x, const unsigned int y, const unsigned int width, const unsigned int height );
    virtual void text ( const unsigned int x, const unsigned int y, const QString& value );
    virtual void line ( const unsigned int x1, const unsigned int y1, const unsigned int x2, const unsigned int y2, const unsigned int strokeWidth = 1 );
    virtual void point ( const unsigned int x, const unsigned int y, const unsigned int size, const QString& colour );

private:
    QXmlStreamAttributes svgRootAttributes;
    QString svgImage;
    QXmlStreamWriter svgStreamWriter;

public slots:
    void OnTimerDailyUpdate() { Update(); }
};
