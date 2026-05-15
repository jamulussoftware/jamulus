#include "jamulus_core_juce.h"
#include "jamulus_wrapper.h"
#include "plugin/DebugLogger.h"
#include "src/buffer.h"

#include <new>
#include <string>
#include <thread>
#include <atomic>
#include <vector>
#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <sstream>
#include <iomanip>
#include <mutex>
#include <deque>
#include <array>
#include <cmath>
#include <chrono>
#include <unordered_map>
#if __has_include ( "opus/opus_custom.h" )
#    include "opus/opus_custom.h"
#elif __has_include ( "opus_custom.h" )
#    include "opus_custom.h"
#elif __has_include ( "libs/opus/include/opus_custom.h" )
#    include "libs/opus/include/opus_custom.h"
#else
#    error "Unable to locate opus_custom.h"
#endif

#ifdef _WIN32
#    include <winsock2.h>
#    include <ws2tcpip.h>
#else
#    include <arpa/inet.h>
#    include <netinet/in.h>
#    include <sys/socket.h>
#    include <unistd.h>
#    include <fcntl.h>
#    include <errno.h>
#    include <netdb.h>
#endif

struct JamulusCoreServerEntry
{
    std::string address;
    std::string name;
    std::string city;
    std::string country;
    std::string version;
    std::string players;
    int         ping        = -1;
    int         numClients  = -1;
    int         maxClients  = -1;
    bool        isPermanent = false;
};

struct JamulusCodecConfig
{
    int codingType    = 2;
    int codedBytes    = 45;
    int blockFactor   = 1;
    int audioChannels = 1;
    int frameSamples  = 128;
    bool useSequenceNumber = false;
};

struct JamulusEncodedPacket
{
    std::vector<uint8_t> rawPayload;
    std::vector<uint8_t> payload;
    bool                 hasSequence = false;
    uint8_t              sequence    = 0;
};

static void jamulus_core_log ( const std::string& msg )
{
    DebugLogger::instance().log ( "[jamulus_core] " + msg );
}

static std::string jamulus_hex_dump ( const uint8_t* data, int len, int maxBytes = 32 )
{
    if ( !data || len <= 0 )
        return {};

    std::ostringstream oss;
    oss << std::hex << std::setfill ( '0' );
    const int count = std::min ( len, maxBytes );
    for ( int i = 0; i < count; ++i )
    {
        if ( i != 0 )
            oss << ' ';
        oss << std::setw ( 2 ) << static_cast<int> ( data[i] );
    }
    if ( len > maxBytes )
        oss << " ...";
    return oss.str();
}

struct JamulusAudioEngine
{
#if defined( JAMULUS_NONQT_CORE )
    bool        running      = false;
    bool        connected    = false;
    bool        autoJitter   = true;
    bool        smallBuffers = false;
    std::string serverAddr;
    std::string name;
    std::string chatMessage;
    int         instrument   = 0;
    int         country      = 0;
    int         skill        = 0;
    int         jitterBuffer = 10;
    int         serverJitterBuffer = 10;
#else
    jamulus_client_t client = nullptr;
    bool             workerMode = false;
#endif

    bool init ( unsigned short port, const char* serverAddr, const char* clientName, bool enableIPv6 )
    {
#if defined( JAMULUS_NONQT_CORE )
        (void) port;
        (void) enableIPv6;
        this->serverAddr = serverAddr ? serverAddr : "";
        this->name       = clientName ? clientName : "";
        running          = false;
        connected        = false;
        return true;
#else
        client = jamulus_client_create ( port, serverAddr, clientName, enableIPv6 );
        return client != nullptr;
#endif
    }

    void shutdown()
    {
#if defined( JAMULUS_NONQT_CORE )
        running      = false;
        connected    = false;
        chatMessage.clear();
#else
        if ( client )
        {
            jamulus_client_destroy ( client );
            client = nullptr;
        }
        workerMode = false;
#endif
    }

    int setServerAddr ( const char* addr )
    {
#if defined( JAMULUS_NONQT_CORE )
        serverAddr = addr ? addr : "";
        connected  = running && !serverAddr.empty();
        return 0;
#else
        if ( !client )
            return -1;
        return jamulus_client_set_server_addr ( client, addr ? addr : "" );
#endif
    }

    void start ( const std::string& serverAddress )
    {
#if defined( JAMULUS_NONQT_CORE )
        if ( !serverAddress.empty() )
            serverAddr = serverAddress;
        running   = true;
        connected = !serverAddr.empty();
#else
        if ( !client )
            return;
        if ( jamulus_client_is_running ( client ) )
        {
            jamulus_client_disconnect ( client );
            jamulus_client_stop ( client );
        }
        if ( !serverAddress.empty() )
            jamulus_client_set_server_addr ( client, serverAddress.c_str() );
        if ( jamulus_client_start ( client ) == 0 )
        {
            workerMode = ( jamulus_client_start_worker ( client, 128 ) == 0 );
        }
        else
        {
            workerMode = false;
        }
#endif
    }

    void stop()
    {
#if defined( JAMULUS_NONQT_CORE )
        running   = false;
        connected = false;
#else
        if ( client )
        {
            if ( workerMode )
                jamulus_client_stop_worker ( client );
            jamulus_client_stop ( client );
        }
        workerMode = false;
#endif
    }

    void disconnect()
    {
#if defined( JAMULUS_NONQT_CORE )
        connected = false;
#else
        if ( client )
            jamulus_client_disconnect ( client );
#endif
    }

    bool isRunning() const
    {
#if defined( JAMULUS_NONQT_CORE )
        return running;
#else
        return client && jamulus_client_is_running ( client );
#endif
    }

    bool isConnected() const
    {
#if defined( JAMULUS_NONQT_CORE )
        return connected;
#else
        return client && jamulus_client_is_connected ( client );
#endif
    }

    int processAudio ( const float* in, float* out, int frames, int channels )
    {
#if defined( JAMULUS_NONQT_CORE )
        if ( !out || frames <= 0 || channels <= 0 )
            return 0;
        const int total = frames * channels;
        if ( in )
            std::memcpy ( out, in, static_cast<size_t> ( total ) * sizeof ( float ) );
        else
            std::fill ( out, out + total, 0.0f );
        return 0;
#else
        if ( !client )
            return -1;
        if ( !workerMode )
            return jamulus_client_process_audio ( client, in, out, frames, channels );

        if ( !out || frames <= 0 || channels <= 0 )
            return 0;

        std::vector<float> silence;
        const float*       pushInput = in;
        if ( !pushInput )
        {
            silence.assign ( static_cast<size_t> ( frames ) * static_cast<size_t> ( channels ), 0.0f );
            pushInput = silence.data();
        }

        if ( jamulus_client_push_audio ( client, pushInput, frames, channels ) != 0 )
            return -1;

        const int got = jamulus_client_pop_audio ( client, out, frames, channels );
        if ( got < 0 )
            return got;
        if ( got < frames )
        {
            std::fill ( out + static_cast<size_t> ( got ) * static_cast<size_t> ( channels ),
                        out + static_cast<size_t> ( frames ) * static_cast<size_t> ( channels ),
                        0.0f );
        }
        return 0;
#endif
    }

    int getMyChannelId() const
    {
#if defined( JAMULUS_NONQT_CORE )
        return connected ? 0 : -1;
#else
        if ( !client )
            return -1;
        return jamulus_client_get_my_channel_id ( client );
#endif
    }

    bool getChannelInfo ( int index, jamulus_channel_info_t* info ) const
    {
#if defined( JAMULUS_NONQT_CORE )
        (void) index;
        (void) info;
        return false;
#else
        if ( !client || !info )
            return false;
        return jamulus_client_get_channel_info ( client, index, info );
#endif
    }

    int getNumChannels() const
    {
#if defined( JAMULUS_NONQT_CORE )
        return connected ? 1 : 0;
#else
        if ( !client )
            return 0;
        return jamulus_client_get_num_channels ( client );
#endif
    }

    void setChannelGain ( int channelId, float gain )
    {
#if defined( JAMULUS_NONQT_CORE )
        (void) channelId;
        (void) gain;
#else
        if ( client )
            jamulus_client_set_channel_gain ( client, channelId, gain );
#endif
    }

    void setChannelPan ( int channelId, float pan )
    {
#if defined( JAMULUS_NONQT_CORE )
        (void) channelId;
        (void) pan;
#else
        if ( client )
            jamulus_client_set_channel_pan ( client, channelId, pan );
#endif
    }

    void setChannelMute ( int channelId, bool muted )
    {
#if defined( JAMULUS_NONQT_CORE )
        (void) channelId;
        (void) muted;
#else
        if ( client )
            jamulus_client_set_channel_mute ( client, channelId, muted );
#endif
    }

    void setChannelSolo ( int channelId, bool solo )
    {
#if defined( JAMULUS_NONQT_CORE )
        (void) channelId;
        (void) solo;
#else
        if ( client )
            jamulus_client_set_channel_solo ( client, channelId, solo );
#endif
    }

    void sendChatMessage ( const char* message )
    {
#if defined( JAMULUS_NONQT_CORE )
        chatMessage = message ? message : "";
#else
        if ( client )
            jamulus_client_send_chat_message ( client, message );
#endif
    }

    void sendPing()
    {
#if defined( JAMULUS_NONQT_CORE )
#else
        if ( client )
            jamulus_client_send_ping ( client );
#endif
    }

    bool getNetworkStats ( jamulus_network_stats_t* stats ) const
    {
#if defined( JAMULUS_NONQT_CORE )
        if ( !stats )
            return false;
        std::memset ( stats, 0, sizeof ( *stats ) );
        return true;
#else
        if ( !client || !stats )
            return false;
        jamulus_client_get_network_stats ( client, stats );
        return true;
#endif
    }

    const char* getChatMessage() const
    {
#if defined( JAMULUS_NONQT_CORE )
        if ( chatMessage.empty() )
            return nullptr;
        return chatMessage.c_str();
#else
        if ( !client )
            return nullptr;
        return jamulus_client_get_chat_message ( client );
#endif
    }

    const char* getName() const
    {
#if defined( JAMULUS_NONQT_CORE )
        if ( name.empty() )
            return nullptr;
        return name.c_str();
#else
        if ( !client )
            return nullptr;
        return jamulus_client_get_name ( client );
#endif
    }

    void setName ( const char* name )
    {
#if defined( JAMULUS_NONQT_CORE )
        this->name = name ? name : "";
#else
        if ( client )
            jamulus_client_set_name ( client, name );
#endif
    }

    int getInstrument() const
    {
#if defined( JAMULUS_NONQT_CORE )
        return instrument;
#else
        if ( !client )
            return 0;
        return jamulus_client_get_instrument ( client );
#endif
    }

    void setInstrument ( int instrument )
    {
#if defined( JAMULUS_NONQT_CORE )
        this->instrument = instrument;
#else
        if ( client )
            jamulus_client_set_instrument ( client, instrument );
#endif
    }

    int getCountry() const
    {
#if defined( JAMULUS_NONQT_CORE )
        return country;
#else
        if ( !client )
            return 0;
        return jamulus_client_get_country ( client );
#endif
    }

    void setCountry ( int country )
    {
#if defined( JAMULUS_NONQT_CORE )
        this->country = country;
#else
        if ( client )
            jamulus_client_set_country ( client, country );
#endif
    }

    int getSkill() const
    {
#if defined( JAMULUS_NONQT_CORE )
        return skill;
#else
        if ( !client )
            return 0;
        return jamulus_client_get_skill ( client );
#endif
    }

    void setSkill ( int skill )
    {
#if defined( JAMULUS_NONQT_CORE )
        this->skill = skill;
#else
        if ( client )
            jamulus_client_set_skill ( client, skill );
#endif
    }

    bool getAutoJitter() const
    {
#if defined( JAMULUS_NONQT_CORE )
        return autoJitter;
#else
        if ( !client )
            return true;
        return jamulus_client_get_auto_jitter ( client );
#endif
    }

    void setAutoJitter ( bool enabled )
    {
#if defined( JAMULUS_NONQT_CORE )
        autoJitter = enabled;
#else
        if ( client )
            jamulus_client_set_auto_jitter ( client, enabled );
#endif
    }

    int getJitterBuffer() const
    {
#if defined( JAMULUS_NONQT_CORE )
        return jitterBuffer;
#else
        if ( !client )
            return 10;
        return jamulus_client_get_jitter_buffer ( client );
#endif
    }

    void setJitterBuffer ( int size )
    {
#if defined( JAMULUS_NONQT_CORE )
        jitterBuffer = size;
#else
        if ( client )
            jamulus_client_set_jitter_buffer ( client, size );
#endif
    }

    int getServerJitterBuffer() const
    {
#if defined( JAMULUS_NONQT_CORE )
        return serverJitterBuffer;
#else
        return getJitterBuffer();
#endif
    }

    void setServerJitterBuffer ( int size )
    {
#if defined( JAMULUS_NONQT_CORE )
        serverJitterBuffer = size;
#else
        setJitterBuffer ( size );
#endif
    }

    bool getSmallBuffers() const
    {
#if defined( JAMULUS_NONQT_CORE )
        return smallBuffers;
#else
        if ( !client )
            return false;
        return jamulus_client_get_small_buffers ( client );
#endif
    }

    void setSmallBuffers ( bool enabled )
    {
#if defined( JAMULUS_NONQT_CORE )
        smallBuffers = enabled;
#else
        if ( client )
            jamulus_client_set_small_buffers ( client, enabled );
#endif
    }
};

struct JamulusCoreInstance
{
    unsigned short             port          = 0;
    std::string                serverAddress;
    std::string                clientName;
    bool                       enableIPv6    = false;
    std::atomic<bool>          runningFlag{ false };
    std::atomic<bool>          connectedFlag{ false };
    std::thread                workerThread;
    std::atomic<bool>          workerStopRequested{ false };
    std::vector<float>         inputBuffer;
    std::vector<float>         outputBuffer;
    std::atomic<uint64_t>      processedFrames{ 0 };

    int                        udpSocket     = -1;
    sockaddr_storage           serverSockAddr{};
    socklen_t                  serverSockAddrLen = 0;

    std::string                profileName;
    int                        profileInstrument = 0;
    int                        profileCountry    = 0;
    std::string                profileCountryCode;
    std::string                profileCity;
    int                        profileSkill      = 0;

    std::mutex                          serverListMutex;
    std::vector<JamulusCoreServerEntry> serverList;

    std::mutex                          channelMutex;
    std::vector<jamulus_channel_info_t> channels;
    std::atomic<int>                    myChannelId{ -1 };
    std::atomic<int>                    lastPingMs{ -1 };
    std::atomic<int>                    lastTotalDelayMs{ 0 };
    std::atomic<int>                    lastJitterStatus{ 0 };

    struct PendingPanUpdate
    {
        float lastSentPan = 0.0f;
        float pendingPan  = 0.0f;
        bool  hasPending  = false;
        bool  hasLastSent = false;
    };

    std::mutex                          panRateMutex;
    std::unordered_map<int, PendingPanUpdate> pendingPanUpdates;
    uint32_t                            nextPanSendAtMs = 0;

    std::mutex                          audioMutex;
    std::mutex                          lifecycleMutex;
    JamulusCodecConfig                  codecConfig;
    std::deque<JamulusEncodedPacket>    encodedIncomingPackets;
    std::deque<int16_t>                 decodedPcmQueue;
    CNetBufWithStats                    sockBuf;
    CVector<uint8_t>                    sockBufPacket;
    std::vector<int16_t>                encodePcm;
    std::vector<int16_t>                decodePcm;
    std::vector<float>                  decodeFloatPcm;
    std::vector<uint8_t>                encodedTempPacket;
    OpusCustomMode*                     opusMode              = nullptr;
    OpusCustomMode*                     opus64Mode            = nullptr;
    OpusCustomEncoder*                  opusEncoder           = nullptr;
    OpusCustomDecoder*                  opusDecoder           = nullptr;
    int                                 opusCodecType         = 2;
    int                                 opusCodecChannels     = 1;
    int                                 opusCodecFrameSamples = 128;
    uint8_t                             sendSequenceNumber    = 0;
    std::atomic<uint8_t>                nextProtocolCounter{ 0 };
    std::atomic<uint64_t>               audioPacketsQueued{ 0 };
    std::atomic<uint64_t>               audioPacketsRejected{ 0 };
    std::atomic<uint64_t>               audioRejectTooLarge{ 0 };
    std::atomic<uint64_t>               audioRejectFramedMessage{ 0 };
    std::atomic<uint64_t>               audioRejectSizeMismatch{ 0 };
    std::atomic<uint64_t>               wrappedAudioQueued{ 0 };
    std::atomic<uint64_t>               wrappedAudioRejected{ 0 };
    std::atomic<uint64_t>               decodeAttempts{ 0 };
    std::atomic<uint64_t>               decodeSuccess{ 0 };
    std::atomic<uint64_t>               decodeFail{ 0 };
    std::atomic<uint64_t>               decodedSamples{ 0 };
    std::atomic<uint64_t>               mixedFramesWithSignal{ 0 };
    std::atomic<uint64_t>               channelLevelMessages{ 0 };
    std::atomic<uint64_t>               channelLevelUpdates{ 0 };
    std::atomic<uint32_t>               lastAudioDebugLogMs{ 0 };
    std::atomic<uint32_t>               lastAudioRejectLogMs{ 0 };
    std::atomic<uint32_t>               lastDecodeFailLogMs{ 0 };
    std::atomic<bool>                   gotNetTransportProps{ false };
    std::atomic<bool>                   codecConfigChanged{ false };
    std::atomic<bool>                   bootstrapMessagesSent{ false };
    bool                                receiveSequenceLocked = false;
    uint8_t                             expectedReceiveSequence = 0;
    int                                 jitterTargetFrames = DEF_NET_BUF_SIZE_NUM_BL;
    int                                 jitterUnderRuns    = 0;
    int                                 jitterOverRuns     = 0;
    float                               streamOutputGain   = 0.0f;
    int                                 streamNoDataBlocks = 0;

    JamulusAudioEngine audioEngine;
};

static int jamulus_calculate_upload_rate_kbps ( const JamulusCodecConfig& cfg )
{
    const int audioFrameSamples = std::max ( 1, cfg.frameSamples );
    const int blockFactor       = std::max ( 1, cfg.blockFactor );
    const int codedBytes        = std::max ( 1, cfg.codedBytes );
    const int audioSizeOut      = blockFactor * audioFrameSamples;

    // Match the upstream-rate estimate used by the original client.
    return ( codedBytes * blockFactor + 28 + 26 + 23 ) * 8 * SYSTEM_SAMPLE_RATE_HZ / audioSizeOut / 1000;
}

static constexpr uint16_t JAMULUS_CLM_PING_MS                = 1001;
static constexpr uint16_t JAMULUS_CLM_PING_MS_WITHNUMCLIENTS = 1002;
static constexpr uint16_t JAMULUS_CLM_SERVER_LIST            = 1006;
static constexpr uint16_t JAMULUS_CLM_REQ_SERVER_LIST        = 1007;
static constexpr uint16_t JAMULUS_CLM_VERSION_AND_OS         = 1011;
static constexpr uint16_t JAMULUS_CLM_REQ_VERSION_AND_OS     = 1012;
static constexpr uint16_t JAMULUS_CLM_CONN_CLIENTS_LIST      = 1013;
static constexpr uint16_t JAMULUS_CLM_REQ_CONN_CLIENTS_LIST  = 1014;
static constexpr uint16_t JAMULUS_CLM_RED_SERVER_LIST        = 1018;
static constexpr uint16_t JAMULUS_CLM_CHANNEL_LEVEL_LIST     = 1015;
static constexpr uint16_t JAMULUS_PM_ACKN                    = 1;
static constexpr uint16_t JAMULUS_PM_JITT_BUF_SIZE           = 10;
static constexpr uint16_t JAMULUS_PM_REQ_JITT_BUF_SIZE       = 11;
static constexpr uint16_t JAMULUS_PM_REQ_CONN_CLIENTS_LIST   = 16;
static constexpr uint16_t JAMULUS_PM_CONN_CLIENTS_LIST       = 24;
static constexpr uint16_t JAMULUS_PM_NETW_TRANSPORT_PROPS    = 20;
static constexpr uint16_t JAMULUS_PM_REQ_NETW_TRANSPORT_PROPS = 21;
static constexpr uint16_t JAMULUS_PM_REQ_CHANNEL_INFOS       = 23;
static constexpr uint16_t JAMULUS_PM_CHANNEL_INFOS           = 25;
static constexpr uint16_t JAMULUS_PM_OPUS_SUPPORTED          = 26;
static constexpr uint16_t JAMULUS_PM_REQ_CHANNEL_LEVEL_LIST  = 28;
static constexpr uint16_t JAMULUS_PM_VERSION_AND_OS          = 29;
static constexpr uint16_t JAMULUS_PM_CHANNEL_GAIN            = 13;
static constexpr uint16_t JAMULUS_PM_CHANNEL_PAN             = 30;
static constexpr uint16_t JAMULUS_PM_MUTE_STATE_CHANGED      = 31;
static constexpr uint16_t JAMULUS_PM_CLIENT_ID               = 32;
static constexpr uint16_t JAMULUS_PM_REQ_SPLIT_SUPPORT       = 34;
static constexpr uint16_t JAMULUS_PM_SPLIT_SUPPORTED         = 35;
static constexpr int      JAMULUS_CODING_TYPE_OPUS           = 2;
static constexpr int      JAMULUS_CODING_TYPE_OPUS64         = 3;
static constexpr int      JAMULUS_SAMPLE_RATE_HZ             = 48000;
static constexpr int      JAMULUS_FRAME_SAMPLES_64           = 64;
static constexpr int      JAMULUS_FRAME_SAMPLES_128          = 128;
static constexpr int      JAMULUS_AUTO_JITTER_PROTOCOL       = 21;
static constexpr uint16_t JAMULUS_SPECIAL_SPLIT_MESSAGE      = 2001;
static constexpr int      JAMULUS_MESS_HEADER_LENGTH_BYTE    = 7;
static constexpr int      JAMULUS_OPUS64_MONO_NORMAL_BYTES   = 22;
static constexpr int      JAMULUS_OPUS_MONO_NORMAL_BYTES     = 45;
static constexpr int      JAMULUS_OPUS64_STEREO_NORMAL_BYTES = 35;
static constexpr int      JAMULUS_OPUS_STEREO_NORMAL_BYTES   = 71;

