#include "../jamulus_wrapper.h"
#include "../jamulus_core_juce.h"
#include "BinaryData.h"
#include <unordered_map>
#include <string>
#include <mutex>
#include <cstring>
#include <cstdlib>
#include <cctype>

namespace
{
struct CompatState
{
    int  audioQuality       = 1;
    int  audioChannels      = 2;
    int  inputFader         = 50;
    int  reverbLevel        = 0;
    bool reverbOnLeft       = false;
    int  serverJitterBuffer = 0;
    int  inputBoost         = 1;
    int  newClientLevel     = 50;
    bool feedbackDetection  = false;
    bool audioAlerts        = false;
};

std::unordered_map<void*, CompatState> states;
std::mutex                             statesMutex;
std::string                            customDirectoryAddress;

jamulus_core_t asCore ( jamulus_client_t c ) { return reinterpret_cast<jamulus_core_t> ( c ); }

CompatState& getState ( jamulus_client_t c )
{
    std::lock_guard<std::mutex> lock ( statesMutex );
    return states[c];
}

std::string toLower ( const std::string& value )
{
    std::string lowered = value;
    for ( auto& ch : lowered )
        ch = static_cast<char> ( std::tolower ( static_cast<unsigned char> ( ch ) ) );
    return lowered;
}

std::string extractFilename ( const char* path )
{
    if ( !path )
        return {};

    std::string fullPath ( path );
    const auto  slashPos = fullPath.find_last_of ( "/\\" );
    if ( slashPos == std::string::npos )
        return fullPath;
    return fullPath.substr ( slashPos + 1 );
}

const char* lookupResourceData ( const char* pathOrName, int& dataSize )
{
    dataSize = 0;
    if ( !pathOrName || !pathOrName[0] )
        return nullptr;

    const auto requestedLower = toLower ( extractFilename ( pathOrName ) );
    const auto fullLower      = toLower ( pathOrName );

    for ( int i = 0; i < BinaryData::namedResourceListSize; ++i )
    {
        const auto originalLower = toLower ( BinaryData::originalFilenames[i] );
        const auto namedLower    = toLower ( BinaryData::namedResourceList[i] );
        if ( originalLower == requestedLower || namedLower == requestedLower || namedLower == fullLower )
            return BinaryData::getNamedResource ( BinaryData::namedResourceList[i], dataSize );
    }

    return nullptr;
}
}

