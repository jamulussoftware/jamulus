#include "PluginProcessor.h"
#include <cstring>
#include <chrono>
#include <vector>
#include <thread>

JamulusPluginProcessor::JamulusPluginProcessor()
{
    // Intentionally avoid heavy initialization here.
    // Many hosts instantiate plugins during scanning; doing network/Qt init in the
    // constructor can crash or blacklist the plugin.
}

JamulusPluginProcessor::~JamulusPluginProcessor()
{
    releaseResources();
    if (client)
    {
        jamulus_client_destroy(client);
        client = nullptr;
    }
}

void JamulusPluginProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    procNumChannels = 2; // stereo for now
    procBlockSize = samplesPerBlock > 0 ? samplesPerBlock : 128;

    // Create and start Jamulus client
    if (!client)
    {
        client = jamulus_client_create(0, nullptr, "JUCE Jamulus Client", false);
        clientStarted = false;
        workerStarted = false;
    }

    if (client)
    {
        // Start network/codec client
        if (!clientStarted)
        {
            jamulus_client_start(client);
            clientStarted = true;
        }

        // Start worker thread
        if (!workerStarted)
        {
            jamulus_client_start_worker(client, procBlockSize);
            workerStarted = true;
        }
    }
}

void JamulusPluginProcessor::processBlock(const float** inputs, float** outputs, int numChannels, int numSamples)
{
    // Interleave input channels into a temp buffer
    std::vector<float> interleavedIn(static_cast<size_t>(numSamples) * numChannels);
    for (int i = 0; i < numSamples; ++i)
    {
        for (int ch = 0; ch < numChannels; ++ch)
        {
            const float* in = inputs[ch];
            interleavedIn[static_cast<size_t>(i) * numChannels + ch] = in ? in[i] : 0.0f;
        }
    }

    // Directly process audio through Jamulus client (synchronous).
    std::vector<float> interleavedOut(static_cast<size_t>(numSamples) * numChannels);
    if (client)
    {
        int rc = jamulus_client_process_audio(client, interleavedIn.data(), interleavedOut.data(), numSamples, numChannels);
        if (rc != 0)
        {
            // on error, output silence
            std::fill(interleavedOut.begin(), interleavedOut.end(), 0.0f);
        }
    }
    else
    {
        std::fill(interleavedOut.begin(), interleavedOut.end(), 0.0f);
    }

    // Deinterleave to outputs
    for (int i = 0; i < numSamples; ++i)
    {
        for (int ch = 0; ch < numChannels; ++ch)
        {
            float* out = outputs[ch];
            out[i] = interleavedOut[static_cast<size_t>(i) * numChannels + ch];
        }
    }
}

void JamulusPluginProcessor::releaseResources()
{
    // Stop worker
    workerRunning.store(false);
    if (workerThread.joinable()) workerThread.join();

    if (client && workerStarted)
    {
        jamulus_client_stop_worker(client);
        workerStarted = false;
    }

    // stop network/codec client
    if (client && clientStarted)
    {
        jamulus_client_stop(client);
        clientStarted = false;
    }

    delete inputFifo; inputFifo = nullptr;
    delete outputFifo; outputFifo = nullptr;
}
