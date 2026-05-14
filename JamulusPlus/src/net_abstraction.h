#pragma once

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

struct NetEndpoint
{
    std::string address;
    std::uint16_t port = 0;
};

class INetworkSocket
{
public:
    virtual ~INetworkSocket() = default;

    virtual bool bind ( const NetEndpoint& local )                                      = 0;
    virtual bool joinMulticast ( const NetEndpoint& group )                             = 0;
    virtual bool sendTo ( const NetEndpoint& remote, const void* data, std::size_t size ) = 0;
    virtual int  recvFrom ( NetEndpoint& remote, void* buffer, std::size_t maxSize )    = 0;
};

using TimerId = std::uint64_t;

class ITimerScheduler
{
public:
    virtual ~ITimerScheduler() = default;

    virtual TimerId startTimerMs ( int intervalMs, std::function<void()> callback ) = 0;
    virtual void    cancelTimer ( TimerId id )                                      = 0;
};

class IResolver
{
public:
    virtual ~IResolver() = default;

    virtual std::vector<NetEndpoint> resolveSrv ( const std::string& domain ) = 0;
    virtual bool                     resolveHost ( const std::string& host, NetEndpoint& out ) = 0;
};

