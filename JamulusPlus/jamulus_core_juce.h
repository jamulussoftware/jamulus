#pragma once

#include <stdbool.h>
#include "jamulus_wrapper.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void* jamulus_core_t;

jamulus_core_t jamulus_core_create ( unsigned short port,
                                     const char*    server_addr,
                                     const char*    client_name,
                                     bool           enable_ipv6 );

void jamulus_core_destroy ( jamulus_core_t core );

int jamulus_core_start ( jamulus_core_t core );

int jamulus_core_stop ( jamulus_core_t core );

int jamulus_core_set_server_addr ( jamulus_core_t core, const char* addr );

int jamulus_core_is_running ( jamulus_core_t core );

int jamulus_core_is_connected ( jamulus_core_t core );

void jamulus_core_disconnect ( jamulus_core_t core );

int jamulus_core_process_audio ( jamulus_core_t core,
                                 const float*   in,
                                 float*         out,
                                 int            frames,
                                 int            channels );

int  jamulus_core_get_num_channels ( jamulus_core_t core );
int  jamulus_core_get_my_channel_id ( jamulus_core_t core );
bool jamulus_core_get_channel_info ( jamulus_core_t core, int index, jamulus_channel_info_t* info );

void jamulus_core_set_channel_gain ( jamulus_core_t core, int channel_id, float gain );
void jamulus_core_set_channel_pan ( jamulus_core_t core, int channel_id, float pan );
void jamulus_core_set_channel_mute ( jamulus_core_t core, int channel_id, bool muted );
void jamulus_core_set_channel_solo ( jamulus_core_t core, int channel_id, bool solo );

int         jamulus_core_send_chat_message ( jamulus_core_t core, const char* message );
const char* jamulus_core_get_chat_message ( jamulus_core_t core );
int         jamulus_core_send_ping ( jamulus_core_t core );
bool        jamulus_core_get_network_stats ( jamulus_core_t core, jamulus_network_stats_t* stats );

const char* jamulus_core_get_name ( jamulus_core_t core );
int         jamulus_core_set_name ( jamulus_core_t core, const char* name );

int jamulus_core_get_instrument ( jamulus_core_t core );
int jamulus_core_set_instrument ( jamulus_core_t core, int instrument );

int jamulus_core_get_country ( jamulus_core_t core );
int jamulus_core_set_country ( jamulus_core_t core, int country );

const char* jamulus_core_get_country_code ( jamulus_core_t core );
int         jamulus_core_set_country_code ( jamulus_core_t core, const char* code );

const char* jamulus_core_get_city ( jamulus_core_t core );
int         jamulus_core_set_city ( jamulus_core_t core, const char* city );

int jamulus_core_get_skill ( jamulus_core_t core );
int jamulus_core_set_skill ( jamulus_core_t core, int skill );

bool jamulus_core_get_auto_jitter ( jamulus_core_t core );
int  jamulus_core_set_auto_jitter ( jamulus_core_t core, bool enabled );
int  jamulus_core_get_jitter_buffer ( jamulus_core_t core );
int  jamulus_core_set_jitter_buffer ( jamulus_core_t core, int size );
int  jamulus_core_get_server_jitter_buffer ( jamulus_core_t core );
int  jamulus_core_set_server_jitter_buffer ( jamulus_core_t core, int size );
bool jamulus_core_get_small_buffers ( jamulus_core_t core );
int  jamulus_core_set_small_buffers ( jamulus_core_t core, bool enabled );
int  jamulus_core_get_audio_channels ( jamulus_core_t core );
int  jamulus_core_set_audio_channels ( jamulus_core_t core, int channels );

int  jamulus_core_get_num_servers ( jamulus_core_t core );
bool jamulus_core_get_server_info ( jamulus_core_t core, int index, jamulus_server_info_t* info );
void jamulus_core_clear_server_list ( jamulus_core_t core );
void jamulus_core_request_server_list ( jamulus_core_t core, int directoryType, const char* customAddress );
void jamulus_core_request_server_client_list ( jamulus_core_t core, const char* address );
void jamulus_core_ping_server_list ( jamulus_core_t core );

#ifdef __cplusplus
}
#endif
