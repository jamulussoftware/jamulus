#include <QObject>
#include "../util.h"

namespace streamer {

class CJamStreamer : public QObject {
    Q_OBJECT

public:
    CJamStreamer();

public slots:
    void process( int iServerFrameSizeSamples, CVector<int16_t> data );

private:
    FILE *pipeout; // pipe for putting out the pcm data to ffmpeg
    int16_t *pcm; // pointer to pcm which will hold the raw pcm data from the server, to be initialised in the constructor
};
}
