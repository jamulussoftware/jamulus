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

#include "jamrecorder.h"

using namespace recorder;

/* ********************************************************************************************************
 * CJamClient
 * ********************************************************************************************************/

/**
 * @brief CJamClient::CJamClient
 * @param frame Start frame of the client within the session
 * @param numChannels 1 for mono, 2 for stereo
 * @param name The client's current name
 * @param address IP and Port
 * @param recordBaseDir Session recording directory
 *
 * Creates a file for the raw PCM data and sets up a QDataStream to which to write received frames.
 * The data is stored Little Endian.
 */
CJamClient::CJamClient ( const qint64 frame, const int _numChannels, const QString name, const CHostAddress address, const QDir recordBaseDir ) :
    startFrame ( frame ),
    numChannels ( static_cast<uint16_t> ( _numChannels ) ),
    name ( name ),
    address ( address ),
    out ( nullptr )
{
    // At this point we may not have much of a name
    QString fileName = ClientName() + "-" + QString::number ( frame ) + "-" + QString::number ( _numChannels );
    QString affix    = "";
    while ( recordBaseDir.exists ( fileName + affix + ".wav" ) )
    {
        affix = affix.length() == 0 ? "_1" : "_" + QString::number ( affix.remove ( 0, 1 ).toInt() + 1 );
    }
    fileName = fileName + affix + ".wav";

    wavFile = new QFile ( recordBaseDir.absoluteFilePath ( fileName ) );
    if ( !wavFile->open ( QFile::OpenMode ( QIODevice::OpenModeFlag::ReadWrite ) ) ) // need to allow rewriting headers
    {
        throw CGenErr ( "Could not write to WAV file " + wavFile->fileName() );
    }
    out = new CWaveStream ( wavFile, numChannels );

    filename = wavFile->fileName();
}

/**
 * @brief CJamClient::Frame Handle a frame of PCM data from a client connected to the server
 * @param _name The client's current name
 * @param pcm The PCM data
 */
void CJamClient::Frame ( const QString _name, const CVector<int16_t>& pcm, int iServerFrameSizeSamples )
{
    name = _name;

    for ( int i = 0; i < numChannels * iServerFrameSizeSamples; i++ )
    {
        *out << pcm[i];
    }

    frameCount++;
}

/**
 * @brief CJamClient::Disconnect Clean up after a disconnected client
 */
void CJamClient::Disconnect()
{
    if ( out )
    {
        static_cast<CWaveStream*> ( out )->finalise();
        delete out;
        out = nullptr;
    }

    wavFile->close();

    delete wavFile;
    wavFile = nullptr;
}

/**
 * @brief CJamClient::TranslateChars Replace non-ASCII chars with nearest equivalent, if any, and change all punctuation to _
 */
QString CJamClient::TranslateChars ( const QString& input ) const
{
    // Allow letters and numbers
    // clang-format off
    static const char charmap[256] = {
        '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_',
        '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_',
        '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_',
        '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '_', '_', '_', '_', '_', '_',
        '_', 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O',
        'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', '_', '_', '_', '_', '_',
        '_', 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o',
        'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z', '_', '_', '_', '_', '_',
        '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', 'S', '_', 'O', '_', 'Z', '_',
        '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', 's', '_', 'o', '_', 'z', 'Y',
        '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_',
        '_', '_', '2', '3', '_', 'u', '_', '_', '_', '1', '_', '_', '_', '_', '_', '_',
        'A', 'A', 'A', 'A', 'A', 'A', 'A', 'C', 'E', 'E', 'E', 'E', 'I', 'I', 'I', 'I',
        'D', 'N', 'O', 'O', 'O', 'O', 'O', 'x', 'O', 'U', 'U', 'U', 'U', 'Y', 'P', 'S',
        'a', 'a', 'a', 'a', 'a', 'a', 'a', 'c', 'e', 'e', 'e', 'e', 'i', 'i', 'i', 'i',
        'd', 'n', 'o', 'o', 'o', 'o', 'o', '_', 'o', 'u', 'u', 'u', 'u', 'y', 'p', 'y'
    };
    // clang-format on

    QByteArray r = input.toLatin1();

    for ( auto& c : r )
    {
        unsigned char uc = c;
        c                = charmap[uc];
    }

    return QString::fromLatin1 ( r );
}

