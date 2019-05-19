#ifndef SVGHISTORYGRAPH_H
#define SVGHISTORYGRAPH_H

#include "historygraph.h"
#include <QXmlStreamWriter>
#include <QXmlStreamAttributes>

/* Definitions ****************************************************************/
// number of history items to store
#ifndef NUM_ITEMS_HISTORY
#define NUM_ITEMS_HISTORY           600
#endif

/* Classes ********************************************************************/
class CSvgHistoryGraph : public QObject
{
    Q_OBJECT

public:
    CSvgHistoryGraph();
    void Start ( const QString& sNewFileName );
    void Add ( const QDateTime& newDateTime, const CHistoryGraph::EHistoryItemType curType );
    void Add ( const QDateTime& newDateTime, const QHostAddress ClientInetAddr );
    void Update();

private:
    struct SHistoryData
    {
        QDateTime DateTime;
        CHistoryGraph::EHistoryItemType Type;
    };
    void DrawFrame ( QXmlStreamWriter &svgStreamWriter, const unsigned int numDays );
    void AddMarker ( QXmlStreamWriter &svgStreamWriter, const SHistoryData& curHistoryData );
    void Save ( const QString sFileName, const QString& svgImage );

    void rect ( QXmlStreamWriter &svgStreamWriter, const unsigned int x, const unsigned int y, const unsigned width, const unsigned height, const QXmlStreamAttributes &attrs = QXmlStreamAttributes() );
    void line ( QXmlStreamWriter &svgStreamWriter, const unsigned int x1, const unsigned int y1, const unsigned int x2, const unsigned int y2, const QXmlStreamAttributes &attrs = QXmlStreamAttributes() );
    void text ( QXmlStreamWriter &svgStreamWriter, const unsigned int x, const unsigned int y, const QString &value, const QXmlStreamAttributes &attrs = QXmlStreamAttributes() );

    QString             sFileName;

    bool                bDoHistory;
    CFIFO<SHistoryData> vHistoryDataFifo;

    const unsigned int plotCanvasRectX;
    const unsigned int plotCanvasRectY;
    const unsigned int plotCanvasRectWidth;
    const unsigned int plotCanvasRectHeight;

          unsigned int numDays;

    const unsigned int  iYAxisStart;
    const unsigned int  iYAxisEnd;
    const unsigned int  iNumTicksY;
    const unsigned int  iGridFrameOffset;
    const unsigned int  iGridWidthWeekend;
    const unsigned int  iTextOffsetToGrid;
    const unsigned int  iXAxisTextHeight;
    const unsigned int  iMarkerSizeNewCon;
    const unsigned int  iMarkerSizeServSt;
    const QString       axisFontFamily;
    const QString       axisFontWeight;
    const unsigned int  axisFontSize;
    const unsigned int  iTextOffsetX;

    const unsigned int plotGridFrameX;
    const unsigned int plotGridFrameY;
    const unsigned int plotGridFrameWidth;
    const unsigned int plotGridFrameHeight;
    const unsigned int plotGridFrameRight;
    const unsigned int plotGridFrameBottom;

    QString             PlotBackgroundColor;
    QString             PlotFrameColor;
    QString             PlotGridColor;
    QString             PlotTextColor;
    QString             PlotMarkerNewColor;
    QString             PlotMarkerNewLocalColor;
    QString             PlotMarkerStopColor;

    double              dayXSpace;
          unsigned int  iYSpace;

    QDate                curDate;
    QXmlStreamAttributes svgRootAttributes;
    QXmlStreamAttributes gridFrameAttributes;
    QXmlStreamAttributes textAttributes;
    QXmlStreamAttributes gridAttributes;
    QXmlStreamAttributes gridWeekendAttributes;
    QTimer               TimerDailyUpdate;

public slots:
    void OnTimerDailyUpdate() { Update(); }

};

#endif // SVGHISTORYGRAPH_H
