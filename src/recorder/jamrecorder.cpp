/******************************************************************************\
 * Copyright (c) 2020-2025
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
#include "cwavestream.h"
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>

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
 *
 */
namespace
{
static inline void writeLE16 ( std::ofstream& out, uint16_t v )
{
    char b[2] = { static_cast<char> ( v & 0xFF ), static_cast<char> ( ( v >> 8 ) & 0xFF ) };
    out.write ( b, 2 );
}
static inline void writeLE32 ( std::ofstream& out, uint32_t v )
{
    char b[4] = { static_cast<char> ( v & 0xFF ),
                  static_cast<char> ( ( v >> 8 ) & 0xFF ),
                  static_cast<char> ( ( v >> 16 ) & 0xFF ),
                  static_cast<char> ( ( v >> 24 ) & 0xFF ) };
    out.write ( b, 4 );
}
static void writeWavHeader ( std::ofstream& out, uint16_t numChannels )
{
    const uint32_t sampleRate   = 48000;
    const uint16_t bitsPerSample = 16;
    const uint16_t blockAlign   = static_cast<uint16_t> ( numChannels * bitsPerSample / 8 );
    const uint32_t byteRate     = sampleRate * blockAlign;
    out.write ( "RIFF", 4 );
    writeLE32 ( out, 0 ); // placeholder for RIFF chunk size
    out.write ( "WAVE", 4 );
    out.write ( "fmt ", 4 );
    writeLE32 ( out, 16 );            // PCM fmt chunk size
    writeLE16 ( out, 1 );             // audio format PCM
    writeLE16 ( out, numChannels );   // channels
    writeLE32 ( out, sampleRate );    // sample rate
    writeLE32 ( out, byteRate );      // byte rate
    writeLE16 ( out, blockAlign );    // block align
    writeLE16 ( out, bitsPerSample ); // bits per sample
    out.write ( "data", 4 );
    writeLE32 ( out, 0 ); // placeholder for data chunk size
}
static void finalizeWav ( std::ofstream& out )
{
    const std::streamoff riffChunkSizeOffset = 4;
    const std::streamoff dataChunkSizeOffset = 40;
    const std::streamoff currentPos          = out.tellp();
    if ( currentPos <= 0 )
        return;
    const uint32_t riffSize = static_cast<uint32_t> ( currentPos - ( riffChunkSizeOffset + 4 ) );
    const uint32_t dataSize = static_cast<uint32_t> ( currentPos - ( dataChunkSizeOffset + 4 ) );
    out.seekp ( riffChunkSizeOffset, std::ios::beg );
    writeLE32 ( out, riffSize );
    out.seekp ( dataChunkSizeOffset, std::ios::beg );
    writeLE32 ( out, dataSize );
    out.seekp ( currentPos, std::ios::beg );
}
} // namespace

CJamClient::CJamClient ( const qint64 frame,
                         const int    _numChannels,
                         const QString name,
                         const CHostAddress& address,
                         const QString        recordBaseDirPath ) :
    startFrame ( frame ),
    numChannels ( static_cast<uint16_t> ( _numChannels ) ),
    name ( name ),
    address ( address )
{
    QString fileName = ClientName() + "-" + QString::number ( frame ) + "-" + QString::number ( _numChannels );
    QString affix    = "";
    std::filesystem::path baseDir ( recordBaseDirPath.toStdWString() );
    while ( std::filesystem::exists ( baseDir / std::filesystem::path ( ( fileName + affix + ".wav" ).toStdWString() ) ) )
    {
        affix = affix.isEmpty() ? "_1" : "_" + QString::number ( affix.mid ( 1 ).toInt() + 1 );
    }
    fileName = fileName + affix + ".wav";

    const QString absPath = QString::fromStdWString ( ( baseDir / std::filesystem::path ( fileName.toStdWString() ) ).wstring() );
    wavFile.open ( absPath.toStdWString(), std::ios::binary | std::ios::trunc | std::ios::out );
    if ( !wavFile.is_open() )
    {
        throw CGenErr ( "Could not write to WAV file " + absPath );
    }
    writeWavHeader ( wavFile, numChannels );
    filename = absPath;
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
        const int16_t sample = pcm[i];
        wavFile.write ( reinterpret_cast<const char*> ( &sample ), sizeof ( int16_t ) );
    }

    frameCount++;
}