static uint32_t jamulus_get_time_ms()
{
    using namespace std::chrono;
    auto now = steady_clock::now().time_since_epoch();
    return static_cast<uint32_t> ( duration_cast<milliseconds> ( now ).count() );
}

static int jamulus_seq_forward_distance ( uint8_t expected, uint8_t value )
{
    return static_cast<int> ( ( static_cast<uint8_t> ( value - expected ) ) );
}

struct JamulusCountryMapEntry
{
    uint16_t    wire;
    const char* iso;
    const char* name;
};

static const JamulusCountryMapEntry kJamulusCountryMap[] = {
    { 1, "af", "Afghanistan" },
    { 2, "al", "Albania" },
    { 3, "dz", "Algeria" },
    { 4, "as", "American Samoa" },
    { 5, "ad", "Andorra" },
    { 6, "ao", "Angola" },
    { 7, "ai", "Anguilla" },
    { 8, "aq", "Antarctica" },
    { 9, "ag", "Antigua and Barbuda" },
    { 10, "ar", "Argentina" },
    { 11, "am", "Armenia" },
    { 12, "aw", "Aruba" },
    { 13, "au", "Australia" },
    { 14, "at", "Austria" },
    { 15, "az", "Azerbaijan" },
    { 16, "bs", "Bahamas" },
    { 17, "bh", "Bahrain" },
    { 18, "bd", "Bangladesh" },
    { 19, "bb", "Barbados" },
    { 20, "by", "Belarus" },
    { 21, "be", "Belgium" },
    { 22, "bz", "Belize" },
    { 23, "bj", "Benin" },
    { 24, "bm", "Bermuda" },
    { 25, "bt", "Bhutan" },
    { 26, "bo", "Bolivia" },
    { 27, "ba", "Bosnia and Herzegovina" },
    { 28, "bw", "Botswana" },
    { 29, "bv", "Bouvet Island" },
    { 30, "br", "Brazil" },
    { 31, "io", "British Indian Ocean Territory" },
    { 32, "bn", "Brunei" },
    { 33, "bg", "Bulgaria" },
    { 34, "bf", "Burkina Faso" },
    { 35, "bi", "Burundi" },
    { 36, "kh", "Cambodia" },
    { 37, "cm", "Cameroon" },
    { 38, "ca", "Canada" },
    { 39, "cv", "Cape Verde" },
    { 40, "ky", "Cayman Islands" },
    { 41, "cf", "Central African Republic" },
    { 42, "td", "Chad" },
    { 43, "cl", "Chile" },
    { 44, "cn", "China" },
    { 45, "cx", "Christmas Island" },
    { 46, "cc", "Cocos Islands" },
    { 47, "co", "Colombia" },
    { 48, "km", "Comoros" },
    { 49, "cd", "Congo - Kinshasa" },
    { 50, "cg", "Congo - Brazzaville" },
    { 51, "ck", "Cook Islands" },
    { 52, "cr", "Costa Rica" },
    { 53, "jm", "Jamaica" },
    { 54, "hr", "Croatia" },
    { 55, "cu", "Cuba" },
    { 56, "cy", "Cyprus" },
    { 57, "cz", "Czechia" },
    { 58, "dk", "Denmark" },
    { 59, "dj", "Djibouti" },
    { 60, "dm", "Dominica" },
    { 61, "do", "Dominican Republic" },
    { 62, "tk", "Tokelau" },
    { 63, "ec", "Ecuador" },
    { 64, "eg", "Egypt" },
    { 65, "sv", "El Salvador" },
    { 66, "gq", "Equatorial Guinea" },
    { 67, "er", "Eritrea" },
    { 68, "ee", "Estonia" },
    { 69, "et", "Ethiopia" },
    { 70, "fo", "Faroe Islands" },
    { 71, "fj", "Fiji" },
    { 72, "fi", "Finland" },
    { 73, "fr", "France" },
    { 74, "gf", "French Guiana" },
    { 75, "gw", "Guinea-Bissau" },
    { 76, "pf", "French Polynesia" },
    { 77, "tf", "French Southern Territories" },
    { 78, "ga", "Gabon" },
    { 79, "gm", "Gambia" },
    { 80, "ge", "Georgia" },
    { 81, "de", "Germany" },
    { 82, "gh", "Ghana" },
    { 83, "gi", "Gibraltar" },
    { 84, "gr", "Greece" },
    { 85, "gl", "Greenland" },
    { 86, "gd", "Grenada" },
    { 87, "gp", "Guadeloupe" },
    { 88, "gu", "Guam" },
    { 89, "gt", "Guatemala" },
    { 90, "gg", "Guernsey" },
    { 91, "gy", "Guyana" },
    { 92, "gn", "Guinea" },
    { 93, "ht", "Haiti" },
    { 94, "hm", "Heard and McDonald Islands" },
    { 95, "hn", "Honduras" },
    { 96, "hk", "Hong Kong" },
    { 97, "hu", "Hungary" },
    { 98, "is", "Iceland" },
    { 99, "in", "India" },
    { 100, "id", "Indonesia" },
    { 101, "ir", "Iran" },
    { 102, "iq", "Iraq" },
    { 103, "ie", "Ireland" },
    { 104, "im", "Isle of Man" },
    { 105, "it", "Italy" },
    { 106, "ci", "Ivory Coast" },
    { 107, "jp", "Japan" },
    { 108, "je", "Jersey" },
    { 109, "kz", "Kazakhstan" },
    { 110, "ke", "Kenya" },
    { 111, "ki", "Kiribati" },
    { 112, "xk", "Kosovo" },
    { 113, "om", "Oman" },
    { 114, "es", "Spain" },
    { 115, "kg", "Kyrgyzstan" },
    { 116, "la", "Laos" },
    { 117, "lv", "Latvia" },
    { 118, "ls", "Lesotho" },
    { 119, "lr", "Liberia" },
    { 120, "ly", "Libya" },
    { 121, "li", "Liechtenstein" },
    { 122, "lt", "Lithuania" },
    { 123, "lu", "Luxembourg" },
    { 124, "mo", "Macao" },
    { 125, "mk", "Macedonia" },
    { 126, "mg", "Madagascar" },
    { 127, "mw", "Malawi" },
    { 128, "my", "Malaysia" },
    { 129, "mv", "Maldives" },
    { 130, "ml", "Mali" },
    { 131, "mt", "Malta" },
    { 132, "mh", "Marshall Islands" },
    { 133, "mq", "Martinique" },
    { 134, "mr", "Mauritania" },
    { 135, "mu", "Mauritius" },
    { 136, "yt", "Mayotte" },
    { 137, "mx", "Mexico" },
    { 138, "fm", "Micronesia" },
    { 139, "md", "Moldova" },
    { 140, "mc", "Monaco" },
    { 141, "mn", "Mongolia" },
    { 142, "me", "Montenegro" },
    { 143, "ms", "Montserrat" },
    { 144, "mz", "Mozambique" },
    { 145, "mm", "Myanmar" },
    { 146, "na", "Namibia" },
    { 147, "nr", "Nauru" },
    { 148, "np", "Nepal" },
    { 149, "nl", "Netherlands" },
    { 150, "nc", "New Caledonia" },
    { 151, "nz", "New Zealand" },
    { 152, "cw", "Curacao" },
    { 153, "ni", "Nicaragua" },
    { 154, "ng", "Nigeria" },
    { 155, "ne", "Niger" },
    { 156, "nf", "Norfolk Island" },
    { 157, "nu", "Niue" },
    { 158, "mp", "Northern Mariana Islands" },
    { 159, "kp", "North Korea" },
    { 160, "no", "Norway" },
    { 161, "qo", "Outlying Oceania" },
    { 162, "pk", "Pakistan" },
    { 163, "ps", "Palestinian Territories" },
    { 164, "pa", "Panama" },
    { 165, "pg", "Papua New Guinea" },
    { 166, "py", "Paraguay" },
    { 167, "pe", "Peru" },
    { 168, "ph", "Philippines" },
    { 169, "pn", "Pitcairn" },
    { 170, "pl", "Poland" },
    { 171, "pt", "Portugal" },
    { 172, "pr", "Puerto Rico" },
    { 173, "qa", "Qatar" },
    { 174, "re", "Reunion" },
    { 175, "ro", "Romania" },
    { 176, "ru", "Russia" },
    { 177, "rw", "Rwanda" },
    { 178, "bl", "Saint Barthelemy" },
    { 179, "sh", "Saint Helena" },
    { 180, "mf", "Saint Martin" },
    { 181, "pm", "Saint Pierre and Miquelon" },
    { 182, "sm", "San Marino" },
    { 183, "st", "Sao Tome and Principe" },
    { 184, "sa", "Saudi Arabia" },
    { 185, "sn", "Senegal" },
    { 186, "rs", "Serbia" },
    { 187, "sc", "Seychelles" },
    { 188, "sg", "Singapore" },
    { 189, "sx", "Sint Maarten" },
    { 190, "sk", "Slovakia" },
    { 191, "sb", "Solomon Islands" },
    { 192, "so", "Somalia" },
    { 193, "za", "South Africa" },
    { 194, "gs", "South Georgia and South Sandwich Islands" },
    { 195, "kr", "South Korea" },
    { 196, "ss", "South Sudan" },
    { 197, "sd", "Sudan" },
    { 198, "sr", "Suriname" },
    { 199, "lc", "Saint Lucia" },
    { 200, "ws", "Samoa" },
    { 201, "sj", "Svalbard and Jan Mayen" },
    { 202, "se", "Sweden" },
    { 203, "ch", "Switzerland" },
    { 204, "sz", "Eswatini" },
    { 205, "sy", "Syria" },
    { 206, "tw", "Taiwan" },
    { 207, "tj", "Tajikistan" },
    { 208, "tz", "Tanzania" },
    { 209, "th", "Thailand" },
    { 210, "tl", "Timor-Leste" },
    { 211, "tg", "Togo" },
    { 212, "to", "Tonga" },
    { 213, "tt", "Trinidad and Tobago" },
    { 214, "ta", "Tristan da Cunha" },
    { 215, "tn", "Tunisia" },
    { 216, "tm", "Turkmenistan" },
    { 217, "tc", "Turks and Caicos Islands" },
    { 218, "tv", "Tuvalu" },
    { 219, "ug", "Uganda" },
    { 220, "ua", "Ukraine" },
    { 221, "ae", "United Arab Emirates" },
    { 222, "gb", "United Kingdom" },
    { 223, "um", "United States Outlying Islands" },
    { 224, "us", "United States" },
    { 225, "uy", "Uruguay" },
    { 226, "vi", "United States Virgin Islands" },
    { 227, "vu", "Vanuatu" },
    { 228, "va", "Vatican City" },
    { 229, "ve", "Venezuela" },
    { 230, "vn", "Vietnam" },
    { 231, "wf", "Wallis and Futuna" },
    { 232, "eh", "Western Sahara" },
    { 233, "vg", "British Virgin Islands" },
    { 234, "uz", "Uzbekistan" },
    { 235, "ye", "Yemen" },
    { 236, "zm", "Zambia" },
    { 238, "ic", "Canary Islands" },
    { 241, "cp", "Clipperton Island" },
    { 242, "ma", "Morocco" },
    { 243, "sl", "Sierra Leone" },
    { 244, "kn", "Saint Kitts and Nevis" },
    { 245, "vc", "Saint Vincent and Grenadines" },
    { 246, "lb", "Lebanon" },
    { 247, "ac", "Ascension Island" },
    { 248, "ax", "Aland Islands" },
    { 249, "dg", "Diego Garcia" },
    { 250, "ea", "Ceuta and Melilla" },
    { 251, "il", "Israel" },
    { 252, "jo", "Jordan" },
    { 253, "tr", "Turkey" },
    { 254, "lk", "Sri Lanka" },
    { 255, "bq", "Caribbean Netherlands" },
    { 256, "si", "Slovenia" },
    { 257, "kw", "Kuwait" },
    { 258, "fk", "Falkland Islands" },
    { 259, "pw", "Palau" },
    { 260, "zw", "Zimbabwe" },
    { 261, "eu", "European Union" },
};
static constexpr int kJamulusCountryMapSize = static_cast<int> ( sizeof ( kJamulusCountryMap ) / sizeof ( kJamulusCountryMap[0] ) );

static const JamulusCountryMapEntry* jamulus_country_entry_from_wire ( uint16_t wireCode )
{
    for ( int i = 0; i < kJamulusCountryMapSize; ++i )
    {
        if ( kJamulusCountryMap[i].wire == wireCode )
            return &kJamulusCountryMap[i];
    }
    return nullptr;
}

static std::string jamulus_country_iso_from_wire ( uint16_t wireCode )
{
    if ( wireCode == 0 )
        return std::string();
    const auto* entry = jamulus_country_entry_from_wire ( wireCode );
    return entry ? std::string ( entry->iso ) : std::string();
}

static std::string jamulus_country_name_from_wire ( uint16_t wireCode )
{
    if ( wireCode == 0 )
        return std::string ( "None" );
    const auto* entry = jamulus_country_entry_from_wire ( wireCode );
    return entry ? std::string ( entry->name ) : std::string();
}

static constexpr int JAMULUS_MESS_LEN_WITHOUT_DATA = JAMULUS_MESS_HEADER_LENGTH_BYTE + 2;

struct JamulusCrc
{
    uint32_t poly;
    uint32_t bitOutMask;
    uint32_t stateShiftReg;

    JamulusCrc()
        : poly ( ( 1u << 5 ) | ( 1u << 12 ) ),
          bitOutMask ( 1u << 16 )
    {
        reset();
    }

    void reset()
    {
        stateShiftReg = ~uint32_t ( 0 );
    }

    void addByte ( uint8_t b )
    {
        for ( int i = 0; i < 8; ++i )
        {
            stateShiftReg <<= 1;
            if ( ( stateShiftReg & bitOutMask ) != 0 )
                stateShiftReg |= 1u;

            if ( ( b & ( 1u << ( 8 - i - 1 ) ) ) != 0 )
                stateShiftReg ^= 1u;

            if ( ( stateShiftReg & 1u ) != 0 )
                stateShiftReg ^= poly;
        }
    }

    uint16_t get() 
    {
        stateShiftReg = ~stateShiftReg;
        return static_cast<uint16_t> ( stateShiftReg & ( bitOutMask - 1u ) );
    }
};

static void jamulus_put_val ( std::vector<uint8_t>& buf, int& pos, uint32_t value, int numBytes )
{
    if ( numBytes <= 0 || numBytes > 4 )
        return;

    if ( static_cast<int> ( buf.size() ) < pos + numBytes )
        return;

    for ( int i = 0; i < numBytes; ++i )
    {
        buf[static_cast<size_t> ( pos )] = static_cast<uint8_t> ( ( value >> ( i * 8 ) ) & 0xffu );
        ++pos;
    }
}

static std::vector<uint8_t> jamulus_build_frame ( uint16_t id, uint8_t cnt, const std::vector<uint8_t>& data )
{
    const int dataLen  = static_cast<int> ( data.size() );
    const int totalLen = JAMULUS_MESS_LEN_WITHOUT_DATA + dataLen;

    std::vector<uint8_t> frame ( static_cast<size_t> ( totalLen ) );
    int                  pos = 0;

    jamulus_put_val ( frame, pos, 0u, 2 );
    jamulus_put_val ( frame, pos, id, 2 );
    jamulus_put_val ( frame, pos, cnt, 1 );
    jamulus_put_val ( frame, pos, static_cast<uint32_t> ( dataLen ), 2 );

    for ( int i = 0; i < dataLen; ++i )
        jamulus_put_val ( frame, pos, data[static_cast<size_t> ( i )], 1 );

    JamulusCrc crc;
    int        crcPos     = 0;
    const int  crcLenCalc = JAMULUS_MESS_HEADER_LENGTH_BYTE + dataLen;

    for ( int i = 0; i < crcLenCalc; ++i )
    {
        uint8_t b = frame[static_cast<size_t> ( crcPos )];
        ++crcPos;
        crc.addByte ( b );
    }

    jamulus_put_val ( frame, crcPos, crc.get(), 2 );
    return frame;
}

static uint32_t jamulus_get_val ( const uint8_t* buf, int bufLen, int& pos, int numBytes )
{
    if ( numBytes <= 0 || numBytes > 4 )
        return 0;
    if ( pos + numBytes > bufLen )
        return 0;

    uint32_t value = 0;
    for ( int i = 0; i < numBytes; ++i )
    {
        value |= static_cast<uint32_t> ( buf[pos++] ) << ( i * 8 );
    }
    return value;
}

static bool jamulus_is_valid_frame ( const uint8_t* buf, int bufLen )
{
    if ( !buf || bufLen < JAMULUS_MESS_LEN_WITHOUT_DATA )
        return false;

    if ( buf[0] != 0 || buf[1] != 0 )
        return false;

    const uint16_t dataLen = static_cast<uint16_t> ( buf[5] | ( static_cast<uint16_t> ( buf[6] ) << 8 ) );
    const int expectedLen = JAMULUS_MESS_HEADER_LENGTH_BYTE + static_cast<int> ( dataLen ) + 2;
    if ( expectedLen != bufLen )
        return false;

    JamulusCrc crc;
    for ( int i = 0; i < JAMULUS_MESS_HEADER_LENGTH_BYTE + static_cast<int> ( dataLen ); ++i )
        crc.addByte ( buf[i] );

    const uint16_t actualCrc = static_cast<uint16_t> ( buf[expectedLen - 2] | ( static_cast<uint16_t> ( buf[expectedLen - 1] ) << 8 ) );
    return crc.get() == actualCrc;
}

static std::string jamulus_sockaddr_to_address_string ( const sockaddr_storage& addr, socklen_t addrLen )
{
    if ( addr.ss_family == AF_INET && addrLen >= static_cast<socklen_t> ( sizeof ( sockaddr_in ) ) )
    {
        const sockaddr_in* addr4 = reinterpret_cast<const sockaddr_in*> ( &addr );
        char               ipBuf[INET_ADDRSTRLEN] = {};
        if ( !inet_ntop ( AF_INET, &addr4->sin_addr, ipBuf, sizeof ( ipBuf ) ) )
            return std::string();

        const unsigned short port = ntohs ( addr4->sin_port );
        std::string          result ( ipBuf );
        result.push_back ( ':' );
        result += std::to_string ( port );
        return result;
    }

    if ( addr.ss_family == AF_INET6 && addrLen >= static_cast<socklen_t> ( sizeof ( sockaddr_in6 ) ) )
    {
        const sockaddr_in6* addr6 = reinterpret_cast<const sockaddr_in6*> ( &addr );
        char                ipBuf[INET6_ADDRSTRLEN] = {};
        if ( !inet_ntop ( AF_INET6, &addr6->sin6_addr, ipBuf, sizeof ( ipBuf ) ) )
            return std::string();

        const unsigned short port = ntohs ( addr6->sin6_port );
        std::string          result;
        result.reserve ( std::strlen ( ipBuf ) + 8 );
        result.push_back ( '[' );
        result += ipBuf;
        result.push_back ( ']' );
        result.push_back ( ':' );
        result += std::to_string ( port );
        return result;
    }

    return std::string();
}

static bool jamulus_parse_host_port ( const std::string& address, std::string& host, unsigned short& port )
{
    host = address;
    port = 22124;

    if ( address.empty() )
        return false;

    if ( address.front() == '[' )
    {
        const std::size_t close = address.find ( ']' );
        if ( close == std::string::npos )
            return false;
        host = address.substr ( 1, close - 1 );
        if ( close + 1 < address.size() && address[close + 1] == ':' )
        {
            const std::string portStr = address.substr ( close + 2 );
            const int         parsed  = std::atoi ( portStr.c_str() );
            if ( parsed > 0 && parsed <= 65535 )
                port = static_cast<unsigned short> ( parsed );
        }
        return !host.empty();
    }

    const std::size_t lastColon  = address.rfind ( ':' );
    const std::size_t firstColon = address.find ( ':' );
    if ( lastColon != std::string::npos && lastColon == firstColon )
    {
        host = address.substr ( 0, lastColon );
        const std::string portStr = address.substr ( lastColon + 1 );
        const int         parsed  = std::atoi ( portStr.c_str() );
        if ( parsed > 0 && parsed <= 65535 )
            port = static_cast<unsigned short> ( parsed );
    }

    return !host.empty();
}

static std::string jamulus_selected_server_address ( const JamulusCoreInstance* instance )
{
    if ( !instance )
        return std::string();

    if ( instance->serverSockAddrLen > 0 )
    {
        const std::string resolved = jamulus_sockaddr_to_address_string ( instance->serverSockAddr, instance->serverSockAddrLen );
        if ( !resolved.empty() )
            return resolved;
    }

    return instance->serverAddress;
}

static bool jamulus_is_selected_server ( const JamulusCoreInstance* instance, const std::string& fromAddress )
{
    if ( !instance || fromAddress.empty() )
        return false;

    const std::string selected = jamulus_selected_server_address ( instance );
    if ( selected.empty() )
        return false;

    if ( selected == fromAddress )
        return true;

    std::string    selectedHost;
    std::string    fromHost;
    unsigned short selectedPort = 0;
    unsigned short fromPort     = 0;
    if ( jamulus_parse_host_port ( selected, selectedHost, selectedPort ) && jamulus_parse_host_port ( fromAddress, fromHost, fromPort ) )
    {
        if ( selectedHost == fromHost && selectedPort == fromPort )
            return true;
        if ( selectedHost == fromHost )
            return true;
    }

    return false;
}

static void jamulus_update_network_stats ( JamulusCoreInstance* instance, int pingMs )
{
    if ( !instance )
        return;

    const int clampedPing = std::max ( -1, pingMs );

    if ( clampedPing < 0 )
    {
        instance->lastPingMs.store ( -1, std::memory_order_relaxed );
        instance->lastTotalDelayMs.store ( 0, std::memory_order_relaxed );
        instance->lastJitterStatus.store ( 0, std::memory_order_relaxed );
        return;
    }

    const int previousPing = instance->lastPingMs.load ( std::memory_order_relaxed );
    int       filteredPing = clampedPing;
    if ( previousPing >= 0 )
    {
        const float alpha = 0.2f;
        filteredPing = static_cast<int> ( std::lround ( static_cast<float> ( previousPing ) +
                                                         ( static_cast<float> ( clampedPing - previousPing ) * alpha ) ) );
    }
    instance->lastPingMs.store ( filteredPing, std::memory_order_relaxed );

    const int jitterBufferMs = std::max ( 0, instance->audioEngine.getJitterBuffer() );
    const int totalDelayMs   = filteredPing + jitterBufferMs + 8;
    int       jitterStatus   = instance->lastJitterStatus.load ( std::memory_order_relaxed );
    if ( jitterStatus >= 2 )
    {
        if ( filteredPing <= 75 )
            jitterStatus = 1;
    }
    else if ( jitterStatus == 1 )
    {
        if ( filteredPing > 85 )
            jitterStatus = 2;
        else if ( filteredPing < 40 )
            jitterStatus = 0;
    }
    else
    {
        if ( filteredPing > 50 )
            jitterStatus = 1;
    }

    instance->lastTotalDelayMs.store ( totalDelayMs, std::memory_order_relaxed );
    instance->lastJitterStatus.store ( jitterStatus, std::memory_order_relaxed );
}

