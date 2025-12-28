#include "jamulus_wrapper.h"
#include <QString>
#include "../libjamulus/../src/client.h"
#include "../libjamulus/../src/settings.h"
#include "../libjamulus/../src/clientdlg.h"
#include <QApplication>
#include <QLibraryInfo>
#include <QTimer>
#include <QWindow>
#include <new>
#include "audio_fifo.h"
#include <memory>
#include <thread>
#include <atomic>
#include <chrono>
#include <deque>
#include <vector>
#include <set>
#include <map>

#ifdef _WIN32
#    include <windows.h>

// Get the directory containing the current DLL (jamulus_wrapper or the plugin)
static QString getPluginDirectory()
{
    HMODULE hModule = nullptr;
    // Get handle to this DLL by using the address of this function
    GetModuleHandleExW ( GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                         reinterpret_cast<LPCWSTR> ( &getPluginDirectory ),
                         &hModule );

    if ( hModule )
    {
        wchar_t path[MAX_PATH];
        if ( GetModuleFileNameW ( hModule, path, MAX_PATH ) > 0 )
        {
            QString fullPath  = QString::fromWCharArray ( path );
            int     lastSlash = fullPath.lastIndexOf ( '\\' );
            if ( lastSlash > 0 )
            {
                return fullPath.left ( lastSlash );
            }
        }
    }
    return QString();
}

// Set up Qt library paths before QApplication is created
static void setupQtPaths()
{
    QString pluginDir = getPluginDirectory();
    if ( !pluginDir.isEmpty() )
    {
        // Tell Qt to look for plugins in the same directory as this DLL
        qputenv ( "QT_PLUGIN_PATH", pluginDir.toUtf8() );
    }
}
#else
static void setupQtPaths() {}
#endif

// Global Qt application instance and event pump timer
static QApplication* g_qtApp          = nullptr;
static QTimer*       g_eventPumpTimer = nullptr;

// Global event filter that intercepts Close and other problematic events on ALL widgets
// to prevent dialogs from triggering logic that could crash the plugin.
class GlobalEventFilter : public QObject
{
public:
    explicit GlobalEventFilter ( QObject* parent = nullptr ) : QObject ( parent ) {}

    // Set the main embedded window to protect from focus events
    void setEmbeddedWindow ( QWidget* w ) { embeddedWindow = w; }

protected:
    bool eventFilter ( QObject* watched, QEvent* event ) override
    {
        QWidget* w = qobject_cast<QWidget*> ( watched );

        // Intercept close events on the MAIN embedded window only - convert to hide
        if ( event->type() == QEvent::Close )
        {
            if ( w && w == embeddedWindow )
            {
                w->hide();
                event->ignore();
                return true;
            }
            // Let other dialogs (Settings, Chat, etc.) close normally - they just hide themselves
        }

        // Only block focus/activation events on the EMBEDDED window (not popup dialogs)
        if ( w && w == embeddedWindow )
        {
            switch ( event->type() )
            {
            case QEvent::WindowActivate:
            case QEvent::WindowDeactivate:
            case QEvent::FocusIn:
            case QEvent::FocusOut:
            case QEvent::FocusAboutToChange:
            case QEvent::ActivationChange:
                return true;
            default:
                break;
            }
        }

        return QObject::eventFilter ( watched, event );
    }

private:
    QWidget* embeddedWindow = nullptr;
};

// Global event filter instance
static GlobalEventFilter* g_globalFilter = nullptr;

// Global GUI instances - forward declare
static CClientSettings* g_settings  = nullptr;
static CClientDlg*      g_clientDlg = nullptr;

// Re-entrancy guard for Qt initialization
static bool g_qtAppInitializing = false;

// Ensure QApplication exists and event processing is set up
static void ensureQtApp()
{
    // Guard against re-entrant calls during Qt initialization
    if ( g_qtAppInitializing )
    {
        return;
    }

    static bool s_pathsSetup = false;
    if ( !s_pathsSetup )
    {
        setupQtPaths();
        s_pathsSetup = true;
    }

    if ( !QApplication::instance() )
    {
        g_qtAppInitializing = true;

        static int   fakeArgc   = 1;
        static char  fakeArg0[] = "jamulus_vst3";
        static char* fakeArgv[] = { fakeArg0, nullptr };
        g_qtApp                 = new QApplication ( fakeArgc, fakeArgv );

        // Install global event filter to intercept close events on all windows
        g_globalFilter = new GlobalEventFilter ( g_qtApp );
        g_qtApp->installEventFilter ( g_globalFilter );

        // Create a timer to pump Qt events since we're not running exec()
        g_eventPumpTimer = new QTimer();
        QObject::connect ( g_eventPumpTimer, &QTimer::timeout, []() { QApplication::processEvents ( QEventLoop::AllEvents, 10 ); } );
        g_eventPumpTimer->start ( 16 ); // ~60fps event processing

        g_qtAppInitializing = false;
    }
}

// ============================================================================
// Internal Storage for GUI
// ============================================================================

// Channel info cache
static CVector<CChannelInfo> s_channelList;
static QMutex                s_channelMutex;

// Channel levels cache (updated via CLChannelLevelListReceived signal)
static std::vector<uint16_t> s_channelLevels;
static QMutex                s_channelLevelsMutex;

// Ping time storage
static int s_lastPingTime = -1;
static int s_myChannelId  = -1;

// Chat message queue
static std::deque<std::string> s_chatQueue;
static QMutex                  s_chatMutex;
static char                    s_chatBuffer[4096] = { 0 };

// Server list cache
static std::vector<jamulus_server_info_t> s_serverList;
static QMutex                             s_serverListMutex;

// WrapperClient: exposes a bridge to call CClient::ProcessSndCrdAudioData
// from the plugin/audio thread by converting to/from the Jamulus internal
// integer buffer format.
//
// Key insight: Jamulus ProcessSndCrdAudioData expects buffers of exactly
// iStereoBlockSizeSam samples (256 stereo samples = 128 mono * 2 for default mode).
// DAWs may send arbitrary buffer sizes (512, 1024, etc.), so we need internal
// ring buffers to batch the audio into the correct chunk sizes.
class WrapperClient : public CClient
{
public:
    // Jamulus internal block size: 128 mono samples * 2 channels = 256 stereo samples
    static constexpr int JAMULUS_STEREO_BLOCK_SIZE = 256;

    WrapperClient ( const quint16  iPortNumber,
                    const quint16  iQosNumber,
                    const QString& strConnOnStartupAddress,
                    const QString& strMIDISetup,
                    const bool     bNoAutoJackConnect,
                    const QString& strNClientName,
                    const bool     bNEnableIPv6,
                    const bool     bNMuteMeInPersonalMix ) :
        CClient ( iPortNumber,
                  iQosNumber,
                  strConnOnStartupAddress,
                  strMIDISetup,
                  bNoAutoJackConnect,
                  strNClientName,
                  bNEnableIPv6,
                  bNMuteMeInPersonalMix ),
        inFifo ( nullptr ),
        outFifo ( nullptr ),
        workerRunning ( false )
    {
        // default FIFO capacity: 2 seconds @ 48k -> 96000 frames
        const size_t defaultCapacity = 48000 * 2;
        const int    channels        = 2;
        inFifo                       = std::make_unique<AudioFifo> ( defaultCapacity, channels );
        outFifo                      = std::make_unique<AudioFifo> ( defaultCapacity, channels );

        // Initialize internal ring buffers for block processing
        // Size for ~200ms of audio at 48kHz stereo = 9600 samples
        // Must be multiple of JAMULUS_STEREO_BLOCK_SIZE (256) to maintain channel alignment across wraps
        // 9600 / 256 = 37.5 (BAD!)
        // 9728 / 256 = 38 (GOOD) -> Use 9984 (39 * 256) for margin
        internalInputBuffer.resize ( 9984, 0 );
        internalOutputBuffer.resize ( 9984, 0 );
        inputWritePos  = 0;
        inputReadPos   = 0;
        outputWritePos = 0;
        outputReadPos  = 0;

        // Pre-fill output buffer with one block of silence to avoid initial underrun
        outputWritePos = JAMULUS_STEREO_BLOCK_SIZE;

        outPeakL.store ( 0.0f );
        outPeakR.store ( 0.0f );
    }

