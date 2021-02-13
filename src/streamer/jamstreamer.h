#ifndef _WIN32
#include <QObject>
#include <QProcess>
#include "../util.h"

namespace streamer {

class CJamStreamer : public QObject {
    Q_OBJECT

public:
    CJamStreamer();
    void Init( const QString strStreamDest );

public slots:
    void process( int iServerFrameSizeSamples, const CVector<int16_t>& data );
    void OnStarted();
    void OnStopped();
private slots:
    void onError(QProcess::ProcessError error);
    void onFinished( int exitCode, QProcess::ExitStatus exitStatus );

private:
    QString strStreamDest; // stream destination to pass to ffmpeg as output part of arguments
    QProcess* qproc; // ffmpeg subprocess
};
}
#endif
