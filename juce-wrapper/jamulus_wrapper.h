#pragma once

#include <stdbool.h>

#ifdef __cplusplus
extern "C"
{
#endif

    typedef void* jamulus_client_t;

    // Server info structure for server list
    typedef struct
    {
        char name[128];    // Server name
        char address[256]; // Server address (host:port)
        char city[64];     // City
        char country[64];  // Country name
        int  ping;         // Ping time in ms (-1 if unknown)
        int  numClients;   // Current number of clients
        int  maxClients;   // Maximum number of clients
        bool isPermanent;  // Is server permanently online
    } jamulus_server_info_t;

    // ============================================================================
    // Client Lifecycle
    // ============================================================================

    // Create a Jamulus client instance. Parameters are simplified for initial prototype.
    jamulus_client_t jamulus_client_create ( unsigned short port, const char* server_addr, const char* client_name, bool enable_ipv6 );

    void jamulus_client_destroy ( jamulus_client_t c );

    int jamulus_client_start ( jamulus_client_t c );
    int jamulus_client_stop ( jamulus_client_t c );

    // ============================================================================
    // Connection Control
    // ============================================================================

    int  jamulus_client_set_server_addr ( jamulus_client_t c, const char* addr );
    bool jamulus_client_is_connected ( jamulus_client_t c );
    bool jamulus_client_is_running ( jamulus_client_t c );
    void jamulus_client_disconnect ( jamulus_client_t c );

    // ============================================================================
    // Audio Processing
    // ============================================================================

    // Process audio through the internal Jamulus client: interleaved float
    // `in` (frames * channels) -> processed interleaved float `out`.
    // Returns 0 on success, negative on error.
    int jamulus_client_process_audio ( jamulus_client_t c, const float* in, float* out, int frames, int channels );
    // Async push/pop API (real-time safe): push interleaved float frames into
    // client's input FIFO and pop processed frames from output FIFO.
    int jamulus_client_push_audio ( jamulus_client_t c, const float* in, int frames, int channels );
    int jamulus_client_pop_audio ( jamulus_client_t c, float* out, int maxFrames, int channels );
    int jamulus_client_start_worker ( jamulus_client_t c, int processBlockFrames );
    int jamulus_client_stop_worker ( jamulus_client_t c );

    // ============================================================================
    // Level Metering
    // ============================================================================

    // Get input level meters (dB, -infinity to 0)
    double jamulus_client_get_level_left ( jamulus_client_t c );
    double jamulus_client_get_level_right ( jamulus_client_t c );

    // Get output level meters (0.0 to 1.0)
    float jamulus_client_get_output_level_left ( jamulus_client_t c );
    float jamulus_client_get_output_level_right ( jamulus_client_t c );

    // ============================================================================
    // Audio Settings
    // ============================================================================

    // Audio quality: 0=Low, 1=Normal, 2=High
    int  jamulus_client_get_audio_quality ( jamulus_client_t c );
    void jamulus_client_set_audio_quality ( jamulus_client_t c, int quality );

    // Audio channels: 0=Mono, 1=MonoIn/StereoOut, 2=Stereo
    int  jamulus_client_get_audio_channels ( jamulus_client_t c );
    void jamulus_client_set_audio_channels ( jamulus_client_t c, int channels );

    // Input fader (0-100, 50=center/unity)
    int  jamulus_client_get_input_fader ( jamulus_client_t c );
    void jamulus_client_set_input_fader ( jamulus_client_t c, int value );

    // Reverb (0-100)
    int  jamulus_client_get_reverb_level ( jamulus_client_t c );
    void jamulus_client_set_reverb_level ( jamulus_client_t c, int level );
    bool jamulus_client_get_reverb_on_left ( jamulus_client_t c );
    void jamulus_client_set_reverb_on_left ( jamulus_client_t c, bool on_left );

    // ============================================================================
    // User Info / Profile
    // ============================================================================

    void        jamulus_client_set_name ( jamulus_client_t c, const char* name );
    const char* jamulus_client_get_name ( jamulus_client_t c );

    void jamulus_client_set_instrument ( jamulus_client_t c, int instrument );
    int  jamulus_client_get_instrument ( jamulus_client_t c );

    void jamulus_client_set_country ( jamulus_client_t c, int country );
    int  jamulus_client_get_country ( jamulus_client_t c );

    void        jamulus_client_set_city ( jamulus_client_t c, const char* city );
    const char* jamulus_client_get_city ( jamulus_client_t c );

    void jamulus_client_set_skill ( jamulus_client_t c, int skill );
    int  jamulus_client_get_skill ( jamulus_client_t c );

    // Get country code (ISO 2-letter code, e.g. "us", "gb", or "none")
    const char* jamulus_client_get_country_code ( jamulus_client_t c );

    // ============================================================================
    // Network Status
    // ============================================================================

    // Get the current ping time in ms, or -1 if not connected
    int jamulus_client_get_ping_time ( jamulus_client_t c );

    // Returns ping time in ms, or -1 if not connected
    // Network statistics
    typedef struct
    {
        int ping_time_ms;
        int total_delay_ms;
        // Jitter buffer status: 0=Green, 1=Yellow, 2=Red
        int jitter_buffer_status;
    } jamulus_network_stats_t;

    // Get detailed network statistics
    void jamulus_client_get_network_stats ( jamulus_client_t c, jamulus_network_stats_t* stats );

    // Returns overall delay estimate in ms
    int jamulus_client_get_overall_delay ( jamulus_client_t c );

// ============================================================================
// Channel Info (for mixer)
// ============================================================================

// Max number of channels
#define JAMULUS_MAX_CHANNELS 50

    // Channel info structure
    typedef struct
    {
        int   id;              // Channel ID (-1 if unused)
        char  name[64];        // Channel name
        int   instrument;      // Instrument type (index into table)
        char  country_code[8]; // ISO country code (e.g. "us", "gb", "nl")
        char  city[64];        // City
        int   skill_level;     // Skill level (0=None, 1=Beginner, 2=Intermediate, 3=Expert)
        float gain;            // Current gain (0.0-1.0)
        float pan;             // Current pan (-1.0 to 1.0)
        bool  muted;           // Is muted
        bool  solo;            // Is solo
        int   level;           // Current level (0-65535)
    } jamulus_channel_info_t;

    int jamulus_client_get_num_channels ( jamulus_client_t c );
    int jamulus_client_get_my_channel_id ( jamulus_client_t c );

    // Get channel info. Returns false if channel index is invalid.
    bool jamulus_client_get_channel_info ( jamulus_client_t c, int index, jamulus_channel_info_t* info );

    // Set channel gain (0.0-1.0)
    void jamulus_client_set_channel_gain ( jamulus_client_t c, int channel_id, float gain );

    // Set channel pan (-1.0 to 1.0)
    void jamulus_client_set_channel_pan ( jamulus_client_t c, int channel_id, float pan );

    // Set channel mute
    void jamulus_client_set_channel_mute ( jamulus_client_t c, int channel_id, bool muted );

    // Set channel solo
    void jamulus_client_set_channel_solo ( jamulus_client_t c, int channel_id, bool solo );

    // ============================================================================
    // Advanced Settings
    // ============================================================================

    // Jitter buffer settings
    void jamulus_client_set_auto_jitter ( jamulus_client_t c, bool enabled );
    bool jamulus_client_get_auto_jitter ( jamulus_client_t c );
    void jamulus_client_set_jitter_buffer ( jamulus_client_t c, int size );
    int  jamulus_client_get_jitter_buffer ( jamulus_client_t c );

    // Server Jitter Buffer settings
    void jamulus_client_set_server_jitter_buffer ( jamulus_client_t c, int size );
    int  jamulus_client_get_server_jitter_buffer ( jamulus_client_t c );

    // Input boost (1-4 for 0dB to +18dB)
    void jamulus_client_set_input_boost ( jamulus_client_t c, int boost );
    int  jamulus_client_get_input_boost ( jamulus_client_t c );

    // New client fader level (0-100)
    void jamulus_client_set_new_client_level ( jamulus_client_t c, int level );
    int  jamulus_client_get_new_client_level ( jamulus_client_t c );

    // Feedback detection
    void jamulus_client_set_feedback_detection ( jamulus_client_t c, bool enabled );
    bool jamulus_client_get_feedback_detection ( jamulus_client_t c );

    // Audio alerts
    void jamulus_client_set_audio_alerts ( jamulus_client_t c, bool enabled );
    bool jamulus_client_get_audio_alerts ( jamulus_client_t c );

    // Small network buffers
    void jamulus_client_set_small_buffers ( jamulus_client_t c, bool enabled );
    bool jamulus_client_get_small_buffers ( jamulus_client_t c );

    // ============================================================================
    // Chat
    // ============================================================================

    // Send a chat message
    void jamulus_client_send_chat_message ( jamulus_client_t c, const char* message );
    void jamulus_client_send_ping ( jamulus_client_t c );

    // Get latest received chat message (returns empty string if none)
    // Returns pointer to internal buffer - copy if needed
    const char* jamulus_client_get_chat_message ( jamulus_client_t c );

// ============================================================================
// Server List / Directory
// ============================================================================

// (Moved to top of file)

// Directory types
#define JAMULUS_DIRECTORY_ANY_GENRE       0
#define JAMULUS_DIRECTORY_GENRE_ROCK      1
#define JAMULUS_DIRECTORY_GENRE_JAZZ      2
#define JAMULUS_DIRECTORY_GENRE_CLASSICAL 3
#define JAMULUS_DIRECTORY_GENRE_FOLK      4
#define JAMULUS_DIRECTORY_GENRE_WORLD     5
#define JAMULUS_DIRECTORY_CUSTOM          6

    // Request server list from a directory
    void jamulus_client_request_server_list ( jamulus_client_t c, int directory_type );

    // Ping all servers in the list to get ping times and client counts
    // Call this periodically after requesting server list
    void jamulus_client_ping_server_list ( jamulus_client_t c );

    // Get number of servers in the list
    int jamulus_client_get_num_servers ( jamulus_client_t c );

    // Get server info. Returns false if index is invalid.
    bool jamulus_client_get_server_info ( jamulus_client_t c, int index, jamulus_server_info_t* info );

    // Clear the server list
    void jamulus_client_clear_server_list ( jamulus_client_t c );

    // ============================================================================
    // Qt Event Processing (must be called periodically)
    // ============================================================================

    void jamulus_process_events ( void );

    // ============================================================================
    // Legacy GUI functions (deprecated - use JUCE GUI instead)
    // ============================================================================

    void* jamulus_gui_create ( jamulus_client_t c );
    void  jamulus_gui_destroy ( void* gui );
    void  jamulus_gui_show ( void* gui );
    void  jamulus_gui_hide ( void* gui );
    void* jamulus_gui_get_native_handle ( void* gui );
    void  jamulus_gui_set_parent ( void* gui, void* parentHwnd );
    void  jamulus_gui_resize ( void* gui, int width, int height );
    void  jamulus_gui_position ( void* gui, int x, int y );
    void  jamulus_gui_get_preferred_size ( void* gui, int* width, int* height );
    void  jamulus_gui_repaint ( void* gui );
    void  jamulus_gui_set_active ( bool active );

    // ============================================================================
    // Resource Loading (Bridge to Qt Resources)
    // ============================================================================

    // Load a resource (e.g. image) from the Qt Resource System (qrc).
    // path: The resource path (e.g. "png/instr/res/instruments/violin.png" - no leading ":/")
    // outData: Pointer to buffer pointer. The function allocates memory using new[], caller must delete[].
    // outSize: Pointer to receive the size of the data.
    // Returns true on success, false on failure (not found or empty).
    // NOTE: Use jamulus_free_resource when done with data.
    bool jamulus_load_resource ( const char* path, void** outData, int* outSize );

    void jamulus_free_resource ( void* data );

#ifdef __cplusplus
}
#endif