    // Process interleaved float audio (frames x channels) in-place: input -> output
    // Returns 0 on success.
    int ProcessInterleavedFloat ( const float* in, float* out, int frames, int channels )
    {
        if ( !in || !out || frames <= 0 || channels <= 0 )
            return -1;

        const int totalSamples = frames * channels;

        // Add input to our internal ring buffer
        for ( int i = 0; i < totalSamples; ++i )
        {
            internalInputBuffer[inputWritePos % internalInputBuffer.size()] = in[i];
            inputWritePos++;
        }

        // Calculate how many samples are available in input buffer
        size_t inputAvailable = inputWritePos - inputReadPos;

        // Process complete blocks
        while ( inputAvailable >= JAMULUS_STEREO_BLOCK_SIZE )
        {
            // Get one block from input buffer
            CVector<int16_t> vec;
            vec.Init ( JAMULUS_STEREO_BLOCK_SIZE );

            for ( int i = 0; i < JAMULUS_STEREO_BLOCK_SIZE; ++i )
            {
                float v = internalInputBuffer[( inputReadPos + i ) % internalInputBuffer.size()];
                if ( v > 1.0f )
                    v = 1.0f;
                if ( v < -1.0f )
                    v = -1.0f;
                vec[i] = static_cast<int16_t> ( v * 32767.0f );
            }
            inputReadPos += JAMULUS_STEREO_BLOCK_SIZE;

            // Process through Jamulus
            ProcessSndCrdAudioData ( vec );

            // Write processed block to output buffer
            for ( int i = 0; i < JAMULUS_STEREO_BLOCK_SIZE; ++i )
            {
                float sample                                                       = static_cast<float> ( vec[i] ) / 32767.0f;
                internalOutputBuffer[outputWritePos % internalOutputBuffer.size()] = sample;
                outputWritePos++;
            }

            inputAvailable = inputWritePos - inputReadPos;
        }

        // Read from output buffer
        float curPeakL = 0.0f;
        float curPeakR = 0.0f;

        for ( int i = 0; i < totalSamples; ++i )
        {
            float s = 0.0f;
            if ( outputReadPos < outputWritePos )
            {
                s = internalOutputBuffer[outputReadPos % internalOutputBuffer.size()];
                outputReadPos++;
            }

            out[i] = s;

            // Track peaks for output meter
            float absS = std::abs ( s );
            if ( i % 2 == 0 ) // Left
            {
                if ( absS > curPeakL )
                    curPeakL = absS;
            }
            else // Right
            {
                if ( absS > curPeakR )
                    curPeakR = absS;
            }
        }

        // Apply decay to atomic peaks (simple envelope follower)
        float decay = 0.95f; // Adjust for responsiveness
        float prevL = outPeakL.load();
        float prevR = outPeakR.load();
        outPeakL.store ( std::max ( curPeakL, prevL * decay ) );
        outPeakR.store ( std::max ( curPeakR, prevR * decay ) );

        return 0;
    }

    // Start internal processing worker
    void StartWorker ( int processBlockFrames = 128 )
    {
        if ( workerRunning.load() )
            return;
        workerRunning.store ( true );
        blockFrames = processBlockFrames;
        worker      = std::thread ( [this]() {
            std::vector<float> inbuf;
            std::vector<float> outbuf;
            inbuf.resize ( blockFrames * 2 );
            outbuf.resize ( blockFrames * 2 );
            while ( workerRunning.load() )
            {
                size_t got = inFifo->pop ( inbuf.data(), blockFrames );
                if ( got == 0 )
                {
                    std::this_thread::sleep_for ( std::chrono::milliseconds ( 1 ) );
                    continue;
                }
                // process
                ProcessInterleavedFloat ( inbuf.data(), outbuf.data(), static_cast<int> ( got ), 2 );
                // push to outFifo
                outFifo->push ( outbuf.data(), got );
            }
        } );
    }

    void StopWorker()
    {
        if ( !workerRunning.load() )
            return;
        workerRunning.store ( false );
        if ( worker.joinable() )
            worker.join();
    }

    // Overrides to trick Jamulus into thinking it's running even without a sound device
    void Start() override
    {
        Init();
        ClearClientChannels();
        Channel.SetEnable ( true );
        workerRunning.store ( true );
#if defined( Q_OS_WINDOWS )
        SetThreadExecutionState ( ES_CONTINUOUS | ES_SYSTEM_REQUIRED | ES_DISPLAY_REQUIRED );
#endif
    }

    void Stop() override
    {
        workerRunning.store ( false );
        Channel.SetEnable ( false );
        Channel.Disconnect(); // Explicitly disconnect from server
#if defined( Q_OS_WINDOWS )
        SetThreadExecutionState ( ES_CONTINUOUS );
#endif
    }

    bool IsRunning() override { return workerRunning.load(); }

    float GetChannelGain ( int clientChannelId )
    {
        if ( clientChannelId < 0 || clientChannelId >= MAX_NUM_CHANNELS )
            return 1.0f;
        return this->clientChannels[clientChannelId].newGain;
    }

    float GetChannelPan ( int clientChannelId )
    {
        if ( clientChannelId < 0 || clientChannelId >= MAX_NUM_CHANNELS )
            return 0.0f;
        // Pan in Jamulus is 0.0 to 1.0 internally, map back to -1.0 to 1.0
        return ( this->clientChannels[clientChannelId].newPan * 2.0f ) - 1.0f;
    }

    bool IsChannelMuted ( int clientChannelId )
    {
        if ( clientChannelId < 0 || clientChannelId >= MAX_NUM_CHANNELS )
            return false;
        // Mute state is usually just gain = 0 in Jamulus mixer messages
        return this->clientChannels[clientChannelId].newGain < 0.01f;
    }

    void PublicSetChannelSolo ( int clientChannelId, bool solo )
    {
        if ( clientChannelId < 0 || clientChannelId >= MAX_NUM_CHANNELS )
            return;
        if ( solo )
            soloedChannels.insert ( clientChannelId );
        else
            soloedChannels.erase ( clientChannelId );
    }

    bool PublicIsChannelSolo ( int clientChannelId )
    {
        if ( clientChannelId < 0 )
            return false;
        return soloedChannels.find ( clientChannelId ) != soloedChannels.end();
    }

    bool AnyChannelSoloed() const { return !soloedChannels.empty(); }

    // FIFO access for async API
    AudioFifo* GetInputFifo() { return inFifo.get(); }
    AudioFifo* GetOutputFifo() { return outFifo.get(); }

    // Expose FindClientChannel for ID mapping
    int PublicFindClientChannel ( int serverId )
    {
        if ( serverId < 0 )
            return -1;
        return FindClientChannel ( serverId, true );
    }

    // Capture raw data from server via virtual overrides
    void OnConClientListMesReceived ( CVector<CChannelInfo> vecChanInfo ) override
    {
        // First, let the base class update its internal slots/channels
        CClient::OnConClientListMesReceived ( vecChanInfo );

        // Now cache the RAW list for the VST GUI
        QMutexLocker lock ( &s_channelMutex );
        s_channelList = vecChanInfo;
    }

