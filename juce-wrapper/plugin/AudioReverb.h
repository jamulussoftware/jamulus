#pragma once

#include <vector>
#include <cmath>
#include <algorithm>

/**
 * Simple stereo reverb effect with wet/dry mix.
 *
 * Uses a basic Schroeder-style algorithm with:
 * - 4 parallel comb filters for the late reverb
 * - 2 series allpass filters for diffusion
 *
 * Low latency - dry signal passes through immediately.
 *
 * Features:
 * - Adjustable decay time (0.1-5.0 seconds)
 * - Wet/dry mix (0-100%)
 * - Damping (high-frequency rolloff in the reverb tail)
 */
class AudioReverb
{
public:
    AudioReverb() = default;

    void init ( int sampleRate )
    {
        this->sampleRate = sampleRate;

        // Comb filter delay times (in samples) - prime numbers for less coloration
        // Slightly different for L/R for stereo width
        int combDelays[4]  = { 1557, 1617, 1491, 1422 }; // ~32-34ms at 48kHz
        int combDelaysR[4] = { 1580, 1640, 1514, 1445 }; // Slightly offset for stereo

        // Allpass delay times
        int apDelays[2]  = { 225, 556 }; // ~5ms, ~12ms
        int apDelaysR[2] = { 241, 572 };

        // Scale for sample rate
        float scale = static_cast<float> ( sampleRate ) / 48000.0f;

        // Initialize comb filters
        for ( int i = 0; i < 4; ++i )
        {
            int delayL = static_cast<int> ( combDelays[i] * scale );
            int delayR = static_cast<int> ( combDelaysR[i] * scale );
            combL[i].init ( delayL );
            combR[i].init ( delayR );
        }

        // Initialize allpass filters
        for ( int i = 0; i < 2; ++i )
        {
            int delayL = static_cast<int> ( apDelays[i] * scale );
            int delayR = static_cast<int> ( apDelaysR[i] * scale );
            apL[i].init ( delayL );
            apR[i].init ( delayR );
        }

        // Default settings
        setDecay ( 1.5f );   // 1.5 second decay
        setMix ( 0.3f );     // 30% wet
        setDamping ( 0.3f ); // Moderate damping

        clear();
    }

    void clear()
    {
        for ( int i = 0; i < 4; ++i )
        {
            combL[i].clear();
            combR[i].clear();
        }
        for ( int i = 0; i < 2; ++i )
        {
            apL[i].clear();
            apR[i].clear();
        }
    }

    // Decay time in seconds (0.1 - 5.0)
    void setDecay ( float seconds )
    {
        decayTime = std::max ( 0.1f, std::min ( seconds, 5.0f ) );
        updateFeedback();
    }

    float getDecay() const { return decayTime; }

    // Wet/dry mix (0.0 = dry only, 1.0 = wet only)
    void setMix ( float m ) { mix = std::max ( 0.0f, std::min ( m, 1.0f ) ); }

    float getMix() const { return mix; }

    // Damping - high frequency rolloff (0.0 = bright, 1.0 = very dark)
    void setDamping ( float d )
    {
        damping = std::max ( 0.0f, std::min ( d, 0.9f ) );
        for ( int i = 0; i < 4; ++i )
        {
            combL[i].setDamping ( damping );
            combR[i].setDamping ( damping );
        }
    }

    float getDamping() const { return damping; }

    // Process interleaved stereo audio (in-place)
    void process ( float* stereoBuffer, int numFrames )
    {
        if ( mix <= 0.0f )
            return; // No effect if mix is zero

        const float dryGain = 1.0f - mix;
        const float wetGain = mix * 0.5f; // Scale down wet to prevent clipping

        for ( int i = 0; i < numFrames; ++i )
        {
            float inL = stereoBuffer[i * 2];
            float inR = stereoBuffer[i * 2 + 1];

            // Mix input to mono for reverb input (common practice)
            float mono = ( inL + inR ) * 0.5f;

            // Process through parallel comb filters
            float combOutL = 0.0f;
            float combOutR = 0.0f;

            for ( int c = 0; c < 4; ++c )
            {
                combOutL += combL[c].process ( mono );
                combOutR += combR[c].process ( mono );
            }

            // Scale comb output
            combOutL *= 0.25f;
            combOutR *= 0.25f;

            // Process through series allpass filters for diffusion
            float wetL = combOutL;
            float wetR = combOutR;

            for ( int a = 0; a < 2; ++a )
            {
                wetL = apL[a].process ( wetL );
                wetR = apR[a].process ( wetR );
            }

            // Mix dry and wet signals
            stereoBuffer[i * 2]     = dryGain * inL + wetGain * wetL;
            stereoBuffer[i * 2 + 1] = dryGain * inR + wetGain * wetR;
        }
    }

private:
    // Lowpass comb filter for reverb
    class LPCombFilter
    {
    public:
        void init ( int delaySamples )
        {
            this->delaySamples = delaySamples;
            buffer.resize ( delaySamples, 0.0f );
            writePos    = 0;
            filterState = 0.0f;
        }

        void clear()
        {
            std::fill ( buffer.begin(), buffer.end(), 0.0f );
            filterState = 0.0f;
        }

        void setFeedback ( float fb ) { feedback = fb; }
        void setDamping ( float d ) { damping = d; }

        float process ( float input )
        {
            if ( buffer.empty() )
                return input;

            float output = buffer[writePos];

            // Apply lowpass filter (damping) to feedback
            filterState = output * ( 1.0f - damping ) + filterState * damping;

            // Write input + filtered feedback
            buffer[writePos] = input + filterState * feedback;

            writePos++;
            if ( writePos >= delaySamples )
                writePos = 0;

            return output;
        }

    private:
        std::vector<float> buffer;
        int                delaySamples = 0;
        int                writePos     = 0;
        float              feedback     = 0.8f;
        float              damping      = 0.3f;
        float              filterState  = 0.0f;
    };

    // Allpass filter for diffusion
    class AllpassFilter
    {
    public:
        void init ( int delaySamples )
        {
            this->delaySamples = delaySamples;
            buffer.resize ( delaySamples, 0.0f );
            writePos = 0;
        }

        void clear() { std::fill ( buffer.begin(), buffer.end(), 0.0f ); }

        float process ( float input )
        {
            if ( buffer.empty() )
                return input;

            float delayed    = buffer[writePos];
            float output     = -input + delayed;
            buffer[writePos] = input + delayed * feedback;

            writePos++;
            if ( writePos >= delaySamples )
                writePos = 0;

            return output;
        }

    private:
        std::vector<float> buffer;
        int                delaySamples = 0;
        int                writePos     = 0;
        const float        feedback     = 0.5f; // Fixed allpass coefficient
    };

    void updateFeedback()
    {
        // Calculate feedback based on decay time
        // RT60 formula: fb = 10^(-3 * delay / (RT60 * sampleRate))
        // Simplified: use average comb delay
        float avgDelay = 1500.0f; // ~31ms at 48kHz base
        float fb       = std::pow ( 0.001f, avgDelay / ( decayTime * sampleRate ) );
        fb             = std::max ( 0.0f, std::min ( fb, 0.98f ) );

        for ( int i = 0; i < 4; ++i )
        {
            combL[i].setFeedback ( fb );
            combR[i].setFeedback ( fb );
        }
    }

    int   sampleRate = 48000;
    float decayTime  = 1.5f;
    float mix        = 0.3f;
    float damping    = 0.3f;

    LPCombFilter  combL[4];
    LPCombFilter  combR[4];
    AllpassFilter apL[2];
    AllpassFilter apR[2];
};
