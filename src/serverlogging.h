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

#if !defined ( SERVERLOGGING_HOIHOKIH83JH8_3_43445KJIUHF1912__INCLUDED_ )
#define SERVERLOGGING_HOIHOKIH83JH8_3_43445KJIUHF1912__INCLUDED_

#include <qimage.h>
#include <qpainter.h>
#include <qdatetime.h>
#include <qhostaddress.h>
#include <qfile.h>
#include <qstring.h>
#include <qtimer.h>
#include "global.h"
#include "util.h"


/* Definitions ****************************************************************/
// number of history items to store
#define NUM_ITEMS_HISTORY           600


/* Classes ********************************************************************/
class CHistoryGraph : public QObject
{
    Q_OBJECT

public:
    enum EHistoryItemType
    {
        HIT_LOCAL_CONNECTION,
        HIT_REMOTE_CONNECTION,
        HIT_SERVER_STOP
    };

    CHistoryGraph();
    void Start ( const QString& sNewFileName );
    void Add ( const QDateTime& newDateTime, const EHistoryItemType curType );
    void Add ( const QDateTime& newDateTime, const QHostAddress ClientInetAddr );
    void Update();

protected:
    struct SHistoryData
    {
        QDateTime        DateTime;
        EHistoryItemType Type;
    };
    void DrawFrame ( const int iNewNumTicksX );
    void AddMarker ( const SHistoryData& curHistoryData );
    void Save ( const QString sFileName );

    bool    bDoHistory;
    int     iYAxisStart;
    int     iYAxisEnd;
    int     iNumTicksX;
    int     iNumTicksY;
    int     iGridFrameOffset;
    int     iGridWidthWeekend;
    int     iTextOffsetToGrid;
    int     iTextOffsetX;
    int     iXAxisTextHeight;
    int     iMarkerSizeNewCon;
    int     iMarkerSizeServSt;
    double  dXSpace;
    int     iYSpace;
    QFont   AxisFont;
    QColor  PlotBackgroundColor;
    QColor  PlotFrameColor;
    QColor  PlotGridColor;
    QColor  PlotTextColor;
    QColor  PlotMarkerNewColor;
    QColor  PlotMarkerNewLocalColor;
    QColor  PlotMarkerStopColor;
    QDate   curDate;
    QImage  PlotPixmap;
    QRect   PlotCanvasRect;
    QRect   PlotGridFrame;
    QString sFileName;
    QTimer  TimerDailyUpdate;

    CFIFO<SHistoryData> vHistoryDataFifo;

public slots:
    void OnTimerDailyUpdate() { Update(); }
};


class CServerLogging
{
public:
    CServerLogging() : bDoLogging ( false ),
        File ( DEFAULT_LOG_FILE_NAME ) {}
    virtual ~CServerLogging();

    void Start ( const QString& strLoggingFileName );
    void EnableHistory ( const QString& strHistoryFileName );
    void AddNewConnection ( const QHostAddress& ClientInetAddr );
    void AddServerStopped();
    void ParseLogFile ( const QString& strFileName );

protected:
    void operator<< ( const QString& sNewStr );
    QString CurTimeDatetoLogString();

    CHistoryGraph HistoryGraph;
    bool          bDoLogging;
    QFile         File;
};

#endif /* !defined ( SERVERLOGGING_HOIHOKIH83JH8_3_43445KJIUHF1912__INCLUDED_ ) */
