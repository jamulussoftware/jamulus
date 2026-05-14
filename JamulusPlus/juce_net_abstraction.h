#pragma once

#include "../src/net_abstraction.h"
#include <juce_core/juce_core.h>
#include <juce_events/juce_events.h>

class JUCE_NetworkSocket : public INetworkSocket
{
public:
    JUCE_NetworkSocket();
    ~JUCE_NetworkSocket() override;

    bool bind ( const NetEndpoint& local ) override;
    bool joinMulticast ( const NetEndpoint& group ) override;
    bool sendTo ( const NetEndpoint& remote, const void* data, std::size_t size ) override;
    int  recvFrom ( NetEndpoint& remote, void* buffer, std::size_t maxSize ) override;

private:
    std::unique_ptr<juce::DatagramSocket> socket;
};

class JUCE_Resolver : public IResolver
{
public:
    JUCE_Resolver();
    ~JUCE_Resolver() override;

    std::vector<NetEndpoint> resolveSrv ( const std::string& domain ) override;
    bool                     resolveHost ( const std::string& host, NetEndpoint& out ) override;
};
