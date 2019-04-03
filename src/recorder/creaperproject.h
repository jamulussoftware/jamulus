#ifndef CREAPERPROJECT_H
#define CREAPERPROJECT_H

#include <QUuid>
#include <QFileInfo>
#include <QDir>
#include "util.h"

#include "cwavestream.h"

namespace recorder {

struct STrackItem
{
    STrackItem(int numAudioChannels, qint64 startFrame, qint64 frameCount, QString fileName) :
        numAudioChannels(numAudioChannels),
        startFrame(startFrame),
        frameCount(frameCount),
        fileName(fileName)
    {
    }
    int numAudioChannels;
    qint64 startFrame;
    qint64 frameCount;
    QString fileName;
};

class CReaperItem : public QObject
{
    Q_OBJECT

public:
    CReaperItem(const QString& name, const STrackItem& trackItem, const qint32& iid);
    QString toString() { return out; }

private:
    const QUuid iguid = QUuid::createUuid();
    const QUuid guid = QUuid::createUuid();
    QString out;

    inline QString secondsAt48K(const qint64 frames) { return QString::number(static_cast<double>(frames * SYSTEM_FRAME_SIZE_SAMPLES) / 48000, 'f', 14); }
};

class CReaperTrack : public QObject
{
    Q_OBJECT

public:
    CReaperTrack(QString name, qint32 &iid, QList<STrackItem> items);
    QString toString() { return out; }

private:
    QUuid trackId = QUuid::createUuid();
    QString out;
};

class CReaperProject : public QObject
{
    Q_OBJECT

public:
    CReaperProject(QMap<QString, QList<STrackItem> > tracks);
    QString toString() { return out; }

private:
    QString out;
};

}
#endif // CREAPERPROJECT_H
