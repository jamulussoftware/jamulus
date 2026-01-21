#include "PluginProcessor.h"
#include "Resampler.h"
#include "DebugLogger.h"
#include <cstring>
#include <vector>
#include <cmath>

// Jamulus native sample rate
static constexpr double JAMULUS_SAMPLE_RATE = 48000.0;

JamulusPluginProcessor::JamulusPluginProcessor()
{
    DebugLogger::instance().log("[JamulusPluginProcessor] Constructor called");
    // Intentionally avoid heavy initialization here.
    // Many hosts instantiate plugins during scanning; doing network/Qt init in the
    // constructor can crash or blacklist the plugin.
}

JamulusPluginProcessor::~JamulusPluginProcessor()
{
    DebugLogger::instance().log("[JamulusPluginProcessor] Destructor called");
    releaseResources();
    if ( client )
    {
        DebugLogger::instance().log("[JamulusPluginProcessor] Destroying client");
        jamulus_client_destroy ( client );
        client = nullptr;
    }
}

void JamulusPluginProcessor::prepareToPlay ( double sampleRate, int samplesPerBlock )
{
    DebugLogger::instance().log("[JamulusPluginProcessor] prepareToPlay called. SampleRate: " + std::to_string(sampleRate) + ", BlockSize: " + std::to_string(samplesPerBlock));
    procNumChannels = 2; // stereo for now
    procBlockSize   = samplesPerBlock > 0 ? samplesPerBlock : 128;
    hostSampleRate  = sampleRate;
    audioReverb.init ( static_cast<int> ( sampleRate ) );
    audioDelay.init ( static_cast<int> ( sampleRate ), 1000 );

    // Setup resampler if host sample rate differs from Jamulus (48kHz)
    needsResampling = std::abs ( hostSampleRate - JAMULUS_SAMPLE_RATE ) >= 1.0;
    if ( needsResampling )
    {
        DebugLogger::instance().log("[JamulusPluginProcessor] Host sample rate differs from Jamulus, enabling resampling");
        inputResampler.prepare ( hostSampleRate, JAMULUS_SAMPLE_RATE, procNumChannels );
        outputResampler.prepare ( JAMULUS_SAMPLE_RATE, hostSampleRate, procNumChannels );
    }

    // Create and start Jamulus client
    if ( !client )
    {
        DebugLogger::instance().log("[JamulusPluginProcessor] Creating Jamulus client");
        client        = jamulus_client_create ( 0, nullptr, "JUCE Jamulus Client", false );
        clientStarted = false;
        if (!client) {
            DebugLogger::instance().log("[JamulusPluginProcessor] ERROR: jamulus_client_create returned nullptr!");
        }
    }

    if ( client )
    {
        // Start network/codec client (enables channel for network communication)
        if ( !clientStarted )
        {
            DebugLogger::instance().log("[JamulusPluginProcessor] Starting Jamulus client");
            jamulus_client_start ( client );
            clientStarted = true;
        }
        // Note: No worker thread needed - processBlock calls jamulus_client_process_audio directly
    }
    else {
        DebugLogger::instance().log("[JamulusPluginProcessor] ERROR: client is nullptr after creation!");
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

    // Apply soft-knee limiter (if enabled)
    if ( limiterEnabled )
    {
        const float attackCoeff  = 0.01f;   // Fast attack (~0.2ms at 48kHz)
        const float releaseCoeff = 0.0001f; // Slow release (~200ms at 48kHz)
        const float ratio        = 10.0f;   // High ratio for limiting effect
        const float knee         = 0.1f;    // Soft knee width

        for ( int i = 0; i < numSamples; ++i )
        {
            // Get stereo sample pair
            float left  = internalOut[static_cast<size_t> ( i ) * 2 + 0];
            float right = internalOut[static_cast<size_t> ( i ) * 2 + 1];

            // Peak detection (use max of L/R)
            float peak = std::max ( std::abs ( left ), std::abs ( right ) );

            // Envelope follower
            if ( peak > limiterEnvelope )
                limiterEnvelope += ( peak - limiterEnvelope ) * attackCoeff;
            else
                limiterEnvelope += ( peak - limiterEnvelope ) * releaseCoeff;

            // Calculate gain reduction
            float gain = 1.0f;
            if ( limiterEnvelope > limiterThreshold )
            {
                // Soft-knee compression above threshold
                float overshoot  = limiterEnvelope - limiterThreshold;
                float compressed = limiterThreshold + overshoot / ratio;
                gain             = compressed / limiterEnvelope;
            }
            else if ( limiterEnvelope > limiterThreshold - knee )
            {
                // Soft knee transition
                float x = ( limiterEnvelope - ( limiterThreshold - knee ) ) / knee;
                gain    = 1.0f - x * x * 0.05f; // Gentle reduction in knee region
            }

            // Apply gain reduction
            internalOut[static_cast<size_t> ( i ) * 2 + 0] = left * gain;
            internalOut[static_cast<size_t> ( i ) * 2 + 1] = right * gain;
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
    DebugLogger::instance().log("[JamulusPluginProcessor] releaseResources called");
    // stop network/codec client
    if ( client && clientStarted )
    {
        DebugLogger::instance().log("[JamulusPluginProcessor] Stopping Jamulus client");
        jamulus_client_stop ( client );
        clientStarted = false;
    }

    // Reset resamplers
    if ( needsResampling )
    {
        DebugLogger::instance().log("[JamulusPluginProcessor] Resetting resamplers");
        inputResampler.reset();
        outputResampler.reset();
    }

    delete inputFifo;
    inputFifo = nullptr;
    delete outputFifo;
    outputFifo = nullptr;
}