/**
 * @brief CJamClient::Disconnect Clean up after a disconnected client
 */
void CJamClient::Disconnect()
{
    if ( wavFile.is_open() )
    {
        finalizeWav ( wavFile );
        wavFile.flush();
        wavFile.close();
    }
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
namespace
{
QString MakeUtcTimestampForSession()
{
    const auto now         = std::chrono::system_clock::now();
    const auto nowTimeT    = std::chrono::system_clock::to_time_t ( now );
    const auto msPart      = std::chrono::duration_cast<std::chrono::milliseconds> ( now.time_since_epoch() ) % 1000;
    std::tm     utcTime    = {};
#ifdef _WIN32
    gmtime_s ( &utcTime, &nowTimeT );
#else
    gmtime_r ( &nowTimeT, &utcTime );
#endif

    std::ostringstream oss;
    oss << std::put_time ( &utcTime, "%Y%m%d-%H%M%S" ) << std::setw ( 3 ) << std::setfill ( '0' ) << msPart.count();
    return QString::fromStdString ( oss.str() );
}
}

CJamSession::CJamSession ( QString recordBaseDirPath ) :
    sessionDirPath ( QString::fromStdWString ( ( std::filesystem::path ( recordBaseDirPath.toStdWString() ) /
                                                 std::filesystem::path (
                                                     QString ( "Jam-" + MakeUtcTimestampForSession() ).toStdWString() ) )
                                                   .wstring() ) ),
    currentFrame ( 0 ),
    chIdDisconnected ( -1 ),
    vecptrJamClients ( MAX_NUM_CHANNELS ),
    jamClientConnections()
{
    std::filesystem::path dirPath ( sessionDirPath.toStdWString() );
    std::error_code       ec;
    if ( !std::filesystem::exists ( dirPath ) )
    {
        std::filesystem::create_directories ( dirPath, ec );
        if ( ec )
        {
            throw CGenErr ( sessionDirPath + " does not exist and could not be created" );
        }
    }
    if ( !std::filesystem::is_directory ( dirPath ) )
    {
        throw CGenErr ( sessionDirPath + " exists but is not a directory" );
    }
    {
        const std::filesystem::path tmp = dirPath / L".write_test.tmp";
        std::ofstream               test ( tmp, std::ios::out | std::ios::binary | std::ios::trunc );
        if ( !test.is_open() )
        {
            throw CGenErr ( sessionDirPath + " is a directory but cannot be written to" );
        }
        test.close();
        std::filesystem::remove ( tmp, ec );
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
        vecptrJamClients[iChID] = new CJamClient ( currentFrame, numAudioChannels, name, address, sessionDirPath );
    }
    else if ( numAudioChannels != vecptrJamClients[iChID]->NumAudioChannels() ||
              address.toString() != vecptrJamClients[iChID]->ClientAddress().toString() )
    {
        DisconnectClient ( iChID );
        if ( numAudioChannels == 0 )
        {
            vecptrJamClients[iChID] = nullptr;
        }
        else
        {
            vecptrJamClients[iChID] = new CJamClient ( currentFrame, numAudioChannels, name, address, sessionDirPath );
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

    std::filesystem::path dir ( sessionDirName.toStdWString() );
    if ( std::filesystem::exists ( dir ) && std::filesystem::is_directory ( dir ) )
    {
        for ( const auto& de : std::filesystem::directory_iterator ( dir ) )
        {
            if ( de.is_regular_file() && de.path().extension() == L".pcm" )
            {
                const QString entry = QString::fromStdWString ( de.path().filename().wstring() );

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

                const qint64 length = static_cast<qint64> ( std::filesystem::file_size ( de.path() ) ) / numChannels.toInt() / iServerFrameSizeSamples;
                STrackItem    track ( numChannels.toInt(), frame.toLongLong(), length, QString::fromStdWString ( de.path().wstring() ) );
                tracks[trackName].append ( track );
            }
        }
    }

    return tracks;
}

/* ********************************************************************************************************
 * CJamRecorder
 * ********************************************************************************************************/

/**
 * @brief CJamRecorder::Init Create recording directory, if necessary, and connect signal handlers
 * @param server Server object emitting signals
 * @return QString() on success else the failure reason
 */
QString CJamRecorder::Init()
{
    QString   errmsg = QString();
    std::filesystem::path dirPath ( recordBaseDirPath.toStdWString() );
    std::error_code       ec;
    if ( !std::filesystem::exists ( dirPath ) )
    {
        std::filesystem::create_directories ( dirPath, ec );
        if ( ec )
        {
            errmsg = QString ( "'%1' does not exist but could not be created." ).arg ( recordBaseDirPath );
            qCritical() << qUtf8Printable ( errmsg );
            return errmsg;
        }
    }
    if ( !std::filesystem::is_directory ( dirPath ) )
    {
        errmsg = QString ( "'%1' exists but is not a directory" ).arg ( recordBaseDirPath );
        qCritical() << qUtf8Printable ( errmsg );
        return errmsg;
    }
    {
        const std::filesystem::path tmp = dirPath / L".write_test.tmp";
        std::ofstream               test ( tmp, std::ios::out | std::ios::binary | std::ios::trunc );
        if ( !test.is_open() )
        {
            errmsg = QString ( "'%1' is a directory but cannot be written to" ).arg ( recordBaseDirPath );
            qCritical() << qUtf8Printable ( errmsg );
            return errmsg;
        }
        test.close();
        std::filesystem::remove ( tmp, ec );
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
#if defined( HEADLESS )
        std::lock_guard<std::mutex> mutexLocker ( ChIdMutex );
#elif defined( JAMULUS_USE_JUCE_NET )
        juce::ScopedLock mutexLocker ( ChIdMutex );
#else
        QMutexLocker mutexLocker ( &ChIdMutex );
#endif
        try
        {
            currentSession = new CJamSession ( recordBaseDirPath );
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

    emit RecordingSessionStarted ( currentSession->SessionDirPath() );
}

/**
 * @brief CJamRecorder::OnEnd Finalise the recording and write the Reaper RPP file
 */
void CJamRecorder::OnEnd()
{
#if defined( HEADLESS )
    std::lock_guard<std::mutex> mutexLocker ( ChIdMutex );
#elif defined( JAMULUS_USE_JUCE_NET )
    juce::ScopedLock mutexLocker ( ChIdMutex );
#else
    QMutexLocker mutexLocker ( &ChIdMutex );
#endif
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
    std::filesystem::path rppPath ( std::filesystem::path ( currentSession->SessionDirPath().toStdWString() ) /
                                    std::filesystem::path ( QString ( currentSession->Name() ).append ( ".rpp" ).toStdWString() ) );
    QString reaperProjectFileName = QString::fromStdWString ( rppPath.wstring() );
    if ( std::filesystem::exists ( rppPath ) )
    {
        qWarning() << "CJamRecorder::ReaperProjectFromCurrentSession():" << QString::fromStdWString ( rppPath.parent_path().wstring() )
                   << "exists and will not be overwritten.";
    }
    else
    {
        std::ofstream outf ( rppPath, std::ios::out | std::ios::binary | std::ios::trunc );
        if ( outf.is_open() )
        {
            const QByteArray bytes = CReaperProject ( currentSession->Tracks(), iServerFrameSizeSamples ).toString().toUtf8();
            outf.write ( bytes.constData(), static_cast<std::streamsize> ( bytes.size() ) );
            outf.put ( '\n' );
            qDebug() << "Session RPP:" << reaperProjectFileName;
        }
        else
        {
            qWarning() << "CJamRecorder::ReaperProjectFromCurrentSession():" << QString::fromStdWString ( rppPath.parent_path().wstring() )
                       << "could not be created, no RPP written.";
        }
    }
}

void CJamRecorder::AudacityLofFromCurrentSession()
{
    std::filesystem::path lofPath ( std::filesystem::path ( currentSession->SessionDirPath().toStdWString() ) /
                                    std::filesystem::path ( QString ( currentSession->Name() ).append ( ".lof" ).toStdWString() ) );
    QString audacityLofFileName = QString::fromStdWString ( lofPath.wstring() );
    if ( std::filesystem::exists ( lofPath ) )
    {
        qWarning() << "CJamRecorder::AudacityLofFromCurrentSession():" << QString::fromStdWString ( lofPath.parent_path().wstring() )
                   << "exists and will not be overwritten.";
    }
    else
    {
        std::ofstream outf ( lofPath, std::ios::out | std::ios::binary | std::ios::trunc );
        if ( outf.is_open() )
        {
            auto writeLine = [&outf] ( const QString& s )
            {
                const QByteArray bytes = s.toUtf8();
                outf.write ( bytes.constData(), static_cast<std::streamsize> ( bytes.size() ) );
            };

            foreach ( auto trackName, currentSession->Tracks().keys() )
            {
                foreach ( auto item, currentSession->Tracks()[trackName] )
                {
                    std::filesystem::path itemPath ( item.fileName.toStdWString() );
                    const QString         baseName = QString::fromStdWString ( itemPath.filename().wstring() );
                    writeLine ( QStringLiteral ( "file " ) );
                    writeLine ( QStringLiteral ( "\"" ) + baseName + QStringLiteral ( "\"" ) );
                    writeLine ( QStringLiteral ( " offset " ) + secondsAt48K ( item.startFrame, iServerFrameSizeSamples ) +
                                QStringLiteral ( "\n" ) );
                }
            }

            qDebug() << "Session LOF:" << audacityLofFileName;
        }
        else
        {
            qWarning() << "CJamRecorder::AudacityLofFromCurrentSession():" << QString::fromStdWString ( lofPath.parent_path().wstring() )
                       << "could not be created, no LOF written.";
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
    std::filesystem::path dirPath ( strSessionDirName.toStdWString() );
    if ( !std::filesystem::exists ( dirPath ) || !std::filesystem::is_directory ( dirPath ) )
    {
        throw CGenErr ( QString::fromStdWString ( std::filesystem::absolute ( dirPath ).wstring() ) +
                        " does not exist or is not a directory.  Aborting." );
    }

    const std::filesystem::path dirAbs = std::filesystem::absolute ( dirPath );
    QString                     base   = QString::fromStdWString ( dirAbs.filename().wstring() );
    const std::filesystem::path rpp    = dirAbs / std::filesystem::path ( base.append ( ".rpp" ).toStdWString() );
    if ( std::filesystem::exists ( rpp ) )
    {
        throw CGenErr ( QString::fromStdWString ( rpp.wstring() ) + " exists and will not be overwritten.  Aborting." );
    }

    std::ofstream outf ( rpp, std::ios::out | std::ios::binary | std::ios::trunc );
    if ( !outf.is_open() )
    {
        throw CGenErr ( QString::fromStdWString ( rpp.wstring() ) + " could not be written.  Aborting." );
    }

    {
        const QByteArray bytes =
            CReaperProject ( CJamSession::TracksFromSessionDir ( QString::fromStdWString ( dirAbs.wstring() ), serverFrameSizeSamples ),
                             serverFrameSizeSamples )
                .toString()
                .toUtf8();
        outf.write ( bytes.constData(), static_cast<std::streamsize> ( bytes.size() ) );
        outf.put ( '\n' );
    }

    qDebug() << "Session RPP:" << QString::fromStdWString ( rpp.wstring() );
}

/**
 * @brief CJamRecorder::OnDisconnected Handle disconnection of a client
 * @param iChID the client channel id
 */
void CJamRecorder::OnDisconnected ( int iChID )
{
#if defined( HEADLESS )
    std::lock_guard<std::mutex> mutexLocker ( ChIdMutex );
#elif defined( JAMULUS_USE_JUCE_NET )
    juce::ScopedLock mutexLocker ( ChIdMutex );
#else
    QMutexLocker mutexLocker ( &ChIdMutex );
#endif
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
#if defined( HEADLESS )
        std::lock_guard<std::mutex> mutexLocker ( ChIdMutex );
#elif defined( JAMULUS_USE_JUCE_NET )
        juce::ScopedLock mutexLocker ( ChIdMutex );
#else
        QMutexLocker mutexLocker ( &ChIdMutex );
#endif
        currentSession->Frame ( iChID, name, address, numAudioChannels, data, iServerFrameSizeSamples );
    }
}