static int jamulus_infer_my_channel_id ( const JamulusCoreInstance* instance, const std::vector<jamulus_channel_info_t>& channels )
{
    if ( !instance || channels.empty() )
        return -1;

    std::vector<int> exactMatches;
    exactMatches.reserve ( channels.size() );

    for ( const auto& ch : channels )
    {
        if ( ch.id < 0 )
            continue;

        bool nameMatch = false;
        if ( !instance->profileName.empty() )
            nameMatch = ( instance->profileName == std::string ( ch.name ) );

        bool instrumentMatch = ( instance->profileInstrument > 0 && ch.instrument == instance->profileInstrument );

        if ( nameMatch && instrumentMatch )
            exactMatches.push_back ( ch.id );
    }

    if ( exactMatches.size() == 1 )
        return exactMatches.front();

    if ( !instance->profileName.empty() )
    {
        std::vector<int> nameOnlyMatches;
        nameOnlyMatches.reserve ( channels.size() );
        for ( const auto& ch : channels )
        {
            if ( ch.id < 0 )
                continue;
            if ( instance->profileName == std::string ( ch.name ) )
                nameOnlyMatches.push_back ( ch.id );
        }
        if ( nameOnlyMatches.size() == 1 )
            return nameOnlyMatches.front();
    }

    return -1;
}

static void jamulus_core_close_socket ( JamulusCoreInstance* instance )
{
    if ( !instance )
        return;

    if ( instance->udpSocket < 0 )
        return;

#ifdef _WIN32
    closesocket ( instance->udpSocket );
#else
    close ( instance->udpSocket );
#endif

    instance->udpSocket        = -1;
    instance->serverSockAddrLen = 0;
}

static bool jamulus_core_open_socket ( JamulusCoreInstance* instance )
{
    if ( !instance )
        return false;

    if ( instance->udpSocket < 0 )
    {
#ifdef _WIN32
        WSADATA wsa;
        WSAStartup ( MAKEWORD ( 2, 2 ), &wsa );
#endif

        instance->udpSocket = static_cast<int> ( ::socket ( AF_INET, SOCK_DGRAM, 0 ) );
        if ( instance->udpSocket < 0 )
            return false;

    #ifdef _WIN32
        {
            u_long mode = 1;
            ioctlsocket ( instance->udpSocket, FIONBIO, &mode );
        }
    #else
        {
            int flags = fcntl ( instance->udpSocket, F_GETFL, 0 );
            if ( flags >= 0 )
            {
                fcntl ( instance->udpSocket, F_SETFL, flags | O_NONBLOCK );
            }
        }
    #endif

        sockaddr_in addr{};
        addr.sin_family      = AF_INET;
        // If port is 0, let OS choose an ephemeral local port instead of colliding with server port.
        addr.sin_port        = htons ( instance->port != 0 ? instance->port : 0 );
        addr.sin_addr.s_addr = INADDR_ANY;

        if ( ::bind ( instance->udpSocket, reinterpret_cast<sockaddr*> ( &addr ), sizeof ( addr ) ) != 0 )
        {
            jamulus_core_close_socket ( instance );
            return false;
        }
    }

    instance->serverSockAddrLen = 0;
    if ( !instance->serverAddress.empty() )
    {
        std::string    host;
        unsigned short port = 22124;
        if ( !jamulus_parse_host_port ( instance->serverAddress, host, port ) )
            return true;

        sockaddr_in serverAddr{};
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_port   = htons ( port );
        if ( ::inet_pton ( AF_INET, host.c_str(), &serverAddr.sin_addr ) == 1 )
        {
            std::memcpy ( &instance->serverSockAddr, &serverAddr, sizeof ( serverAddr ) );
            instance->serverSockAddrLen = sizeof ( serverAddr );
        }
        else
        {
            addrinfo  hints{};
            addrinfo* res = nullptr;
            std::memset ( &hints, 0, sizeof ( hints ) );
            hints.ai_family   = AF_INET;
            hints.ai_socktype = SOCK_DGRAM;
            char portBuf[16];
            std::snprintf ( portBuf, sizeof ( portBuf ), "%u", static_cast<unsigned int> ( port ) );
            if ( ::getaddrinfo ( host.c_str(), portBuf, &hints, &res ) == 0 && res )
            {
                const sockaddr_in* a = reinterpret_cast<const sockaddr_in*> ( res->ai_addr );
                if ( a )
                {
                    std::memcpy ( &instance->serverSockAddr, a, sizeof ( *a ) );
                    instance->serverSockAddrLen = sizeof ( *a );
                }
            }
            if ( res )
                ::freeaddrinfo ( res );
        }
    }

    return true;
}

static void jamulus_codec_release ( JamulusCoreInstance* instance )
{
    if ( !instance )
        return;

    if ( instance->opusEncoder )
    {
        opus_custom_encoder_destroy ( instance->opusEncoder );
        instance->opusEncoder = nullptr;
    }
    if ( instance->opusDecoder )
    {
        opus_custom_decoder_destroy ( instance->opusDecoder );
        instance->opusDecoder = nullptr;
    }
    if ( instance->opusMode )
    {
        opus_custom_mode_destroy ( instance->opusMode );
        instance->opusMode = nullptr;
    }
    if ( instance->opus64Mode )
    {
        opus_custom_mode_destroy ( instance->opus64Mode );
        instance->opus64Mode = nullptr;
    }
}

static bool jamulus_codec_ensure_ready ( JamulusCoreInstance* instance )
{
    if ( !instance )
        return false;

    JamulusCodecConfig cfg;
    {
        std::lock_guard<std::mutex> lock ( instance->audioMutex );
        cfg = instance->codecConfig;
    }

    const int codingType    = ( cfg.codingType == JAMULUS_CODING_TYPE_OPUS64 ) ? JAMULUS_CODING_TYPE_OPUS64 : JAMULUS_CODING_TYPE_OPUS;
    const int frameSamples  = ( codingType == JAMULUS_CODING_TYPE_OPUS64 ) ? JAMULUS_FRAME_SAMPLES_64 : JAMULUS_FRAME_SAMPLES_128;
    const int audioChannels = ( cfg.audioChannels == 2 ) ? 2 : 1;

    if ( instance->opusEncoder && instance->opusDecoder &&
         instance->opusCodecType == codingType &&
         instance->opusCodecChannels == audioChannels &&
         instance->opusCodecFrameSamples == frameSamples )
    {
        return true;
    }

    jamulus_codec_release ( instance );

    int err = 0;
    if ( !instance->opusMode )
    {
        instance->opusMode = opus_custom_mode_create ( JAMULUS_SAMPLE_RATE_HZ, JAMULUS_FRAME_SAMPLES_128, &err );
        if ( err != OPUS_OK || !instance->opusMode )
            return false;
    }
    if ( !instance->opus64Mode )
    {
        instance->opus64Mode = opus_custom_mode_create ( JAMULUS_SAMPLE_RATE_HZ, JAMULUS_FRAME_SAMPLES_64, &err );
        if ( err != OPUS_OK || !instance->opus64Mode )
            return false;
    }

    OpusCustomMode* selectedMode = ( codingType == JAMULUS_CODING_TYPE_OPUS64 ) ? instance->opus64Mode : instance->opusMode;
    instance->opusEncoder = opus_custom_encoder_create ( selectedMode, audioChannels, &err );
    if ( err != OPUS_OK || !instance->opusEncoder )
        return false;

    instance->opusDecoder = opus_custom_decoder_create ( selectedMode, audioChannels, &err );
    if ( err != OPUS_OK || !instance->opusDecoder )
        return false;

    opus_custom_encoder_ctl ( instance->opusEncoder, OPUS_SET_VBR ( 0 ) );
    opus_custom_encoder_ctl ( instance->opusEncoder, OPUS_SET_APPLICATION ( OPUS_APPLICATION_RESTRICTED_LOWDELAY ) );
    if ( codingType == JAMULUS_CODING_TYPE_OPUS64 )
    {
        opus_custom_encoder_ctl ( instance->opusEncoder, OPUS_SET_PACKET_LOSS_PERC ( 35 ) );
    }
    else
    {
        opus_custom_encoder_ctl ( instance->opusEncoder, OPUS_SET_COMPLEXITY ( 1 ) );
    }

    instance->opusCodecType         = codingType;
    instance->opusCodecChannels     = audioChannels;
    instance->opusCodecFrameSamples = frameSamples;
    return true;
}

static int jamulus_probe_decode_once ( OpusCustomMode* mode, int audioChannels, const uint8_t* data, int dataBytes, int frameSamples )
{
    if ( !mode || !data || dataBytes <= 0 || frameSamples <= 0 || ( audioChannels != 1 && audioChannels != 2 ) )
        return OPUS_BAD_ARG;

    int err = 0;
    OpusCustomDecoder* decoder = opus_custom_decoder_create ( mode, audioChannels, &err );
    if ( err != OPUS_OK || !decoder )
        return ( err != OPUS_OK ) ? err : OPUS_ALLOC_FAIL;

    std::vector<int16_t> pcm ( static_cast<size_t> ( frameSamples * audioChannels ) );
    const int decRc = opus_custom_decode ( decoder,
                                           data,
                                           dataBytes,
                                           reinterpret_cast<opus_int16*> ( pcm.data() ),
                                           frameSamples );
    opus_custom_decoder_destroy ( decoder );
    return decRc;
}

static int jamulus_probe_decode_once_float ( OpusCustomMode* mode, int audioChannels, const uint8_t* data, int dataBytes, int frameSamples )
{
    if ( !mode || !data || dataBytes <= 0 || frameSamples <= 0 || ( audioChannels != 1 && audioChannels != 2 ) )
        return OPUS_BAD_ARG;

    int err = 0;
    OpusCustomDecoder* decoder = opus_custom_decoder_create ( mode, audioChannels, &err );
    if ( err != OPUS_OK || !decoder )
        return ( err != OPUS_OK ) ? err : OPUS_ALLOC_FAIL;

    std::vector<float> pcm ( static_cast<size_t> ( frameSamples * audioChannels ), 0.0f );
    const int decRc = opus_custom_decode_float ( decoder, data, dataBytes, pcm.data(), frameSamples );
    opus_custom_decoder_destroy ( decoder );
    return decRc;
}

static bool jamulus_try_autodetect_codec ( JamulusCoreInstance* instance, const uint8_t* pktData, int pktSize, JamulusCodecConfig& outCfg )
{
    if ( !instance || !pktData || pktSize <= 0 )
        return false;

    if ( pktSize < 10 )
        return false;

    struct Candidate
    {
        int codingType;
        int frameSamples;
        int channels;
    };

    const Candidate candidates[] = {
        { JAMULUS_CODING_TYPE_OPUS, JAMULUS_FRAME_SAMPLES_128, 1 },
        { JAMULUS_CODING_TYPE_OPUS, JAMULUS_FRAME_SAMPLES_128, 2 },
        { JAMULUS_CODING_TYPE_OPUS64, JAMULUS_FRAME_SAMPLES_64, 1 },
        { JAMULUS_CODING_TYPE_OPUS64, JAMULUS_FRAME_SAMPLES_64, 2 },
    };

    for ( const auto& candidate : candidates )
    {
        int err = 0;
        OpusCustomMode* mode = opus_custom_mode_create ( JAMULUS_SAMPLE_RATE_HZ, candidate.frameSamples, &err );
        if ( err != OPUS_OK || !mode )
            continue;
        std::vector<int16_t> decodeBuf ( static_cast<size_t> ( candidate.frameSamples * candidate.channels ) );

        for ( int blockFactor = 1; blockFactor <= 8; ++blockFactor )
        {
            if ( ( pktSize % blockFactor ) != 0 )
                continue;

            const int framePacketBytes = pktSize / blockFactor;
            if ( framePacketBytes < 10 || framePacketBytes > 512 )
                continue;

            for ( int variant = 0; variant < 3; ++variant )
            {
                int decodeBytes = framePacketBytes;
                int dataOffset  = 0;
                if ( variant == 1 || variant == 2 )
                {
                    decodeBytes = framePacketBytes - 1;
                    if ( decodeBytes < 10 )
                        continue;
                    dataOffset = ( variant == 2 ) ? 1 : 0;
                }

                OpusCustomDecoder* decoder = opus_custom_decoder_create ( mode, candidate.channels, &err );
                if ( err != OPUS_OK || !decoder )
                    continue;

                bool packetOk    = true;
                int  packetStart = 0;
                for ( int b = 0; b < blockFactor; ++b )
                {
                    const int blockOffset = packetStart + dataOffset;
                    if ( blockOffset + decodeBytes > pktSize )
                    {
                        packetOk = false;
                        break;
                    }

                    const int decRc = opus_custom_decode ( decoder,
                                                           pktData + static_cast<size_t> ( blockOffset ),
                                                           decodeBytes,
                                                           reinterpret_cast<opus_int16*> ( decodeBuf.data() ),
                                                           candidate.frameSamples );
                    if ( decRc <= 0 )
                    {
                        packetOk = false;
                        break;
                    }
                    packetStart += framePacketBytes;
                }

                opus_custom_decoder_destroy ( decoder );
                if ( !packetOk )
                    continue;

                outCfg.codingType        = candidate.codingType;
                outCfg.frameSamples      = candidate.frameSamples;
                outCfg.audioChannels     = candidate.channels;
                outCfg.blockFactor       = blockFactor;
                outCfg.codedBytes        = decodeBytes;
                outCfg.useSequenceNumber = ( variant != 0 );
                opus_custom_mode_destroy ( mode );
                return true;
            }
        }

        opus_custom_mode_destroy ( mode );
    }

    return false;
}

static void jamulus_send_wrapped_message ( JamulusCoreInstance* instance, uint16_t msgId, const std::vector<uint8_t>& data )
{
    if ( !instance || instance->udpSocket < 0 || instance->serverSockAddrLen <= 0 )
        return;

    uint8_t msgCounter = 0;
    if ( msgId < 1000 && msgId != JAMULUS_PM_ACKN )
        msgCounter = instance->nextProtocolCounter.fetch_add ( 1, std::memory_order_relaxed );

    std::vector<uint8_t> frame = jamulus_build_frame ( msgId, msgCounter, data );
    ::sendto ( instance->udpSocket,
               reinterpret_cast<const char*> ( frame.data() ),
               static_cast<int> ( frame.size() ),
               0,
               reinterpret_cast<const sockaddr*> ( &instance->serverSockAddr ),
               instance->serverSockAddrLen );
}

static int jamulus_resolve_target_channel_id ( const JamulusCoreInstance* instance, int channelId )
{
    if ( channelId >= 0 )
        return channelId;
    if ( !instance )
        return -1;
    return instance->myChannelId.load ( std::memory_order_relaxed );
}

static void jamulus_send_channel_gain_message ( JamulusCoreInstance* instance, int channelId, float gain )
{
    if ( !instance )
        return;

    const int targetId = jamulus_resolve_target_channel_id ( instance, channelId );
    if ( targetId < 0 || targetId > 255 )
        return;

    const float clampedGain = std::max ( 0.0f, std::min ( 1.0f, gain ) );
    const int   gainInt     = static_cast<int> ( std::lround ( clampedGain * 32768.0f ) );

    std::vector<uint8_t> data ( 3 );
    int                  pos = 0;
    jamulus_put_val ( data, pos, static_cast<uint32_t> ( targetId ), 1 );
    jamulus_put_val ( data, pos, static_cast<uint32_t> ( gainInt ), 2 );
    jamulus_send_wrapped_message ( instance, JAMULUS_PM_CHANNEL_GAIN, data );
}

static constexpr int JAMULUS_GAIN_OR_PAN_DELAY_PERIOD_MS = 50;

static int jamulus_get_gain_or_pan_delay_ms ( const JamulusCoreInstance* instance )
{
    if ( !instance )
        return JAMULUS_GAIN_OR_PAN_DELAY_PERIOD_MS;

    const int pingMs = instance->lastPingMs.load ( std::memory_order_relaxed );
    if ( pingMs < ( JAMULUS_GAIN_OR_PAN_DELAY_PERIOD_MS / 2 ) )
        return JAMULUS_GAIN_OR_PAN_DELAY_PERIOD_MS;

    return std::max ( JAMULUS_GAIN_OR_PAN_DELAY_PERIOD_MS, pingMs * 2 );
}

static float jamulus_sanitize_pan_value ( float pan )
{
    float clamped = std::max ( -1.0f, std::min ( 1.0f, pan ) );
    if ( std::abs ( clamped ) <= 0.01f )
        return 0.0f;
    if ( clamped >= 0.98f )
        return 1.0f;
    if ( clamped <= -0.98f )
        return -1.0f;
    return clamped;
}

static void jamulus_reset_pan_rate_limit ( JamulusCoreInstance* instance )
{
    if ( !instance )
        return;

    std::lock_guard<std::mutex> lock ( instance->panRateMutex );
    instance->pendingPanUpdates.clear();
    instance->nextPanSendAtMs = 0;
}

static bool jamulus_send_channel_pan_message_now ( JamulusCoreInstance* instance, int channelId, float pan )
{
    if ( !instance )
        return false;

    const int targetId = jamulus_resolve_target_channel_id ( instance, channelId );
    if ( targetId < 0 || targetId > 255 )
        return false;

    const float sanitizedPan  = jamulus_sanitize_pan_value ( pan );
    const float normalizedPan = std::max ( 0.0f, std::min ( 1.0f, ( sanitizedPan + 1.0f ) * 0.5f ) );
    const int   panInt        = static_cast<int> ( std::lround ( normalizedPan * 32768.0f ) );

    std::vector<uint8_t> data ( 3 );
    int                  pos = 0;
    jamulus_put_val ( data, pos, static_cast<uint32_t> ( targetId ), 1 );
    jamulus_put_val ( data, pos, static_cast<uint32_t> ( panInt ), 2 );
    jamulus_send_wrapped_message ( instance, JAMULUS_PM_CHANNEL_PAN, data );
    return true;
}

static void jamulus_flush_pending_pan_messages ( JamulusCoreInstance* instance )
{
    if ( !instance )
        return;

    const uint32_t nowMs = jamulus_get_time_ms();

    std::lock_guard<std::mutex> lock ( instance->panRateMutex );
    if ( instance->nextPanSendAtMs == 0 || nowMs < instance->nextPanSendAtMs )
        return;

    bool sentAny     = false;
    bool keepWaiting = false;

    for ( auto& [channelId, state] : instance->pendingPanUpdates )
    {
        if ( !state.hasPending )
            continue;

        if ( state.hasLastSent && std::abs ( state.pendingPan - state.lastSentPan ) <= 0.0001f )
        {
            state.hasPending = false;
            continue;
        }

        if ( jamulus_send_channel_pan_message_now ( instance, channelId, state.pendingPan ) )
        {
            state.lastSentPan = state.pendingPan;
            state.hasLastSent = true;
            state.hasPending  = false;
            sentAny           = true;
        }
        else
        {
            keepWaiting = true;
        }
    }

    if ( sentAny || keepWaiting )
        instance->nextPanSendAtMs = nowMs + static_cast<uint32_t> ( jamulus_get_gain_or_pan_delay_ms ( instance ) );
    else
        instance->nextPanSendAtMs = 0;
}

static void jamulus_send_channel_pan_message ( JamulusCoreInstance* instance, int channelId, float pan )
{
    if ( !instance )
        return;

    const uint32_t nowMs = jamulus_get_time_ms();

    std::lock_guard<std::mutex> lock ( instance->panRateMutex );
    auto& state = instance->pendingPanUpdates[channelId];

    if ( instance->nextPanSendAtMs != 0 && nowMs < instance->nextPanSendAtMs )
    {
        state.pendingPan = pan;
        state.hasPending = true;
        return;
    }

    if ( jamulus_send_channel_pan_message_now ( instance, channelId, pan ) )
    {
        state.lastSentPan = pan;
        state.hasLastSent = true;
        state.hasPending  = false;
    }
    else
    {
        state.pendingPan = pan;
        state.hasPending = true;
    }

    instance->nextPanSendAtMs = nowMs + static_cast<uint32_t> ( jamulus_get_gain_or_pan_delay_ms ( instance ) );
}

static void jamulus_send_channel_mute_message ( JamulusCoreInstance* instance, int channelId, bool muted )
{
    if ( !instance )
        return;

    const int targetId = jamulus_resolve_target_channel_id ( instance, channelId );
    if ( targetId < 0 || targetId > 255 )
        return;

    std::vector<uint8_t> data ( 2 );
    int                  pos = 0;
    jamulus_put_val ( data, pos, static_cast<uint32_t> ( targetId ), 1 );
    jamulus_put_val ( data, pos, muted ? 1u : 0u, 1 );
    jamulus_send_wrapped_message ( instance, JAMULUS_PM_MUTE_STATE_CHANGED, data );
}

static void jamulus_put_short_string ( std::vector<uint8_t>& data, const std::string& text )
{
    const size_t maxLen = 65535;
    const size_t len    = std::min ( text.size(), maxLen );
    data.push_back ( static_cast<uint8_t> ( len & 0xff ) );
    data.push_back ( static_cast<uint8_t> ( ( len >> 8 ) & 0xff ) );
    data.insert ( data.end(), text.begin(), text.begin() + static_cast<std::ptrdiff_t> ( len ) );
}

static bool jamulus_get_short_string ( const uint8_t* data, int dataLen, int& pos, std::string& out )
{
    if ( !data || dataLen < 0 || pos < 0 || ( dataLen - pos ) < 2 )
        return false;

    const int len = static_cast<int> ( data[pos] ) | ( static_cast<int> ( data[pos + 1] ) << 8 );
    pos += 2;

    if ( len < 0 || pos + len > dataLen )
        return false;

    out.assign ( reinterpret_cast<const char*> ( data + pos ), reinterpret_cast<const char*> ( data + pos + len ) );
    pos += len;
    return true;
}

