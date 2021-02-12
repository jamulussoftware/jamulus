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
        QObject::connect ( qproc, &QProcess::errorOccurred, this, &CJamStreamer::onError );
        QObject::connect ( qproc, QOverload<int, QProcess::ExitStatus>::of( &QProcess::finished ), this, &CJamStreamer::onFinished );
        qproc->setStandardOutputFile ( QProcess::nullDevice() );
    }
    QStringList argumentList = { "-loglevel", "error",
                                 "-y", "-f", "s16le",
                                 "-ar", "48000", "-ac", "2",
                                 "-i", "-" };
    argumentList += strStreamDest.split( QRegExp("\\s+") );
    // Note that program name is also repeated as first argument
    qproc->start ( "ffmpeg", argumentList );
}

void CJamStreamer::onError(QProcess::ProcessError error)
{
    QString errDesc;
    switch (error) {
    case QProcess::FailedToStart:
        errDesc = "failed to start";
        break;
    case QProcess::Crashed:
        errDesc = "crashed";
        break;
    case QProcess::Timedout:
        errDesc = "timed out";
        break;
    case QProcess::WriteError:
        errDesc = "write error";
        break;
    case QProcess::ReadError:
        errDesc = "read error";
        break;
    case QProcess::UnknownError:
        errDesc = "unknown error";
        break;
    default:
        errDesc = "UNKNOWN unknown error";
        break;
    }
    qWarning() << "QProcess Error: " << errDesc << "\n";
}

void CJamStreamer::onFinished( int exitCode, QProcess::ExitStatus exitStatus )
{
    Q_UNUSED ( exitStatus );
    QByteArray stderr = qproc->readAllStandardError ();
    qInfo() << "ffmpeg exited with exitCode" << exitCode << ", stderr:" << stderr;
}

void CJamStreamer::OnStopped() {
    qproc->closeWriteChannel ();
}
#endif