/* ********************************************************************************************************
 * CJamSession
 * ********************************************************************************************************/

/**
 * @brief CJamSession::CJamSession Construct a new jam recording session
 * @param recordBaseDir The recording base directory
 *
 * Each session is stored into its own subdirectory of the recording base directory.
 */
CJamSession::CJamSession ( QDir recordBaseDir ) :
    sessionDir ( QDir ( recordBaseDir.absoluteFilePath ( "Jam-" + QDateTime().currentDateTimeUtc().toString ( "yyyyMMdd-HHmmsszzz" ) ) ) ),
    currentFrame ( 0 ),
    chIdDisconnected ( -1 ),
    vecptrJamClients ( MAX_NUM_CHANNELS ),
    jamClientConnections()
{
    QFileInfo fi ( sessionDir.absolutePath() );
    fi.setCaching ( false );

    if ( !fi.exists() && !QDir().mkpath ( sessionDir.absolutePath() ) )
    {
        throw CGenErr ( sessionDir.absolutePath() + " does not exist and could not be created" );
    }
    if ( !fi.isDir() )
    {
        throw CGenErr ( sessionDir.absolutePath() + " exists but is not a directory" );
    }
    if ( !fi.isWritable() )
    {
        throw CGenErr ( sessionDir.absolutePath() + " is a directory but cannot be written to" );
    }

    // Explicitly set all the pointers to "empty"
    vecptrJamClients.fill ( nullptr );
}

/**
 * @brief CJamSession::~CJamSession
 */
CJamSession::~CJamSession()
{
    // free up any active jamClientConnections
    for ( int i = 0; i < jamClientConnections.count(); i++ )
    {
        if ( jamClientConnections[i] )
        {
            delete jamClientConnections[i];
            jamClientConnections[i] = nullptr;
        }
    }
}

/**
 * @brief CJamSession::DisconnectClient Capture details of the departing client's connection
 * @param iChID the channel id of the client that disconnected
 */
void CJamSession::DisconnectClient ( int iChID )
{
    vecptrJamClients[iChID]->Disconnect();

    jamClientConnections.append ( new CJamClientConnection ( vecptrJamClients[iChID]->NumAudioChannels(),
                                                             vecptrJamClients[iChID]->StartFrame(),
                                                             vecptrJamClients[iChID]->FrameCount(),
                                                             vecptrJamClients[iChID]->ClientName(),
                                                             vecptrJamClients[iChID]->FileName() ) );

    delete vecptrJamClients[iChID];
    vecptrJamClients[iChID] = nullptr;
    chIdDisconnected        = iChID;
}

/**
 * @brief CJamSession::Frame Process a frame emitted for a client by the server
 * @param iChID the client channel id
 * @param name the client name
 * @param address the client IP and port number
 * @param numAudioChannels the client number of audio channels
 * @param data the frame data
 *
 * Manages changes that affect how the recording is stored - i.e. if the number of audio channels changes, we need a new file.
 * Files are grouped by IP and port number, so if either of those change for a connection, we also start a new file.
 *
 * Also manages the overall current frame counter for the session.
 */
