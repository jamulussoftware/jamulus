#pragma once

#pragma once

#include "../jamulus_wrapper.h"
#include "../audio_fifo.h"
#include "../virtual_sound/virtual_sound.h"
#include "Resampler.h"
#include "AudioDelay.h"
#include "AudioReverb.h"

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

    // Reverb Control Methods
public:
    void setReverbEnabled ( bool enabled ) { reverbEnabled = enabled; }
    void setReverbMix ( float mix ) { audioReverb.setMix ( mix ); }
    void setReverbDecay ( float seconds ) { audioReverb.setDecay ( seconds ); }
    void setReverbDamping ( float damping ) { audioReverb.setDamping ( damping ); }

private:
    // Internal Reverb
    bool        reverbEnabled = false;
    AudioReverb audioReverb;

    // Delay Control Methods
public:
    void setDelayEnabled ( bool enabled ) { delayEnabled = enabled; }
    void setDelayMix ( float mix ) { audioDelay.setMix ( mix ); }
    void setDelayTime ( float ms ) { audioDelay.setDelayTime ( ms ); }
    void setDelayFeedback ( float fb ) { audioDelay.setFeedback ( fb ); }
    void setDelayPingPong ( bool pp ) { audioDelay.setPingPong ( pp ); }
    void setDelayHighPass ( float freq ) { audioDelay.setHighPassFreq ( freq ); }

private:
    // Internal Delay
    bool       delayEnabled = false;
    AudioDelay audioDelay;

    // Limiter Control Methods
public:
    void  setLimiterEnabled ( bool enabled ) { limiterEnabled = enabled; }
    bool  isLimiterEnabled() const { return limiterEnabled; }
    void  setLimiterThreshold ( float thresh ) { limiterThreshold = thresh; } // 0.0 to 1.0
    float getLimiterThreshold() const { return limiterThreshold; }

private:
    // Internal Limiter (soft-knee)
    bool  limiterEnabled   = false;
    float limiterThreshold = 0.9f; // Default threshold at ~-0.9dB
    float limiterEnvelope  = 0.0f; // Envelope follower state
};