static std::string jamulus_get_display_version ( const std::string& version )
{
    const auto colon = version.rfind ( ':' );
    return colon == std::string::npos ? version : version.substr ( 0, colon );
}

static void jamulus_send_ack_message ( JamulusCoreInstance* instance, uint16_t receivedMsgId, uint8_t receivedCounter )
{
    if ( !instance || instance->udpSocket < 0 || instance->serverSockAddrLen <= 0 )
        return;

    std::vector<uint8_t> ackData ( 2 );
    int                  pos = 0;
    jamulus_put_val ( ackData, pos, receivedMsgId, 2 );
    const std::vector<uint8_t> ackFrame = jamulus_build_frame ( JAMULUS_PM_ACKN, receivedCounter, ackData );

    ::sendto ( instance->udpSocket,
               reinterpret_cast<const char*> ( ackFrame.data() ),
               static_cast<int> ( ackFrame.size() ),
               0,
               reinterpret_cast<const sockaddr*> ( &instance->serverSockAddr ),
               instance->serverSockAddrLen );
}

static void jamulus_send_jitter_buffer_message ( JamulusCoreInstance* instance )
{
    if ( !instance )
        return;

    std::vector<uint8_t> jitterData ( 2 );
    int                  pos = 0;
    const int requestedValue = instance->audioEngine.getAutoJitter()
                                   ? JAMULUS_AUTO_JITTER_PROTOCOL
                                   : std::clamp ( instance->audioEngine.getServerJitterBuffer(), 1, MAX_NET_BUF_SIZE_NUM_BL );
    jamulus_put_val ( jitterData, pos, static_cast<uint32_t> ( requestedValue ), 2 );
    jamulus_send_wrapped_message ( instance, JAMULUS_PM_JITT_BUF_SIZE, jitterData );
}

static void jamulus_send_split_supported_message ( JamulusCoreInstance* instance )
{
    if ( !instance )
        return;

    jamulus_send_wrapped_message ( instance, JAMULUS_PM_SPLIT_SUPPORTED, {} );
}

static void jamulus_apply_small_buffers_mode ( JamulusCoreInstance* instance, bool enabled )
{
    if ( !instance )
        return;

    std::lock_guard<std::mutex> lock ( instance->audioMutex );
    instance->codecConfig.codingType   = enabled ? JAMULUS_CODING_TYPE_OPUS64 : JAMULUS_CODING_TYPE_OPUS;
    instance->codecConfig.frameSamples = enabled ? JAMULUS_FRAME_SAMPLES_64 : JAMULUS_FRAME_SAMPLES_128;

    const bool stereo = ( instance->codecConfig.audioChannels == 2 );
    instance->codecConfig.codedBytes = enabled ? ( stereo ? JAMULUS_OPUS64_STEREO_NORMAL_BYTES : JAMULUS_OPUS64_MONO_NORMAL_BYTES )
                                               : ( stereo ? JAMULUS_OPUS_STEREO_NORMAL_BYTES : JAMULUS_OPUS_MONO_NORMAL_BYTES );
}

static void jamulus_send_transport_props_message ( JamulusCoreInstance* instance )
{
    if ( !instance )
        return;

    JamulusCodecConfig cfg;
    {
        std::lock_guard<std::mutex> lock ( instance->audioMutex );
        cfg = instance->codecConfig;
    }

    std::vector<uint8_t> props;
    props.reserve ( 19 );
    const uint32_t basePacketBytes = static_cast<uint32_t> ( std::max ( 1, cfg.codedBytes ) + ( cfg.useSequenceNumber ? 1 : 0 ) );
    const uint16_t blockFactor     = static_cast<uint16_t> ( std::max ( 1, cfg.blockFactor ) );
    const uint8_t audioChannels    = static_cast<uint8_t> ( ( cfg.audioChannels == 2 ) ? 2 : 1 );
    const uint32_t sampleRate      = JAMULUS_SAMPLE_RATE_HZ;
    const uint16_t codingType      = static_cast<uint16_t> ( ( cfg.codingType == JAMULUS_CODING_TYPE_OPUS64 ) ? JAMULUS_CODING_TYPE_OPUS64 : JAMULUS_CODING_TYPE_OPUS );
    const uint16_t flags           = cfg.useSequenceNumber ? 1 : 0;
    const uint32_t codingArg       = 0;

    props.push_back ( static_cast<uint8_t> ( basePacketBytes & 0xff ) );
    props.push_back ( static_cast<uint8_t> ( ( basePacketBytes >> 8 ) & 0xff ) );
    props.push_back ( static_cast<uint8_t> ( ( basePacketBytes >> 16 ) & 0xff ) );
    props.push_back ( static_cast<uint8_t> ( ( basePacketBytes >> 24 ) & 0xff ) );
    props.push_back ( static_cast<uint8_t> ( blockFactor & 0xff ) );
    props.push_back ( static_cast<uint8_t> ( ( blockFactor >> 8 ) & 0xff ) );
    props.push_back ( audioChannels );
    props.push_back ( static_cast<uint8_t> ( sampleRate & 0xff ) );
    props.push_back ( static_cast<uint8_t> ( ( sampleRate >> 8 ) & 0xff ) );
    props.push_back ( static_cast<uint8_t> ( ( sampleRate >> 16 ) & 0xff ) );
    props.push_back ( static_cast<uint8_t> ( ( sampleRate >> 24 ) & 0xff ) );
    props.push_back ( static_cast<uint8_t> ( codingType & 0xff ) );
    props.push_back ( static_cast<uint8_t> ( ( codingType >> 8 ) & 0xff ) );
    props.push_back ( static_cast<uint8_t> ( flags & 0xff ) );
    props.push_back ( static_cast<uint8_t> ( ( flags >> 8 ) & 0xff ) );
    props.push_back ( static_cast<uint8_t> ( codingArg & 0xff ) );
    props.push_back ( static_cast<uint8_t> ( ( codingArg >> 8 ) & 0xff ) );
    props.push_back ( static_cast<uint8_t> ( ( codingArg >> 16 ) & 0xff ) );
    props.push_back ( static_cast<uint8_t> ( ( codingArg >> 24 ) & 0xff ) );
    jamulus_send_wrapped_message ( instance, JAMULUS_PM_NETW_TRANSPORT_PROPS, props );
}

static void jamulus_send_channel_info_message ( JamulusCoreInstance* instance )
{
    if ( !instance )
        return;

    std::vector<uint8_t> chanInfo;
    chanInfo.reserve ( 16 + instance->profileName.size() );
    const uint16_t country = static_cast<uint16_t> ( std::max ( 0, instance->profileCountry ) );
    const uint32_t instrument = static_cast<uint32_t> ( std::max ( 0, instance->profileInstrument ) );
    const uint8_t skill = static_cast<uint8_t> ( std::max ( 0, instance->profileSkill ) );
    chanInfo.push_back ( static_cast<uint8_t> ( country & 0xff ) );
    chanInfo.push_back ( static_cast<uint8_t> ( ( country >> 8 ) & 0xff ) );
    chanInfo.push_back ( static_cast<uint8_t> ( instrument & 0xff ) );
    chanInfo.push_back ( static_cast<uint8_t> ( ( instrument >> 8 ) & 0xff ) );
    chanInfo.push_back ( static_cast<uint8_t> ( ( instrument >> 16 ) & 0xff ) );
    chanInfo.push_back ( static_cast<uint8_t> ( ( instrument >> 24 ) & 0xff ) );
    chanInfo.push_back ( skill );
    jamulus_put_short_string ( chanInfo, instance->profileName.empty() ? instance->clientName : instance->profileName );
    jamulus_put_short_string ( chanInfo, instance->profileCity );
    jamulus_send_wrapped_message ( instance, JAMULUS_PM_CHANNEL_INFOS, chanInfo );
}

static void jamulus_send_version_and_os_message ( JamulusCoreInstance* instance )
{
    if ( !instance )
        return;

    std::vector<uint8_t> versionData;
    versionData.reserve ( 32 );
    versionData.push_back ( 3 ); // Windows
    jamulus_put_short_string ( versionData, "r3_12_0-jamulusplus" );
    jamulus_send_wrapped_message ( instance, JAMULUS_PM_VERSION_AND_OS, versionData );
}

static void jamulus_send_connection_setup_messages ( JamulusCoreInstance* instance )
{
    if ( !instance )
        return;

    jamulus_send_split_supported_message ( instance );
    jamulus_send_channel_info_message ( instance );
    jamulus_send_version_and_os_message ( instance );

    jamulus_send_wrapped_message ( instance, JAMULUS_PM_REQ_NETW_TRANSPORT_PROPS, {} );
    jamulus_send_wrapped_message ( instance, JAMULUS_PM_REQ_CONN_CLIENTS_LIST, {} );
    jamulus_send_wrapped_message ( instance, JAMULUS_PM_REQ_CHANNEL_LEVEL_LIST, { 1 } );
    jamulus_send_jitter_buffer_message ( instance );
}

static void jamulus_handle_net_transport_props ( JamulusCoreInstance* instance, const uint8_t* data, int dataLen )
{
    static constexpr int netTransportPropsBodyBytes = 19;

    if ( !instance || !data || dataLen != netTransportPropsBodyBytes )
    {
        if ( instance )
        {
            jamulus_core_log ( "net_transport_props reject=invalid_length dataLen=" + std::to_string ( dataLen ) +
                               " raw=" + jamulus_hex_dump ( data, std::max ( 0, dataLen ) ) );
        }
        return;
    }

    int pos = 0;
    const int basePacketSize = static_cast<int> ( jamulus_get_val ( data, dataLen, pos, 4 ) );
    const int blockFactor    = static_cast<int> ( jamulus_get_val ( data, dataLen, pos, 2 ) );
    const int audioChannels  = static_cast<int> ( jamulus_get_val ( data, dataLen, pos, 1 ) );
    (void) jamulus_get_val ( data, dataLen, pos, 4 );
    const int codingType     = static_cast<int> ( jamulus_get_val ( data, dataLen, pos, 2 ) );
    const int flags          = static_cast<int> ( jamulus_get_val ( data, dataLen, pos, 2 ) );
    const int codingArg      = static_cast<int> ( jamulus_get_val ( data, dataLen, pos, 4 ) );

    const bool validBlockFactor = ( blockFactor == 1 || blockFactor == 2 || blockFactor == 4 );
    const bool validChannels    = ( audioChannels == 1 || audioChannels == 2 );
    const bool validCodingType  = ( codingType == JAMULUS_CODING_TYPE_OPUS || codingType == JAMULUS_CODING_TYPE_OPUS64 );
    const bool validFlags       = ( flags == 0 || flags == 1 );
    const bool validBasePacket  = ( basePacketSize >= 10 && basePacketSize <= 20000 );

    if ( !validBasePacket || !validBlockFactor || !validChannels || !validCodingType || !validFlags )
    {
        jamulus_core_log ( "net_transport_props reject=invalid_fields" +
                           std::string ( " basePacket=" ) + std::to_string ( basePacketSize ) +
                           " blockFactor=" + std::to_string ( blockFactor ) +
                           " channels=" + std::to_string ( audioChannels ) +
                           " codingTypeRaw=" + std::to_string ( codingType ) +
                           " flags=" + std::to_string ( flags ) +
                           " codingArg=" + std::to_string ( codingArg ) +
                           " raw=" + jamulus_hex_dump ( data, dataLen ) );
        return;
    }

    const bool useSequenceNumber = ( ( flags & 1 ) != 0 );
    int codedBytes = ( codingArg > 0 ) ? codingArg : ( basePacketSize - ( useSequenceNumber ? 1 : 0 ) );
    if ( codedBytes <= 0 )
        codedBytes = std::max ( 1, basePacketSize );

    JamulusCodecConfig cfg;
    cfg.codingType    = codingType;
    cfg.codedBytes    = codedBytes;
    cfg.blockFactor   = std::max ( 1, blockFactor );
    cfg.audioChannels = audioChannels;
    cfg.frameSamples  = ( cfg.codingType == JAMULUS_CODING_TYPE_OPUS64 ) ? JAMULUS_FRAME_SAMPLES_64 : JAMULUS_FRAME_SAMPLES_128;
    cfg.useSequenceNumber = useSequenceNumber;

    bool codecChanged = false;
    {
        std::lock_guard<std::mutex> lock ( instance->audioMutex );
        const JamulusCodecConfig prevCfg = instance->codecConfig;
        codecChanged = ( prevCfg.codingType != cfg.codingType ) ||
                       ( prevCfg.codedBytes != cfg.codedBytes ) ||
                       ( prevCfg.blockFactor != cfg.blockFactor ) ||
                       ( prevCfg.audioChannels != cfg.audioChannels ) ||
                       ( prevCfg.useSequenceNumber != cfg.useSequenceNumber );
        instance->codecConfig = cfg;
        instance->sockBuf.SetUseDoubleSystemFrameSize ( cfg.codingType == JAMULUS_CODING_TYPE_OPUS );
        const int initialJitterFrames = instance->audioEngine.getAutoJitter()
                                            ? DEF_NET_BUF_SIZE_NUM_BL
                                            : std::clamp ( instance->audioEngine.getJitterBuffer(), 1, MAX_NET_BUF_SIZE_NUM_BL );
        instance->sockBuf.Init ( cfg.codedBytes, initialJitterFrames, cfg.useSequenceNumber, codecChanged );
        instance->sockBufPacket.Init ( cfg.codedBytes );
        instance->jitterTargetFrames = initialJitterFrames;
        if ( codecChanged )
        {
            instance->encodedIncomingPackets.clear();
            instance->decodedPcmQueue.clear();
        }
    }
    instance->gotNetTransportProps.store ( true, std::memory_order_relaxed );
    if ( codecChanged )
        instance->codecConfigChanged.store ( true, std::memory_order_relaxed );
    jamulus_core_log ( "net_transport_props basePacket=" + std::to_string ( basePacketSize ) +
                       " blockFactor=" + std::to_string ( blockFactor ) +
                       " channels=" + std::to_string ( cfg.audioChannels ) +
                       " codingTypeRaw=" + std::to_string ( codingType ) +
                       " flags=" + std::to_string ( flags ) +
                       " codingArg=" + std::to_string ( codingArg ) +
                       " decodedCodedBytes=" + std::to_string ( cfg.codedBytes ) +
                       " seq=" + std::to_string ( cfg.useSequenceNumber ? 1 : 0 ) );
}

static bool jamulus_try_store_audio_packet ( JamulusCoreInstance* instance, const uint8_t* data, int dataLen )
{
    if ( !instance || !data || dataLen <= 0 )
    {
        if ( instance )
            instance->audioPacketsRejected.fetch_add ( 1, std::memory_order_relaxed );
        return false;
    }

    auto logReject = [&] ( const std::string& reason )
    {
        const uint32_t nowMs  = jamulus_get_time_ms();
        uint32_t       lastMs = instance->lastAudioRejectLogMs.load ( std::memory_order_relaxed );
        if ( nowMs - lastMs >= 1000 && instance->lastAudioRejectLogMs.compare_exchange_strong ( lastMs, nowMs, std::memory_order_relaxed ) )
        {
            JamulusCodecConfig cfgForLog;
            {
                std::lock_guard<std::mutex> lock ( instance->audioMutex );
                cfgForLog = instance->codecConfig;
            }
            const int codedBytes  = std::max ( 1, cfgForLog.codedBytes );
            const int blockFactor = std::max ( 1, cfgForLog.blockFactor );
            const int expectedNoSequence = codedBytes * blockFactor;
            const int expectedWithSeq    = ( codedBytes + 1 ) * blockFactor;
            jamulus_core_log ( "audio_reject reason=" + reason +
                               " dataLen=" + std::to_string ( dataLen ) +
                               " cfgType=" + std::to_string ( cfgForLog.codingType ) +
                               " cfgCh=" + std::to_string ( cfgForLog.audioChannels ) +
                               " cfgBytes=" + std::to_string ( cfgForLog.codedBytes ) +
                               " cfgBlock=" + std::to_string ( cfgForLog.blockFactor ) +
                               " cfgSeq=" + std::to_string ( cfgForLog.useSequenceNumber ? 1 : 0 ) +
                               " gotProps=" + std::to_string ( instance->gotNetTransportProps.load ( std::memory_order_relaxed ) ? 1 : 0 ) +
                               " expectedNoSeq=" + std::to_string ( expectedNoSequence ) +
                               " expectedWithSeq=" + std::to_string ( expectedWithSeq ) );
        }
    };

    if ( dataLen > 4096 )
    {
        instance->audioPacketsRejected.fetch_add ( 1, std::memory_order_relaxed );
        instance->audioRejectTooLarge.fetch_add ( 1, std::memory_order_relaxed );
        logReject ( "too_large" );
        return false;
    }

    if ( jamulus_is_valid_frame ( data, dataLen ) )
    {
        instance->audioPacketsRejected.fetch_add ( 1, std::memory_order_relaxed );
        instance->audioRejectFramedMessage.fetch_add ( 1, std::memory_order_relaxed );
        logReject ( "wrapped_control_frame" );
        return false;
    }

    JamulusCodecConfig cfg;
    {
        std::lock_guard<std::mutex> lock ( instance->audioMutex );
        cfg = instance->codecConfig;
    }

    const int codedBytes  = std::max ( 1, cfg.codedBytes );
    const int blockFactor = std::max ( 1, cfg.blockFactor );
    const int expectedNoSequence = codedBytes * blockFactor;
    const int expectedWithSeq    = ( codedBytes + 1 ) * blockFactor;
    const bool gotTransportProps = instance->gotNetTransportProps.load ( std::memory_order_relaxed );

    if ( !gotTransportProps && dataLen < 10 )
    {
        instance->audioPacketsRejected.fetch_add ( 1, std::memory_order_relaxed );
        instance->audioRejectSizeMismatch.fetch_add ( 1, std::memory_order_relaxed );
        logReject ( "preprops_too_small" );
        return false;
    }

    if ( gotTransportProps )
    {
        const int expectedPacketBytes = cfg.useSequenceNumber ? expectedWithSeq : expectedNoSequence;
        if ( dataLen != expectedPacketBytes )
        {
            instance->audioPacketsRejected.fetch_add ( 1, std::memory_order_relaxed );
            instance->audioRejectSizeMismatch.fetch_add ( 1, std::memory_order_relaxed );
            logReject ( "size_mismatch" );
            return false;
        }

        CVector<uint8_t> packetData;
        packetData.Init ( dataLen );
        std::copy ( data, data + dataLen, packetData.begin() );

        std::lock_guard<std::mutex> lock ( instance->audioMutex );
        const bool putOk = instance->sockBuf.Put ( packetData, dataLen );
        if ( !putOk )
        {
            instance->audioPacketsRejected.fetch_add ( 1, std::memory_order_relaxed );
            instance->audioRejectSizeMismatch.fetch_add ( 1, std::memory_order_relaxed );
            logReject ( "sockbuf_reject" );
            return false;
        }

        instance->audioPacketsQueued.fetch_add ( 1, std::memory_order_relaxed );
        return true;
    }

    JamulusEncodedPacket packet;
    packet.rawPayload.assign ( data, data + dataLen );
    packet.payload.assign ( data, data + dataLen );

    std::lock_guard<std::mutex> lock ( instance->audioMutex );
    if ( instance->encodedIncomingPackets.size() >= 512 )
        instance->encodedIncomingPackets.pop_front();
    instance->encodedIncomingPackets.emplace_back ( std::move ( packet ) );
    instance->audioPacketsQueued.fetch_add ( 1, std::memory_order_relaxed );
    return true;
}

