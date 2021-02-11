#ifndef _WIN32
#include "jamstreamer.h"

using namespace streamer;

CJamStreamer::CJamStreamer() {}

// put the received pcm data into the pipe to ffmpeg
void CJamStreamer::process( int iServerFrameSizeSamples, const CVector<int16_t>& data ) {
    fwrite(&data[0], 2, ( 2 * iServerFrameSizeSamples ), pipeout);
}

void CJamStreamer::Init( const QString strStreamDest ) {
    this->strStreamDest = strStreamDest;
}

// create a pipe to ffmpeg called "pipeout" to being able to put out the pcm data to it
void CJamStreamer::OnStarted() {
    QString command = "ffmpeg -y -f s16le -ar 48000 -ac 2 -i - " + strStreamDest;
    pipeout = popen(command.toUtf8().constData(), "w");
}

void CJamStreamer::OnStopped() {
    pclose(pipeout);
}
#endif
