// Lightweight linker stubs to satisfy symbols referenced by jamulus_core
// when building the wrapper/test harness. These implementations are
// intentionally minimal and not suitable for production audio use.

#include <cstdlib>
#include <cstring>
#include "jamulus_wrapper.h"

// Opus custom stubs
extern "C" {
    typedef struct OpusCustomMode OpusCustomMode;
    typedef struct OpusCustomEncoder OpusCustomEncoder;
    typedef struct OpusCustomDecoder OpusCustomDecoder;

    OpusCustomMode* opus_custom_mode_create(int /*Fs*/, int /*frame_size*/, int* error)
    {
        if (error) *error = 0;
        return reinterpret_cast<OpusCustomMode*>(std::malloc(1));
    }

    void opus_custom_mode_destroy(OpusCustomMode* m)
    {
        std::free(m);
    }

    OpusCustomEncoder* opus_custom_encoder_create(const OpusCustomMode* /*mode*/, int /*channels*/, int* error)
    {
        if (error) *error = 0;
        return reinterpret_cast<OpusCustomEncoder*>(std::malloc(1));
    }

    void opus_custom_encoder_destroy(OpusCustomEncoder* s)
    {
        std::free(s);
    }

    int opus_custom_encode(OpusCustomEncoder* /*st*/, const short* /*pcm*/, int /*frame_size*/, unsigned char* /*compressed*/, int /*maxCompressedBytes*/)
    {
        return 0; // no bytes produced
    }

    int opus_custom_encode_float(OpusCustomEncoder* /*st*/, const float* /*pcm*/, int /*frame_size*/, unsigned char* /*compressed*/, int /*maxCompressedBytes*/)
    {
        return 0;
    }

    int opus_custom_encoder_ctl(OpusCustomEncoder* /*st*/, int /*request*/, ...)
    {
        return 0;
    }

    OpusCustomDecoder* opus_custom_decoder_create(const OpusCustomMode* /*mode*/, int /*channels*/, int* error)
    {
        if (error) *error = 0;
        return reinterpret_cast<OpusCustomDecoder*>(std::malloc(1));
    }

    void opus_custom_decoder_destroy(OpusCustomDecoder* s)
    {
        std::free(s);
    }

    int opus_custom_decode(OpusCustomDecoder* /*st*/, const unsigned char* /*data*/, int /*len*/, short* /*pcm*/, int /*frame_size*/, int /*decode_fec*/)
    {
        return 0;
    }
}

// ASIO function stubs
extern "C" {
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
}

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