void CJamSession::Frame ( const int              iChID,
                          const QString          name,
                          const CHostAddress     address,
                          const int              numAudioChannels,
                          const CVector<int16_t> data,
                          int                    iServerFrameSizeSamples )
{
    if ( iChID == chIdDisconnected )
    {
        // DisconnectClient has just been called for this channel - this frame is "too late"
        chIdDisconnected = -1;
        return;
    }

    if ( vecptrJamClients[iChID] == nullptr )
    {
        // then we have not seen this client this session
        vecptrJamClients[iChID] = new CJamClient ( currentFrame, numAudioChannels, name, address, sessionDir );
    }
    else if ( numAudioChannels != vecptrJamClients[iChID]->NumAudioChannels() ||
              address.InetAddr != vecptrJamClients[iChID]->ClientAddress().InetAddr ||
              address.iPort != vecptrJamClients[iChID]->ClientAddress().iPort )
    {
        DisconnectClient ( iChID );
        if ( numAudioChannels == 0 )
        {
            vecptrJamClients[iChID] = nullptr;
        }
        else
        {
            vecptrJamClients[iChID] = new CJamClient ( currentFrame, numAudioChannels, name, address, sessionDir );
        }
    }

    if ( vecptrJamClients[iChID] == nullptr )
    {
        // Frame allegedly from iChID but unable to establish client details
        return;
    }

    vecptrJamClients[iChID]->Frame ( name, data, iServerFrameSizeSamples );

    // If _any_ connected client frame steps past currentFrame, increase currentFrame
    if ( vecptrJamClients[iChID]->StartFrame() + vecptrJamClients[iChID]->FrameCount() > currentFrame )
    {
        currentFrame++;
    }
}

/**
 * @brief CJamSession::End Clean up any "hanging" clients when the server thinks they all left
 */
void CJamSession::End()
{
    for ( int iChID = 0; iChID < vecptrJamClients.size(); iChID++ )
    {
        if ( vecptrJamClients[iChID] != nullptr )
        {
            DisconnectClient ( iChID );
            vecptrJamClients[iChID] = nullptr;
        }
    }
}

/**
 * @brief CJamSession::Tracks Retrieve a map of (latest) client name to connection items
 * @return a map of (latest) client name to connection items
 */
QMap<QString, QList<STrackItem>> CJamSession::Tracks()
{
    QMap<QString, QList<STrackItem>> tracks;

    for ( int i = 0; i < jamClientConnections.count(); i++ )
    {
        STrackItem track ( jamClientConnections[i]->Format(),
                           jamClientConnections[i]->StartFrame(),
                           jamClientConnections[i]->Length(),
                           jamClientConnections[i]->FileName() );

        if ( !tracks.contains ( jamClientConnections[i]->Name() ) )
        {
            tracks.insert ( jamClientConnections[i]->Name(), {} );
        }

        tracks[jamClientConnections[i]->Name()].append ( track );
    }

    return tracks;
}

/**
 * @brief CJamSession::TracksFromSessionDir Replica of CJamSession::Tracks but using the directory contents to construct the track item map
 * @param sessionDirName the directory name to scan
 * @return a map of (latest) client name to connection items
 */
QMap<QString, QList<STrackItem>> CJamSession::TracksFromSessionDir ( const QString& sessionDirName, int iServerFrameSizeSamples )
{
    QMap<QString, QList<STrackItem>> tracks;

    const QDir sessionDir ( sessionDirName );
    foreach ( auto entry, sessionDir.entryList ( { "*.pcm" } ) )
    {

        auto    split       = entry.split ( "." )[0].split ( "-" );
        QString name        = split[0];
        QString hostPort    = split[1];
        QString frame       = split[2];
        QString tail        = split[3]; // numChannels may have _nnn
        QString numChannels = tail.count ( "_" ) > 0 ? tail.split ( "_" )[0] : tail;

        QString trackName = name + "-" + hostPort;
        if ( !tracks.contains ( trackName ) )
        {
            tracks.insert ( trackName, {} );
        }

        QFileInfo fiEntry ( sessionDir.absoluteFilePath ( entry ) );
        qint64    length = fiEntry.size() / numChannels.toInt() / iServerFrameSizeSamples;

        STrackItem track ( numChannels.toInt(), frame.toLongLong(), length, sessionDir.absoluteFilePath ( entry ) );

        tracks[trackName].append ( track );
    }

    return tracks;
}

/* ********************************************************************************************************
 * CJamRecorder
 * ********************************************************************************************************/

/**
 * @brief CJamRecorder::Init Create recording directory, if necessary, and connect signal handlers
 * @param server Server object emitting signals
 * @return QString::null on success else the failure reason
 */
