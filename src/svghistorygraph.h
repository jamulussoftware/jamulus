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
class CSvgHistoryGraph : public QObject, virtual public AHistoryGraph
{
    Q_OBJECT

public:
    CSvgHistoryGraph();
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

#endif // SVGHISTORYGRAPH_H
