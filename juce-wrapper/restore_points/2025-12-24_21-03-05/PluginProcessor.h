#pragma once

#include "../jamulus_wrapper.h"
#include "../audio_fifo.h"
#include "../virtual_sound/virtual_sound.h"
#include <thread>
#include <atomic>

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
    bool workerStarted = false;
    AudioFifo* inputFifo = nullptr;
    AudioFifo* outputFifo = nullptr;
    VirtualSoundBackend vsBackend;
    std::thread workerThread;
    std::atomic<bool> workerRunning{false};
    int procNumChannels = 2;
    int procBlockSize = 128;
};