QString CJamRecorder::Init()
{
    QString   errmsg = QString::null;
    QFileInfo fi ( recordBaseDir.absolutePath() );
    fi.setCaching ( false );

    if ( !fi.exists() && !QDir().mkpath ( recordBaseDir.absolutePath() ) )
    {
        errmsg = QString ( "'%1' does not exist but could not be created." ).arg ( recordBaseDir.absolutePath() );
        qCritical() << qUtf8Printable ( errmsg );
        return errmsg;
    }
    if ( !fi.isDir() )
    {
        errmsg = QString ( "'%1' exists but is not a directory" ).arg ( recordBaseDir.absolutePath() );
        qCritical() << qUtf8Printable ( errmsg );
        return errmsg;
    }
    if ( !fi.isWritable() )
    {
        errmsg = QString ( "'%1' is a directory but cannot be written to" ).arg ( recordBaseDir.absolutePath() );
        qCritical() << qUtf8Printable ( errmsg );
        return errmsg;
    }

    return errmsg;
}

/**
 * @brief CJamRecorder::Start Start up tasks for a new session
 */
void CJamRecorder::Start()
{
    // Ensure any previous cleaning up has been done.
    OnEnd();

    QString error;

    {
        // needs to be after OnEnd() as that also locks
        QMutexLocker mutexLocker ( &ChIdMutex );
        try
        {
            currentSession = new CJamSession ( recordBaseDir );
            isRecording    = true;
        }
        catch ( const CGenErr& err )
        {
            currentSession = nullptr;
            error          = err.GetErrorText();
        }
    }

    if ( !currentSession )
    {
        emit RecordingFailed ( error );
        return;
    }

    emit RecordingSessionStarted ( currentSession->SessionDir().path() );
}

/**
 * @brief CJamRecorder::OnEnd Finalise the recording and write the Reaper RPP file
 */
void CJamRecorder::OnEnd()
{
    QMutexLocker mutexLocker ( &ChIdMutex );
    if ( isRecording )
    {
        isRecording = false;
        currentSession->End();

        ReaperProjectFromCurrentSession();
        AudacityLofFromCurrentSession();

        delete currentSession;
        currentSession = nullptr;
    }
}

/**
 * @brief CJamRecorder::OnTriggerSession End one session and start a new one
 */
void CJamRecorder::OnTriggerSession()
{
    // This should magically get everything right...
    if ( isRecording )
    {
        Start();
    }
}

/**
 * @brief CJamRecorder::OnAboutToQuit End any recording and exit thread
 */
void CJamRecorder::OnAboutToQuit()
{
    OnEnd();

    QThread::currentThread()->exit();
}

void CJamRecorder::ReaperProjectFromCurrentSession()
{
    QString         reaperProjectFileName = currentSession->SessionDir().filePath ( currentSession->Name().append ( ".rpp" ) );
    const QFileInfo fi ( reaperProjectFileName );

    if ( fi.exists() )
    {
        qWarning() << "CJamRecorder::ReaperProjectFromCurrentSession():" << fi.absolutePath() << "exists and will not be overwritten.";
    }
    else
    {
        QFile outf ( reaperProjectFileName );
        if ( outf.open ( QFile::WriteOnly ) )
        {
            QTextStream out ( &outf );
            out << CReaperProject ( currentSession->Tracks(), iServerFrameSizeSamples ).toString() << endl;
            qDebug() << "Session RPP:" << reaperProjectFileName;
        }
        else
        {
            qWarning() << "CJamRecorder::ReaperProjectFromCurrentSession():" << fi.absolutePath() << "could not be created, no RPP written.";
        }
    }
}

