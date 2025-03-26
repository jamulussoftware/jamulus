/******************************************************************************\
 * Copyright (c) 2020-2025
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

#include "creaperproject.h"

/**
 * @brief operator << Write details of the STrackItem to the QTextStream
 * @param os the QTextStream
 * @param trackItem the STrackItem
 * @return the QTextStream
 *
 * Note: unused?
 */
QTextStream& operator<< ( QTextStream& os, const recorder::STrackItem& trackItem )
{
    os << "_track( "
       << "numAudioChannels(" << trackItem.numAudioChannels << ")"
       << ", startFrame(" << trackItem.startFrame << ")"
       << ", frameCount(" << trackItem.frameCount << ")"
       << ", fileName(" << trackItem.fileName << ")"
       << " );";
    return os;
}

/******************************************************************************\
* recorder methods                                                             *
\******************************************************************************/
using namespace recorder;

// Reaper Project writer -------------------------------------------------------

/**
 * @brief CReaperItem::CReaperItem Construct a Reaper RPP "<ITEM>" for a given RIFF WAVE file
 * @param name the item name
 * @param trackItem the details of where the item is in the track, along with the RIFF WAVE filename
 * @param iid the sequential item id
 */
CReaperItem::CReaperItem ( const QString& name, const STrackItem& trackItem, const qint32& iid, int frameSize )
{
    QString wavName = trackItem.fileName; // assume RPP in same location...

    QTextStream sOut ( &out );

    sOut << "    <ITEM " << '\n'
         << "      FADEIN 0 0 0 0 0 0" << '\n'
         << "      FADEOUT 0 0 0 0 0 0" << '\n'
         << "      POSITION " << secondsAt48K ( trackItem.startFrame, frameSize ) << '\n'
         << "      LENGTH " << secondsAt48K ( trackItem.frameCount, frameSize ) << '\n'
         << "      IGUID " << iguid.toString() << '\n'
         << "      IID " << iid << '\n'
         << "      NAME " << name << '\n'
         << "      GUID " << guid.toString() << '\n'

         << "      <SOURCE WAVE" << '\n'
         << "        FILE " << '"' << wavName << '"' << '\n'
         << "      >" << '\n'

         << "    >";

    sOut.flush();
}

/**
 * @brief CReaperTrack::CReaperTrack Construct a Reaper RPP "<TRACK>" for a given list of track items
 * @param name the track name
 * @param iid the sequential track id
 * @param items the list of items in the track
 */
CReaperTrack::CReaperTrack ( QString name, qint32& iid, QList<STrackItem> items, int frameSize )
{
    QTextStream sOut ( &out );

    sOut << "  <TRACK " << trackId.toString() << '\n';
    sOut << "    NAME " << name << '\n';
    sOut << "    TRACKID " << trackId.toString() << '\n';

    int ino = 1;
    foreach ( auto item, items )
    {
        sOut << CReaperItem ( name + " (" + QString::number ( ino ) + ")", item, iid, frameSize ).toString() << '\n';
        ino++;
        iid++;
    }
    sOut << "  >";

    sOut.flush();
}

/**
 * @brief CReaperProject::CReaperProject Construct a Reaper RPP "<REAPER_PROJECT>" for a given list of tracks
 * @param tracks the list of tracks
 */
CReaperProject::CReaperProject ( QMap<QString, QList<STrackItem>> tracks, int frameSize )
{
    QTextStream sOut ( &out );

    sOut << "<REAPER_PROJECT 0.1 \"5.0\" 1551567848" << '\n'
         << "  RECORD_PATH \"\" \"\"" << '\n'
         << "  SAMPLERATE 48000 0 0" << '\n'
         << "  TEMPO 120 4 4" << '\n';

    qint32 iid = 0;
    foreach ( auto trackName, tracks.keys() )
    {
        sOut << CReaperTrack ( trackName, iid, tracks[trackName], frameSize ).toString() << '\n';
    }

    sOut << ">";

    sOut.flush();
}
