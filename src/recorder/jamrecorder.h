/******************************************************************************\
 * Copyright (c) 2020
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

#pragma once

#include <QDir>
#include <QFile>
#include <QDateTime>
#include <QMutex>

#include "../util.h"
#include "../channel.h"

#include "creaperproject.h"
#include "cwavestream.h"

namespace recorder
{

class CJamClientConnection : public QObject
{
    Q_OBJECT

public:
    CJamClientConnection ( const int     _numAudioChannels,
                           const qint64  _startFrame,
                           const qint64  _length,
                           const QString _name,
                           const QString _fileName ) :
        numAudioChannels ( _numAudioChannels ),
        startFrame ( _startFrame ),
        length ( _length ),
        name ( _name ),
        fileName ( _fileName )
    {}

    int     Format() { return numAudioChannels; }
    qint64  StartFrame() { return startFrame; }
    qint64  Length() { return length; }
    QString Name() { return name; }
    QString FileName() { return fileName; }

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
    CJamClient ( const qint64 frame, const int numChannels, const QString name, const CHostAddress address, const QDir recordBaseDir );

    void Frame ( const QString name, const CVector<int16_t>& pcm, int iServerFrameSizeSamples );

    void Disconnect();

    qint64   StartFrame() { return startFrame; }
    qint64   FrameCount() { return frameCount; }
    uint16_t NumAudioChannels() { return numChannels; }
    QString  ClientName()
    {
        return TranslateChars ( name )
            .leftJustified ( 4, '_', false )
            .append ( "-" )
            .append ( TranslateChars ( address.toString ( CHostAddress::EStringMode::SM_IP_NO_LAST_BYTE_PORT ) ) );
    }
    CHostAddress ClientAddress() { return address; }

    QString FileName() { return filename; }

private:
    QString TranslateChars ( const QString& input ) const;

    const qint64       startFrame;
    const uint16_t     numChannels;
    QString            name;
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
    CJamSession ( QDir recordBaseDir );

    virtual ~CJamSession();

    void Frame ( const int              iChID,
                 const QString          name,
                 const CHostAddress     address,
                 const int              numAudioChannels,
                 const CVector<int16_t> data,
                 int                    iServerFrameSizeSamples );

    void End();

    QVector<CJamClient*> Clients() { return vecptrJamClients; }

    QMap<QString, QList<STrackItem>> Tracks();

    QString Name() { return sessionDir.dirName(); }

    const QDir SessionDir() { return sessionDir; }

    void DisconnectClient ( int iChID );

    static QMap<QString, QList<STrackItem>> TracksFromSessionDir ( const QString& name, int iServerFrameSizeSamples );

private:
    CJamSession();

    const QDir sessionDir;

    qint64                       currentFrame;
    int                          chIdDisconnected;
    QVector<CJamClient*>         vecptrJamClients;
    QList<CJamClientConnection*> jamClientConnections;
};

class CJamRecorder : public QObject
{
    Q_OBJECT

public:
    CJamRecorder ( const QString strRecordingBaseDir, const int iServerFrameSizeSamples ) :
        recordBaseDir ( strRecordingBaseDir ),
        iServerFrameSizeSamples ( iServerFrameSizeSamples ),
        isRecording ( false ),
        currentSession ( nullptr )
    {}

    /**
     * @brief Create recording directory, if necessary, and connect signal handlers
     * @param server Server object emitting signals
     */
    QString Init();

    /**
     * @brief SessionDirToReaper Method that allows an RPP file to be recreated
     * @param strSessionDirName Where the session wave files are
     * @param serverFrameSizeSamples What the server frame size was for the session
     */
    static void SessionDirToReaper ( QString& strSessionDirName, int serverFrameSizeSamples );

private:
    void Start();
    void ReaperProjectFromCurrentSession();
    void AudacityLofFromCurrentSession();

    QDir         recordBaseDir;
    int          iServerFrameSizeSamples;
    bool         isRecording;
    CJamSession* currentSession;
    QMutex       ChIdMutex;

signals:
    void RecordingSessionStarted ( QString sessionDir );
    void RecordingFailed ( QString error );

public slots:
    /**
     * @brief Handle last client leaving the server, ends the recording.
     */
    void OnEnd();

    /**
     * @brief Handle request to end one session and start a new one.
     */
    void OnTriggerSession();

    /**
     * @brief Handle application stopping
     */
    void OnAboutToQuit();

    /**
     * @brief Handle an existing client leaving the server.
     * @param iChID channel number of client
     */
    void OnDisconnected ( int iChID );

    /**
     * @brief Handle a frame of data to process
     */
    void OnFrame ( const int iChID, const QString name, const CHostAddress address, const int numAudioChannels, const CVector<int16_t> data );
};

} // namespace recorder
