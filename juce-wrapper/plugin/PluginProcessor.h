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

    void prepareToPlay(double sampleRate, int samplesPerBlock);
    void processBlock(const float** inputs, float** outputs, int numChannels, int numSamples);
    void releaseResources();

    // Get the Jamulus client handle for GUI creation
    jamulus_client_t getClient() const { return client; }

private:
    jamulus_client_t client = nullptr;
    bool clientStarted = false;
    AudioFifo* inputFifo = nullptr;
    AudioFifo* outputFifo = nullptr;
    VirtualSoundBackend vsBackend;
    int procNumChannels = 2;
    int procBlockSize = 128;
    double hostSampleRate = 48000.0;  // Jamulus native rate is 48kHz
    
    // Sample rate conversion
    bool needsResampling = false;
    LinearResampler inputResampler;   // Host rate -> 48kHz
    LinearResampler outputResampler;  // 48kHz -> Host rate
};
