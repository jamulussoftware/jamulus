#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>
#include <thread>
#include <vector>
#include "../jamulus_wrapper.h"

int main ( int argc, char** argv )
{
    if ( argc < 3 )
    {
        std::cout << "usage: jamulus_test <server:port> <name> [seconds]\n";
        return 2;
    }

    const char* serverAddress = argv[1];
    const char* clientName    = argv[2];
    const int runtimeSeconds  = ( argc >= 4 ) ? std::max ( 5, std::atoi ( argv[3] ) ) : 60;

    auto client = jamulus_client_create ( 0, serverAddress, clientName, false );
    if ( !client )
    {
        std::cerr << "failed to create client\n";
        return 1;
    }

    jamulus_client_set_name ( client, clientName );
    jamulus_client_set_instrument ( client, 4 );
    jamulus_client_set_country ( client, 36 );
    jamulus_client_set_city ( client, "JuceWrapper" );

    if ( jamulus_client_start ( client ) != 0 )
    {
        std::cerr << "failed to start client\n";
        jamulus_client_destroy ( client );
        return 1;
    }

    const int              audioFrames   = 128;
    const int              audioChannels = 2;
    std::vector<float>     in ( static_cast<size_t> ( audioFrames * audioChannels ), 0.0f );
    std::vector<float>     out ( static_cast<size_t> ( audioFrames * audioChannels ), 0.0f );
    const auto             startTime = std::chrono::steady_clock::now();
    auto                   lastPrint = startTime - std::chrono::seconds ( 1 );
    int                    lastChannelCount = -1;
    jamulus_network_stats_t stats {};

    while ( true )
    {
        for ( int i = 0; i < 4; ++i )
            jamulus_client_process_audio ( client, in.data(), out.data(), audioFrames, audioChannels );

        jamulus_process_events();
        jamulus_client_get_network_stats ( client, &stats );

        const auto now = std::chrono::steady_clock::now();
        if ( now - lastPrint >= std::chrono::seconds ( 1 ) )
        {
            lastPrint = now;
            const bool connected = jamulus_client_is_connected ( client );
            const int  channels  = jamulus_client_get_num_channels ( client );
            std::cout << "connected=" << ( connected ? 1 : 0 ) << " ping=" << stats.ping_time_ms
                      << " delay=" << stats.total_delay_ms << " jitter=" << stats.jitter_buffer_status
                      << " channels=" << channels << "\n";

            if ( channels != lastChannelCount && channels > 0 )
            {
                for ( int i = 0; i < channels; ++i )
                {
                    jamulus_channel_info_t info {};
                    if ( jamulus_client_get_channel_info ( client, i, &info ) )
                    {
                        std::cout << "ch[" << i << "] id=" << info.id << " name=" << info.name
                                  << " instrument=" << info.instrument << " country=" << info.country_code
                                  << " city=" << info.city << "\n";
                    }
                }
                lastChannelCount = channels;
            }
            jamulus_client_send_ping ( client );
        }

        if ( std::chrono::duration_cast<std::chrono::seconds> ( now - startTime ).count() >= runtimeSeconds )
            break;

        std::this_thread::sleep_for ( std::chrono::milliseconds ( 10 ) );
    }

    jamulus_client_stop ( client );
    jamulus_client_destroy ( client );
    return 0;
}
