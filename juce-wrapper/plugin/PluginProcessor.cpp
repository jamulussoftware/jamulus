#include "PluginProcessor.h"
#include "Resampler.h"
#include <cstring>
#include <vector>
#include <cmath>

// Jamulus native sample rate
static constexpr double JAMULUS_SAMPLE_RATE = 48000.0;

JamulusPluginProcessor::JamulusPluginProcessor()
{
    // Intentionally avoid heavy initialization here.
    // Many hosts instantiate plugins during scanning; doing network/Qt init in the
    // constructor can crash or blacklist the plugin.
}

JamulusPluginProcessor::~JamulusPluginProcessor()
{
    releaseResources();
    if ( client )
    {
        jamulus_client_destroy ( client );
        client = nullptr;
    }
}

void JamulusPluginProcessor::prepareToPlay ( double sampleRate, int samplesPerBlock )
{
    procNumChannels = 2; // stereo for now
    procBlockSize   = samplesPerBlock > 0 ? samplesPerBlock : 128;
    hostSampleRate  = sampleRate;

    // Setup resampler if host sample rate differs from Jamulus (48kHz)
    needsResampling = std::abs ( hostSampleRate - JAMULUS_SAMPLE_RATE ) >= 1.0;
    if ( needsResampling )
    {
        inputResampler.prepare ( hostSampleRate, JAMULUS_SAMPLE_RATE, procNumChannels );
        outputResampler.prepare ( JAMULUS_SAMPLE_RATE, hostSampleRate, procNumChannels );
    }

    // Create and start Jamulus client
    if ( !client )
    {
        client        = jamulus_client_create ( 0, nullptr, "JUCE Jamulus Client", false );
        clientStarted = false;
    }

    if ( client )
    {
        // Start network/codec client (enables channel for network communication)
        if ( !clientStarted )
        {
            jamulus_client_start ( client );
            clientStarted = true;
        }
        // Note: No worker thread needed - processBlock calls jamulus_client_process_audio directly
    }
}

void JamulusPluginProcessor::processBlock ( const float** inputs, float** outputs, int numChannels, int numSamples )
{
    std::vector<float> interleavedIn ( static_cast<size_t> ( numSamples ) * numChannels );
    if ( testToneEnabled )
    {
        constexpr double PI       = 3.14159265358979323846;
        double           phaseInc = 2.0 * PI * testToneFreq / hostSampleRate;
        for ( int i = 0; i < numSamples; ++i )
        {
            float sample = static_cast<float> ( std::sin ( phase ) * testToneAmp );
            phase += phaseInc;
            if ( phase > 2.0 * PI )
                phase -= 2.0 * PI;
            for ( int ch = 0; ch < numChannels; ++ch )
                interleavedIn[static_cast<size_t> ( i ) * numChannels + ch] = sample;
        }
    }
    else
    {
        for ( int i = 0; i < numSamples; ++i )
        {
            for ( int ch = 0; ch < numChannels; ++ch )
            {
                const float* in                                             = inputs[ch];
                interleavedIn[static_cast<size_t> ( i ) * numChannels + ch] = in ? in[i] : 0.0f;
            }
        }
    }

    std::vector<float> interleavedOut ( static_cast<size_t> ( numSamples ) * numChannels );

    if ( client )
    {
        if ( needsResampling )
        {
            // Resample from host rate to 48kHz for Jamulus
            // Calculate max output size (add some margin)
            int                maxJamulusFrames = static_cast<int> ( numSamples * JAMULUS_SAMPLE_RATE / hostSampleRate ) + 16;
            std::vector<float> jamulusIn ( maxJamulusFrames * numChannels );
            std::vector<float> jamulusOut ( maxJamulusFrames * numChannels );

            int jamulusFrames = inputResampler.resampleTo48k ( interleavedIn.data(), numSamples, jamulusIn.data(), maxJamulusFrames );

            // Process at 48kHz
            int rc = jamulus_client_process_audio ( client, jamulusIn.data(), jamulusOut.data(), jamulusFrames, numChannels );
            if ( rc != 0 )
            {
                std::fill ( jamulusOut.begin(), jamulusOut.end(), 0.0f );
            }

            // Resample back from 48kHz to host rate
            outputResampler.resampleFrom48k ( jamulusOut.data(), jamulusFrames, interleavedOut.data(), numSamples );
        }
        else
        {
            // No resampling needed - direct processing at 48kHz
            int rc = jamulus_client_process_audio ( client, interleavedIn.data(), interleavedOut.data(), numSamples, numChannels );
            if ( rc != 0 )
            {
                std::fill ( interleavedOut.begin(), interleavedOut.end(), 0.0f );
            }
        }
    }
    else
    {
        std::fill ( interleavedOut.begin(), interleavedOut.end(), 0.0f );
    }

    // Apply main volume to Jamulus output (before adding direct monitor)
    if ( mainVolume < 1.0f )
    {
        for ( size_t i = 0; i < interleavedOut.size(); ++i )
        {
            interleavedOut[i] *= mainVolume;
        }
    }

    // Direct Monitoring: Mix input to output if enabled (not affected by main volume)
    if ( monitorMode && interleavedIn.size() == interleavedOut.size() )
    {
        for ( size_t i = 0; i < interleavedOut.size(); ++i )
        {
            interleavedOut[i] += interleavedIn[i];
        }
    }

    // Deinterleave to outputs
    for ( int i = 0; i < numSamples; ++i )
    {
        for ( int ch = 0; ch < numChannels; ++ch )
        {
            float* out = outputs[ch];
            out[i]     = interleavedOut[static_cast<size_t> ( i ) * numChannels + ch];
        }
    }
}

void JamulusPluginProcessor::releaseResources()
{
    // stop network/codec client
    if ( client && clientStarted )
    {
        jamulus_client_stop ( client );
        clientStarted = false;
    }

    // Reset resamplers
    if ( needsResampling )
    {
        inputResampler.reset();
        outputResampler.reset();
    }

    delete inputFifo;
    inputFifo = nullptr;
    delete outputFifo;
    outputFifo = nullptr;
}