static int jamulus_nonqt_process_audio ( JamulusCoreInstance* instance, const float* in, float* out, int frames, int channels )
{
    if ( !instance || !out || frames <= 0 || channels <= 0 )
        return 0;

    const int totalSamples = frames * channels;
    if ( in )
        std::memcpy ( out, in, static_cast<size_t> ( totalSamples ) * sizeof ( float ) );
    else
        std::fill ( out, out + totalSamples, 0.0f );

    if ( !instance->runningFlag.load ( std::memory_order_relaxed ) || instance->udpSocket < 0 || instance->serverSockAddrLen <= 0 )
        return 0;

    if ( instance->codecConfigChanged.exchange ( false, std::memory_order_relaxed ) )
    {
        jamulus_codec_release ( instance );
        instance->sendSequenceNumber = 0;
        std::lock_guard<std::mutex> lock ( instance->audioMutex );
        instance->encodedIncomingPackets.clear();
        instance->decodedPcmQueue.clear();
        instance->receiveSequenceLocked = false;
        instance->expectedReceiveSequence = 0;
        instance->jitterTargetFrames = DEF_NET_BUF_SIZE_NUM_BL;
        instance->jitterUnderRuns = 0;
        instance->jitterOverRuns = 0;
        instance->streamOutputGain = 0.0f;
        instance->streamNoDataBlocks = 0;
    }

    if ( !jamulus_codec_ensure_ready ( instance ) )
        return 0;

    JamulusCodecConfig cfg;
    {
        std::lock_guard<std::mutex> lock ( instance->audioMutex );
        cfg = instance->codecConfig;
    }

    const int frameSamples = ( cfg.codingType == JAMULUS_CODING_TYPE_OPUS64 ) ? JAMULUS_FRAME_SAMPLES_64 : JAMULUS_FRAME_SAMPLES_128;
    const int codecCh      = ( cfg.audioChannels == 2 ) ? 2 : 1;
    const int codedBytes   = std::max ( 1, cfg.codedBytes );
    const int blockFactor  = std::max ( 1, cfg.blockFactor );
    const bool useSequenceNumber = cfg.useSequenceNumber;
    OpusCustomMode* decodeMode     = ( cfg.codingType == JAMULUS_CODING_TYPE_OPUS64 ) ? instance->opus64Mode : instance->opusMode;
    const int framePacketBytes   = codedBytes + ( useSequenceNumber ? 1 : 0 );
    const int packetBytes        = framePacketBytes * blockFactor;

    if ( static_cast<int> ( instance->encodePcm.size() ) < frameSamples * codecCh )
        instance->encodePcm.resize ( static_cast<size_t> ( frameSamples * codecCh ) );
    if ( static_cast<int> ( instance->decodePcm.size() ) < frameSamples * codecCh )
        instance->decodePcm.resize ( static_cast<size_t> ( frameSamples * codecCh ) );
    if ( static_cast<int> ( instance->decodeFloatPcm.size() ) < frameSamples * codecCh )
        instance->decodeFloatPcm.resize ( static_cast<size_t> ( frameSamples * codecCh ) );
    if ( static_cast<int> ( instance->encodedTempPacket.size() ) < packetBytes )
        instance->encodedTempPacket.resize ( static_cast<size_t> ( packetBytes ) );

    int inputOffset = 0;
    while ( inputOffset < frames )
    {
        int encodedOffset = 0;
        for ( int b = 0; b < blockFactor; ++b )
        {
            for ( int i = 0; i < frameSamples; ++i )
            {
                const int srcFrame = inputOffset + i;
                float l = 0.0f;
                float r = 0.0f;
                if ( srcFrame < frames && in )
                {
                    const int srcBase = srcFrame * channels;
                    l = in[srcBase];
                    r = ( channels > 1 ) ? in[srcBase + 1] : l;
                }
                if ( codecCh == 1 )
                {
                    const float m = 0.5f * ( l + r );
                    const float s = std::max ( -1.0f, std::min ( 1.0f, m ) );
                    instance->encodePcm[i] = static_cast<int16_t> ( std::lrintf ( s * 32767.0f ) );
                }
                else
                {
                    const float sl = std::max ( -1.0f, std::min ( 1.0f, l ) );
                    const float sr = std::max ( -1.0f, std::min ( 1.0f, r ) );
                    instance->encodePcm[i * 2]     = static_cast<int16_t> ( std::lrintf ( sl * 32767.0f ) );
                    instance->encodePcm[i * 2 + 1] = static_cast<int16_t> ( std::lrintf ( sr * 32767.0f ) );
                }
            }

            const int encRc = opus_custom_encode ( instance->opusEncoder,
                                                   reinterpret_cast<const opus_int16*> ( instance->encodePcm.data() ),
                                                   frameSamples,
                                                   instance->encodedTempPacket.data() + encodedOffset,
                                                   codedBytes );
            if ( encRc < 0 )
                break;

            encodedOffset += codedBytes;
            if ( useSequenceNumber )
                instance->encodedTempPacket[static_cast<size_t> ( encodedOffset++ )] = instance->sendSequenceNumber++;
            inputOffset += frameSamples;
        }

        if ( encodedOffset == packetBytes )
        {
            ::sendto ( instance->udpSocket,
                       reinterpret_cast<const char*> ( instance->encodedTempPacket.data() ),
                       packetBytes,
                       0,
                       reinterpret_cast<const sockaddr*> ( &instance->serverSockAddr ),
                       instance->serverSockAddrLen );
        }
        else
        {
            break;
        }
    }

    std::vector<JamulusEncodedPacket> packets;
    {
        std::lock_guard<std::mutex> lock ( instance->audioMutex );
        if ( instance->gotNetTransportProps.load ( std::memory_order_relaxed ) )
        {
            const int desiredJitterFrames = instance->audioEngine.getAutoJitter()
                                                ? std::clamp ( instance->sockBuf.GetAutoSetting(), 1, MAX_NET_BUF_SIZE_NUM_BL )
                                                : std::clamp ( instance->audioEngine.getJitterBuffer(), 1, MAX_NET_BUF_SIZE_NUM_BL );
            if ( desiredJitterFrames != instance->jitterTargetFrames )
            {
                instance->sockBuf.Init ( cfg.codedBytes, desiredJitterFrames, cfg.useSequenceNumber, true );
                instance->jitterTargetFrames = desiredJitterFrames;
            }

            const int packetsNeeded = std::max ( 4, ( frames + frameSamples - 1 ) / frameSamples + 4 );
            const int maxAttempts   = packetsNeeded + MAX_NET_BUF_SIZE_NUM_BL;
            packets.reserve ( static_cast<size_t> ( packetsNeeded ) );
            for ( int attempt = 0; attempt < maxAttempts; ++attempt )
            {
                if ( !instance->sockBuf.Get ( instance->sockBufPacket, cfg.codedBytes ) )
                    continue;

                JamulusEncodedPacket pkt;
                pkt.payload.assign ( instance->sockBufPacket.begin(), instance->sockBufPacket.begin() + cfg.codedBytes );
                packets.push_back ( std::move ( pkt ) );
                if ( static_cast<int> ( packets.size() ) >= packetsNeeded )
                    break;
            }
        }
        else
        {
            const size_t take = std::min<size_t> ( instance->encodedIncomingPackets.size(), 16 );
            packets.reserve ( take );
            for ( size_t i = 0; i < take; ++i )
            {
                packets.push_back ( std::move ( instance->encodedIncomingPackets.front() ) );
                instance->encodedIncomingPackets.pop_front();
            }
        }
    }

    if ( useSequenceNumber && !packets.empty() )
    {
        std::vector<JamulusEncodedPacket> sequenced;
        std::vector<JamulusEncodedPacket> unsequenced;
        sequenced.reserve ( packets.size() );
        unsequenced.reserve ( packets.size() );
        for ( auto& pkt : packets )
        {
            if ( pkt.hasSequence )
                sequenced.push_back ( std::move ( pkt ) );
            else
                unsequenced.push_back ( std::move ( pkt ) );
        }

        packets.clear();
        if ( !sequenced.empty() )
        {
            if ( !instance->receiveSequenceLocked )
            {
                instance->expectedReceiveSequence = sequenced.front().sequence;
                instance->receiveSequenceLocked = true;
            }

            std::sort ( sequenced.begin(),
                        sequenced.end(),
                        [&] ( const JamulusEncodedPacket& a, const JamulusEncodedPacket& b )
                        {
                            const int da = jamulus_seq_forward_distance ( instance->expectedReceiveSequence, a.sequence );
                            const int db = jamulus_seq_forward_distance ( instance->expectedReceiveSequence, b.sequence );
                            return da < db;
                        } );

            for ( auto& pkt : sequenced )
            {
                const int distance = jamulus_seq_forward_distance ( instance->expectedReceiveSequence, pkt.sequence );
                if ( distance > 192 )
                    continue;
                packets.push_back ( std::move ( pkt ) );
                instance->expectedReceiveSequence = static_cast<uint8_t> ( pkt.sequence + static_cast<uint8_t> ( blockFactor ) );
            }
        }

        for ( auto& pkt : unsequenced )
            packets.push_back ( std::move ( pkt ) );
    }

    bool attemptedCodecAutodetect = false;
    bool hadDecodeFailureThisCall = false;
    for ( const auto& pkt : packets )
    {
        const int pktSize = static_cast<int> ( pkt.payload.size() );
        int decodeBlockFactor = blockFactor;
        if ( decodeBlockFactor <= 0 )
            decodeBlockFactor = 1;
        if ( ( pktSize % decodeBlockFactor ) != 0 )
            decodeBlockFactor = 1;
        if ( ( pktSize % decodeBlockFactor ) != 0 )
            continue;
        const int frameBytes = pktSize / decodeBlockFactor;
        if ( frameBytes <= 0 )
            continue;
        const bool packetUsesSequence = false;
        const int  decodeCodedBytes   = frameBytes;
        const int  decodeFramePacketBytes = decodeCodedBytes;
        int packetOffset = 0;
        for ( int b = 0; b < decodeBlockFactor; ++b )
        {
            instance->decodeAttempts.fetch_add ( 1, std::memory_order_relaxed );
            std::fill ( instance->decodeFloatPcm.begin(), instance->decodeFloatPcm.end(), 0.0f );
            int decRc = opus_custom_decode_float ( instance->opusDecoder,
                                                   pkt.payload.data() + static_cast<size_t> ( packetOffset ),
                                                   decodeCodedBytes,
                                                   instance->decodeFloatPcm.data(),
                                                   frameSamples );
            if ( decRc < 0 && decodeFramePacketBytes >= 2 )
            {
                std::fill ( instance->decodeFloatPcm.begin(), instance->decodeFloatPcm.end(), 0.0f );
                decRc = opus_custom_decode_float ( instance->opusDecoder,
                                                   pkt.payload.data() + static_cast<size_t> ( packetOffset ),
                                                   decodeFramePacketBytes - 1,
                                                   instance->decodeFloatPcm.data(),
                                                   frameSamples );
            }

            int freshCurrentRc = OPUS_BAD_ARG;
            int freshRawRc     = OPUS_BAD_ARG;
            int freshRawTailRc = OPUS_BAD_ARG;
            int freshRawHeadRc = OPUS_BAD_ARG;
            if ( decRc < 0 && decodeMode )
            {
                freshCurrentRc = jamulus_probe_decode_once_float ( decodeMode,
                                                                   codecCh,
                                                                   pkt.payload.data() + static_cast<size_t> ( packetOffset ),
                                                                   decodeCodedBytes,
                                                                   frameSamples );

                if ( !pkt.rawPayload.empty() )
                {
                    freshRawRc = jamulus_probe_decode_once_float ( decodeMode,
                                                                  codecCh,
                                                                  pkt.rawPayload.data(),
                                                                  static_cast<int> ( pkt.rawPayload.size() ),
                                                                  frameSamples );

                    if ( pkt.rawPayload.size() >= 2 )
                    {
                        freshRawTailRc = jamulus_probe_decode_once_float ( decodeMode,
                                                                           codecCh,
                                                                           pkt.rawPayload.data(),
                                                                           static_cast<int> ( pkt.rawPayload.size() - 1 ),
                                                                           frameSamples );
                        freshRawHeadRc = jamulus_probe_decode_once_float ( decodeMode,
                                                                           codecCh,
                                                                           pkt.rawPayload.data() + 1,
                                                                           static_cast<int> ( pkt.rawPayload.size() - 1 ),
                                                                           frameSamples );
                    }
                }

                const uint8_t* retryData  = nullptr;
                int            retryBytes = 0;
                if ( freshCurrentRc >= 0 )
                {
                    retryData  = pkt.payload.data() + static_cast<size_t> ( packetOffset );
                    retryBytes = decodeCodedBytes;
                }
                else if ( freshRawRc >= 0 )
                {
                    retryData  = pkt.rawPayload.data();
                    retryBytes = static_cast<int> ( pkt.rawPayload.size() );
                }
                else if ( freshRawTailRc >= 0 )
                {
                    retryData  = pkt.rawPayload.data();
                    retryBytes = static_cast<int> ( pkt.rawPayload.size() - 1 );
                }
                else if ( freshRawHeadRc >= 0 )
                {
                    retryData  = pkt.rawPayload.data() + 1;
                    retryBytes = static_cast<int> ( pkt.rawPayload.size() - 1 );
                }

                if ( retryData && retryBytes > 0 )
                {
                    jamulus_codec_release ( instance );
                    if ( jamulus_codec_ensure_ready ( instance ) )
                    {
                        std::fill ( instance->decodeFloatPcm.begin(), instance->decodeFloatPcm.end(), 0.0f );
                        decRc = opus_custom_decode_float ( instance->opusDecoder,
                                                           retryData,
                                                           retryBytes,
                                                           instance->decodeFloatPcm.data(),
                                                           frameSamples );
                    }
                }
            }

            if ( decRc > 0 )
            {
                const int producedSamples = decRc;
                const size_t decodedCount = static_cast<size_t> ( producedSamples * codecCh );
                for ( size_t i = 0; i < decodedCount; ++i )
                {
                    const float s = std::max ( -1.0f, std::min ( 1.0f, instance->decodeFloatPcm[i] ) );
                    instance->decodePcm[i] = static_cast<int16_t> ( std::lrintf ( s * 32767.0f ) );
                }
                instance->decodeSuccess.fetch_add ( 1, std::memory_order_relaxed );
                instance->decodedSamples.fetch_add ( static_cast<uint64_t> ( producedSamples ) * static_cast<uint64_t> ( codecCh ), std::memory_order_relaxed );
                std::lock_guard<std::mutex> lock ( instance->audioMutex );
                if ( instance->decodedPcmQueue.size() + decodedCount > 262144 )
                {
                    const size_t drop = ( instance->decodedPcmQueue.size() + decodedCount ) - 262144;
                    for ( size_t i = 0; i < drop; ++i )
                        instance->decodedPcmQueue.pop_front();
                }
                for ( size_t i = 0; i < decodedCount; ++i )
                    instance->decodedPcmQueue.push_back ( instance->decodePcm[i] );
            }
            else
            {
                instance->decodeFail.fetch_add ( 1, std::memory_order_relaxed );
                hadDecodeFailureThisCall = true;
                const uint32_t nowMs  = jamulus_get_time_ms();
                uint32_t       lastMs = instance->lastDecodeFailLogMs.load ( std::memory_order_relaxed );
                if ( nowMs - lastMs >= 1000 && instance->lastDecodeFailLogMs.compare_exchange_strong ( lastMs, nowMs, std::memory_order_relaxed ) )
                {
                    jamulus_core_log ( "decode_fail pktSize=" + std::to_string ( pktSize ) +
                                       " decodeBlockFactor=" + std::to_string ( decodeBlockFactor ) +
                                       " frameBytes=" + std::to_string ( frameBytes ) +
                                       " frameSamples=" + std::to_string ( frameSamples ) +
                                       " codecCh=" + std::to_string ( codecCh ) +
                                       " codedBytesCfg=" + std::to_string ( codedBytes ) +
                                       " useSeq=" + std::to_string ( useSequenceNumber ? 1 : 0 ) +
                                       " rawPktSize=" + std::to_string ( static_cast<int> ( pkt.rawPayload.size() ) ) +
                                       " freshCurrentRc=" + std::to_string ( freshCurrentRc ) +
                                       " freshRawRc=" + std::to_string ( freshRawRc ) +
                                       " freshRawTailRc=" + std::to_string ( freshRawTailRc ) +
                                       " freshRawHeadRc=" + std::to_string ( freshRawHeadRc ) );
                }
                if ( !attemptedCodecAutodetect && !instance->gotNetTransportProps.load ( std::memory_order_relaxed ) )
                {
                    attemptedCodecAutodetect = true;
                    JamulusCodecConfig detectedCfg;
                    if ( jamulus_try_autodetect_codec ( instance, pkt.payload.data(), pktSize, detectedCfg ) )
                    {
                        {
                            std::lock_guard<std::mutex> lock ( instance->audioMutex );
                            instance->codecConfig = detectedCfg;
                        }
                        jamulus_codec_ensure_ready ( instance );
                        jamulus_core_log ( "codec_autodetect codingType=" + std::to_string ( detectedCfg.codingType ) +
                                           " channels=" + std::to_string ( detectedCfg.audioChannels ) +
                                           " frameSamples=" + std::to_string ( detectedCfg.frameSamples ) +
                                           " codedBytes=" + std::to_string ( detectedCfg.codedBytes ) +
                                           " seq=" + std::to_string ( detectedCfg.useSequenceNumber ? 1 : 0 ) );
                        return 0;
                    }
                }
            }
            packetOffset += decodeFramePacketBytes;
        }
    }

    std::vector<int16_t> pcm;
    size_t queueAvailableForLog = 0;
    int jitterTargetFramesNow = DEF_NET_BUF_SIZE_NUM_BL;
    size_t availableToReadForLog = 0;
    {
        const int jitterMinFrames = 1;
        const int jitterMaxFrames = MAX_NET_BUF_SIZE_NUM_BL;
        const size_t needed = static_cast<size_t> ( frames ) * static_cast<size_t> ( codecCh );
        const int manualTargetFrames = std::clamp ( instance->audioEngine.getJitterBuffer(), jitterMinFrames, jitterMaxFrames );
        pcm.resize ( needed, 0 );
        std::lock_guard<std::mutex> lock ( instance->audioMutex );

        if ( instance->jitterTargetFrames < jitterMinFrames )
            instance->jitterTargetFrames = jitterMinFrames;
        if ( instance->jitterTargetFrames > jitterMaxFrames )
            instance->jitterTargetFrames = jitterMaxFrames;

        if ( !instance->audioEngine.getAutoJitter() )
            instance->jitterTargetFrames = manualTargetFrames;

        const size_t queueSize = instance->decodedPcmQueue.size();
        const size_t availableToRead = std::min ( needed, queueSize );

        for ( size_t i = 0; i < availableToRead; ++i )
        {
            pcm[i] = instance->decodedPcmQueue.front();
            instance->decodedPcmQueue.pop_front();
        }
        queueAvailableForLog = queueSize;
        jitterTargetFramesNow = instance->jitterTargetFrames;
        availableToReadForLog = availableToRead;
    }

    const bool sawIncomingAudio = !packets.empty();
    const bool hasDecodedAudio  = ( availableToReadForLog > 0 );
    if ( sawIncomingAudio || hasDecodedAudio )
        instance->streamNoDataBlocks = 0;
    else
        ++instance->streamNoDataBlocks;

    const bool keepStreamOpen = ( instance->streamNoDataBlocks <= ( jitterTargetFramesNow + 2 ) );
    const float targetStreamGain = keepStreamOpen ? 1.0f : 0.0f;
    const float fadeTimeSec = 0.040f;
    const float gainStep = ( frames > 0 ) ? ( 1.0f / std::max ( 1.0f, static_cast<float> ( frames ) * fadeTimeSec ) ) : 1.0f;
    float gainBeg = instance->streamOutputGain;
    float gainEnd = gainBeg;
    if ( gainBeg < targetStreamGain )
        gainEnd = std::min ( targetStreamGain, gainBeg + gainStep * static_cast<float> ( frames ) );
    else if ( gainBeg > targetStreamGain )
        gainEnd = std::max ( targetStreamGain, gainBeg - gainStep * static_cast<float> ( frames ) );
    instance->streamOutputGain = gainEnd;

    int   maxPcmAbs = 0;
    float maxMixedAbs = 0.0f;

    if ( codecCh == 1 )
    {
        uint64_t mixedFrames = 0;
        const float frameStep = ( frames > 0 ) ? ( gainEnd - gainBeg ) / static_cast<float> ( frames ) : 0.0f;
        float curGain = gainBeg;
        for ( int i = 0; i < frames; ++i )
        {
            maxPcmAbs = std::max ( maxPcmAbs, std::abs ( static_cast<int> ( pcm[static_cast<size_t> ( i )] ) ) );
            const float s = ( static_cast<float> ( pcm[static_cast<size_t> ( i )] ) / 32768.0f ) * curGain;
            maxMixedAbs = std::max ( maxMixedAbs, std::abs ( s ) );
            if ( std::abs ( s ) > 1.0e-6f )
                ++mixedFrames;
            const int   base = i * channels;
            for ( int ch = 0; ch < channels; ++ch )
                out[base + ch] += s;
            curGain += frameStep;
        }
        instance->mixedFramesWithSignal.fetch_add ( mixedFrames, std::memory_order_relaxed );
    }
    else
    {
        uint64_t mixedFrames = 0;
        const float frameStep = ( frames > 0 ) ? ( gainEnd - gainBeg ) / static_cast<float> ( frames ) : 0.0f;
        float curGain = gainBeg;
        for ( int i = 0; i < frames; ++i )
        {
            maxPcmAbs = std::max ( maxPcmAbs, std::abs ( static_cast<int> ( pcm[static_cast<size_t> ( i * 2 )] ) ) );
            maxPcmAbs = std::max ( maxPcmAbs, std::abs ( static_cast<int> ( pcm[static_cast<size_t> ( i * 2 + 1 )] ) ) );
            const float l = ( static_cast<float> ( pcm[static_cast<size_t> ( i * 2 )] ) / 32768.0f ) * curGain;
            const float r = ( static_cast<float> ( pcm[static_cast<size_t> ( i * 2 + 1 )] ) / 32768.0f ) * curGain;
            maxMixedAbs = std::max ( maxMixedAbs, std::max ( std::abs ( l ), std::abs ( r ) ) );
            if ( std::max ( std::abs ( l ), std::abs ( r ) ) > 1.0e-6f )
                ++mixedFrames;
            const int   base = i * channels;
            out[base] += l;
            if ( channels > 1 )
                out[base + 1] += r;
            for ( int ch = 2; ch < channels; ++ch )
                out[base + ch] += 0.5f * ( l + r );
            curGain += frameStep;
        }
        instance->mixedFramesWithSignal.fetch_add ( mixedFrames, std::memory_order_relaxed );
    }

    {
        const uint32_t nowMs  = jamulus_get_time_ms();
        uint32_t       lastMs = instance->lastAudioDebugLogMs.load ( std::memory_order_relaxed );
        if ( nowMs - lastMs >= 1000 && instance->lastAudioDebugLogMs.compare_exchange_strong ( lastMs, nowMs, std::memory_order_relaxed ) )
        {
            const uint64_t queued      = instance->audioPacketsQueued.load ( std::memory_order_relaxed );
            const uint64_t rejected    = instance->audioPacketsRejected.load ( std::memory_order_relaxed );
            const uint64_t rejectLarge = instance->audioRejectTooLarge.load ( std::memory_order_relaxed );
            const uint64_t rejectFrame = instance->audioRejectFramedMessage.load ( std::memory_order_relaxed );
            const uint64_t rejectSize  = instance->audioRejectSizeMismatch.load ( std::memory_order_relaxed );
            const uint64_t wrappedQ    = instance->wrappedAudioQueued.load ( std::memory_order_relaxed );
            const uint64_t wrappedR    = instance->wrappedAudioRejected.load ( std::memory_order_relaxed );
            const uint64_t decTry      = instance->decodeAttempts.load ( std::memory_order_relaxed );
            const uint64_t decOk       = instance->decodeSuccess.load ( std::memory_order_relaxed );
            const uint64_t decFail     = instance->decodeFail.load ( std::memory_order_relaxed );
            const uint64_t decSamples  = instance->decodedSamples.load ( std::memory_order_relaxed );
            const uint64_t mixedFrames = instance->mixedFramesWithSignal.load ( std::memory_order_relaxed );
            const uint64_t levelMsg    = instance->channelLevelMessages.load ( std::memory_order_relaxed );
            const uint64_t levelUp     = instance->channelLevelUpdates.load ( std::memory_order_relaxed );
            const int      cfgBytes    = codedBytes;
            const int      cfgBlock    = blockFactor;
            const int      cfgCh       = codecCh;
            const int      cfgType     = cfg.codingType;
            const int      cfgSeq      = useSequenceNumber ? 1 : 0;
            const int      cfgProps    = instance->gotNetTransportProps.load ( std::memory_order_relaxed ) ? 1 : 0;
            const int      cfgJitterFrames = jitterTargetFramesNow;
            const int      cfgQueueSamples = static_cast<int> ( queueAvailableForLog );
            const int      cfgReadSamples  = static_cast<int> ( availableToReadForLog );
            const int      cfgNoDataBlocks = instance->streamNoDataBlocks;
            const float    cfgMixGain      = instance->streamOutputGain;
            const int      cfgMaxPcmAbs    = maxPcmAbs;
            const float    cfgMaxMixedAbs  = maxMixedAbs;
            jamulus_core_log ( "audio_trace queued=" + std::to_string ( queued ) +
                               " rejected=" + std::to_string ( rejected ) +
                               " rejectTooLarge=" + std::to_string ( rejectLarge ) +
                               " rejectFrame=" + std::to_string ( rejectFrame ) +
                               " rejectSize=" + std::to_string ( rejectSize ) +
                               " wrappedQueued=" + std::to_string ( wrappedQ ) +
                               " wrappedRejected=" + std::to_string ( wrappedR ) +
                               " decodeTry=" + std::to_string ( decTry ) +
                               " decodeOk=" + std::to_string ( decOk ) +
                               " decodeFail=" + std::to_string ( decFail ) +
                               " decodedSamples=" + std::to_string ( decSamples ) +
                               " mixedFrames=" + std::to_string ( mixedFrames ) +
                               " levelMsgs=" + std::to_string ( levelMsg ) +
                               " levelUpdates=" + std::to_string ( levelUp ) +
                               " cfgType=" + std::to_string ( cfgType ) +
                               " cfgCh=" + std::to_string ( cfgCh ) +
                               " cfgBytes=" + std::to_string ( cfgBytes ) +
                               " cfgBlock=" + std::to_string ( cfgBlock ) +
                               " cfgSeq=" + std::to_string ( cfgSeq ) +
                               " cfgProps=" + std::to_string ( cfgProps ) +
                               " jitterFrames=" + std::to_string ( cfgJitterFrames ) +
                               " queueSamples=" + std::to_string ( cfgQueueSamples ) +
                               " readSamples=" + std::to_string ( cfgReadSamples ) +
                               " noDataBlocks=" + std::to_string ( cfgNoDataBlocks ) +
                               " mixGain=" + std::to_string ( cfgMixGain ) +
                               " maxPcmAbs=" + std::to_string ( cfgMaxPcmAbs ) +
                               " maxMixedAbs=" + std::to_string ( cfgMaxMixedAbs ) );
        }
    }

    return 0;
}

