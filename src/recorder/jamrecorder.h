/******************************************************************************\
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
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
\******************************************************************************/

#pragma once

#include <QDir>
#include <QFile>
#include <QDateTime>

#include "../util.h"
#include "../channel.h"

#include "creaperproject.h"
#include "cwavestream.h"

namespace recorder {

class CJamClientConnection : public QObject
{
    Q_OBJECT

public:
    CJamClientConnection(const int _numAudioChannels, const qint64 _startFrame, const qint64 _length, const QString _name, const QString _fileName) :
        numAudioChannels (_numAudioChannels),
        startFrame (_startFrame),
        length (_length),
        name (_name),
        fileName (_fileName)
    {
    }

    int     Format()      { return numAudioChannels; }
    qint64  StartFrame()  { return startFrame; }
    qint64  Length()      { return length; }
    QString Name()        { return name; }
    QString FileName()    { return fileName; }

private:
    const int     numAudioChannels;
    const qint64  startFrame;
    const qint64  length;
    const QString name;
    const QString fileName;
};

class CJamClient : public QObject
{
    Q_OBJECT

public:
    CJamClient(const qint64 frame, const int numChannels, const QString name, const CHostAddress address, const QDir recordBaseDir);

    void Frame(const QString name, const CVector<int16_t>& pcm, int iServerFrameSizeSamples);

    void Disconnect();

    qint64       StartFrame()       { return startFrame; }
    qint64       FrameCount()       { return frameCount; }
    uint16_t     NumAudioChannels() { return numChannels; }
    QString      ClientName()       { return name.leftJustified(4, '_', false).replace(QRegExp("[-.:/\\ ]"), "_")
                                                .append("-")
                                                .append(address.toString(CHostAddress::EStringMode::SM_IP_NO_LAST_BYTE_PORT).replace(QRegExp("[-.:/\\ ]"), "_"))
                                             ;
                                    }
    CHostAddress ClientAddress()    { return address; }

    QString      FileName()         { return filename; }

private:
    const qint64       startFrame;
    const uint16_t     numChannels;
          QString      name;
    const CHostAddress address;

          QString      filename;
          QFile*       wavFile;
          QDataStream* out;
          qint64       frameCount = 0;
};

class CJamSession : public QObject
{
    Q_OBJECT

public:

    CJamSession(QDir recordBaseDir);

    void Frame(const int iChID, const QString name, const CHostAddress address, const int numAudioChannels, const CVector<int16_t> data, int iServerFrameSizeSamples);

    void End();

    QVector<CJamClient*> Clients() { return vecptrJamClients; }

    QMap<QString, QList<STrackItem>> Tracks();

    QString Name() { return sessionDir.dirName(); }

    const QDir SessionDir() { return sessionDir; }

    void DisconnectClient(int iChID);

    static QMap<QString, QList<STrackItem>> TracksFromSessionDir(const QString& name, int iServerFrameSizeSamples);

private:
    CJamSession();

    const QDir sessionDir;

    qint64 currentFrame;
    QVector<CJamClient*> vecptrJamClients;
    QList<CJamClientConnection*> jamClientConnections;
};

class CJamRecorder : public QThread
{
    Q_OBJECT

public:
    CJamRecorder(const QString recordingDirName) :
        recordBaseDir (recordingDirName), isRecording (false) {}

    void Init( const CServer* server, const int _iServerFrameSizeSamples );

    static void SessionDirToReaper( QString& strSessionDirName, int serverFrameSizeSamples );

public slots:
    /**
     * @brief Raised when first client joins the server, triggering a new recording.
     */
    void OnStart();

    /**
     * @brief Raised when last client leaves the server, ending the recording.
     */
    void OnEnd();

    /**
     * @brief Raised when an existing client leaves the server.
     * @param iChID channel number of client
     */
    void OnDisconnected(int iChID);

    /**
     * @brief Raised when a frame of data fis available to process
     */
    void OnFrame(const int iChID, const QString name, const CHostAddress address, const int numAudioChannels, const CVector<int16_t> data);

private:
    QDir recordBaseDir;

    bool         isRecording;
    CJamSession* currentSession;
    int          iServerFrameSizeSamples;
};

}

Q_DECLARE_METATYPE(int16_t)
Q_DECLARE_METATYPE(CVector<int16_t>)