void CJamRecorder::AudacityLofFromCurrentSession()
{
    QString         audacityLofFileName = currentSession->SessionDir().filePath ( currentSession->Name().append ( ".lof" ) );
    const QFileInfo fi ( audacityLofFileName );

    if ( fi.exists() )
    {
        qWarning() << "CJamRecorder::AudacityLofFromCurrentSession():" << fi.absolutePath() << "exists and will not be overwritten.";
    }
    else
    {
        QFile outf ( audacityLofFileName );
        if ( outf.open ( QFile::WriteOnly ) )
        {
            QTextStream sOut ( &outf );

            foreach ( auto trackName, currentSession->Tracks().keys() )
            {
                foreach ( auto item, currentSession->Tracks()[trackName] )
                {
                    QFileInfo fi ( item.fileName );
                    sOut << "file " << '"' << fi.fileName() << '"';
                    sOut << " offset " << secondsAt48K ( item.startFrame, iServerFrameSizeSamples ) << endl;
                }
            }

            sOut.flush();
            qDebug() << "Session LOF:" << audacityLofFileName;
        }
        else
        {
            qWarning() << "CJamRecorder::AudacityLofFromCurrentSession():" << fi.absolutePath() << "could not be created, no LOF written.";
        }
    }
}

/**
 * @brief CJamRecorder::SessionDirToReaper Replica of CJamRecorder::OnEnd() but using the directory contents to construct the CReaperProject object
 * @param strSessionDirName
 *
 * This is used for testing and is not called from the regular Jamulus code.
 */
void CJamRecorder::SessionDirToReaper ( QString& strSessionDirName, int serverFrameSizeSamples )
{
    const QFileInfo fiSessionDir ( QDir::cleanPath ( strSessionDirName ) );
    if ( !fiSessionDir.exists() || !fiSessionDir.isDir() )
    {
        throw CGenErr ( fiSessionDir.absoluteFilePath() + " does not exist or is not a directory.  Aborting." );
    }

    const QDir      dSessionDir ( fiSessionDir.absoluteFilePath() );
    const QString   reaperProjectFileName = dSessionDir.absoluteFilePath ( fiSessionDir.baseName().append ( ".rpp" ) );
    const QFileInfo fiRPP ( reaperProjectFileName );
    if ( fiRPP.exists() )
    {
        throw CGenErr ( fiRPP.absoluteFilePath() + " exists and will not be overwritten.  Aborting." );
    }

    QFile outf ( fiRPP.absoluteFilePath() );
    if ( !outf.open ( QFile::WriteOnly ) )
    {
        throw CGenErr ( fiRPP.absoluteFilePath() + " could not be written.  Aborting." );
    }
    QTextStream out ( &outf );

    out << CReaperProject ( CJamSession::TracksFromSessionDir ( fiSessionDir.absoluteFilePath(), serverFrameSizeSamples ), serverFrameSizeSamples )
               .toString()
        << endl;

    qDebug() << "Session RPP:" << reaperProjectFileName;
}

/**
 * @brief CJamRecorder::OnDisconnected Handle disconnection of a client
 * @param iChID the client channel id
 */
void CJamRecorder::OnDisconnected ( int iChID )
{
    QMutexLocker mutexLocker ( &ChIdMutex );
    if ( !isRecording )
    {
        qWarning() << "CJamRecorder::OnDisconnected: channel" << iChID << "disconnected but not recording";
    }
    if ( currentSession == nullptr )
    {
        qWarning() << "CJamRecorder::OnDisconnected: channel" << iChID << "disconnected but no currentSession";
        return;
    }

    currentSession->DisconnectClient ( iChID );
}

/**
 * @brief CJamRecorder::OnFrame Handle a frame emitted for a client by the server
 * @param iChID the client channel id
 * @param name the client name
 * @param address the client IP and port number
 * @param numAudioChannels the client number of audio channels
 * @param data the frame data
 *
 * Ensures recording has started.
 */
void CJamRecorder::OnFrame ( const int              iChID,
                             const QString          name,
                             const CHostAddress     address,
                             const int              numAudioChannels,
                             const CVector<int16_t> data )
{
    // Make sure we are ready
    if ( !isRecording )
    {
        Start();
    }

    // Start() may have failed, so check again:
    if ( !isRecording )
    {
        return;
    }

    // needs to be after Start() as that also locks
    {
        QMutexLocker mutexLocker ( &ChIdMutex );
        currentSession->Frame ( iChID, name, address, numAudioChannels, data, iServerFrameSizeSamples );
    }
}