jamulus_core_t jamulus_core_create ( unsigned short port,
                                     const char*    server_addr,
                                     const char*    client_name,
                                     bool           enable_ipv6 )
{
    JamulusCoreInstance* instance = new ( std::nothrow ) JamulusCoreInstance();
    if ( !instance )
        return nullptr;

    instance->port = port;
    if ( server_addr )
        instance->serverAddress = server_addr;
    if ( client_name )
        instance->clientName = client_name;
    instance->enableIPv6 = enable_ipv6;
    if ( !instance->audioEngine.init ( port,
                                       server_addr,
                                       client_name,
                                       enable_ipv6 ) )
    {
        delete instance;
        return nullptr;
    }

    return instance;
}

void jamulus_core_destroy ( jamulus_core_t core )
{
    JamulusCoreInstance* instance = static_cast<JamulusCoreInstance*> ( core );
    if ( !instance )
        return;
    if ( instance->workerThread.joinable() )
    {
        instance->workerStopRequested.store( true, std::memory_order_relaxed );
        instance->workerThread.join();
    }
    jamulus_core_close_socket ( instance );
    jamulus_codec_release ( instance );
    instance->audioEngine.shutdown();
    delete instance;
}

static void jamulus_core_worker ( JamulusCoreInstance* instance )
{
    uint64_t counter = 0;
    int      splitMessageCnt = 0;
    int      splitMessageDataIndex = 0;
    std::vector<uint8_t> splitMessageStorage ( 65535 );

    jamulus_core_log ( "worker: start" );

    while ( !instance->workerStopRequested.load( std::memory_order_relaxed ) )
    {
        jamulus_flush_pending_pan_messages ( instance );

        if ( instance->runningFlag.load( std::memory_order_relaxed ) && instance->udpSocket >= 0 )
        {
            if ( instance->serverSockAddrLen > 0 )
            {
                if ( ( counter % 500 ) == 0 )
                {
                    std::vector<uint8_t> data ( 4 );
                    const uint32_t       stamp = jamulus_get_time_ms();

                    int pos = 0;
                    jamulus_put_val ( data, pos, stamp, 4 );

                    std::vector<uint8_t> frame = jamulus_build_frame ( JAMULUS_CLM_PING_MS, 0, data );

                    jamulus_core_log ( "worker: sending connected ping to " + jamulus_sockaddr_to_address_string ( instance->serverSockAddr, instance->serverSockAddrLen ) );
                    ::sendto ( instance->udpSocket,
                               reinterpret_cast<const char*> ( frame.data() ),
                               static_cast<int> ( frame.size() ),
                               0,
                               reinterpret_cast<const sockaddr*> ( &instance->serverSockAddr ),
                               instance->serverSockAddrLen );
                }

                if ( !instance->bootstrapMessagesSent.exchange ( true, std::memory_order_relaxed ) )
                    jamulus_send_connection_setup_messages ( instance );

                if ( ( counter % 250 ) == 0 && instance->audioEngine.isConnected() )
                {
                    std::vector<uint8_t> data;
                    std::vector<uint8_t> frame = jamulus_build_frame ( JAMULUS_CLM_REQ_CONN_CLIENTS_LIST, 0, data );
                    ::sendto ( instance->udpSocket,
                               reinterpret_cast<const char*> ( frame.data() ),
                               static_cast<int> ( frame.size() ),
                               0,
                               reinterpret_cast<const sockaddr*> ( &instance->serverSockAddr ),
                               instance->serverSockAddrLen );
                }
            }

            for ( ;; )
            {
                static constexpr int BUF_SIZE = 65535;
                uint8_t        buf[BUF_SIZE];
                sockaddr_storage fromAddr{};
                socklen_t      fromLen = sizeof ( fromAddr );

                int bytes = static_cast<int> ( ::recvfrom ( instance->udpSocket,
                                                           reinterpret_cast<char*> ( buf ),
                                                           BUF_SIZE,
                                                           0,
                                                           reinterpret_cast<sockaddr*> ( &fromAddr ),
                                                           &fromLen ) );

                if ( bytes <= 0 )
                {
#ifdef _WIN32
                    int err = WSAGetLastError();
                    if ( err == WSAEWOULDBLOCK )
                        break;
                    jamulus_core_log ( "worker: recvfrom error err=" + std::to_string ( err ) );
#else
                    if ( errno == EAGAIN || errno == EWOULDBLOCK )
                        break;
                    jamulus_core_log ( "worker: recvfrom error errno=" + std::to_string ( errno ) );
#endif
                    break;
                }

                const std::string fromAddress = jamulus_sockaddr_to_address_string ( fromAddr, fromLen );
                const bool fromSelectedServer = !fromAddress.empty() && jamulus_is_selected_server ( instance, fromAddress );

                if ( fromSelectedServer && jamulus_try_store_audio_packet ( instance, buf, bytes ) )
                    continue;

                if ( fromSelectedServer && !jamulus_is_valid_frame ( buf, bytes ) )
                    continue;

                if ( bytes < JAMULUS_MESS_HEADER_LENGTH_BYTE + 2 )
                    continue;

                const uint8_t* buf8    = buf;
                const int      bufLen  = bytes;
                uint16_t msgId   = static_cast<uint16_t> ( buf8[2] | ( static_cast<uint16_t> ( buf8[3] ) << 8 ) );
                const uint8_t  msgCounter = buf8[4];
                const uint16_t dataLen = static_cast<uint16_t> ( buf8[5] | ( static_cast<uint16_t> ( buf8[6] ) << 8 ) );

                if ( bufLen < JAMULUS_MESS_HEADER_LENGTH_BYTE + static_cast<int> ( dataLen ) + 2 )
                    continue;

                const uint8_t* data       = buf8 + JAMULUS_MESS_HEADER_LENGTH_BYTE;
                int            dataLenInt = static_cast<int> ( dataLen );

                if ( msgId == JAMULUS_SPECIAL_SPLIT_MESSAGE )
                {
                    if ( dataLenInt < 4 )
                    {
                        splitMessageCnt = 0;
                        splitMessageDataIndex = 0;
                        continue;
                    }

                    int splitPos = 0;
                    const uint16_t wrappedMsgId = static_cast<uint16_t> ( jamulus_get_val ( data, dataLenInt, splitPos, 2 ) );
                    const int      numParts     = static_cast<int> ( jamulus_get_val ( data, dataLenInt, splitPos, 1 ) );
                    const int      splitCnt     = static_cast<int> ( jamulus_get_val ( data, dataLenInt, splitPos, 1 ) );
                    const int      partSize     = dataLenInt - splitPos;

                    if ( numParts <= 0 || splitCnt < 0 || splitCnt >= numParts || partSize < 0 )
                    {
                        splitMessageCnt = 0;
                        splitMessageDataIndex = 0;
                        continue;
                    }

                    if ( splitCnt == 0 )
                    {
                        splitMessageCnt = 0;
                        splitMessageDataIndex = 0;
                    }

                    if ( splitMessageCnt != splitCnt )
                    {
                        splitMessageCnt = 0;
                        splitMessageDataIndex = 0;
                        continue;
                    }

                    if ( splitMessageDataIndex + partSize > static_cast<int> ( splitMessageStorage.size() ) )
                        splitMessageStorage.resize ( static_cast<size_t> ( splitMessageDataIndex + partSize ) );

                    if ( partSize > 0 )
                    {
                        std::memcpy ( splitMessageStorage.data() + splitMessageDataIndex, data + splitPos, static_cast<size_t> ( partSize ) );
                        splitMessageDataIndex += partSize;
                    }

                    splitMessageCnt++;

                    if ( splitMessageCnt < numParts )
                        continue;

                    msgId = wrappedMsgId;
                    data = splitMessageStorage.data();
                    dataLenInt = splitMessageDataIndex;
                    splitMessageCnt = 0;
                    splitMessageDataIndex = 0;
                }

                if ( msgId != JAMULUS_CLM_PING_MS && msgId != JAMULUS_CLM_PING_MS_WITHNUMCLIENTS )
                    jamulus_core_log ( "worker: received msgId=" + std::to_string ( msgId ) + " bytes=" + std::to_string ( bytes ) );

                if ( fromSelectedServer && msgId < 1000 && msgId != JAMULUS_PM_ACKN )
                    jamulus_send_ack_message ( instance, msgId, msgCounter );

                if ( msgId == JAMULUS_PM_CLIENT_ID )
                {
                    int pos = 0;
                    if ( dataLenInt >= 1 )
                    {
                        const int myId = static_cast<int> ( jamulus_get_val ( data, dataLenInt, pos, 1 ) );
                        instance->myChannelId.store ( myId, std::memory_order_relaxed );
                        instance->connectedFlag.store ( true, std::memory_order_relaxed );
                    }
                    continue;
                }

                if ( msgId == JAMULUS_PM_NETW_TRANSPORT_PROPS )
                {
                    jamulus_handle_net_transport_props ( instance, data, dataLenInt );
                    continue;
                }

                if ( msgId == JAMULUS_PM_REQ_SPLIT_SUPPORT )
                {
                    jamulus_send_split_supported_message ( instance );
                    continue;
                }

                if ( msgId == JAMULUS_PM_REQ_NETW_TRANSPORT_PROPS )
                {
                    jamulus_send_transport_props_message ( instance );
                    continue;
                }

                if ( msgId == JAMULUS_PM_JITT_BUF_SIZE )
                {
                    int pos = 0;
                    if ( dataLenInt >= 2 && instance->audioEngine.getAutoJitter() )
                    {
                        const int newJitBufSize = std::clamp ( static_cast<int> ( jamulus_get_val ( data, dataLenInt, pos, 2 ) ), 1, MAX_NET_BUF_SIZE_NUM_BL );
                        instance->audioEngine.setServerJitterBuffer ( newJitBufSize );
                    }
                    continue;
                }

                if ( msgId == JAMULUS_PM_REQ_JITT_BUF_SIZE )
                {
                    jamulus_send_jitter_buffer_message ( instance );
                    continue;
                }

                if ( msgId == JAMULUS_PM_REQ_CHANNEL_INFOS )
                {
                    jamulus_send_channel_info_message ( instance );
                    continue;
                }

                if ( msgId == JAMULUS_CLM_PING_MS_WITHNUMCLIENTS )
                {
                    int pos = 0;
                    if ( dataLenInt < 5 )
                        continue;

                    const uint32_t stamp      = jamulus_get_val ( data, dataLenInt, pos, 4 );
                    const uint32_t numClients = jamulus_get_val ( data, dataLenInt, pos, 1 );

                    const uint32_t nowMs = jamulus_get_time_ms();
                    uint32_t       ping  = 0;
                    if ( nowMs > stamp )
                        ping = nowMs - stamp;

                    if ( fromAddress.empty() )
                        continue;
                    if ( fromSelectedServer )
                        jamulus_update_network_stats ( instance, static_cast<int> ( ping ) );

                    std::lock_guard<std::mutex> lock ( instance->serverListMutex );
                    bool updated = false;
                    for ( auto& entry : instance->serverList )
                    {
                        if ( entry.address == fromAddress )
                        {
                            entry.ping       = static_cast<int> ( ping );
                            entry.numClients = static_cast<int> ( numClients );
                            jamulus_core_log ( "ping_with_num: " + fromAddress + " ping=" + std::to_string ( ping ) + "ms clients=" + std::to_string ( entry.numClients ) );
                            updated = true;
                            break;
                        }
                    }
                    if ( !updated )
                    {
                        JamulusCoreServerEntry e{};
                        e.address    = fromAddress;
                        e.ping       = static_cast<int> ( ping );
                        e.numClients = static_cast<int> ( numClients );
                        instance->serverList.push_back ( std::move ( e ) );
                        jamulus_core_log ( "ping_with_num: created entry " + fromAddress + " ping=" + std::to_string ( ping ) + "ms clients=" + std::to_string ( numClients ) );
                    }
                }
                else if ( msgId == JAMULUS_CLM_PING_MS )
                {
                    int pos = 0;
                    if ( dataLenInt < 4 )
                        continue;
                    const uint32_t stamp = jamulus_get_val ( data, dataLenInt, pos, 4 );
                    const uint32_t nowMs = jamulus_get_time_ms();
                    uint32_t       ping  = 0;
                    if ( nowMs > stamp )
                        ping = nowMs - stamp;
                    if ( fromAddress.empty() )
                        continue;
                    if ( fromSelectedServer )
                        jamulus_update_network_stats ( instance, static_cast<int> ( ping ) );
                    std::lock_guard<std::mutex> lock ( instance->serverListMutex );
                    bool updated = false;
                    for ( auto& entry : instance->serverList )
                    {
                        if ( entry.address == fromAddress )
                        {
                            entry.ping = static_cast<int> ( ping );
                            jamulus_core_log ( "ping_ms: " + fromAddress + " ping=" + std::to_string ( ping ) + "ms" );
                            updated = true;
                            break;
                        }
                    }
                    if ( !updated )
                    {
                        JamulusCoreServerEntry e{};
                        e.address = fromAddress;
                        e.ping    = static_cast<int> ( ping );
                        instance->serverList.push_back ( std::move ( e ) );
                        jamulus_core_log ( "ping_ms: created entry " + fromAddress + " ping=" + std::to_string ( ping ) + "ms" );
                    }
                }
                else if ( msgId == JAMULUS_CLM_VERSION_AND_OS )
                {
                    int pos = 0;
                    if ( dataLenInt < 3 || fromAddress.empty() )
                        continue;

                    (void) jamulus_get_val ( data, dataLenInt, pos, 1 );

                    std::string version;
                    if ( !jamulus_get_short_string ( data, dataLenInt, pos, version ) )
                        continue;

                    if ( pos != dataLenInt )
                        continue;

                    const auto displayVersion = jamulus_get_display_version ( version );

                    std::lock_guard<std::mutex> lock ( instance->serverListMutex );
                    bool updated = false;
                    for ( auto& entry : instance->serverList )
                    {
                        if ( entry.address == fromAddress )
                        {
                            entry.version = displayVersion;
                            updated       = true;
                            jamulus_core_log ( "server_version: " + fromAddress + " version=" + displayVersion );
                            break;
                        }
                    }
                    if ( !updated )
                    {
                        JamulusCoreServerEntry e{};
                        e.address = fromAddress;
                        e.version = displayVersion;
                        instance->serverList.push_back ( std::move ( e ) );
                        jamulus_core_log ( "server_version: created entry " + fromAddress + " version=" + displayVersion );
                    }
                }
                else if ( msgId == JAMULUS_CLM_RED_SERVER_LIST )
                {
                    int                                 pos = 0;
                    std::vector<JamulusCoreServerEntry> newList;

                    while ( pos < dataLenInt )
                    {
                        if ( ( dataLenInt - pos ) < 6 )
                            break;

                        const uint32_t ip   = jamulus_get_val ( data, dataLenInt, pos, 4 );
                        const uint32_t port = jamulus_get_val ( data, dataLenInt, pos, 2 );

                        if ( ( dataLenInt - pos ) < 1 )
                        {
                            newList.clear();
                            break;
                        }

                        const uint32_t nameLen = jamulus_get_val ( data, dataLenInt, pos, 1 );
                        if ( nameLen > 0 )
                        {
                            if ( pos + static_cast<int> ( nameLen ) > dataLenInt )
                            {
                                newList.clear();
                                break;
                            }

                            std::string name ( reinterpret_cast<const char*> ( data + pos ),
                                               reinterpret_cast<const char*> ( data + pos + static_cast<int> ( nameLen ) ) );
                            pos += static_cast<int> ( nameLen );

                            struct in_addr v4addr;
                            v4addr.s_addr = htonl ( ip );

                            char addrBuf[INET_ADDRSTRLEN] = {};
                            if ( inet_ntop ( AF_INET, &v4addr, addrBuf, sizeof ( addrBuf ) ) != nullptr )
                            {
                                JamulusCoreServerEntry entry;
                                entry.address = std::string ( addrBuf ) + ":" + std::to_string ( static_cast<unsigned short> ( port ) );
                                entry.name    = name;
                                newList.push_back ( entry );
                            }
                        }
                    }

                    if ( !newList.empty() )
                    {
                        std::lock_guard<std::mutex> lock ( instance->serverListMutex );
                        instance->serverList = std::move ( newList );
                        jamulus_core_log ( "worker: updated serverList entries=" + std::to_string ( instance->serverList.size() ) );
                    }
                }
                else if ( msgId == JAMULUS_CLM_SERVER_LIST )
                {
                    int                                 pos = 0;
                    std::vector<JamulusCoreServerEntry> newList;

                    while ( pos < dataLenInt )
                    {
                        if ( ( dataLenInt - pos ) < 10 )
                            break;

                        const uint32_t ip   = jamulus_get_val ( data, dataLenInt, pos, 4 );
                        const uint32_t port = jamulus_get_val ( data, dataLenInt, pos, 2 );

                        const uint16_t countryWire = static_cast<uint16_t> ( jamulus_get_val ( data, dataLenInt, pos, 2 ) );
                        const uint32_t maxClients  = jamulus_get_val ( data, dataLenInt, pos, 1 );
                        const uint32_t isPermanent = jamulus_get_val ( data, dataLenInt, pos, 1 );

                        if ( ( dataLenInt - pos ) < 2 )
                        {
                            newList.clear();
                            break;
                        }

                        const uint32_t nameLen = jamulus_get_val ( data, dataLenInt, pos, 2 );
                        if ( pos + static_cast<int> ( nameLen ) > dataLenInt )
                        {
                            newList.clear();
                            break;
                        }

                        std::string name ( reinterpret_cast<const char*> ( data + pos ),
                                           reinterpret_cast<const char*> ( data + pos + static_cast<int> ( nameLen ) ) );
                        pos += static_cast<int> ( nameLen );

                        if ( ( dataLenInt - pos ) < 2 )
                        {
                            newList.clear();
                            break;
                        }

                        const uint32_t emptyLen = jamulus_get_val ( data, dataLenInt, pos, 2 );
                        if ( pos + static_cast<int> ( emptyLen ) > dataLenInt )
                        {
                            newList.clear();
                            break;
                        }
                        pos += static_cast<int> ( emptyLen );

                        if ( ( dataLenInt - pos ) < 2 )
                        {
                            newList.clear();
                            break;
                        }

                        const uint32_t cityLen = jamulus_get_val ( data, dataLenInt, pos, 2 );
                        if ( pos + static_cast<int> ( cityLen ) > dataLenInt )
                        {
                            newList.clear();
                            break;
                        }

                        std::string city ( reinterpret_cast<const char*> ( data + pos ),
                                           reinterpret_cast<const char*> ( data + pos + static_cast<int> ( cityLen ) ) );
                        pos += static_cast<int> ( cityLen );

                        struct in_addr v4addr;
                        v4addr.s_addr = htonl ( ip );

                        char addrBuf[INET_ADDRSTRLEN] = {};
                        if ( inet_ntop ( AF_INET, &v4addr, addrBuf, sizeof ( addrBuf ) ) != nullptr )
                        {
                            JamulusCoreServerEntry entry;
                            entry.address     = std::string ( addrBuf ) + ":" +
                                            std::to_string ( static_cast<unsigned short> ( port ) );
                            entry.name        = name;
                            entry.city        = city;
                            entry.country     = jamulus_country_name_from_wire ( countryWire );
                            entry.maxClients  = static_cast<int> ( maxClients );
                            entry.isPermanent = ( isPermanent != 0 );
                            newList.push_back ( entry );
                        }
                    }

                    if ( !newList.empty() )
                    {
                        std::lock_guard<std::mutex> lock ( instance->serverListMutex );

                        for ( auto& entry : newList )
                        {
                            for ( const auto& oldEntry : instance->serverList )
                            {
                                if ( oldEntry.address == entry.address )
                                {
                                    entry.ping       = oldEntry.ping;
                                    entry.numClients = oldEntry.numClients;
                                    entry.version    = oldEntry.version;
                                    entry.players    = oldEntry.players;
                                    break;
                                }
                            }
                        }

                        instance->serverList = std::move ( newList );
                        jamulus_core_log ( "worker: updated full serverList entries=" +
                                           std::to_string ( instance->serverList.size() ) );
                    }
                }
                else if ( msgId == JAMULUS_CLM_CONN_CLIENTS_LIST || msgId == JAMULUS_PM_CONN_CLIENTS_LIST )
                {
                    const std::string fromAddress = jamulus_sockaddr_to_address_string ( fromAddr, fromLen );
                    if ( !fromSelectedServer )
                    {
                        if ( !fromAddress.empty() )
                            jamulus_core_log ( "clients_list: ignored stale sender " + fromAddress );
                        continue;
                    }

                    int         pos   = 0;
                    std::string names;
                    bool        first = true;
                    bool        parseOk = true;
                    std::vector<jamulus_channel_info_t> newChannels;

                    while ( pos < dataLenInt )
                    {
                        if ( ( dataLenInt - pos ) < 12 )
                        {
                            parseOk = false;
                            break;
                        }

                        const uint32_t chanId      = jamulus_get_val ( data, dataLenInt, pos, 1 );
                        const uint16_t countryWire = static_cast<uint16_t> ( jamulus_get_val ( data, dataLenInt, pos, 2 ) );
                        const uint32_t instrument  = jamulus_get_val ( data, dataLenInt, pos, 4 );
                        const uint32_t skill       = jamulus_get_val ( data, dataLenInt, pos, 1 );

                        if ( ( dataLenInt - pos ) < 4 )
                        {
                            names.clear();
                            parseOk = false;
                            break;
                        }
                        pos += 4;

                        if ( ( dataLenInt - pos ) < 2 )
                        {
                            names.clear();
                            parseOk = false;
                            break;
                        }
                        const uint32_t nameLen = jamulus_get_val ( data, dataLenInt, pos, 2 );
                        if ( pos + static_cast<int> ( nameLen ) > dataLenInt )
                        {
                            names.clear();
                            newChannels.clear();
                            parseOk = false;
                            break;
                        }

                        std::string name ( reinterpret_cast<const char*> ( data + pos ),
                                           reinterpret_cast<const char*> ( data + pos + static_cast<int> ( nameLen ) ) );
                        pos += static_cast<int> ( nameLen );

                        if ( ( dataLenInt - pos ) < 2 )
                        {
                            names.clear();
                            newChannels.clear();
                            parseOk = false;
                            break;
                        }
                        const uint32_t cityLen = jamulus_get_val ( data, dataLenInt, pos, 2 );
                        if ( pos + static_cast<int> ( cityLen ) > dataLenInt )
                        {
                            names.clear();
                            newChannels.clear();
                            parseOk = false;
                            break;
                        }

                        std::string city ( reinterpret_cast<const char*> ( data + pos ),
                                           reinterpret_cast<const char*> ( data + pos + static_cast<int> ( cityLen ) ) );
                        pos += static_cast<int> ( cityLen );

                        std::string iso = jamulus_country_iso_from_wire ( countryWire );
                        if ( iso.empty() )
                            iso = "none";

                        if ( !name.empty() )
                        {
                            if ( !first )
                                names.push_back ( '\n' );

                            names += name;
                            names.push_back ( '|' );
                            names += std::to_string ( static_cast<int> ( instrument ) );
                            names.push_back ( '|' );
                            names += iso;
                            first = false;
                        }

                        jamulus_channel_info_t ch{};
                        ch.id          = static_cast<int> ( chanId );
                        ch.instrument  = static_cast<int> ( instrument );
                        ch.skill_level = static_cast<int> ( skill );
                        ch.gain        = 1.0f;
                        ch.pan         = 0.0f;
                        ch.muted       = false;
                        ch.solo        = false;

                        std::strncpy ( ch.name, name.c_str(), sizeof ( ch.name ) - 1 );
                        ch.name[sizeof ( ch.name ) - 1] = '\0';

                        std::strncpy ( ch.country_code, iso.c_str(), sizeof ( ch.country_code ) - 1 );
                        ch.country_code[sizeof ( ch.country_code ) - 1] = '\0';

                        std::strncpy ( ch.city, city.c_str(), sizeof ( ch.city ) - 1 );
                        ch.city[sizeof ( ch.city ) - 1] = '\0';

                        newChannels.push_back ( ch );
                    }

                    if ( parseOk )
                    {
                        instance->connectedFlag.store ( true, std::memory_order_relaxed );
                        if ( !fromAddress.empty() )
                        {
                            std::lock_guard<std::mutex> lock ( instance->serverListMutex );
                            bool updated = false;
                            for ( auto& entry : instance->serverList )
                            {
                                if ( entry.address == fromAddress )
                                {
                                    entry.players    = names;
                                    entry.numClients = static_cast<int> ( newChannels.size() );
                                    updated          = true;
                                    break;
                                }
                            }
                            if ( !updated )
                            {
                                JamulusCoreServerEntry e{};
                                e.address    = fromAddress;
                                e.players    = names;
                                e.numClients = static_cast<int> ( newChannels.size() );
                                instance->serverList.push_back ( std::move ( e ) );
                                jamulus_core_log ( "clients_list: created entry " + fromAddress + " players=" + std::to_string ( newChannels.size() ) );
                            }
                        }

                        std::lock_guard<std::mutex> chLock ( instance->channelMutex );
                        for ( auto& newChannel : newChannels )
                        {
                            for ( const auto& oldChannel : instance->channels )
                            {
                                if ( oldChannel.id != newChannel.id )
                                    continue;
                                if ( newChannel.instrument <= 0 && oldChannel.instrument > 0 )
                                    newChannel.instrument = oldChannel.instrument;
                                if ( ( std::strcmp ( newChannel.country_code, "none" ) == 0 || newChannel.country_code[0] == '\0' ) &&
                                     oldChannel.country_code[0] != '\0' )
                                {
                                    std::strncpy ( newChannel.country_code, oldChannel.country_code, sizeof ( newChannel.country_code ) - 1 );
                                    newChannel.country_code[sizeof ( newChannel.country_code ) - 1] = '\0';
                                }
                                if ( newChannel.city[0] == '\0' && oldChannel.city[0] != '\0' )
                                {
                                    std::strncpy ( newChannel.city, oldChannel.city, sizeof ( newChannel.city ) - 1 );
                                    newChannel.city[sizeof ( newChannel.city ) - 1] = '\0';
                                }
                                newChannel.gain  = oldChannel.gain;
                                newChannel.pan   = oldChannel.pan;
                                newChannel.muted = oldChannel.muted;
                                newChannel.solo  = oldChannel.solo;
                                newChannel.level = oldChannel.level;
                                break;
                            }
                        }
                        instance->channels = std::move ( newChannels );
                        const int inferredMyId = jamulus_infer_my_channel_id ( instance, instance->channels );
                        if ( inferredMyId >= 0 )
                            instance->myChannelId.store ( inferredMyId, std::memory_order_relaxed );
                        else
                            instance->myChannelId.store ( -1, std::memory_order_relaxed );
                    }
                }
                else if ( msgId == JAMULUS_CLM_CHANNEL_LEVEL_LIST )
                {
                    if ( !fromSelectedServer )
                        continue;

                    instance->channelLevelMessages.fetch_add ( 1, std::memory_order_relaxed );
                    int                     pos         = 0;
                    std::vector<uint16_t>   levels;
                    levels.reserve ( dataLenInt * 2 );

                    while ( pos < dataLenInt )
                    {
                        uint8_t byte = 0;
                        byte         = data[pos];
                        ++pos;

                        const uint16_t levelLo = static_cast<uint16_t> ( byte & 0x0f );
                        const uint16_t levelHi = static_cast<uint16_t> ( ( byte >> 4 ) & 0x0f );

                        levels.push_back ( levelLo );

                        if ( levelHi != 0x0f )
                        {
                            levels.push_back ( levelHi );
                        }
                        else
                        {
                            break;
                        }
                    }

                    if ( !levels.empty() )
                    {
                        instance->channelLevelUpdates.fetch_add ( static_cast<uint64_t> ( levels.size() ), std::memory_order_relaxed );
                        std::lock_guard<std::mutex> chLock ( instance->channelMutex );
                        for ( size_t i = 0; i < instance->channels.size(); ++i )
                        {
                            auto& ch = instance->channels[i];
                            if ( i < levels.size() )
                                ch.level = static_cast<uint16_t> ( levels[i] * 4096u );
                            else
                                ch.level = 0;
                        }
                    }
                }
                else if ( msgId == JAMULUS_PM_MUTE_STATE_CHANGED )
                {
                    if ( !fromSelectedServer )
                        continue;

                    int pos = 0;
                    if ( dataLenInt >= 2 )
                    {
                        const int  chanId = static_cast<int> ( jamulus_get_val ( data, dataLenInt, pos, 1 ) );
                        const bool muted  = ( jamulus_get_val ( data, dataLenInt, pos, 1 ) != 0 );
                        std::lock_guard<std::mutex> chLock ( instance->channelMutex );
                        for ( auto& ch : instance->channels )
                        {
                            if ( ch.id == chanId )
                            {
                                ch.muted = muted;
                                break;
                            }
                        }
                    }
                }
                else
                {
                    if ( jamulus_try_store_audio_packet ( instance, data, dataLenInt ) )
                        instance->wrappedAudioQueued.fetch_add ( 1, std::memory_order_relaxed );
                    else
                        instance->wrappedAudioRejected.fetch_add ( 1, std::memory_order_relaxed );
                }
            }

            ++counter;
        }

        std::this_thread::sleep_for ( std::chrono::milliseconds ( 1 ) );
    }
}

