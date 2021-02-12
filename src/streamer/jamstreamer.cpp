#ifndef _WIN32
#include "jamstreamer.h"

using namespace streamer;

CJamStreamer::CJamStreamer() {}

// put the received pcm data into the pipe to ffmpeg
void CJamStreamer::process( int iServerFrameSizeSamples, const CVector<int16_t>& data ) {
    qproc->write ( reinterpret_cast<const char*> (&data[0]), sizeof (int16_t) * ( 2 * iServerFrameSizeSamples ) );
}

void CJamStreamer::Init( const QString strStreamDest ) {
    this->strStreamDest = strStreamDest;
}

// create a pipe to ffmpeg called "pipeout" to being able to put out the pcm data to it
void CJamStreamer::OnStarted() {
    if ( !qproc )
    {
        qproc = new QProcess;
    }
    QStringList argumentList = { "ffmpeg", "-y", "-f", "s16le",
                                 "-ar", "48000", "-ac", "2",
                                 "-i", "-", strStreamDest };
    // Note that program name is also repeated as first argument
    qproc->start ( "ffmpeg", argumentList, QIODevice::WriteOnly );
}

void CJamStreamer::OnStopped() {
    qproc->closeWriteChannel ();
}
#endif
