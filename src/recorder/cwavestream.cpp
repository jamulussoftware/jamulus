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

#include "cwavestream.h"

/******************************************************************************\
* Overrides in global namespace                                                *
\******************************************************************************/

/**
 * @brief operator << Emit a hdr_riff object to a QDataStream
 * @param os a QDataStream
 * @param obj a hdr_riff object
 * @return the QDataStream passed
 */
recorder::CWaveStream& operator<< ( recorder::CWaveStream& os, const recorder::HdrRiff& obj )
{
    (QDataStream&) os << obj.chunkId << obj.chunkSize << obj.format;
    return os;
}

/**
 * @brief operator << Emit a fmtSubChunk object to a QDataStream
 * @param os a QDataStream
 * @param obj a fmtSubChunk object
 * @return the QDataStream passed
 */
recorder::CWaveStream& operator<< ( recorder::CWaveStream& os, const recorder::FmtSubChunk& obj )
{
    (QDataStream&) os << obj.chunkId << obj.chunkSize << obj.audioFormat << obj.numChannels << obj.sampleRate << obj.byteRate << obj.blockAlign
                      << obj.bitsPerSample;
    return os;
}

/**
 * @brief operator << Emit a dataSubChunkHdr object to a QDataStream
 * @param os a QDataStream
 * @param obj a dataSubChunkHdr object
 * @return the QDataStream passed
 */
recorder::CWaveStream& operator<< ( recorder::CWaveStream& os, const recorder::DataSubChunkHdr& obj )
{
    (QDataStream&) os << obj.chunkId << obj.chunkSize;
    return os;
}

/******************************************************************************\
* Implementations of recorder.CWaveStream methods                              *
\******************************************************************************/

using namespace recorder;

CWaveStream::CWaveStream ( const uint16_t numChannels ) :
    QDataStream(),
    numChannels ( numChannels ),
    initialPos ( device()->pos() ),
    initialByteOrder ( byteOrder() )
{
    waveStreamHeaders();
}
CWaveStream::CWaveStream ( QIODevice* iod, const uint16_t numChannels ) :
    QDataStream ( iod ),
    numChannels ( numChannels ),
    initialPos ( device()->pos() ),
    initialByteOrder ( byteOrder() )
{
    waveStreamHeaders();
}
CWaveStream::CWaveStream ( QByteArray* iod, QIODevice::OpenMode flags, const uint16_t numChannels ) :
    QDataStream ( iod, flags ),
    numChannels ( numChannels ),
    initialPos ( device()->pos() ),
    initialByteOrder ( byteOrder() )
{
    waveStreamHeaders();
}
CWaveStream::CWaveStream ( const QByteArray& ba, const uint16_t numChannels ) :
    QDataStream ( ba ),
    numChannels ( numChannels ),
    initialPos ( device()->pos() ),
    initialByteOrder ( byteOrder() )
{
    waveStreamHeaders();
}

void CWaveStream::waveStreamHeaders()
{
    static const HdrRiff         scHdrRiff;
    const FmtSubChunk            cFmtSubChunk ( numChannels );
    static const DataSubChunkHdr scDataSubChunkHdr;

    setByteOrder ( LittleEndian );
    *this << scHdrRiff << cFmtSubChunk << scDataSubChunkHdr;
}

void CWaveStream::finalise()
{
    static const uint64_t hdrRiffChunkSize = sizeof ( uint32_t ) + sizeof ( uint32_t ) + sizeof ( uint32_t );
    static const uint64_t fmtSubChunkSize  = sizeof ( uint32_t ) + sizeof ( uint32_t ) + sizeof ( uint16_t ) + sizeof ( uint16_t ) +
                                            sizeof ( uint32_t ) + sizeof ( uint32_t ) + sizeof ( uint16_t ) + sizeof ( uint16_t );

    static const uint64_t hdrRiffChunkSizeOffset         = sizeof ( uint32_t );
    static const uint64_t dataSubChunkHdrChunkSizeOffset = hdrRiffChunkSize + fmtSubChunkSize + sizeof ( uint32_t );

    const int64_t  currentPos     = this->device()->pos();
    const uint64_t fileLengthRiff = static_cast<uint64_t> ( currentPos - initialPos - ( hdrRiffChunkSizeOffset + sizeof ( uint32_t ) ) );
    const uint64_t fileLengthData = static_cast<uint64_t> ( currentPos - initialPos - ( dataSubChunkHdrChunkSizeOffset + sizeof ( uint32_t ) ) );

    // check if lengths are within the range of the WAV file format
    if ( fileLengthRiff < 0x100000000ULL && fileLengthData < 0x100000000ULL )
    {
        QDataStream& out = static_cast<QDataStream&> ( *this );

        // Overwrite hdr_riff.chunkSize
        this->device()->seek ( initialPos + hdrRiffChunkSizeOffset );
        out << static_cast<uint32_t> ( fileLengthRiff );

        // Overwrite dataSubChunkHdr.chunkSize
        this->device()->seek ( initialPos + dataSubChunkHdrChunkSizeOffset );
        out << static_cast<uint32_t> ( fileLengthData );

        // And restore the position
        this->device()->seek ( currentPos );
    }
    // restore the byte order
    setByteOrder ( initialByteOrder );
}