int jamulus_core_start ( jamulus_core_t core )
{
    JamulusCoreInstance* instance = static_cast<JamulusCoreInstance*> ( core );
    if ( !instance )
        return -1;

    std::lock_guard<std::mutex> lifecycleLock ( instance->lifecycleMutex );

    if ( !jamulus_core_open_socket ( instance ) )
        return -1;

    instance->runningFlag.store( true, std::memory_order_relaxed );
    instance->connectedFlag.store( !instance->serverAddress.empty(), std::memory_order_relaxed );
    instance->bootstrapMessagesSent.store( false, std::memory_order_relaxed );
    instance->myChannelId.store( -1, std::memory_order_relaxed );
    jamulus_update_network_stats ( instance, -1 );
    jamulus_reset_pan_rate_limit ( instance );
    {
        std::lock_guard<std::mutex> lock ( instance->audioMutex );
        instance->encodedIncomingPackets.clear();
        instance->decodedPcmQueue.clear();
        instance->sendSequenceNumber = 0;
        instance->receiveSequenceLocked = false;
        instance->expectedReceiveSequence = 0;
        instance->jitterTargetFrames = DEF_NET_BUF_SIZE_NUM_BL;
        instance->jitterUnderRuns = 0;
        instance->jitterOverRuns = 0;
        instance->streamOutputGain = 0.0f;
        instance->streamNoDataBlocks = 0;
    }

    instance->audioEngine.start ( instance->serverAddress );

    jamulus_core_log ( "core_start: port=" + std::to_string ( instance->port ) + " serverAddr=" + instance->serverAddress );

    if ( !instance->workerThread.joinable() )
    {
        instance->workerStopRequested.store( false, std::memory_order_relaxed );
        instance->workerThread = std::thread( jamulus_core_worker, instance );
    }

    return 0;
}

int jamulus_core_stop ( jamulus_core_t core )
{
    JamulusCoreInstance* instance = static_cast<JamulusCoreInstance*> ( core );
    if ( !instance )
        return -1;

    std::lock_guard<std::mutex> lifecycleLock ( instance->lifecycleMutex );

    instance->runningFlag.store( false, std::memory_order_relaxed );
    instance->connectedFlag.store( false, std::memory_order_relaxed );
    instance->bootstrapMessagesSent.store( false, std::memory_order_relaxed );
    instance->workerStopRequested.store( true, std::memory_order_relaxed );
    instance->myChannelId.store( -1, std::memory_order_relaxed );
    jamulus_update_network_stats ( instance, -1 );
    jamulus_reset_pan_rate_limit ( instance );

    instance->audioEngine.stop();

    if ( instance->workerThread.joinable() )
    {
        instance->workerThread.join();
    }

    {
        std::lock_guard<std::mutex> lock( instance->channelMutex );
        instance->channels.clear();
    }
    {
        std::lock_guard<std::mutex> lock ( instance->audioMutex );
        instance->encodedIncomingPackets.clear();
        instance->decodedPcmQueue.clear();
        instance->sendSequenceNumber = 0;
        instance->receiveSequenceLocked = false;
        instance->expectedReceiveSequence = 0;
        instance->jitterTargetFrames = DEF_NET_BUF_SIZE_NUM_BL;
        instance->jitterUnderRuns = 0;
        instance->jitterOverRuns = 0;
        instance->streamOutputGain = 0.0f;
        instance->streamNoDataBlocks = 0;
    }

    return 0;
}

int jamulus_core_set_server_addr ( jamulus_core_t core, const char* addr )
{
    JamulusCoreInstance* instance = static_cast<JamulusCoreInstance*> ( core );
    if ( !instance )
        return -1;

    std::lock_guard<std::mutex> lifecycleLock ( instance->lifecycleMutex );

    if ( addr )
        instance->serverAddress = addr;
    else
        instance->serverAddress.clear();

    instance->bootstrapMessagesSent.store( false, std::memory_order_relaxed );

    // Ensure the selected server appears in serverList so ping/players can be tracked
    if ( !instance->serverAddress.empty() )
    {
        std::lock_guard<std::mutex> lock ( instance->serverListMutex );
        bool found = false;
        for ( const auto& entry : instance->serverList )
        {
            if ( entry.address == instance->serverAddress )
            {
                found = true;
                break;
            }
        }
        if ( !found )
        {
            JamulusCoreServerEntry e{};
            e.address = instance->serverAddress;
            instance->serverList.push_back ( std::move ( e ) );
            jamulus_core_log ( "added selected server to list: " + instance->serverAddress );
        }
    }

    if ( !jamulus_core_open_socket ( instance ) )
        return -1;

    const int setAddrRc = instance->audioEngine.setServerAddr ( addr );
    if ( setAddrRc != 0 )
        return -1;
    instance->myChannelId.store( -1, std::memory_order_relaxed );
    jamulus_update_network_stats ( instance, -1 );
    jamulus_reset_pan_rate_limit ( instance );
    {
        std::lock_guard<std::mutex> lock ( instance->audioMutex );
        instance->encodedIncomingPackets.clear();
        instance->decodedPcmQueue.clear();
        instance->sendSequenceNumber = 0;
        instance->receiveSequenceLocked = false;
        instance->expectedReceiveSequence = 0;
        instance->jitterTargetFrames = DEF_NET_BUF_SIZE_NUM_BL;
        instance->jitterUnderRuns = 0;
        instance->jitterOverRuns = 0;
        instance->streamOutputGain = 0.0f;
        instance->streamNoDataBlocks = 0;
    }

    return 0;
}

int jamulus_core_is_running ( jamulus_core_t core )
{
    JamulusCoreInstance* instance = static_cast<JamulusCoreInstance*> ( core );
    if ( !instance )
        return 0;

    return instance->runningFlag.load( std::memory_order_relaxed ) ? 1 : 0;
}

int jamulus_core_is_connected ( jamulus_core_t core )
{
    JamulusCoreInstance* instance = static_cast<JamulusCoreInstance*> ( core );
    if ( !instance )
        return 0;
    if ( !instance->runningFlag.load( std::memory_order_relaxed ) )
        return 0;
    return instance->audioEngine.isConnected() ? 1 : 0;
}

void jamulus_core_disconnect ( jamulus_core_t core )
{
    JamulusCoreInstance* instance = static_cast<JamulusCoreInstance*> ( core );
    if ( !instance )
        return;

    std::lock_guard<std::mutex> lifecycleLock ( instance->lifecycleMutex );
    instance->runningFlag.store( false, std::memory_order_relaxed );
    instance->connectedFlag.store( false, std::memory_order_relaxed );
    instance->bootstrapMessagesSent.store( false, std::memory_order_relaxed );
    instance->myChannelId.store( -1, std::memory_order_relaxed );
    jamulus_update_network_stats ( instance, -1 );
    jamulus_reset_pan_rate_limit ( instance );
    {
        std::lock_guard<std::mutex> lock( instance->channelMutex );
        instance->channels.clear();
    }
    instance->audioEngine.stop();
    instance->audioEngine.disconnect();
}

int jamulus_core_process_audio ( jamulus_core_t core,
                                 const float*   in,
                                 float*         out,
                                 int            frames,
                                 int            channels )
{
    JamulusCoreInstance* instance = static_cast<JamulusCoreInstance*> ( core );
    if ( !instance )
        return -1;

    std::lock_guard<std::mutex> lifecycleLock ( instance->lifecycleMutex );

#if defined( JAMULUS_NONQT_CORE )
    return jamulus_nonqt_process_audio ( instance, in, out, frames, channels );
#else

    if ( !out || frames <= 0 || channels <= 0 )
        return 0;

    const int totalSamples = frames * channels;
    bool       usedEngine   = false;

    if ( instance->audioEngine.isRunning() )
    {
        int rc = instance->audioEngine.processAudio ( in, out, frames, channels );
        if ( rc >= 0 )
            usedEngine = true;
        else
            std::fill ( out, out + totalSamples, 0.0f );
    }

    if ( !usedEngine )
    {
        if ( in )
        {
            for ( int i = 0; i < totalSamples; ++i )
                out[i] = in[i];
        }
        else
        {
            for ( int i = 0; i < totalSamples; ++i )
                out[i] = 0.0f;
        }
    }

    return 0;
#endif
}

int jamulus_core_get_num_channels ( jamulus_core_t core )
{
    JamulusCoreInstance* instance = static_cast<JamulusCoreInstance*> ( core );
    if ( !instance )
        return 0;
    std::lock_guard<std::mutex> lock ( instance->channelMutex );
    return static_cast<int> ( instance->channels.size() );
}

int jamulus_core_get_my_channel_id ( jamulus_core_t core )
{
    JamulusCoreInstance* instance = static_cast<JamulusCoreInstance*> ( core );
    if ( !instance )
        return -1;
    return instance->myChannelId.load ( std::memory_order_relaxed );
}

bool jamulus_core_get_channel_info ( jamulus_core_t core, int index, jamulus_channel_info_t* info )
{
    JamulusCoreInstance* instance = static_cast<JamulusCoreInstance*> ( core );
    if ( !instance || !info )
        return false;
    {
        std::lock_guard<std::mutex> lock ( instance->channelMutex );
        if ( index >= 0 && index < static_cast<int> ( instance->channels.size() ) )
        {
            *info = instance->channels[static_cast<size_t> ( index )];
            jamulus_channel_info_t clientInfo{};
            if ( instance->audioEngine.getChannelInfo ( index, &clientInfo ) )
            {
                info->gain  = clientInfo.gain;
                info->pan   = clientInfo.pan;
                info->muted = clientInfo.muted;
                info->solo  = clientInfo.solo;
                info->level = clientInfo.level;
            }
            return true;
        }
    }
    return false;
}

void jamulus_core_set_channel_gain ( jamulus_core_t core, int channel_id, float gain )
{
    JamulusCoreInstance* instance = static_cast<JamulusCoreInstance*> ( core );
    if ( !instance )
        return;
#if defined( JAMULUS_NONQT_CORE )
    jamulus_send_channel_gain_message ( instance, channel_id, gain );
#else
    instance->audioEngine.setChannelGain ( channel_id, gain );
#endif
}

void jamulus_core_set_channel_pan ( jamulus_core_t core, int channel_id, float pan )
{
    JamulusCoreInstance* instance = static_cast<JamulusCoreInstance*> ( core );
    if ( !instance )
        return;
#if defined( JAMULUS_NONQT_CORE )
    const float sanitizedPan = jamulus_sanitize_pan_value ( pan );
    {
        std::lock_guard<std::mutex> lock ( instance->channelMutex );
        for ( auto& ch : instance->channels )
        {
            if ( ch.id == channel_id )
            {
                ch.pan = sanitizedPan;
                break;
            }
        }
    }
    jamulus_send_channel_pan_message ( instance, channel_id, sanitizedPan );
#else
    instance->audioEngine.setChannelPan ( channel_id, pan );
#endif
}

void jamulus_core_set_channel_mute ( jamulus_core_t core, int channel_id, bool muted )
{
    JamulusCoreInstance* instance = static_cast<JamulusCoreInstance*> ( core );
    if ( !instance )
        return;
#if defined( JAMULUS_NONQT_CORE )
    jamulus_send_channel_mute_message ( instance, channel_id, muted );
#else
    instance->audioEngine.setChannelMute ( channel_id, muted );
#endif
}

void jamulus_core_set_channel_solo ( jamulus_core_t core, int channel_id, bool solo )
{
    JamulusCoreInstance* instance = static_cast<JamulusCoreInstance*> ( core );
    if ( !instance )
        return;
    instance->audioEngine.setChannelSolo ( channel_id, solo );
}

int jamulus_core_send_chat_message ( jamulus_core_t core, const char* message )
{
    JamulusCoreInstance* instance = static_cast<JamulusCoreInstance*> ( core );
    if ( !instance )
        return -1;
    if ( !message || !*message )
        return 0;
    instance->audioEngine.sendChatMessage ( message );
    return 0;
}

int jamulus_core_send_ping ( jamulus_core_t core )
{
    JamulusCoreInstance* instance = static_cast<JamulusCoreInstance*> ( core );
    if ( !instance )
        return -1;
#if defined( JAMULUS_NONQT_CORE )
    if ( instance->udpSocket < 0 || instance->serverSockAddrLen <= 0 )
        return -1;
    std::vector<uint8_t> data ( 4 );
    int                  pos = 0;
    jamulus_put_val ( data, pos, jamulus_get_time_ms(), 4 );
    std::vector<uint8_t> frame = jamulus_build_frame ( JAMULUS_CLM_PING_MS, 0, data );
    const int            rc    = ::sendto ( instance->udpSocket,
                                 reinterpret_cast<const char*> ( frame.data() ),
                                 static_cast<int> ( frame.size() ),
                                 0,
                                 reinterpret_cast<const sockaddr*> ( &instance->serverSockAddr ),
                                 instance->serverSockAddrLen );
    return rc >= 0 ? 0 : -1;
#else
    instance->audioEngine.sendPing();
    return 0;
#endif
}