    void OnCLChannelLevelListReceived ( CHostAddress InetAddr, CVector<uint16_t> vecLevelList ) override
    {
        // Update raw levels before base class might reorder them
        {
            QMutexLocker lock ( &s_channelLevelsMutex );
            s_channelLevels.clear();
            for ( int i = 0; i < vecLevelList.Size(); ++i )
            {
                s_channelLevels.push_back ( vecLevelList[i] );
            }
        }

        // Let base class do its reordering if needed
        CClient::OnCLChannelLevelListReceived ( InetAddr, vecLevelList );
    }

    void OnClientIDReceived ( int iServerChanID ) override
    {
        s_myChannelId = iServerChanID;
        CClient::OnClientIDReceived ( iServerChanID );
    }

    void ClearInternalLists()
    {
        QMutexLocker lock ( &s_channelMutex );
        s_channelList.Init ( 0 );
        QMutexLocker levelLock ( &s_channelLevelsMutex );
        s_channelLevels.clear();
        s_myChannelId = -1;
        soloedChannels.clear();
        outPeakL.store ( 0.0f );
        outPeakR.store ( 0.0f );
    }

    float GetOutputPeakL() const { return outPeakL.load(); }
    float GetOutputPeakR() const { return outPeakR.load(); }

private:
    std::atomic<float>         outPeakL;
    std::atomic<float>         outPeakR;
    std::set<int>              soloedChannels;
    std::unique_ptr<AudioFifo> inFifo;
    std::unique_ptr<AudioFifo> outFifo;
    std::thread                worker;
    std::atomic<bool>          workerRunning;
    int                        blockFrames{ 128 };

    // Internal ring buffers for block processing
    std::vector<float> internalInputBuffer;
    std::vector<float> internalOutputBuffer;
    size_t             inputWritePos;
    size_t             inputReadPos;
    size_t             outputWritePos;
    size_t             outputReadPos;
};

static std::map<int, float> s_preMuteGains;
static QMutex               s_muteMutex;

