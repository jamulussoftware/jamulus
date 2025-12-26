// Minimal ASIO shim: provides ASIO functions and a tiny AsioDrivers implementation
// This is a compile-time shim only — it does not provide real audio I/O.

// Include ASIO header with normal C++ linkage so definitions match how
// Jamulus compiles against the header (C++ mangled names on MSVC).
#include "libjamulus/include_asio/asio.h"

ASIOError ASIOInit(ASIODriverInfo* /*info*/) { return ASE_NotPresent; }
ASIOError ASIOExit(void) { return ASE_NotPresent; }
ASIOError ASIOStart(void) { return ASE_NotPresent; }
ASIOError ASIOStop(void) { return ASE_NotPresent; }
ASIOError ASIOGetChannels(long* /*numInputChannels*/, long* /*numOutputChannels*/) { return ASE_NotPresent; }
ASIOError ASIOGetLatencies(long* /*inputLatency*/, long* /*outputLatency*/) { return ASE_NotPresent; }
ASIOError ASIOGetBufferSize(long* /*minSize*/, long* /*maxSize*/, long* /*preferredSize*/, long* /*granularity*/) { return ASE_NotPresent; }
ASIOError ASIOCanSampleRate(ASIOSampleRate /*sampleRate*/) { return ASE_NotPresent; }
ASIOError ASIOGetSampleRate(ASIOSampleRate* /*currentRate*/) { return ASE_NotPresent; }
ASIOError ASIOSetSampleRate(ASIOSampleRate /*sampleRate*/) { return ASE_NotPresent; }
ASIOError ASIOGetClockSources(ASIOClockSource* /*clocks*/, long* /*numSources*/) { return ASE_NotPresent; }
ASIOError ASIOSetClockSource(long /*reference*/) { return ASE_NotPresent; }
ASIOError ASIOGetSamplePosition(ASIOSamples* /*sPos*/, ASIOTimeStamp* /*tStamp*/) { return ASE_NotPresent; }
ASIOError ASIOGetChannelInfo(ASIOChannelInfo* /*info*/) { return ASE_NotPresent; }
ASIOError ASIOCreateBuffers(ASIOBufferInfo* /*bufferInfos*/, long /*numChannels*/, long /*bufferSize*/, ASIOCallbacks* /*callbacks*/) { return ASE_NotPresent; }
ASIOError ASIODisposeBuffers(void) { return ASE_NotPresent; }
ASIOError ASIOControlPanel(void) { return ASE_NotPresent; }
ASIOError ASIOFuture(long /*selector*/, void* /*params*/) { return ASE_NotPresent; }
ASIOError ASIOOutputReady(void) { return ASE_NotPresent; }

// Minimal AsioDrivers implementation to satisfy linker
class AsioDrivers {
public:
    AsioDrivers() {}
    ~AsioDrivers() {}
    bool getCurrentDriverName(char* /*name*/) { return false; }
    long getDriverNames(char** /*names*/, long /*maxDrivers*/) { return 0; }
    bool loadDriver(char* /*name*/) { return false; }
    void removeCurrentDriver() {}
    long getCurrentDriverIndex() { return -1; }
};

AsioDrivers* asioDrivers = nullptr;

bool loadAsioDriver(char* /*name*/) { return false; }
