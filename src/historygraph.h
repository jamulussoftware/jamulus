#ifndef HISTORYGRAPH_H
#define HISTORYGRAPH_H

#include <QImage>
#include <QPainter>
#include <QDateTime>
#include <QHostAddress>
#include <QFile>
#include <QString>
#include <QTimer>
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
    void Save      ( const QString sFileName );

    QString             sFileName;

    bool                bDoHistory;
    CFIFO<SHistoryData> vHistoryDataFifo;

    QRect               PlotCanvasRect;

    int                 iNumTicksX;
    int                 iYAxisStart;
    int                 iYAxisEnd;
    int                 iNumTicksY;
    int                 iGridFrameOffset;
    int                 iGridWidthWeekend;
    int                 iTextOffsetToGrid;
    int                 iXAxisTextHeight;
    int                 iMarkerSizeNewCon;
    int                 iMarkerSizeServSt;

    QFont               AxisFont;
    int                 iTextOffsetX;

    QColor              PlotBackgroundColor;
    QColor              PlotFrameColor;
    QColor              PlotGridColor;
    QColor              PlotTextColor;
    QColor              PlotMarkerNewColor;
    QColor              PlotMarkerNewLocalColor;
    QColor              PlotMarkerStopColor;

    QImage              PlotPixmap;

    double              dXSpace;
    int                 iYSpace;

    QDate               curDate;
    QRect               PlotGridFrame;
    QTimer              TimerDailyUpdate;

public slots:
    void OnTimerDailyUpdate() { Update(); }
};


#endif // HISTORYGRAPH_H