extern "C"
{
jamulus_client_t jamulus_client_create ( unsigned short port, const char* server_addr, const char* client_name, bool enable_ipv6 )
{
    return reinterpret_cast<jamulus_client_t> ( jamulus_core_create ( port, server_addr, client_name, enable_ipv6 ) );
}

void jamulus_client_destroy ( jamulus_client_t c ) { jamulus_core_destroy ( asCore ( c ) ); }

int jamulus_client_start ( jamulus_client_t c ) { return jamulus_core_start ( asCore ( c ) ); }

int jamulus_client_stop ( jamulus_client_t c ) { return jamulus_core_stop ( asCore ( c ) ); }

int jamulus_client_set_server_addr ( jamulus_client_t c, const char* addr ) { return jamulus_core_set_server_addr ( asCore ( c ), addr ); }

bool jamulus_client_is_connected ( jamulus_client_t c ) { return jamulus_core_is_connected ( asCore ( c ) ) != 0; }

bool jamulus_client_is_running ( jamulus_client_t c ) { return jamulus_core_is_running ( asCore ( c ) ) != 0; }

void jamulus_client_disconnect ( jamulus_client_t c ) { jamulus_core_disconnect ( asCore ( c ) ); }

int jamulus_client_process_audio ( jamulus_client_t c, const float* in, float* out, int frames, int channels )
{
    return jamulus_core_process_audio ( asCore ( c ), in, out, frames, channels );
}

int jamulus_client_push_audio ( jamulus_client_t, const float*, int, int ) { return -1; }
int jamulus_client_pop_audio ( jamulus_client_t, float*, int, int ) { return -1; }
int jamulus_client_start_worker ( jamulus_client_t, int ) { return 0; }
int jamulus_client_stop_worker ( jamulus_client_t ) { return 0; }

double jamulus_client_get_level_left ( jamulus_client_t ) { return -120.0; }
double jamulus_client_get_level_right ( jamulus_client_t ) { return -120.0; }
float  jamulus_client_get_output_level_left ( jamulus_client_t ) { return 0.0f; }
float  jamulus_client_get_output_level_right ( jamulus_client_t ) { return 0.0f; }

int jamulus_client_get_audio_quality ( jamulus_client_t c ) { return getState ( c ).audioQuality; }
void jamulus_client_set_audio_quality ( jamulus_client_t c, int quality ) { getState ( c ).audioQuality = quality; }

int jamulus_client_get_audio_channels ( jamulus_client_t c )
{
    CompatState& state = getState ( c );
    state.audioChannels = jamulus_core_get_audio_channels ( asCore ( c ) );
    return state.audioChannels;
}

void jamulus_client_set_audio_channels ( jamulus_client_t c, int channels )
{
    getState ( c ).audioChannels = channels;
    jamulus_core_set_audio_channels ( asCore ( c ), channels );
}

int jamulus_client_get_input_fader ( jamulus_client_t c ) { return getState ( c ).inputFader; }
void jamulus_client_set_input_fader ( jamulus_client_t c, int value ) { getState ( c ).inputFader = value; }

int  jamulus_client_get_reverb_level ( jamulus_client_t c ) { return getState ( c ).reverbLevel; }
void jamulus_client_set_reverb_level ( jamulus_client_t c, int level ) { getState ( c ).reverbLevel = level; }
bool jamulus_client_get_reverb_on_left ( jamulus_client_t c ) { return getState ( c ).reverbOnLeft; }
void jamulus_client_set_reverb_on_left ( jamulus_client_t c, bool on_left ) { getState ( c ).reverbOnLeft = on_left; }

void jamulus_client_set_name ( jamulus_client_t c, const char* name ) { jamulus_core_set_name ( asCore ( c ), name ); }

const char* jamulus_client_get_name ( jamulus_client_t c ) { return jamulus_core_get_name ( asCore ( c ) ); }

void jamulus_client_set_instrument ( jamulus_client_t c, int instrument ) { jamulus_core_set_instrument ( asCore ( c ), instrument ); }

int jamulus_client_get_instrument ( jamulus_client_t c ) { return jamulus_core_get_instrument ( asCore ( c ) ); }

void jamulus_client_set_country ( jamulus_client_t c, int country ) { jamulus_core_set_country ( asCore ( c ), country ); }

int jamulus_client_get_country ( jamulus_client_t c ) { return jamulus_core_get_country ( asCore ( c ) ); }

void jamulus_client_set_city ( jamulus_client_t c, const char* city ) { jamulus_core_set_city ( asCore ( c ), city ); }

const char* jamulus_client_get_city ( jamulus_client_t c ) { return jamulus_core_get_city ( asCore ( c ) ); }

void jamulus_client_set_skill ( jamulus_client_t c, int skill ) { jamulus_core_set_skill ( asCore ( c ), skill ); }

int jamulus_client_get_skill ( jamulus_client_t c ) { return jamulus_core_get_skill ( asCore ( c ) ); }

const char* jamulus_client_get_country_code ( jamulus_client_t c ) { return jamulus_core_get_country_code ( asCore ( c ) ); }

int jamulus_client_get_ping_time ( jamulus_client_t c )
{
    jamulus_network_stats_t stats {};
    if ( jamulus_core_get_network_stats ( asCore ( c ), &stats ) )
        return stats.ping_time_ms;
    return -1;
}

void jamulus_client_get_network_stats ( jamulus_client_t c, jamulus_network_stats_t* stats )
{
    if ( !stats )
        return;
    if ( !jamulus_core_get_network_stats ( asCore ( c ), stats ) )
        std::memset ( stats, 0, sizeof ( *stats ) );
}

int jamulus_client_get_overall_delay ( jamulus_client_t c )
{
    jamulus_network_stats_t stats {};
    if ( jamulus_core_get_network_stats ( asCore ( c ), &stats ) )
        return stats.total_delay_ms;
    return 0;
}

int jamulus_client_get_num_channels ( jamulus_client_t c ) { return jamulus_core_get_num_channels ( asCore ( c ) ); }

int jamulus_client_get_my_channel_id ( jamulus_client_t c ) { return jamulus_core_get_my_channel_id ( asCore ( c ) ); }

bool jamulus_client_get_channel_info ( jamulus_client_t c, int index, jamulus_channel_info_t* info )
{
    return jamulus_core_get_channel_info ( asCore ( c ), index, info );
}

void jamulus_client_set_channel_gain ( jamulus_client_t c, int channel_id, float gain ) { jamulus_core_set_channel_gain ( asCore ( c ), channel_id, gain ); }

void jamulus_client_set_channel_pan ( jamulus_client_t c, int channel_id, float pan ) { jamulus_core_set_channel_pan ( asCore ( c ), channel_id, pan ); }

void jamulus_client_set_channel_mute ( jamulus_client_t c, int channel_id, bool muted ) { jamulus_core_set_channel_mute ( asCore ( c ), channel_id, muted ); }

void jamulus_client_set_channel_solo ( jamulus_client_t c, int channel_id, bool solo ) { jamulus_core_set_channel_solo ( asCore ( c ), channel_id, solo ); }

void jamulus_client_set_auto_jitter ( jamulus_client_t c, bool enabled ) { jamulus_core_set_auto_jitter ( asCore ( c ), enabled ); }

bool jamulus_client_get_auto_jitter ( jamulus_client_t c ) { return jamulus_core_get_auto_jitter ( asCore ( c ) ); }

void jamulus_client_set_jitter_buffer ( jamulus_client_t c, int size ) { jamulus_core_set_jitter_buffer ( asCore ( c ), size ); }

int jamulus_client_get_jitter_buffer ( jamulus_client_t c ) { return jamulus_core_get_jitter_buffer ( asCore ( c ) ); }

void jamulus_client_set_server_jitter_buffer ( jamulus_client_t c, int size ) { jamulus_core_set_server_jitter_buffer ( asCore ( c ), size ); }

int jamulus_client_get_server_jitter_buffer ( jamulus_client_t c ) { return jamulus_core_get_server_jitter_buffer ( asCore ( c ) ); }

void jamulus_client_set_input_boost ( jamulus_client_t c, int boost ) { getState ( c ).inputBoost = boost; }

int jamulus_client_get_input_boost ( jamulus_client_t c ) { return getState ( c ).inputBoost; }

void jamulus_client_set_new_client_level ( jamulus_client_t c, int level ) { getState ( c ).newClientLevel = level; }

int jamulus_client_get_new_client_level ( jamulus_client_t c ) { return getState ( c ).newClientLevel; }

void jamulus_client_set_feedback_detection ( jamulus_client_t c, bool enabled ) { getState ( c ).feedbackDetection = enabled; }

bool jamulus_client_get_feedback_detection ( jamulus_client_t c ) { return getState ( c ).feedbackDetection; }

void jamulus_client_set_audio_alerts ( jamulus_client_t c, bool enabled ) { getState ( c ).audioAlerts = enabled; }

bool jamulus_client_get_audio_alerts ( jamulus_client_t c ) { return getState ( c ).audioAlerts; }

void jamulus_client_set_small_buffers ( jamulus_client_t c, bool enabled ) { jamulus_core_set_small_buffers ( asCore ( c ), enabled ); }

bool jamulus_client_get_small_buffers ( jamulus_client_t c ) { return jamulus_core_get_small_buffers ( asCore ( c ) ); }

void jamulus_client_send_chat_message ( jamulus_client_t c, const char* message ) { jamulus_core_send_chat_message ( asCore ( c ), message ? message : "" ); }

void jamulus_client_send_ping ( jamulus_client_t c ) { jamulus_core_send_ping ( asCore ( c ) ); }

const char* jamulus_client_get_chat_message ( jamulus_client_t c ) { return jamulus_core_get_chat_message ( asCore ( c ) ); }

void jamulus_client_request_server_list ( jamulus_client_t c, int directory_type )
{
    const char* custom = directory_type == JAMULUS_DIRECTORY_CUSTOM ? customDirectoryAddress.c_str() : "";
    jamulus_core_request_server_list ( asCore ( c ), directory_type, custom );
}

void jamulus_client_request_server_client_list ( jamulus_client_t c, const char* serverAddress )
{
    jamulus_core_request_server_client_list ( asCore ( c ), serverAddress ? serverAddress : "" );
}

void jamulus_client_set_custom_directory_address ( const char* directoryAddress )
{
    customDirectoryAddress = directoryAddress ? directoryAddress : "";
}

void jamulus_client_ping_single ( jamulus_client_t, const char* ) {}

void jamulus_client_ping_server_list ( jamulus_client_t c ) { jamulus_core_ping_server_list ( asCore ( c ) ); }

int jamulus_client_get_num_servers ( jamulus_client_t c ) { return jamulus_core_get_num_servers ( asCore ( c ) ); }

bool jamulus_client_get_server_info ( jamulus_client_t c, int index, jamulus_server_info_t* info )
{
    return jamulus_core_get_server_info ( asCore ( c ), index, info );
}

void jamulus_client_clear_server_list ( jamulus_client_t c ) { jamulus_core_clear_server_list ( asCore ( c ) ); }

void* jamulus_gui_create ( jamulus_client_t ) { return nullptr; }
void  jamulus_gui_destroy ( void* ) {}
void  jamulus_gui_show ( void* ) {}
void  jamulus_gui_hide ( void* ) {}
void* jamulus_gui_get_native_handle ( void* ) { return nullptr; }
void  jamulus_gui_set_parent ( void*, void* ) {}
void  jamulus_gui_resize ( void*, int, int ) {}
void  jamulus_gui_position ( void*, int, int ) {}
void  jamulus_gui_get_preferred_size ( void*, int* width, int* height )
{
    if ( width )
        *width = 900;
    if ( height )
        *height = 600;
}
void jamulus_gui_repaint ( void* ) {}
void jamulus_gui_set_active ( bool ) {}

bool jamulus_load_resource ( const char* path, void** outData, int* outSize )
{
    if ( !outData || !outSize )
        return false;

    *outData = nullptr;
    *outSize = 0;

    int         dataSize = 0;
    const char* source   = lookupResourceData ( path, dataSize );
    if ( !source || dataSize <= 0 )
        return false;

    void* buffer = std::malloc ( static_cast<size_t> ( dataSize ) );
    if ( !buffer )
        return false;

    std::memcpy ( buffer, source, static_cast<size_t> ( dataSize ) );
    *outData = buffer;
    *outSize = dataSize;
    return true;
}

void jamulus_free_resource ( void* data ) { std::free ( data ); }

const char* jamulus_get_log_file_path ( void ) { return ""; }
}
