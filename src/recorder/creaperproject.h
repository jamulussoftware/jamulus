/******************************************************************************\
 *
 * Author(s):
 *  pljones
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
    CReaperItem( const QString& name, const STrackItem& trackItem, const qint32& iid, int frameSize );
    QString toString() { return out; }

private:
    const QUuid iguid = QUuid::createUuid();
    const QUuid guid = QUuid::createUuid();
    QString out;

    inline QString secondsAt48K( const qint64 frames,
                                 const int    frameSize )
    {
        return QString::number( static_cast<double>( frames * frameSize ) / 48000, 'f', 14 );
    }
};

class CReaperTrack : public QObject
{
    Q_OBJECT

public:
    CReaperTrack( QString name, qint32 &iid, QList<STrackItem> items, int frameSize );
    QString toString() { return out; }

private:
    QUuid trackId = QUuid::createUuid();
    QString out;
};

class CReaperProject : public QObject
{
    Q_OBJECT

public:
    CReaperProject( QMap<QString, QList<STrackItem> > tracks, int frameSize );
    QString toString() { return out; }

private:
    QString out;
};

}