extern "C"
{

    // Signal handlers for updating cached data
    // (Moved logic into WrapperClient virtual overrides for perfect sync)

    static void updatePingTime ( int pingTime ) { s_lastPingTime = pingTime; }

    // (My ID is now updated directly in WrapperClient override)

    static void updateChatMessage ( QString strChatText )
    {
        QMutexLocker lock ( &s_chatMutex );
        s_chatQueue.push_back ( strChatText.toStdString() );
    }

    // (Storage moved up to top of file)

    static void updateServerList ( CHostAddress InetAddr, CVector<CServerInfo> vecServerInfo )
    {
        (void) InetAddr;
        QMutexLocker lock ( &s_serverListMutex );

        // Don't clear - merge/update entries
        for ( int i = 0; i < vecServerInfo.Size(); ++i )
        {
            const CServerInfo& info = vecServerInfo[i];

            jamulus_server_info_t srvInfo;
            memset ( &srvInfo, 0, sizeof ( srvInfo ) );

            // Name
            QByteArray nameUtf8 = info.strName.toUtf8();
            strncpy ( srvInfo.name, nameUtf8.constData(), sizeof ( srvInfo.name ) - 1 );

            // Address
            QString    addrStr  = info.HostAddr.toString();
            QByteArray addrUtf8 = addrStr.toUtf8();
            strncpy ( srvInfo.address, addrUtf8.constData(), sizeof ( srvInfo.address ) - 1 );

            // City
            QByteArray cityUtf8 = info.strCity.toUtf8();
            strncpy ( srvInfo.city, cityUtf8.constData(), sizeof ( srvInfo.city ) - 1 );

            // Country
            QString    countryName = QLocale::countryToString ( info.eCountry );
            QByteArray countryUtf8 = countryName.toUtf8();
            strncpy ( srvInfo.country, countryUtf8.constData(), sizeof ( srvInfo.country ) - 1 );

            srvInfo.ping        = -1; // Will be updated by ping
            srvInfo.numClients  = 0;  // Will be updated
            srvInfo.maxClients  = info.iMaxNumClients;
            srvInfo.isPermanent = info.bPermanentOnline;

            // Check if server already exists (by address)
            bool found = false;
            for ( auto& existing : s_serverList )
            {
                if ( strcmp ( existing.address, srvInfo.address ) == 0 )
                {
                    // Update existing entry (keep ping if we have it)
                    int oldPing       = existing.ping;
                    int oldNumClients = existing.numClients;
                    existing          = srvInfo;
                    if ( oldPing >= 0 )
                        existing.ping = oldPing;
                    if ( oldNumClients > 0 )
                        existing.numClients = oldNumClients;
                    found = true;
                    break;
                }
            }

            if ( !found )
            {
                s_serverList.push_back ( srvInfo );
            }
        }
    }

    static void updateServerPingAndClients ( CHostAddress InetAddr, int iPingTime, int iNumClients )
    {
        QMutexLocker lock ( &s_serverListMutex );
        QString      addrStr  = InetAddr.toString();
        QByteArray   addrUtf8 = addrStr.toUtf8();

        for ( auto& srv : s_serverList )
        {
            if ( strcmp ( srv.address, addrUtf8.constData() ) == 0 )
            {
                srv.ping       = iPingTime;
                srv.numClients = iNumClients;
                break;
            }
        }
    }

    // ============================================================================
    // Client Lifecycle
    // ============================================================================

    jamulus_client_t jamulus_client_create ( unsigned short port, const char* server_addr, const char* client_name, bool enable_ipv6 )
    {
        // Ensure QApplication exists for Qt widgets
        ensureQtApp();

        // Minimal parameters: qos=0, empty MIDI setup, no auto jack connect, not muted
        // Allocate without throwing; return null on allocation failure.
        WrapperClient* c = new ( std::nothrow ) WrapperClient ( (quint16) port,
                                                                (quint16) 0,
                                                                QString ( server_addr ? server_addr : QString() ),
                                                                QString ( "" ),
                                                                true,
                                                                QString ( client_name ? client_name : "jamulus_wrapper" ),
                                                                enable_ipv6,
                                                                false );
        if ( !c )
            return nullptr;

        // Connect other useful signals
        QObject::connect ( c, &CClient::PingTimeReceived, updatePingTime );
        QObject::connect ( c, &CClient::ChatTextReceived, updateChatMessage );
        QObject::connect ( c, &CClient::CLServerListReceived, updateServerList );
        QObject::connect ( c, &CClient::CLRedServerListReceived, updateServerList );
        QObject::connect ( c, &CClient::CLPingTimeWithNumClientsReceived, updateServerPingAndClients );
        QObject::connect ( c, &CClient::Disconnected, [c]() {
            static_cast<WrapperClient*> ( c )->ClearInternalLists();
            s_lastPingTime = -1;
        } );

        return reinterpret_cast<jamulus_client_t> ( c );
    }

    void jamulus_client_destroy ( jamulus_client_t c )
    {
        if ( !c )
            return;

        // If this client is associated with the GUI, clean up GUI first
        WrapperClient* wc = reinterpret_cast<WrapperClient*> ( c );
        if ( g_clientDlg )
        {
            // Hide and delete the dialog before deleting the client it references
            g_clientDlg->hide();
            // delete g_clientDlg; // DISABLED: Crash risk on unload
            g_clientDlg = nullptr;
        }
        if ( g_settings )
        {
            delete g_settings;
            g_settings = nullptr;
        }

        delete wc;
    }

    int jamulus_client_start ( jamulus_client_t c )
    {
        CClient* client = reinterpret_cast<CClient*> ( c );
        if ( !client )
            return -1;
        client->Start();
        return 0;
    }

    int jamulus_client_stop ( jamulus_client_t c )
    {
        CClient* client = reinterpret_cast<CClient*> ( c );
        if ( !client )
            return -1;
        client->Stop();
        return 0;
    }

    int jamulus_client_set_server_addr ( jamulus_client_t c, const char* addr )
    {
        CClient* client = reinterpret_cast<CClient*> ( c );
        if ( !client )
            return -1;
        bool ok = client->SetServerAddr ( QString ( addr ? addr : "" ) );
        return ok ? 0 : -1;
    }

    int jamulus_client_process_audio ( jamulus_client_t c, const float* in, float* out, int frames, int channels )
    {
        if ( !c )
            return -1;
        WrapperClient* wc = reinterpret_cast<WrapperClient*> ( c );
        return wc->ProcessInterleavedFloat ( in, out, frames, channels );
    }

    int jamulus_client_push_audio ( jamulus_client_t c, const float* in, int frames, int channels )
    {
        if ( !c || !in || frames <= 0 )
            return -1;
        WrapperClient* wc = reinterpret_cast<WrapperClient*> ( c );
        AudioFifo*     f  = wc->GetInputFifo();
        if ( !f )
            return -1;
        bool ok = f->push ( in, static_cast<size_t> ( frames ) );
        return ok ? 0 : -1;
    }

    int jamulus_client_pop_audio ( jamulus_client_t c, float* out, int maxFrames, int channels )
    {
        if ( !c || !out || maxFrames <= 0 )
            return -1;
        WrapperClient* wc = reinterpret_cast<WrapperClient*> ( c );
        AudioFifo*     f  = wc->GetOutputFifo();
        if ( !f )
            return -1;
        size_t got = f->pop ( out, static_cast<size_t> ( maxFrames ) );
        return static_cast<int> ( got );
    }

    int jamulus_client_start_worker ( jamulus_client_t c, int processBlockFrames )
    {
        if ( !c )
            return -1;
        WrapperClient* wc = reinterpret_cast<WrapperClient*> ( c );
        wc->StartWorker ( processBlockFrames );
        return 0;
    }

    int jamulus_client_stop_worker ( jamulus_client_t c )
    {
        if ( !c )
            return -1;
        WrapperClient* wc = reinterpret_cast<WrapperClient*> ( c );
        wc->StopWorker();
        return 0;
    }

    // ============================================================================
    // Connection Control
    // ============================================================================

    bool jamulus_client_is_connected ( jamulus_client_t c )
    {
        if ( !c )
            return false;
        CClient* client = reinterpret_cast<CClient*> ( c );
        return client->IsConnected();
    }

    bool jamulus_client_is_running ( jamulus_client_t c )
    {
        if ( !c )
            return false;
        CClient* client = reinterpret_cast<CClient*> ( c );
        return client->IsRunning();
    }

    void jamulus_client_disconnect ( jamulus_client_t c )
    {
        if ( !c )
            return;
        WrapperClient* wc = reinterpret_cast<WrapperClient*> ( c );
        wc->Stop();
        wc->ClearInternalLists();
    }

    // ============================================================================
    // Level Metering
    // ============================================================================

    double jamulus_client_get_level_left ( jamulus_client_t c )
    {
        if ( !c )
            return -100.0;
        CClient* client = reinterpret_cast<CClient*> ( c );
        return client->GetLevelForMeterdBLeft();
    }

    double jamulus_client_get_level_right ( jamulus_client_t c )
    {
        if ( !c )
            return -100.0;
        CClient* client = reinterpret_cast<CClient*> ( c );
        return client->GetLevelForMeterdBRight();
    }

    float jamulus_client_get_output_level_left ( jamulus_client_t c )
    {
        if ( !c )
            return 0.0f;
        WrapperClient* wc = reinterpret_cast<WrapperClient*> ( c );
        return wc->GetOutputPeakL();
    }

    float jamulus_client_get_output_level_right ( jamulus_client_t c )
    {
        if ( !c )
            return 0.0f;
        WrapperClient* wc = reinterpret_cast<WrapperClient*> ( c );
        return wc->GetOutputPeakR();
    }

    // ============================================================================
    // Audio Settings
    // ============================================================================

    int jamulus_client_get_audio_quality ( jamulus_client_t c )
    {
        if ( !c )
            return 1;
        CClient* client = reinterpret_cast<CClient*> ( c );
        return static_cast<int> ( client->GetAudioQuality() );
    }

    void jamulus_client_set_audio_quality ( jamulus_client_t c, int quality )
    {
        if ( !c )
            return;
        CClient* client = reinterpret_cast<CClient*> ( c );
        client->SetAudioQuality ( static_cast<EAudioQuality> ( quality ) );
    }

    int jamulus_client_get_audio_channels ( jamulus_client_t c )
    {
        if ( !c )
            return 2;
        CClient* client = reinterpret_cast<CClient*> ( c );
        return static_cast<int> ( client->GetAudioChannels() );
    }

    void jamulus_client_set_audio_channels ( jamulus_client_t c, int channels )
    {
        if ( !c )
            return;
        CClient* client = reinterpret_cast<CClient*> ( c );
        client->SetAudioChannels ( static_cast<EAudChanConf> ( channels ) );
    }

    int jamulus_client_get_input_fader ( jamulus_client_t c )
    {
        if ( !c )
            return 50;
        CClient* client = reinterpret_cast<CClient*> ( c );
        return client->GetAudioInFader();
    }

    void jamulus_client_set_input_fader ( jamulus_client_t c, int value )
    {
        if ( !c )
            return;
        CClient* client = reinterpret_cast<CClient*> ( c );
        client->SetAudioInFader ( value );
    }

    int jamulus_client_get_reverb_level ( jamulus_client_t c )
    {
        if ( !c )
            return 0;
        CClient* client = reinterpret_cast<CClient*> ( c );
        return client->GetReverbLevel();
    }

    void jamulus_client_set_reverb_level ( jamulus_client_t c, int level )
    {
        if ( !c )
            return;
        CClient* client = reinterpret_cast<CClient*> ( c );
        client->SetReverbLevel ( level );
    }

    bool jamulus_client_get_reverb_on_left ( jamulus_client_t c )
    {
        if ( !c )
            return true;
        CClient* client = reinterpret_cast<CClient*> ( c );
        return client->IsReverbOnLeftChan();
    }

    void jamulus_client_set_reverb_on_left ( jamulus_client_t c, bool on_left )
    {
        if ( !c )
            return;
        CClient* client = reinterpret_cast<CClient*> ( c );
        client->SetReverbOnLeftChan ( on_left );
    }

    // ============================================================================
    // User Info
    // ============================================================================

    // Static buffer for name return
    static char s_nameBuffer[256]      = { 0 };
    static char s_cityBuffer[256]      = { 0 };
    static char s_countryCodeBuffer[8] = { 0 };

    void jamulus_client_set_name ( jamulus_client_t c, const char* name )
    {
        if ( !c || !name )
            return;
        CClient* client             = reinterpret_cast<CClient*> ( c );
        client->ChannelInfo.strName = QString::fromUtf8 ( name );
        client->SetRemoteInfo();
    }

    const char* jamulus_client_get_name ( jamulus_client_t c )
    {
        if ( !c )
            return "";
        CClient*   client = reinterpret_cast<CClient*> ( c );
        QByteArray utf8   = client->ChannelInfo.strName.toUtf8();
        strncpy ( s_nameBuffer, utf8.constData(), sizeof ( s_nameBuffer ) - 1 );
        s_nameBuffer[sizeof ( s_nameBuffer ) - 1] = '\0';
        return s_nameBuffer;
    }

    void jamulus_client_set_instrument ( jamulus_client_t c, int instrument )
    {
        if ( !c )
            return;
        CClient* client                 = reinterpret_cast<CClient*> ( c );
        client->ChannelInfo.iInstrument = instrument;
        client->SetRemoteInfo();
    }

    int jamulus_client_get_instrument ( jamulus_client_t c )
    {
        if ( !c )
            return 0;
        CClient* client = reinterpret_cast<CClient*> ( c );
        return client->ChannelInfo.iInstrument;
    }

    void jamulus_client_set_country ( jamulus_client_t c, int country )
    {
        if ( !c )
            return;
        CClient* client = reinterpret_cast<CClient*> ( c );
        // Country parameter is a Qt5 wire format code - convert to Qt-native country
        client->ChannelInfo.eCountry = CLocale::WireFormatCountryCodeToQtCountry ( static_cast<unsigned short> ( country ) );
        client->SetRemoteInfo();
    }

    int jamulus_client_get_country ( jamulus_client_t c )
    {
        if ( !c )
            return 0;
        CClient* client = reinterpret_cast<CClient*> ( c );
        // Return Qt5 wire format code for compatibility
        return static_cast<int> ( CLocale::QtCountryToWireFormatCountryCode ( client->ChannelInfo.eCountry ) );
    }

    const char* jamulus_client_get_country_code ( jamulus_client_t c )
    {
        if ( !c )
            return "none";
        CClient* client = reinterpret_cast<CClient*> ( c );

        // Special case for "none"
        if ( client->ChannelInfo.eCountry == QLocale::AnyCountry )
            return "none";

        // Use QLocale::matchingLocales like Jamulus does to get correct country code
        QList<QLocale> vCurLocaleList = QLocale::matchingLocales ( QLocale::AnyLanguage, QLocale::AnyScript, client->ChannelInfo.eCountry );

        if ( vCurLocaleList.size() < 1 )
            return "none";

        QStringList vstrLocParts = vCurLocaleList.at ( 0 ).name().split ( "_" );

        if ( vstrLocParts.size() <= 1 )
            return "none";

        QByteArray code = vstrLocParts.at ( 1 ).toLower().toUtf8();
        strncpy ( s_countryCodeBuffer, code.constData(), sizeof ( s_countryCodeBuffer ) - 1 );
        s_countryCodeBuffer[sizeof ( s_countryCodeBuffer ) - 1] = '\0';
        return s_countryCodeBuffer;
    }

    void jamulus_client_set_city ( jamulus_client_t c, const char* city )
    {
        if ( !c || !city )
            return;
        CClient* client             = reinterpret_cast<CClient*> ( c );
        client->ChannelInfo.strCity = QString::fromUtf8 ( city );
        client->SetRemoteInfo();
    }

    const char* jamulus_client_get_city ( jamulus_client_t c )
    {
        if ( !c )
            return "";
        CClient*   client = reinterpret_cast<CClient*> ( c );
        QByteArray utf8   = client->ChannelInfo.strCity.toUtf8();
        strncpy ( s_cityBuffer, utf8.constData(), sizeof ( s_cityBuffer ) - 1 );
        s_cityBuffer[sizeof ( s_cityBuffer ) - 1] = '\0';
        return s_cityBuffer;
    }

    void jamulus_client_set_skill ( jamulus_client_t c, int skill )
    {
        if ( !c )
            return;
        CClient* client                 = reinterpret_cast<CClient*> ( c );
        client->ChannelInfo.eSkillLevel = static_cast<ESkillLevel> ( skill );
        client->SetRemoteInfo();
    }

    int jamulus_client_get_skill ( jamulus_client_t c )
    {
        if ( !c )
            return 0;
        CClient* client = reinterpret_cast<CClient*> ( c );
        return static_cast<int> ( client->ChannelInfo.eSkillLevel );
    }

    // ============================================================================
    // Network Status
    // ============================================================================

    int jamulus_client_get_ping_time ( jamulus_client_t c )
    {
        if ( !c )
            return -1;
        CClient* client = reinterpret_cast<CClient*> ( c );
        if ( !client->IsConnected() )
            return -1;
        return s_lastPingTime;
    }

    int jamulus_client_get_overall_delay ( jamulus_client_t c )
    {
        if ( !c )
            return -1;
        CClient* client = reinterpret_cast<CClient*> ( c );
        if ( !client->IsConnected() || s_lastPingTime < 0 )
            return -1;
        return client->EstimatedOverallDelay ( s_lastPingTime );
    }

    void jamulus_client_get_network_stats ( jamulus_client_t c, jamulus_network_stats_t* stats )
    {
        if ( !c || !stats )
            return;
        CClient* client = reinterpret_cast<CClient*> ( c );

        if ( !client->IsConnected() )
        {
            stats->ping_time_ms         = -1;
            stats->total_delay_ms       = -1;
            stats->jitter_buffer_status = 0; // Default to green if disconnected
            return;
        }

        stats->ping_time_ms = s_lastPingTime;

        if ( s_lastPingTime >= 0 )
        {
            stats->total_delay_ms = client->EstimatedOverallDelay ( s_lastPingTime );
        }
        else
        {
            stats->total_delay_ms = -1;
        }

        // Jitter status logic matching CClientDlg::OnTimerBuffersLED
        if ( client->GetAndResetbJitterBufferOKFlag() )
        {
            stats->jitter_buffer_status = 0; // Green
        }
        else
        {
            stats->jitter_buffer_status = 2; // Red
        }
    }

    // ============================================================================
    // Channel Info (for mixer)
    // ============================================================================

    int jamulus_client_get_num_channels ( jamulus_client_t c )
    {
        (void) c;
        QMutexLocker lock ( &s_channelMutex );
        return s_channelList.Size();
    }

    int jamulus_client_get_my_channel_id ( jamulus_client_t c )
    {
        if ( !c )
            return -1;

        // Return the actual channel ID assigned by the server
        // This matches info.id (ch.iChanID) in jamulus_client_get_channel_info
        return s_myChannelId;
    }

    bool jamulus_client_get_channel_info ( jamulus_client_t c, int index, jamulus_channel_info_t* info )
    {
        if ( !c || !info )
            return false;

        QMutexLocker lock ( &s_channelMutex );
        if ( index < 0 || index >= s_channelList.Size() )
            return false;

        const CChannelInfo& ch = s_channelList[index];
        // Use the channel ID from the server list. This ID corresponds to the index in clientChannels array.
        info->id = ch.iChanID;

        QByteArray nameUtf8 = ch.strName.toUtf8();
        strncpy ( info->name, nameUtf8.constData(), sizeof ( info->name ) - 1 );
        info->name[sizeof ( info->name ) - 1] = '\0';

        info->instrument = ch.iInstrument;

        // Extract country code (ISO 3166-1 alpha-2)
        QString        strCountryCode;
        QList<QLocale> vCurLocaleList = QLocale::matchingLocales ( QLocale::AnyLanguage, QLocale::AnyScript, ch.eCountry );
        if ( !vCurLocaleList.isEmpty() )
        {
            QStringList vstrLocParts = vCurLocaleList.at ( 0 ).name().split ( "_" );
            if ( vstrLocParts.size() >= 2 )
                strCountryCode = vstrLocParts.at ( 1 ).toLower();
        }

        if ( strCountryCode.isEmpty() )
            strncpy ( info->country_code, "none", sizeof ( info->country_code ) - 1 );
        else
            strncpy ( info->country_code, strCountryCode.toUtf8().constData(), sizeof ( info->country_code ) - 1 );
        info->country_code[sizeof ( info->country_code ) - 1] = '\0';

        QByteArray cityUtf8 = ch.strCity.toUtf8();
        strncpy ( info->city, cityUtf8.constData(), sizeof ( info->city ) - 1 );
        info->city[sizeof ( info->city ) - 1] = '\0';

        info->skill_level = static_cast<int> ( ch.eSkillLevel );

        // Get actual gain/pan from the client instance
        WrapperClient* wc = reinterpret_cast<WrapperClient*> ( c );
        info->gain        = wc->GetChannelGain ( ch.iChanID );
        info->pan         = wc->GetChannelPan ( ch.iChanID );
        info->muted       = wc->IsChannelMuted ( ch.iChanID );
        info->solo        = wc->PublicIsChannelSolo ( ch.iChanID );

        // Get level from cached channel levels (received from server)
        {
            QMutexLocker levelLock ( &s_channelLevelsMutex );
            if ( index >= 0 && index < static_cast<int> ( s_channelLevels.size() ) )
            {
                // Level is 0-15 from server, scale to 0-65535 for consistency
                info->level = s_channelLevels[index] * 4096;
            }
            else
            {
                info->level = 0;
            }
        }

        return true;
    }

    void jamulus_client_set_channel_gain ( jamulus_client_t c, int channel_id, float gain )
    {
        if ( !c )
            return;
        WrapperClient* wc = reinterpret_cast<WrapperClient*> ( c );

        int myChannelId = s_myChannelId; // Server channel ID for "me"
        if ( channel_id == -1 || channel_id == myChannelId )
        {
            // This is my local fader. Update the personal monitoring mix.
            // Note: Do NOT call SetAudioInFader here - that controls L/R balance, not volume!
            // The input fader should stay at AUD_FADER_IN_MIDDLE (50) for balanced stereo.
            if ( myChannelId != -1 )
            {
                // Convert server channel ID to client channel index
                int clientIdx = wc->PublicFindClientChannel ( myChannelId );
                if ( clientIdx >= 0 )
                    wc->SetRemoteChanGain ( clientIdx, gain, false );
            }
        }
        else
        {
            // Convert server channel ID to client channel index
            int clientIdx = wc->PublicFindClientChannel ( channel_id );
            if ( clientIdx >= 0 )
                wc->SetRemoteChanGain ( clientIdx, gain, false );
        }
    }

    void jamulus_client_set_channel_pan ( jamulus_client_t c, int channel_id, float pan )
    {
        if ( !c )
            return;
        WrapperClient* wc = reinterpret_cast<WrapperClient*> ( c );

        int myChannelId = s_myChannelId; // Server channel ID for "me"
        if ( channel_id == -1 || channel_id == myChannelId )
        {
            // Input pan isn't really a single setting in Jamulus profile,
            // but we can update the personal mix if connected.
            if ( myChannelId != -1 )
            {
                // Convert server channel ID to client channel index
                int clientIdx = wc->PublicFindClientChannel ( myChannelId );
                if ( clientIdx >= 0 )
                    wc->SetRemoteChanPan ( clientIdx, ( pan + 1.0f ) / 2.0f );
            }
        }
        else
        {
            // Convert server channel ID to client channel index
            int clientIdx = wc->PublicFindClientChannel ( channel_id );
            if ( clientIdx >= 0 )
                wc->SetRemoteChanPan ( clientIdx, ( pan + 1.0f ) / 2.0f );
        }
    }

    void jamulus_client_set_channel_mute ( jamulus_client_t c, int channel_id, bool mute )
    {
        if ( !c )
            return;
        WrapperClient* wc = reinterpret_cast<WrapperClient*> ( c );

        int myChannelId = s_myChannelId; // Server channel ID for "me"
        if ( channel_id == -1 || channel_id == myChannelId )
        {
            if ( mute )
            {
                // Store current gain if not already muted
                float currentGain = jamulus_client_get_input_fader ( c ) / 100.0f;
                if ( currentGain > 0.01f )
                {
                    QMutexLocker lock ( &s_muteMutex );
                    s_preMuteGains[-1] = currentGain;
                }
                jamulus_client_set_channel_gain ( c, channel_id, 0.0f );
            }
            else
            {
                float restoreGain = 0.75f;
                {
                    QMutexLocker lock ( &s_muteMutex );
                    if ( s_preMuteGains.count ( -1 ) )
                        restoreGain = s_preMuteGains[-1];
                }
                jamulus_client_set_channel_gain ( c, channel_id, restoreGain );
            }
        }
        else
        {
            // Convert server channel ID to client channel index
            int clientIdx = wc->PublicFindClientChannel ( channel_id );
            if ( clientIdx < 0 )
                return;

            if ( mute )
            {
                float currentGain = wc->GetChannelGain ( clientIdx );
                if ( currentGain > 0.01f )
                {
                    QMutexLocker lock ( &s_muteMutex );
                    s_preMuteGains[channel_id] = currentGain;
                }
                wc->SetRemoteChanGain ( clientIdx, 0.0f, false );
            }
            else
            {
                float restoreGain = 1.0f;
                {
                    QMutexLocker lock ( &s_muteMutex );
                    if ( s_preMuteGains.count ( channel_id ) )
                        restoreGain = s_preMuteGains[channel_id];
                }
                wc->SetRemoteChanGain ( clientIdx, restoreGain, false );
            }
        }
    }

    void jamulus_client_set_channel_solo ( jamulus_client_t c, int channel_id, bool solo )
    {
        if ( !c )
            return;
        WrapperClient* wc = reinterpret_cast<WrapperClient*> ( c );

        // Convert server channel ID to client channel index for solo tracking
        int clientIdx = ( channel_id == -1 ) ? -1 : wc->PublicFindClientChannel ( channel_id );

        // Check if anything was previously soloed
        bool wasSoloed = wc->AnyChannelSoloed();

        // Update solo state
        wc->PublicSetChannelSolo ( clientIdx, solo );

        // Check if anything is now soloed
        bool isSoloed = wc->AnyChannelSoloed();

        // Get all channels and apply solo logic
        QMutexLocker lock ( &s_channelMutex );
        int          numChannels = s_channelList.Size();

        for ( int i = 0; i < numChannels; ++i )
        {
            int serverId      = s_channelList[i].iChanID;
            int chanClientIdx = wc->PublicFindClientChannel ( serverId );
            if ( chanClientIdx < 0 )
                continue;

            bool thisChannelSoloed = wc->PublicIsChannelSolo ( chanClientIdx );

            if ( isSoloed )
            {
                // Some channel is soloed - mute non-soloed channels
                if ( !thisChannelSoloed )
                {
                    // Save gain and mute
                    float currentGain = wc->GetChannelGain ( chanClientIdx );
                    if ( currentGain > 0.01f )
                    {
                        QMutexLocker muteLock ( &s_muteMutex );
                        s_preMuteGains[serverId + 10000] = currentGain; // Use offset to not conflict with mute storage
                    }
                    wc->SetRemoteChanGain ( chanClientIdx, 0.0f, false );
                }
            }
            else if ( wasSoloed && !isSoloed )
            {
                // Solo was just turned off - restore gains
                float restoreGain = 1.0f;
                {
                    QMutexLocker muteLock ( &s_muteMutex );
                    if ( s_preMuteGains.count ( serverId + 10000 ) )
                    {
                        restoreGain = s_preMuteGains[serverId + 10000];
                        s_preMuteGains.erase ( serverId + 10000 );
                    }
                }
                wc->SetRemoteChanGain ( chanClientIdx, restoreGain, false );
            }
        }
    }

    // ============================================================================
    // Advanced Settings
    // ============================================================================

    void jamulus_client_set_auto_jitter ( jamulus_client_t c, bool enabled )
    {
        if ( !c )
            return;
        CClient* client = reinterpret_cast<CClient*> ( c );
        client->SetDoAutoSockBufSize ( enabled );
    }

    bool jamulus_client_get_auto_jitter ( jamulus_client_t c )
    {
        if ( !c )
            return true;
        CClient* client = reinterpret_cast<CClient*> ( c );
        return client->GetDoAutoSockBufSize();
    }

    void jamulus_client_set_jitter_buffer ( jamulus_client_t c, int size )
    {
        if ( !c )
            return;
        CClient* client = reinterpret_cast<CClient*> ( c );
        client->SetSockBufNumFrames ( size );
    }

    int jamulus_client_get_jitter_buffer ( jamulus_client_t c )
    {
        if ( !c )
            return 10;
        CClient* client = reinterpret_cast<CClient*> ( c );
        return client->GetSockBufNumFrames();
    }

    void jamulus_client_set_server_jitter_buffer ( jamulus_client_t c, int size )
    {
        if ( !c )
            return;
        CClient* client = reinterpret_cast<CClient*> ( c );
        client->SetServerSockBufNumFrames ( size );
    }

    int jamulus_client_get_server_jitter_buffer ( jamulus_client_t c )
    {
        if ( !c )
            return 10;
        CClient* client = reinterpret_cast<CClient*> ( c );
        return client->GetServerSockBufNumFrames();
    }

    void jamulus_client_set_input_boost ( jamulus_client_t c, int boost )
    {
        if ( !c )
            return;
        CClient* client = reinterpret_cast<CClient*> ( c );
        // boost is 1-4, map to actual boost factor
        client->SetInputBoost ( boost );
    }

    int jamulus_client_get_input_boost ( jamulus_client_t c )
    {
        if ( !c )
            return 1;
        // InputBoost doesn't have a getter, so we store it in settings
        (void) c;
        return 1; // Default
    }

    void jamulus_client_set_new_client_level ( jamulus_client_t c, int level )
    {
        // This is typically stored in settings, not directly in client
        // For now, store locally if settings available
        if ( g_settings )
        {
            g_settings->iNewClientFaderLevel = level;
        }
        (void) c;
    }

    int jamulus_client_get_new_client_level ( jamulus_client_t c )
    {
        if ( g_settings )
        {
            return g_settings->iNewClientFaderLevel;
        }
        (void) c;
        return 100;
    }

    void jamulus_client_set_feedback_detection ( jamulus_client_t c, bool enabled )
    {
        // Feedback detection is in settings, not client
        if ( g_settings )
        {
            g_settings->bEnableFeedbackDetection = enabled;
        }
        (void) c;
    }

    bool jamulus_client_get_feedback_detection ( jamulus_client_t c )
    {
        if ( g_settings )
        {
            return g_settings->bEnableFeedbackDetection;
        }
        (void) c;
        return true;
    }

    void jamulus_client_set_audio_alerts ( jamulus_client_t c, bool enabled )
    {
        // Audio alerts are handled in settings
        if ( g_settings )
        {
            g_settings->bEnableAudioAlerts = enabled;
        }
        (void) c;
    }

    bool jamulus_client_get_audio_alerts ( jamulus_client_t c )
    {
        if ( g_settings )
        {
            return g_settings->bEnableAudioAlerts;
        }
        (void) c;
        return false;
    }

    void jamulus_client_set_small_buffers ( jamulus_client_t c, bool enabled )
    {
        // Small buffers is a sound card setting - in VST context, not really applicable
        (void) c;
        (void) enabled;
    }

    bool jamulus_client_get_small_buffers ( jamulus_client_t c )
    {
        (void) c;
        return false;
    }

    // ============================================================================
    // Chat
    // ============================================================================

    void jamulus_client_send_chat_message ( jamulus_client_t c, const char* message )
    {
        if ( !c || !message )
            return;
        CClient* client = reinterpret_cast<CClient*> ( c );
        client->CreateChatTextMes ( QString::fromUtf8 ( message ) );
    }

    const char* jamulus_client_get_chat_message ( jamulus_client_t c )
    {
        (void) c;
        QMutexLocker lock ( &s_chatMutex );
        if ( s_chatQueue.empty() )
        {
            return "";
        }

        std::string msg = s_chatQueue.front();
        s_chatQueue.pop_front();

        strncpy ( s_chatBuffer, msg.size() < 4095 ? msg.c_str() : msg.substr ( 0, 4095 ).c_str(), sizeof ( s_chatBuffer ) - 1 );
        s_chatBuffer[sizeof ( s_chatBuffer ) - 1] = '\0';
        return s_chatBuffer;
    }

    // ============================================================================
    // Network Stats implementation
    // ============================================================================

    void jamulus_client_send_ping ( jamulus_client_t c )
    {
        if ( !c )
            return;
        CClient* client = reinterpret_cast<CClient*> ( c );
        client->CreateCLPingMes();
    }

    // ============================================================================
    // Server List / Directory
    // ============================================================================

    // Directory addresses (matching Jamulus global.h)
    // Index matches EDirectoryType enum: AT_DEFAULT=0, AT_ANY_GENRE2=1, AT_ANY_GENRE3=2, etc.
    static const char* DIRECTORY_ADDRESSES[] = {
        "anygenre1.jamulus.io:22124", // AT_DEFAULT (Any Genre 1)
        "anygenre2.jamulus.io:22224", // AT_ANY_GENRE2
        "anygenre3.jamulus.io:22624", // AT_ANY_GENRE3
        "rock.jamulus.io:22424",      // AT_GENRE_ROCK
        "jazz.jamulus.io:22324",      // AT_GENRE_JAZZ
        "classical.jamulus.io:22524", // AT_GENRE_CLASSICAL_FOLK
        "choral.jamulus.io:22724",    // AT_GENRE_CHORAL
        ""                            // AT_CUSTOM
    };

    void jamulus_client_request_server_list ( jamulus_client_t c, int directory_type )
    {
        if ( !c )
            return;
        CClient* client = reinterpret_cast<CClient*> ( c );

        // Clamp directory type (0-6, matching EDirectoryType)
        if ( directory_type < 0 || directory_type > 6 )
            directory_type = 0;

        const char* dirAddr = DIRECTORY_ADDRESSES[directory_type];
        if ( !dirAddr || dirAddr[0] == '\0' )
            return;

        // Parse the directory address
        CHostAddress DirectoryAddress;
        if ( NetworkUtil::ParseNetworkAddress ( QString ( dirAddr ), DirectoryAddress, false ) )
        {
            // Request the server list
            client->CreateCLReqServerListMes ( DirectoryAddress );
        }
    }

    void jamulus_client_ping_server_list ( jamulus_client_t c )
    {
        if ( !c )
            return;
        CClient* client = reinterpret_cast<CClient*> ( c );

        // Get copy of server addresses to ping (avoid holding lock during network ops)
        std::vector<QString> addressesToPing;
        {
            QMutexLocker lock ( &s_serverListMutex );
            for ( const auto& srv : s_serverList )
            {
                addressesToPing.push_back ( QString ( srv.address ) );
            }
        }

        // Ping each server
        for ( const QString& addrStr : addressesToPing )
        {
            CHostAddress serverAddr;
            if ( NetworkUtil::ParseNetworkAddress ( addrStr, serverAddr, false ) )
            {
                // Send ping with client count request
                client->CreateCLServerListPingMes ( serverAddr );
            }
        }
    }

    int jamulus_client_get_num_servers ( jamulus_client_t c )
    {
        (void) c;
        QMutexLocker lock ( &s_serverListMutex );
        return static_cast<int> ( s_serverList.size() );
    }

    bool jamulus_client_get_server_info ( jamulus_client_t c, int index, jamulus_server_info_t* info )
    {
        (void) c;
        if ( !info )
            return false;

        QMutexLocker lock ( &s_serverListMutex );
        if ( index < 0 || index >= static_cast<int> ( s_serverList.size() ) )
            return false;

        *info = s_serverList[static_cast<size_t> ( index )];
        return true;
    }

    void jamulus_client_clear_server_list ( jamulus_client_t c )
    {
        (void) c;
        QMutexLocker lock ( &s_serverListMutex );
        s_serverList.clear();
    }

    // ============================================================================
    // Qt Event Processing
    // ============================================================================

    void jamulus_process_events ( void )
    {
        if ( QApplication::instance() )
        {
            QApplication::processEvents ( QEventLoop::AllEvents, 10 );
        }
    }

    // ============================================================================
    // GUI functions (legacy)

    void* jamulus_gui_create ( jamulus_client_t c )
    {
        if ( !c )
            return nullptr;

        ensureQtApp();

        WrapperClient* wc = reinterpret_cast<WrapperClient*> ( c );

        // Create settings if not exists (needs CClient* and an ini file name)
        if ( !g_settings )
        {
            g_settings = new CClientSettings ( wc, QString() ); // Empty string = use default ini location
        }

        // Create dialog if not exists
        if ( !g_clientDlg )
        {
            g_clientDlg = new CClientDlg ( wc,         // CClient*
                                           g_settings, // CClientSettings*
                                           QString(),  // strConnOnStartupAddress
                                           QString(),  // strMIDISetup
                                           false,      // bShowComplRegConnList
                                           false,      // bShowAnalyzerConsole
                                           false,      // bMuteStream
                                           false,      // bEnableIPv6
                                           nullptr     // parent
            );

            // Make it frameless and non-focusable to prevent crashes
            // Don't use WindowStaysOnTopHint - it's annoying when switching apps
            g_clientDlg->setWindowFlags ( Qt::FramelessWindowHint | Qt::WindowDoesNotAcceptFocus );
            g_clientDlg->setAttribute ( Qt::WA_ShowWithoutActivating, true );

            // Register this as the embedded window to protect from focus events
            if ( g_globalFilter )
            {
                g_globalFilter->setEmbeddedWindow ( g_clientDlg );
            }

            // Hide ASIO/sound card related widgets - audio comes from host, not ASIO
            // The settings dialog is a separate top-level window, so we need to search
            // all application top-level widgets

            QStringList widgetsToHide = {
                "cbxSoundcard",                   // Sound card dropdown
                "lblSoundcardDevice",             // "Audio Device" label
                "butDriverSetup",                 // "ASIO Device Settings" button
                "FrameSoundcardChannelSelection", // Channel selection frame
                "lblInputChannelL",
                "lblInputChannelR",
                "lblOutputChannelL",
                "lblOutputChannelR",
                "cbxLInChan",
                "cbxRInChan",
                "cbxLOutChan",
                "cbxROutChan",
                "rbtBufferDelayPreferred", // Buffer delay options
                "rbtBufferDelayDefault",
                "rbtBufferDelaySafe",
                "grbSoundCrdBufDelay", // Buffer delay group
            };

            // Search in ALL top-level widgets (dialogs) and hide matching widgets
            for ( QWidget* topLevel : QApplication::topLevelWidgets() )
            {
                QList<QWidget*> allWidgets = topLevel->findChildren<QWidget*>();
                for ( QWidget* w : allWidgets )
                {
                    if ( widgetsToHide.contains ( w->objectName() ) )
                    {
                        w->hide();
                    }
                }
            }
            // Note: CloseToHideFilter is now installed globally on QApplication in ensureQtApp()
        }

        return g_clientDlg;
    }

    void jamulus_gui_destroy ( void* gui )
    {
        // Don't actually destroy - keep the singleton alive
        // The dialog will be hidden when the editor closes
        (void) gui;
    }

    // Helper to ensure window stays non-activatable
    static void ensureNoActivate ( QWidget* w )
    {
#ifdef _WIN32
        if ( w )
        {
            HWND hwnd    = reinterpret_cast<HWND> ( w->winId() );
            LONG exStyle = GetWindowLong ( hwnd, GWL_EXSTYLE );
            if ( !( exStyle & WS_EX_NOACTIVATE ) )
            {
                SetWindowLong ( hwnd, GWL_EXSTYLE, exStyle | WS_EX_NOACTIVATE );
            }
        }
#else
        (void) w;
#endif
    }

    void jamulus_gui_show ( void* gui )
    {
        if ( gui )
        {
            QWidget* w = reinterpret_cast<QWidget*> ( gui );
            w->show();

#ifdef _WIN32
            HWND hwnd = reinterpret_cast<HWND> ( w->winId() );
            // Position at (0,0) and force redraw
            SetWindowPos ( hwnd, HWND_TOP, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOACTIVATE | SWP_SHOWWINDOW | SWP_FRAMECHANGED );
            // Invalidate and force immediate repaint at Windows level
            InvalidateRect ( hwnd, NULL, TRUE );
            UpdateWindow ( hwnd );
#endif

            // Qt-level updates
            w->raise();
            w->repaint();

            // Schedule follow-up repaints
            QTimer::singleShot ( 100, w, [w]() {
                if ( w->isVisible() )
                {
#ifdef _WIN32
                    HWND hwnd = reinterpret_cast<HWND> ( w->winId() );
                    InvalidateRect ( hwnd, NULL, TRUE );
                    UpdateWindow ( hwnd );
#endif
                    w->repaint();
                }
            } );
        }
    }

    void jamulus_gui_hide ( void* gui )
    {
        if ( gui )
        {
            QWidget* w = reinterpret_cast<QWidget*> ( gui );
            w->hide();
        }
    }

    void* jamulus_gui_get_native_handle ( void* gui )
    {
        if ( !gui )
            return nullptr;
        QWidget* w = reinterpret_cast<QWidget*> ( gui );
        return reinterpret_cast<void*> ( w->winId() );
    }

    void jamulus_gui_set_parent ( void* gui, void* parentHwnd )
    {
#ifdef _WIN32
        if ( !gui || !parentHwnd )
            return;

        QWidget* w      = reinterpret_cast<QWidget*> ( gui );
        HWND     qtHwnd = reinterpret_cast<HWND> ( w->winId() );
        HWND     parent = reinterpret_cast<HWND> ( parentHwnd );

        // Make it a child window with simple style
        LONG_PTR style = GetWindowLongPtr ( qtHwnd, GWL_STYLE );
        style          = ( style & ~WS_POPUP ) | WS_CHILD;
        SetWindowLongPtr ( qtHwnd, GWL_STYLE, style );

        // Set the parent
        SetParent ( qtHwnd, parent );

        // Position at origin of parent
        SetWindowPos ( qtHwnd, HWND_TOP, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOACTIVATE | SWP_FRAMECHANGED );
#else
        (void) gui;
        (void) parentHwnd;
#endif
    }

    void jamulus_gui_resize ( void* gui, int width, int height )
    {
        if ( gui )
        {
            QWidget* w = reinterpret_cast<QWidget*> ( gui );
#ifdef _WIN32
            HWND hwnd = reinterpret_cast<HWND> ( w->winId() );
            SetWindowPos ( hwnd, HWND_TOP, 0, 0, width, height, SWP_NOACTIVATE | SWP_NOZORDER );
#else
            w->move ( 0, 0 );
            w->resize ( width, height );
#endif
        }
    }

    void jamulus_gui_position ( void* gui, int x, int y )
    {
        if ( gui )
        {
            QWidget* w = reinterpret_cast<QWidget*> ( gui );
            w->move ( x, y );
            ensureNoActivate ( w ); // Re-apply after move
        }
    }

    void jamulus_gui_get_preferred_size ( void* gui, int* width, int* height )
    {
        if ( gui && width && height )
        {
            QWidget* w    = reinterpret_cast<QWidget*> ( gui );
            QSize    hint = w->sizeHint();
            *width        = hint.width();
            *height       = hint.height();
        }
    }

    void jamulus_gui_repaint ( void* gui )
    {
        if ( gui )
        {
            QWidget* w = reinterpret_cast<QWidget*> ( gui );
            w->repaint();
            // Also repaint all child widgets
            for ( QWidget* child : w->findChildren<QWidget*>() )
            {
                child->repaint();
            }
        }
    }

    void jamulus_gui_set_active ( bool active )
    {
        // Pause or resume Qt event pump based on whether the GUI is visible/active
        if ( g_eventPumpTimer )
        {
            if ( active )
            {
                if ( !g_eventPumpTimer->isActive() )
                {
                    g_eventPumpTimer->start ( 16 );
                }
            }
            else
            {
                g_eventPumpTimer->stop();
            }
        }
    }

} // extern "C"
