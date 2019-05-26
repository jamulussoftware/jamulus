#include "historygraph.h"

CSvgHistoryGraph::CSvgHistoryGraph() :
    AHistoryGraph(),
    svgImage( "" ),
    svgStreamWriter( &svgImage )
{
    // set SVG veiwBox to correct size to ensure correct scaling
    svgRootAttributes.append("viewBox",
        QString("%1, %2, %3, %4")
            .arg(canvasRectX)
            .arg(canvasRectY)
            .arg(canvasRectWidth)
            .arg(canvasRectHeight)
    );

    svgRootAttributes.append("xmlns", "http://www.w3.org/2000/svg");
    svgRootAttributes.append("xmlns:xlink", "http://www.w3.org/1999/xlink");

    QTextStream& tsConsoleStream = *( ( new ConsoleWriterFactory() )->get() );
    tsConsoleStream << "CSvgHistoryGraph" << endl; // on console

    // Connections -------------------------------------------------------------
    QObject::connect ( &TimerDailyUpdate, SIGNAL ( timeout() ),
        this, SLOT ( OnTimerDailyUpdate() ) );
}

// Override Update to create the fresh SVG stream each time
void CSvgHistoryGraph::Update()
{
    if ( bDoHistory )
    {
        // create SVG document
        svgImage = "";

        svgStreamWriter.setAutoFormatting(true);
        svgStreamWriter.writeStartDocument();
        svgStreamWriter.writeStartElement("svg");
        svgStreamWriter.writeAttributes(svgRootAttributes);

        AHistoryGraph::Update();
    }
}

void CSvgHistoryGraph::Save ( const QString sFileName )
{
    svgStreamWriter.writeEndDocument();

    QFile outf (sFileName);
    if (!outf.open(QFile::WriteOnly)) {
        throw std::runtime_error( (sFileName + " could not be written.  Aborting.").toStdString() );
    }
    QTextStream out(&outf);

    out << svgImage << endl;
}

void CSvgHistoryGraph::rect ( const unsigned int x, const unsigned int y, const unsigned int width, const unsigned int height )
{
    svgStreamWriter.writeEmptyElement("rect");
    svgStreamWriter.writeAttribute("x", QString("%1").arg(x));
    svgStreamWriter.writeAttribute("y", QString("%1").arg(y));
    svgStreamWriter.writeAttribute("width", QString("%1").arg(width));
    svgStreamWriter.writeAttribute("height", QString("%1").arg(height));
    svgStreamWriter.writeAttribute("stroke", FrameColor);
    svgStreamWriter.writeAttribute("stroke-width", QString("1"));
    svgStreamWriter.writeAttribute("style", QString("fill: none;"));
}

void CSvgHistoryGraph::text ( const unsigned int x, const unsigned int y, const QString& value )
{
    svgStreamWriter.writeStartElement("text");
    svgStreamWriter.writeAttribute("x", QString("%1").arg(x));
    svgStreamWriter.writeAttribute("y", QString("%1").arg(y));
    svgStreamWriter.writeAttribute("stroke", TextColor);
    svgStreamWriter.writeAttribute("font-family", axisFontFamily);
    svgStreamWriter.writeAttribute("font-weight", axisFontWeight);
    svgStreamWriter.writeAttribute("font-size", QString("%1").arg(axisFontSize));

    svgStreamWriter.writeCharacters( value );
    svgStreamWriter.writeEndElement();
}

void CSvgHistoryGraph::line ( const unsigned int x1, const unsigned int y1, const unsigned int x2, const unsigned int y2, const unsigned int strokeWidth )
{
    svgStreamWriter.writeEmptyElement("line");
    svgStreamWriter.writeAttribute("x1", QString("%1").arg(x1));
    svgStreamWriter.writeAttribute("y1", QString("%1").arg(y1));
    svgStreamWriter.writeAttribute("x2", QString("%1").arg(x2));
    svgStreamWriter.writeAttribute("y2", QString("%1").arg(y2));
    svgStreamWriter.writeAttribute("stroke", GridColor);
    svgStreamWriter.writeAttribute("stroke-width", QString("%1").arg(strokeWidth));
}

void CSvgHistoryGraph::point ( const unsigned int x, const unsigned int y, const unsigned int size, const QString& colour )
{
    svgStreamWriter.writeEmptyElement("rect");
    svgStreamWriter.writeAttribute("x", QString("%1").arg(x));
    svgStreamWriter.writeAttribute("y", QString("%1").arg(y));
    svgStreamWriter.writeAttribute("width", QString("%1").arg(size));
    svgStreamWriter.writeAttribute("height", QString("%1").arg(size));
    svgStreamWriter.writeAttribute("stroke-opacity", "0");
    svgStreamWriter.writeAttribute("fill", colour);
}
