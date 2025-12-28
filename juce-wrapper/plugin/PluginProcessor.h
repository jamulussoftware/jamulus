#pragma once

#pragma once

#include "../jamulus_wrapper.h"
#include "../audio_fifo.h"
#include "../virtual_sound/virtual_sound.h"
#include "Resampler.h"

class JamulusPluginProcessor
{
public:
    JamulusPluginProcessor();
    ~JamulusPluginProcessor();

    void prepareToPlay ( double sampleRate, int samplesPerBlock );
    void processBlock ( const float** inputs, float** outputs, int numChannels, int numSamples );
    void releaseResources();

    // Get the Jamulus client handle for GUI creation
    jamulus_client_t getClient() const { return client; }

    void setTestToneEnabled ( bool enabled )
    {
        testToneEnabled = enabled;
        phase           = 0.0;
    }
    bool isTestToneEnabled() const { return testToneEnabled; }

    void setMonitorMode ( bool enabled ) { monitorMode = enabled; }
    bool isMonitorMode() const { return monitorMode; }

    void  setMainVolume ( float vol ) { mainVolume = vol; }
    float getMainVolume() const { return mainVolume; }

private:
    jamulus_client_t    client        = nullptr;
    bool                clientStarted = false;
    AudioFifo*          inputFifo     = nullptr;
    AudioFifo*          outputFifo    = nullptr;
    VirtualSoundBackend vsBackend;
    int                 procNumChannels = 2;
    int                 procBlockSize   = 128;
    double              hostSampleRate  = 48000.0; // Jamulus native rate is 48kHz

    // Sample rate conversion
    bool            needsResampling = false;
    LinearResampler inputResampler;  // Host rate -> 48kHz
    LinearResampler outputResampler; // 48kHz -> Host rate

    // Test tone state
    bool   testToneEnabled = false;
    bool   monitorMode     = false;
    double phase           = 0.0;
    double testToneFreq    = 440.0;
    double testToneAmp     = 0.2;
    float  mainVolume      = 1.0f; // Main output volume (0.0 - 1.0)
};
