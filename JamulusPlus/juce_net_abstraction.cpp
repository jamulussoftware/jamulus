#include "juce_net_abstraction.h"
#include <juce_core/juce_core.h>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windns.h>
#pragma comment(lib, "dnsapi.lib")
#else
#include <netdb.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <resolv.h>
#include <arpa/nameser.h>
#endif

JUCE_NetworkSocket::JUCE_NetworkSocket()
{
    socket = std::make_unique<juce::DatagramSocket> ( false ); // false = don't use IPv6 by default, though we can bind to it
}

JUCE_NetworkSocket::~JUCE_NetworkSocket()
{
}

bool JUCE_NetworkSocket::bind ( const NetEndpoint& local )
{
    // Jamulus usually binds to any address on a specific port
    return socket->bindToPort ( local.port, local.address );
}

bool JUCE_NetworkSocket::joinMulticast ( const NetEndpoint& group )
{
    if ( group.address.empty() )
        return false;
    return socket->joinMulticast ( group.address );
}

bool JUCE_NetworkSocket::sendTo ( const NetEndpoint& remote, const void* data, std::size_t size )
{
    return socket->write ( remote.address, remote.port, data, (int)size ) >= 0;
}

int JUCE_NetworkSocket::recvFrom ( NetEndpoint& remote, void* buffer, std::size_t maxSize )
{
    juce::String senderAddr;
    int senderPort = 0;
    int bytesRead = socket->read ( buffer, (int)maxSize, false, senderAddr, senderPort );
    
    if ( bytesRead > 0 )
    {
        remote.address = senderAddr.toStdString();
        remote.port = (uint16_t)senderPort;
    }
    
    return bytesRead;
}

JUCE_Resolver::JUCE_Resolver()
{
}

JUCE_Resolver::~JUCE_Resolver()
{
}

std::vector<NetEndpoint> JUCE_Resolver::resolveSrv ( const std::string& domain )
{
    std::vector<NetEndpoint> endpoints;

#ifdef _WIN32
    PDNS_RECORDA pDnsRecord;
    DNS_STATUS  status = DnsQuery_A ( domain.c_str(), DNS_TYPE_SRV, DNS_QUERY_STANDARD, NULL, (PDNS_RECORD*)&pDnsRecord, NULL );

    if ( status == ERROR_SUCCESS )
    {
        for ( PDNS_RECORDA pCur = pDnsRecord; pCur != NULL; pCur = pCur->pNext )
        {
            if ( pCur->wType == DNS_TYPE_SRV )
            {
                NetEndpoint ep;
                ep.port = pCur->Data.SRV.wPort;
                // We still need to resolve the target host
                NetEndpoint targetEp;
                if ( resolveHost ( pCur->Data.SRV.pNameTarget, targetEp ) )
                {
                    ep.address = targetEp.address;
                    endpoints.push_back ( ep );
                }
            }
        }
        DnsRecordListFree ( pDnsRecord, DnsFreeRecordList );
    }
#else
    // POSIX implementation using res_query
    unsigned char response[4096];
    int len = res_query ( domain.c_str(), C_IN, T_SRV, response, sizeof ( response ) );
    if ( len >= 0 )
    {
        ns_msg msg;
        if ( ns_initparse ( response, len, &msg ) >= 0 )
        {
            for ( int i = 0; i < ns_msg_count ( msg, ns_s_an ); i++ )
            {
                ns_rr rr;
                if ( ns_parserr ( &msg, ns_s_an, i, &rr ) >= 0 && ns_rr_type ( rr ) == T_SRV )
                {
                    const unsigned char* rdata = ns_rr_rdata ( rr );
                    uint16_t priority = ns_get16 ( rdata );
                    uint16_t weight   = ns_get16 ( rdata + 2 );
                    uint16_t port     = ns_get16 ( rdata + 4 );
                    char target[NS_MAXDNAME];
                    if ( ns_name_uncompress ( ns_msg_base ( msg ), ns_msg_end ( msg ), rdata + 6, target, sizeof ( target ) ) >= 0 )
                    {
                        NetEndpoint ep;
                        ep.port = port;
                        NetEndpoint targetEp;
                        if ( resolveHost ( target, targetEp ) )
                        {
                            ep.address = targetEp.address;
                            endpoints.push_back ( ep );
                        }
                    }
                }
            }
        }
    }
#endif

    return endpoints;
}

bool JUCE_Resolver::resolveHost ( const std::string& host, NetEndpoint& out )
{
    // Try as literal first
    juce::IPAddress addr ( host );
    if ( !addr.isNull() )
    {
        out.address = addr.toString().toStdString();
        return true;
    }

    // Try resolution
    struct addrinfo hints, *res;
    memset ( &hints, 0, sizeof ( hints ) );
    hints.ai_family   = AF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM;

    if ( getaddrinfo ( host.c_str(), nullptr, &hints, &res ) == 0 )
    {
        for ( struct addrinfo* p = res; p != nullptr; p = p->ai_next )
        {
            char  ipstr[INET6_ADDRSTRLEN];
            void* addr_ptr;
            if ( p->ai_family == AF_INET )
            {
                addr_ptr = & ( ( ( struct sockaddr_in* ) p->ai_addr )->sin_addr );
            }
            else
            {
                addr_ptr = & ( ( ( struct sockaddr_in6* ) p->ai_addr )->sin6_addr );
            }
            inet_ntop ( p->ai_family, addr_ptr, ipstr, sizeof ( ipstr ) );
            out.address = ipstr;
            freeaddrinfo ( res );
            return true;
        }
        freeaddrinfo ( res );
    }

    return false;
}
