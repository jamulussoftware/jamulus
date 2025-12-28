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
    audioReverb.init ( static_cast<int> ( sampleRate ) );
    audioDelay.init ( static_cast<int> ( sampleRate ), 1000 );

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
    // always process internally as stereo to ensure effects/jamulus work correctly
    const int          internalChannels = 2;
    std::vector<float> internalIn ( static_cast<size_t> ( numSamples ) * internalChannels );

    // 1. Prepare Input (Host -> Internal Stereo)
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
            // Write to both channels
            internalIn[static_cast<size_t> ( i ) * 2 + 0] = sample;
            internalIn[static_cast<size_t> ( i ) * 2 + 1] = sample;
        }
    }
    else
    {
        for ( int i = 0; i < numSamples; ++i )
        {
            if ( numChannels >= 2 )
            {
                // Stereo Input
                internalIn[static_cast<size_t> ( i ) * 2 + 0] = inputs[0] ? inputs[0][i] : 0.0f;
                internalIn[static_cast<size_t> ( i ) * 2 + 1] = inputs[1] ? inputs[1][i] : 0.0f;
            }
            else if ( numChannels == 1 )
            {
                // Mono Input -> Duplicate to L/R
                float sample                                  = inputs[0] ? inputs[0][i] : 0.0f;
                internalIn[static_cast<size_t> ( i ) * 2 + 0] = sample;
                internalIn[static_cast<size_t> ( i ) * 2 + 1] = sample;
            }
            else
            {
                internalIn[static_cast<size_t> ( i ) * 2 + 0] = 0.0f;
                internalIn[static_cast<size_t> ( i ) * 2 + 1] = 0.0f;
            }
        }
    }

    std::vector<float> internalOut ( static_cast<size_t> ( numSamples ) * internalChannels );

    if ( client )
    {
        if ( needsResampling )
        {
            // Resample from host rate to 48kHz for Jamulus (Stereo)
            int                maxJamulusFrames = static_cast<int> ( numSamples * JAMULUS_SAMPLE_RATE / hostSampleRate ) + 16;
            std::vector<float> jamulusIn ( maxJamulusFrames * internalChannels );
            std::vector<float> jamulusOut ( maxJamulusFrames * internalChannels );

            int jamulusFrames = inputResampler.resampleTo48k ( internalIn.data(), numSamples, jamulusIn.data(), maxJamulusFrames );

            // Process at 48kHz (Always Stereo)
            int rc = jamulus_client_process_audio ( client, jamulusIn.data(), jamulusOut.data(), jamulusFrames, internalChannels );
            if ( rc != 0 )
            {
                std::fill ( jamulusOut.begin(), jamulusOut.end(), 0.0f );
            }

            // Resample back from 48kHz to host rate (Stereo)
            outputResampler.resampleFrom48k ( jamulusOut.data(), jamulusFrames, internalOut.data(), numSamples );
        }
        else
        {
            // No resampling needed - direct processing at 48kHz
            int rc = jamulus_client_process_audio ( client, internalIn.data(), internalOut.data(), numSamples, internalChannels );
            if ( rc != 0 )
            {
                std::fill ( internalOut.begin(), internalOut.end(), 0.0f );
            }
        }
    }
    else
    {
        std::fill ( internalOut.begin(), internalOut.end(), 0.0f );
    }

    // Apply Reverb (if enabled) - works on Stereo Internal Buffer
    if ( reverbEnabled )
    {
        audioReverb.process ( internalOut.data(), numSamples );
    }

    // Apply Delay (if enabled) - works on Stereo Internal Buffer
    if ( delayEnabled )
    {
        audioDelay.process ( internalOut.data(), numSamples );
    }

    // Apply main volume to Jamulus output
    if ( mainVolume < 1.0f )
    {
        for ( size_t i = 0; i < internalOut.size(); ++i )
        {
            internalOut[i] *= mainVolume;
        }
    }

    // Direct Monitoring: Mix input to output if enabled
    if ( monitorMode )
    {
        for ( size_t i = 0; i < internalOut.size(); ++i )
        {
            internalOut[i] += internalIn[i];
        }
    }

    // 2. Output (Internal Stereo -> Host Output)
    for ( int i = 0; i < numSamples; ++i )
    {
        if ( numChannels >= 2 )
        {
            // Stereo Output
            if ( outputs[0] )
                outputs[0][i] = internalOut[static_cast<size_t> ( i ) * 2 + 0];
            if ( outputs[1] )
                outputs[1][i] = internalOut[static_cast<size_t> ( i ) * 2 + 1];
        }
        else if ( numChannels == 1 )
        {
            // Mono Output -> Average L+R
            float left  = internalOut[static_cast<size_t> ( i ) * 2 + 0];
            float right = internalOut[static_cast<size_t> ( i ) * 2 + 1];
            if ( outputs[0] )
                outputs[0][i] = ( left + right ) * 0.5f;
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