bool jamulus_core_get_network_stats ( jamulus_core_t core, jamulus_network_stats_t* stats )
{
    JamulusCoreInstance* instance = static_cast<JamulusCoreInstance*> ( core );
    if ( !instance || !stats )
        return false;
#if defined( JAMULUS_NONQT_CORE )
    stats->ping_time_ms        = instance->lastPingMs.load ( std::memory_order_relaxed );
    stats->total_delay_ms      = instance->lastTotalDelayMs.load ( std::memory_order_relaxed );
    stats->upload_rate_kbps    = jamulus_calculate_upload_rate_kbps ( instance->codecConfig );
    stats->jitter_buffer_status = instance->lastJitterStatus.load ( std::memory_order_relaxed );
    return true;
#else
    if ( !instance->audioEngine.getNetworkStats ( stats ) )
        return false;
    stats->upload_rate_kbps = jamulus_calculate_upload_rate_kbps ( instance->codecConfig );
    return true;
#endif
}

const char* jamulus_core_get_chat_message ( jamulus_core_t core )
{
    JamulusCoreInstance* instance = static_cast<JamulusCoreInstance*> ( core );
    if ( !instance )
        return nullptr;
    return instance->audioEngine.getChatMessage();
}

const char* jamulus_core_get_name ( jamulus_core_t core )
{
    JamulusCoreInstance* instance = static_cast<JamulusCoreInstance*> ( core );
    if ( !instance )
        return nullptr;
    if ( const char* n = instance->audioEngine.getName() )
        return n;
    return instance->profileName.c_str();
}

int jamulus_core_set_name ( jamulus_core_t core, const char* name )
{
    JamulusCoreInstance* instance = static_cast<JamulusCoreInstance*> ( core );
    if ( !instance )
        return -1;
    instance->profileName = name ? name : "";
    instance->audioEngine.setName ( instance->profileName.c_str() );
    return 0;
}

int jamulus_core_get_instrument ( jamulus_core_t core )
{
    JamulusCoreInstance* instance = static_cast<JamulusCoreInstance*> ( core );
    if ( !instance )
        return 0;
    int fromEngine = instance->audioEngine.getInstrument();
    if ( fromEngine != 0 )
        return fromEngine;
    return instance->profileInstrument;
}

int jamulus_core_set_instrument ( jamulus_core_t core, int instrument )
{
    JamulusCoreInstance* instance = static_cast<JamulusCoreInstance*> ( core );
    if ( !instance )
        return -1;
    instance->profileInstrument = instrument;
    instance->audioEngine.setInstrument ( instrument );
    return 0;
}

int jamulus_core_get_country ( jamulus_core_t core )
{
    JamulusCoreInstance* instance = static_cast<JamulusCoreInstance*> ( core );
    if ( !instance )
        return 0;
    int fromEngine = instance->audioEngine.getCountry();
    if ( fromEngine != 0 )
        return fromEngine;
    return instance->profileCountry;
}

int jamulus_core_set_country ( jamulus_core_t core, int country )
{
    JamulusCoreInstance* instance = static_cast<JamulusCoreInstance*> ( core );
    if ( !instance )
        return -1;
    instance->profileCountry = country;
    instance->profileCountryCode = jamulus_country_iso_from_wire ( static_cast<uint16_t> ( std::max ( 0, country ) ) );
    instance->audioEngine.setCountry ( country );
    return 0;
}

const char* jamulus_core_get_country_code ( jamulus_core_t core )
{
    JamulusCoreInstance* instance = static_cast<JamulusCoreInstance*> ( core );
    if ( !instance )
        return nullptr;
    if ( instance->profileCountryCode.empty() && instance->profileCountry > 0 )
        instance->profileCountryCode = jamulus_country_iso_from_wire ( static_cast<uint16_t> ( instance->profileCountry ) );
    return instance->profileCountryCode.c_str();
}

int jamulus_core_set_country_code ( jamulus_core_t core, const char* code )
{
    JamulusCoreInstance* instance = static_cast<JamulusCoreInstance*> ( core );
    if ( !instance )
        return -1;
    instance->profileCountryCode = code ? code : "";
    return 0;
}

const char* jamulus_core_get_city ( jamulus_core_t core )
{
    JamulusCoreInstance* instance = static_cast<JamulusCoreInstance*> ( core );
    if ( !instance )
        return nullptr;
    return instance->profileCity.c_str();
}

int jamulus_core_set_city ( jamulus_core_t core, const char* city )
{
    JamulusCoreInstance* instance = static_cast<JamulusCoreInstance*> ( core );
    if ( !instance )
        return -1;
    instance->profileCity = city ? city : "";
    return 0;
}

int jamulus_core_get_skill ( jamulus_core_t core )
{
    JamulusCoreInstance* instance = static_cast<JamulusCoreInstance*> ( core );
    if ( !instance )
        return 0;
    int fromEngine = instance->audioEngine.getSkill();
    if ( fromEngine != 0 )
        return fromEngine;
    return instance->profileSkill;
}

int jamulus_core_set_skill ( jamulus_core_t core, int skill )
{
    JamulusCoreInstance* instance = static_cast<JamulusCoreInstance*> ( core );
    if ( !instance )
        return -1;
    instance->profileSkill = skill;
    instance->audioEngine.setSkill ( skill );
    return 0;
}

bool jamulus_core_get_auto_jitter ( jamulus_core_t core )
{
    JamulusCoreInstance* instance = static_cast<JamulusCoreInstance*> ( core );
    if ( !instance )
        return true;
    return instance->audioEngine.getAutoJitter();
}

int jamulus_core_set_auto_jitter ( jamulus_core_t core, bool enabled )
{
    JamulusCoreInstance* instance = static_cast<JamulusCoreInstance*> ( core );
    if ( !instance )
        return -1;
    instance->audioEngine.setAutoJitter ( enabled );
    if ( instance->runningFlag.load ( std::memory_order_relaxed ) && instance->serverSockAddrLen > 0 )
        jamulus_send_jitter_buffer_message ( instance );
    return 0;
}

int jamulus_core_get_jitter_buffer ( jamulus_core_t core )
{
    JamulusCoreInstance* instance = static_cast<JamulusCoreInstance*> ( core );
    if ( !instance )
        return 10;
    if ( instance->audioEngine.getAutoJitter() )
    {
        std::lock_guard<std::mutex> lock ( instance->audioMutex );
        return instance->jitterTargetFrames;
    }
    return instance->audioEngine.getJitterBuffer();
}

int jamulus_core_set_jitter_buffer ( jamulus_core_t core, int size )
{
    JamulusCoreInstance* instance = static_cast<JamulusCoreInstance*> ( core );
    if ( !instance )
        return -1;
    instance->audioEngine.setJitterBuffer ( std::clamp ( size, 1, MAX_NET_BUF_SIZE_NUM_BL ) );
    return 0;
}


int jamulus_core_get_server_jitter_buffer ( jamulus_core_t core )
{
    JamulusCoreInstance* instance = static_cast<JamulusCoreInstance*> ( core );
    if ( !instance )
        return 10;
    return instance->audioEngine.getServerJitterBuffer();
}

int jamulus_core_set_server_jitter_buffer ( jamulus_core_t core, int size )
{
    JamulusCoreInstance* instance = static_cast<JamulusCoreInstance*> ( core );
    if ( !instance )
        return -1;
    instance->audioEngine.setServerJitterBuffer ( std::clamp ( size, 1, MAX_NET_BUF_SIZE_NUM_BL ) );
    if ( instance->runningFlag.load ( std::memory_order_relaxed ) && instance->serverSockAddrLen > 0 && !instance->audioEngine.getAutoJitter() )
        jamulus_send_jitter_buffer_message ( instance );
    return 0;
}
bool jamulus_core_get_small_buffers ( jamulus_core_t core )
{
    JamulusCoreInstance* instance = static_cast<JamulusCoreInstance*> ( core );
    if ( !instance )
        return false;
    return instance->audioEngine.getSmallBuffers();
}

int jamulus_core_set_small_buffers ( jamulus_core_t core, bool enabled )
{
    JamulusCoreInstance* instance = static_cast<JamulusCoreInstance*> ( core );
    if ( !instance )
        return -1;
    instance->audioEngine.setSmallBuffers ( enabled );
    jamulus_apply_small_buffers_mode ( instance, enabled );
    instance->codecConfigChanged.store ( true, std::memory_order_relaxed );
    if ( instance->runningFlag.load ( std::memory_order_relaxed ) && instance->serverSockAddrLen > 0 )
        jamulus_send_transport_props_message ( instance );
    return 0;
}

int jamulus_core_get_audio_channels ( jamulus_core_t core )
{
    JamulusCoreInstance* instance = static_cast<JamulusCoreInstance*> ( core );
    if ( !instance )
        return 0;
    return ( instance->codecConfig.audioChannels == 2 ) ? 2 : 0;
}

int jamulus_core_set_audio_channels ( jamulus_core_t core, int channels )
{
    JamulusCoreInstance* instance = static_cast<JamulusCoreInstance*> ( core );
    if ( !instance )
        return -1;

    const int requestedChannels = ( channels > 0 ) ? 2 : 1;
    {
        std::lock_guard<std::mutex> lock ( instance->audioMutex );
        if ( instance->codecConfig.audioChannels == requestedChannels )
            return 0;

        instance->codecConfig.audioChannels = requestedChannels;
        instance->codecConfigChanged.store ( true, std::memory_order_relaxed );
    }

    if ( instance->runningFlag.load ( std::memory_order_relaxed ) && instance->serverSockAddrLen > 0 )
        jamulus_send_transport_props_message ( instance );
    return 0;
}

int jamulus_core_get_num_servers ( jamulus_core_t core )
{
    JamulusCoreInstance* instance = static_cast<JamulusCoreInstance*> ( core );
    if ( !instance )
        return 0;

    std::lock_guard<std::mutex> lock ( instance->serverListMutex );
    return static_cast<int> ( instance->serverList.size() );
}

bool jamulus_core_get_server_info ( jamulus_core_t core, int index, jamulus_server_info_t* info )
{
    JamulusCoreInstance* instance = static_cast<JamulusCoreInstance*> ( core );
    if ( !instance || !info )
        return false;

    std::memset ( info, 0, sizeof ( *info ) );

    std::lock_guard<std::mutex> lock ( instance->serverListMutex );
    if ( index < 0 || index >= static_cast<int> ( instance->serverList.size() ) )
        return false;

    const JamulusCoreServerEntry& entry = instance->serverList[static_cast<size_t> ( index )];

    std::strncpy ( info->name, entry.name.c_str(), sizeof ( info->name ) - 1 );
    std::strncpy ( info->address, entry.address.c_str(), sizeof ( info->address ) - 1 );
    std::strncpy ( info->city, entry.city.c_str(), sizeof ( info->city ) - 1 );
    std::strncpy ( info->country, entry.country.c_str(), sizeof ( info->country ) - 1 );
    std::strncpy ( info->version, entry.version.c_str(), sizeof ( info->version ) - 1 );
    std::strncpy ( info->players, entry.players.c_str(), sizeof ( info->players ) - 1 );

    info->ping        = entry.ping;
    info->numClients  = entry.numClients;
    info->maxClients  = entry.maxClients;
    info->isPermanent = entry.isPermanent;

    return true;
}

void jamulus_core_clear_server_list ( jamulus_core_t core )
{
    JamulusCoreInstance* instance = static_cast<JamulusCoreInstance*> ( core );
    if ( !instance )
        return;

    std::lock_guard<std::mutex> lock ( instance->serverListMutex );
    instance->serverList.clear();
}

static std::string jamulus_core_resolve_directory_address ( int directoryType, const char* customAddress )
{
    if ( directoryType == JAMULUS_DIRECTORY_CUSTOM )
    {
        if ( customAddress && customAddress[0] != '\0' )
            return std::string ( customAddress );
        return std::string();
    }

    static const char* addresses[] = {
        "anygenre1.jamulus.io:22124",
        "anygenre2.jamulus.io:22224",
        "anygenre3.jamulus.io:22624",
        "rock.jamulus.io:22424",
        "jazz.jamulus.io:22324",
        "classical.jamulus.io:22524",
        "choral.jamulus.io:22724"
    };

    if ( directoryType < 0 || directoryType >= static_cast<int> ( sizeof ( addresses ) / sizeof ( addresses[0] ) ) )
        directoryType = 0;

    const char* addr = addresses[static_cast<size_t> ( directoryType )];
    if ( !addr || addr[0] == '\0' )
        return std::string();

    return std::string ( addr );
}

void jamulus_core_request_server_list ( jamulus_core_t core, int directoryType, const char* customAddress )
{
    JamulusCoreInstance* instance = static_cast<JamulusCoreInstance*> ( core );
    if ( !instance )
        return;

    std::string resolved = jamulus_core_resolve_directory_address ( directoryType, customAddress );
    if ( resolved.empty() )
    {
        jamulus_core_log ( "request_server_list: directoryType=" + std::to_string ( directoryType ) + " resolved empty" );
        return;
    }

    if ( !jamulus_core_open_socket ( instance ) )
    {
        jamulus_core_log ( "request_server_list: open_socket failed" );
        return;
    }

    std::string    host = resolved;
    unsigned short port = 22124;

    const std::size_t colonPos = resolved.find ( ':' );
    if ( colonPos != std::string::npos )
    {
        host = resolved.substr ( 0, colonPos );
        const std::string portStr = resolved.substr ( colonPos + 1 );
        const int         parsed  = std::atoi ( portStr.c_str() );
        if ( parsed > 0 && parsed <= 65535 )
            port = static_cast<unsigned short> ( parsed );
    }

    sockaddr_in dirAddr{};
    dirAddr.sin_family = AF_INET;
    dirAddr.sin_port   = htons ( port );

    bool resolvedOk = false;

    if ( ::inet_pton ( AF_INET, host.c_str(), &dirAddr.sin_addr ) == 1 )
    {
        resolvedOk = true;
    }
    else
    {
        addrinfo  hints{};
        addrinfo* res = nullptr;

        std::memset ( &hints, 0, sizeof ( hints ) );
        hints.ai_family   = AF_INET;
        hints.ai_socktype = SOCK_DGRAM;

        char portBuf[16];
        std::snprintf ( portBuf, sizeof ( portBuf ), "%u", static_cast<unsigned int> ( port ) );

        if ( ::getaddrinfo ( host.c_str(), portBuf, &hints, &res ) == 0 && res )
        {
            const sockaddr_in* a = reinterpret_cast<const sockaddr_in*> ( res->ai_addr );
            if ( a )
            {
                dirAddr    = *a;
                resolvedOk = true;
            }
        }

        if ( res )
            ::freeaddrinfo ( res );
    }

    if ( !resolvedOk )
    {
        jamulus_core_log ( "request_server_list: failed to resolve host=" + host + " port=" + std::to_string ( port ) );
        return;
    }

    std::vector<uint8_t> data;
    std::vector<uint8_t> frame = jamulus_build_frame ( JAMULUS_CLM_REQ_SERVER_LIST, 0, data );

    int sent = static_cast<int> ( ::sendto ( instance->udpSocket,
                                             reinterpret_cast<const char*> ( frame.data() ),
                                             static_cast<int> ( frame.size() ),
                                             0,
                                             reinterpret_cast<const sockaddr*> ( &dirAddr ),
                                             sizeof ( dirAddr ) ) );

#ifdef _WIN32
    if ( sent <= 0 )
    {
        int err = WSAGetLastError();
        jamulus_core_log ( "request_server_list: sendto failed err=" + std::to_string ( err ) );
    }
    else
    {
        jamulus_core_log ( "request_server_list: sent bytes=" + std::to_string ( sent ) + " to directory" );
    }
#else
    if ( sent <= 0 )
    {
        jamulus_core_log ( "request_server_list: sendto failed errno=" + std::to_string ( errno ) );
    }
    else
    {
        jamulus_core_log ( "request_server_list: sent bytes=" + std::to_string ( sent ) + " to directory" );
    }
#endif

    {
        std::lock_guard<std::mutex> lock ( instance->serverListMutex );
        instance->serverList.clear();
    }

    if ( !instance->workerThread.joinable() )
    {
        instance->workerStopRequested.store( false, std::memory_order_relaxed );
        instance->runningFlag.store( true, std::memory_order_relaxed );
        instance->workerThread = std::thread( jamulus_core_worker, instance );
    }
}

void jamulus_core_request_server_client_list ( jamulus_core_t core, const char* address )
{
    JamulusCoreInstance* instance = static_cast<JamulusCoreInstance*> ( core );
    if ( !instance )
        return;
    if ( !address )
        return;

    if ( !jamulus_core_open_socket ( instance ) )
        return;

    std::string    host;
    unsigned short port = 22124;
    if ( !jamulus_parse_host_port ( std::string ( address ), host, port ) )
        return;

    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port   = htons ( port );
    if ( ::inet_pton ( AF_INET, host.c_str(), &serverAddr.sin_addr ) != 1 )
    {
        addrinfo  hints{};
        addrinfo* res = nullptr;
        std::memset ( &hints, 0, sizeof ( hints ) );
        hints.ai_family   = AF_INET;
        hints.ai_socktype = SOCK_DGRAM;
        char portBuf[16];
        std::snprintf ( portBuf, sizeof ( portBuf ), "%u", static_cast<unsigned int> ( port ) );
        if ( ::getaddrinfo ( host.c_str(), portBuf, &hints, &res ) != 0 || !res )
            return;
        const sockaddr_in* a = reinterpret_cast<const sockaddr_in*> ( res->ai_addr );
        if ( !a )
        {
            if ( res )
                ::freeaddrinfo ( res );
            return;
        }
        serverAddr = *a;
        if ( res )
            ::freeaddrinfo ( res );
    }

    std::vector<uint8_t> data;
    std::vector<uint8_t> frame = jamulus_build_frame ( JAMULUS_CLM_REQ_CONN_CLIENTS_LIST, 0, data );

    ::sendto ( instance->udpSocket,
               reinterpret_cast<const char*> ( frame.data() ),
               static_cast<int> ( frame.size() ),
               0,
               reinterpret_cast<const sockaddr*> ( &serverAddr ),
               sizeof ( serverAddr ) );
}

void jamulus_core_ping_server_list ( jamulus_core_t core )
{
    JamulusCoreInstance* instance = static_cast<JamulusCoreInstance*> ( core );
    if ( !instance )
        return;

    if ( !jamulus_core_open_socket ( instance ) )
        return;

    std::vector<std::pair<std::string, bool>> addresses;
    {
        std::lock_guard<std::mutex> lock ( instance->serverListMutex );
        addresses.reserve ( instance->serverList.size() );
        for ( const auto& entry : instance->serverList )
            addresses.emplace_back ( entry.address, entry.version.empty() );
    }

    if ( addresses.empty() )
        return;

    const uint32_t stamp = jamulus_get_time_ms();

    for ( const auto& addressEntry : addresses )
    {
        const auto& addrStr = addressEntry.first;
        const bool  needVersion = addressEntry.second;

        if ( addrStr.empty() )
            continue;

        std::string host;
        unsigned short port = 22124;
        if ( !jamulus_parse_host_port ( addrStr, host, port ) )
            continue;

        sockaddr_in serverAddr{};
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_port   = htons ( port );
        if ( ::inet_pton ( AF_INET, host.c_str(), &serverAddr.sin_addr ) != 1 )
        {
            addrinfo  hints{};
            addrinfo* res = nullptr;
            std::memset ( &hints, 0, sizeof ( hints ) );
            hints.ai_family   = AF_INET;
            hints.ai_socktype = SOCK_DGRAM;
            char portBuf[16];
            std::snprintf ( portBuf, sizeof ( portBuf ), "%u", static_cast<unsigned int> ( port ) );
            if ( ::getaddrinfo ( host.c_str(), portBuf, &hints, &res ) != 0 || !res )
                continue;
            const sockaddr_in* a = reinterpret_cast<const sockaddr_in*> ( res->ai_addr );
            if ( !a )
            {
                if ( res )
                    ::freeaddrinfo ( res );
                continue;
            }
            serverAddr = *a;
            if ( res )
                ::freeaddrinfo ( res );
        }

        std::vector<uint8_t> data ( 5 );
        int                  pos = 0;
        jamulus_put_val ( data, pos, stamp, 4 );
        jamulus_put_val ( data, pos, 0u, 1 );

        std::vector<uint8_t> frame = jamulus_build_frame ( JAMULUS_CLM_PING_MS_WITHNUMCLIENTS, 0, data );

        ::sendto ( instance->udpSocket,
                   reinterpret_cast<const char*> ( frame.data() ),
                   static_cast<int> ( frame.size() ),
                   0,
                   reinterpret_cast<const sockaddr*> ( &serverAddr ),
                   sizeof ( serverAddr ) );

        if ( needVersion )
        {
            std::vector<uint8_t> versionReq = jamulus_build_frame ( JAMULUS_CLM_REQ_VERSION_AND_OS, 0, {} );
            ::sendto ( instance->udpSocket,
                       reinterpret_cast<const char*> ( versionReq.data() ),
                       static_cast<int> ( versionReq.size() ),
                       0,
                       reinterpret_cast<const sockaddr*> ( &serverAddr ),
                       sizeof ( serverAddr ) );
        }
    }
}

int jamulus_get_supported_countries_count()
{
    return 1 + kJamulusCountryMapSize;
}

bool jamulus_get_supported_country ( int index, char* outName, int outNameSize, char* outIsoCode, int outIsoSize, int* outWireCode )
{
    if ( index < 0 )
        return false;

    if ( index == 0 )
    {
        if ( outName && outNameSize > 0 )
        {
            std::strncpy ( outName, "None", static_cast<size_t> ( outNameSize ) - 1 );
            outName[outNameSize - 1] = '\0';
        }
        if ( outIsoCode && outIsoSize > 0 )
        {
            std::strncpy ( outIsoCode, "none", static_cast<size_t> ( outIsoSize ) - 1 );
            outIsoCode[outIsoSize - 1] = '\0';
        }
        if ( outWireCode )
            *outWireCode = 0;
        return true;
    }

    const int mapIndex = index - 1;
    if ( mapIndex < 0 || mapIndex >= kJamulusCountryMapSize )
        return false;

    const auto& entry = kJamulusCountryMap[mapIndex];
    if ( outName && outNameSize > 0 )
    {
        std::strncpy ( outName, entry.name, static_cast<size_t> ( outNameSize ) - 1 );
        outName[outNameSize - 1] = '\0';
    }
    if ( outIsoCode && outIsoSize > 0 )
    {
        std::strncpy ( outIsoCode, entry.iso, static_cast<size_t> ( outIsoSize ) - 1 );
        outIsoCode[outIsoSize - 1] = '\0';
    }
    if ( outWireCode )
        *outWireCode = static_cast<int> ( entry.wire );
    return true;
}

void jamulus_process_events ( void )
{
}
