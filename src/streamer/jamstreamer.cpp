#include "jamstreamer.h"

using namespace streamer;

// --- CONSTRUCTOR ---
CJamStreamer::CJamStreamer() {
    // create a pipe to ffmpeg called "pipeout" to being able to put out the pcm data to it
    pcm = 0; // it is necessary to initialise pcm
    pipeout = popen("ffmpeg -y -f s16le -ar 48000 -ac 2 -i - /tmp/jamstream.wav", "w");
}

// --- PROCESS ---
// put the received pcm data into the pipe to ffmpeg
void CJamStreamer::process( int iServerFrameSizeSamples, CVector<int16_t> data ) {
    if (pcm == 0) // thanks to hselasky
    {
        pcm = new int16_t [2 * iServerFrameSizeSamples];
    }
    for ( int i = 0; i < ( 2 * iServerFrameSizeSamples ); i++ )
    {
        pcm[i] = data[i];
    }
    fwrite(pcm, 2, ( 2 * iServerFrameSizeSamples ), pipeout);
}
