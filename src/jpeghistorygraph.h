#ifndef JPEGHISTORYGRAPH_H
#define JPEGHISTORYGRAPH_H

#include "historygraph.h"

#include <QImage>
#include <QPainter>

/* Classes ********************************************************************/
class CJpegHistoryGraph : public QObject, virtual public AHistoryGraph
{
    Q_OBJECT

public:
    CJpegHistoryGraph();
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

#endif // JPEGHISTORYGRAPH_H
