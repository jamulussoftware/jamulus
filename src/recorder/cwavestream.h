/******************************************************************************\
 * Copyright (c) 2020-2026
 *
 * Author(s):
 *  pljones
 *
 * As of Jamulus 3.12.1dev (commit eb172d47): All new source code contributions must be licensed
 * under AGPL 3.0 or any later version.
 *
 * Existing code: Code contributed before 3.12.1dev (commit eb172d47) was licensed under GPL 2.0+.
 * This code will be licensed under GPL 3.0 (or any later version) from
 * 3.12.1dev (commit eb172d47).  When distributed as part of Jamulus, the AGPL 3.0 terms govern
 * the combined work, including network use provisions.
 *
 ******************************************************************************
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 * ---------------------------------------------------------------------------
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
\******************************************************************************/

#pragma once

#include <QDataStream>
#include <QString>
#include <QIODevice>

namespace recorder
{

inline QString secondsAt48K ( const qint64 frames, const int frameSize )
{
    return QString::number ( static_cast<double> ( frames * frameSize ) / 48000, 'f', 14 );
}

struct STrackItem
{
    STrackItem ( int numAudioChannels, qint64 startFrame, qint64 frameCount, QString fileName ) :
        numAudioChannels ( numAudioChannels ),
        startFrame ( startFrame ),
        frameCount ( frameCount ),
        fileName ( fileName )
    {}

    int     numAudioChannels;
    qint64  startFrame;
    qint64  frameCount;
    QString fileName;
};

class HdrRiff
{
public:
    HdrRiff() {}

    static const uint32_t chunkId   = 0x46464952; // RIFF
    static const uint32_t chunkSize = 0x00000000; // unknown
    static const uint32_t format    = 0x45564157; // WAVE
};

class FmtSubChunk
{
public:
    FmtSubChunk ( const uint16_t _numChannels ) :
        numChannels ( _numChannels ),
        byteRate ( sampleRate * numChannels * bitsPerSample / 8 ),
        blockAlign ( numChannels * bitsPerSample / 8 )
    {}
    static const uint32_t chunkId     = 0x20746d66; // "fmt "
    static const uint32_t chunkSize   = 16;         // bytes in fmtSubChunk after chunkSize
    static const uint16_t audioFormat = 1;          // PCM
    const uint16_t        numChannels;              // 1 for mono, 2 for joy... uh, stereo
    static const uint32_t sampleRate = 48000;       // because it's Jamulus
    const uint32_t        byteRate;                 // sampleRate * numChannels * bitsPerSample/8
    const uint16_t        blockAlign;               // numChannels * bitsPerSample/8
    static const uint16_t bitsPerSample = 16;
};

class DataSubChunkHdr
{
public:
    DataSubChunkHdr() {}

    static const uint32_t chunkId   = 0x61746164; // "data"
    static const uint32_t chunkSize = 0x7ffff000; // magic for unspecified length
};

class CWaveStream : public QDataStream
{
public:
    CWaveStream ( const uint16_t numChannels );
    explicit CWaveStream ( QIODevice* iod, const uint16_t numChannels );
    CWaveStream ( QByteArray* iod, QIODevice::OpenMode flags, const uint16_t numChannels );
    CWaveStream ( const QByteArray& ba, const uint16_t numChannels );

    void finalise();

private:
    void waveStreamHeaders();

    const uint16_t  numChannels;
    const int64_t   initialPos;
    const ByteOrder initialByteOrder;
};

} // namespace recorder

recorder::CWaveStream& operator<< ( recorder::CWaveStream& out, recorder::HdrRiff& hdrRiff );
recorder::CWaveStream& operator<< ( recorder::CWaveStream& out, recorder::FmtSubChunk& fmtSubChunk );
recorder::CWaveStream& operator<< ( recorder::CWaveStream& out, recorder::DataSubChunkHdr& dataSubChunkHdr );
