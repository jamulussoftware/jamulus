/******************************************************************************\
 * Copyright (c) 2020-2022
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
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 *
\******************************************************************************/

#pragma once

#include <QUuid>
#include <QFileInfo>
#include <QDir>
#include "util.h"

#include "cwavestream.h"

namespace recorder
{

class CReaperItem : public QObject
{
    Q_OBJECT

public:
    CReaperItem ( const QString& name, const STrackItem& trackItem, const qint32& iid, int frameSize );
    QString toString() { return out; }

private:
    const QUuid iguid = QUuid::createUuid();
    const QUuid guid  = QUuid::createUuid();
    QString     out;
};

class CReaperTrack : public QObject
{
    Q_OBJECT

public:
    CReaperTrack ( QString name, qint32& iid, QList<STrackItem> items, int frameSize );
    QString toString() { return out; }

private:
    QUuid   trackId = QUuid::createUuid();
    QString out;
};

class CReaperProject : public QObject
{
    Q_OBJECT

public:
    CReaperProject ( QMap<QString, QList<STrackItem>> tracks, int frameSize );
    QString toString() { return out; }

private:
    QString out;
};

} // namespace recorder
