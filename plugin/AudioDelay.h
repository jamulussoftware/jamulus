#pragma once

#include <vector>
#include <cmath>
#include <algorithm>

/**
 * Simple stereo delay effect with wet/dry mix.
 * 
 * Like the built-in reverb, this adds NO LATENCY to the dry signal.
 * The delay is applied only to the wet signal which is mixed in parallel.
 * 
 * Features:
 * - Adjustable delay time (0-1000ms)
 * - Feedback (0-95%)
 * - Wet/dry mix
 * - Stereo ping-pong mode
 * - High-pass filter on feedback to prevent mud buildup
 */
class AudioDelay
{
public:
    AudioDelay() = default;
    
    void init(int sampleRate, int maxDelayMs = 1000)
    {
        this->sampleRate = sampleRate;
        maxDelaySamples = (sampleRate * maxDelayMs) / 1000;
        
        // Allocate delay buffers
        delayBufferL.resize(maxDelaySamples, 0.0f);
        delayBufferR.resize(maxDelaySamples, 0.0f);
        
        writePos = 0;
        
        // Default settings
        setDelayTime(250.0f);  // 250ms default
        setFeedback(0.3f);     // 30% feedback
        setMix(0.3f);          // 30% wet
        setPingPong(false);
        setHighPassFreq(100.0f);  // 100Hz high-pass on feedback
    }
    
    void clear()
    {
        std::fill(delayBufferL.begin(), delayBufferL.end(), 0.0f);
        std::fill(delayBufferR.begin(), delayBufferR.end(), 0.0f);
        hpStateL = 0.0f;
        hpStateR = 0.0f;
    }
    
    // Delay time in milliseconds
    void setDelayTime(float ms)
    {
        delayTimeMs = std::max(1.0f, std::min(ms, 1000.0f));
        delaySamples = static_cast<int>((sampleRate * delayTimeMs) / 1000.0f);
        delaySamples = std::max(1, std::min(delaySamples, maxDelaySamples - 1));
    }
    
    float getDelayTime() const { return delayTimeMs; }
    
    // Feedback amount (0.0 - 0.95)
    void setFeedback(float fb)
    {
        feedback = std::max(0.0f, std::min(fb, 0.95f));
    }
    
    float getFeedback() const { return feedback; }
    
    // Wet/dry mix (0.0 = dry only, 1.0 = wet only)
    void setMix(float m)
    {
        mix = std::max(0.0f, std::min(m, 1.0f));
    }
    
    float getMix() const { return mix; }
    
    // Ping-pong mode (alternates between L and R)
    void setPingPong(bool enabled)
    {
        pingPong = enabled;
    }
    
    bool getPingPong() const { return pingPong; }
    
    // High-pass filter frequency on feedback loop
    void setHighPassFreq(float freq)
    {
        hpFreq = std::max(20.0f, std::min(freq, 1000.0f));
        // Simple one-pole high-pass coefficient
        float rc = 1.0f / (2.0f * 3.14159265f * hpFreq);
        float dt = 1.0f / static_cast<float>(sampleRate);
        hpCoeff = rc / (rc + dt);
    }
    
    // Process interleaved stereo audio (in-place)
    // Returns immediately with dry signal, delay is mixed in parallel
    void process(float* stereoBuffer, int numFrames)
    {
        if (mix <= 0.0f) return;  // No effect if mix is zero
        
        const float dryGain = 1.0f - mix;
        const float wetGain = mix;
        
        for (int i = 0; i < numFrames; ++i)
        {
            float inL = stereoBuffer[i * 2];
            float inR = stereoBuffer[i * 2 + 1];
            
            // Calculate read position
            int readPos = writePos - delaySamples;
            if (readPos < 0) readPos += maxDelaySamples;
            
            // Read delayed samples
            float delayedL = delayBufferL[readPos];
            float delayedR = delayBufferR[readPos];
            
            // Apply high-pass filter to feedback to prevent mud
            float fbL = applyHighPass(delayedL, hpStateL);
            float fbR = applyHighPass(delayedR, hpStateR);
            
            // Write to delay buffer with feedback
            if (pingPong)
            {
                // Ping-pong: L feeds R, R feeds L
                delayBufferL[writePos] = inL + feedback * fbR;
                delayBufferR[writePos] = inR + feedback * fbL;
            }
            else
            {
                // Normal stereo delay
                delayBufferL[writePos] = inL + feedback * fbL;
                delayBufferR[writePos] = inR + feedback * fbR;
            }
            
            // Mix dry and wet signals
            // DRY SIGNAL PASSES THROUGH IMMEDIATELY - NO LATENCY!
            stereoBuffer[i * 2]     = dryGain * inL + wetGain * delayedL;
            stereoBuffer[i * 2 + 1] = dryGain * inR + wetGain * delayedR;
            
            // Advance write position
            writePos++;
            if (writePos >= maxDelaySamples) writePos = 0;
        }
    }
    
    // Process int16_t stereo buffer (like Jamulus uses)
    void process(int16_t* stereoBuffer, int numFrames)
    {
        if (mix <= 0.0f) return;
        
        const float dryGain = 1.0f - mix;
        const float wetGain = mix;
        
        for (int i = 0; i < numFrames; ++i)
        {
            float inL = static_cast<float>(stereoBuffer[i * 2]);
            float inR = static_cast<float>(stereoBuffer[i * 2 + 1]);
            
            int readPos = writePos - delaySamples;
            if (readPos < 0) readPos += maxDelaySamples;
            
            float delayedL = delayBufferL[readPos];
            float delayedR = delayBufferR[readPos];
            
            float fbL = applyHighPass(delayedL, hpStateL);
            float fbR = applyHighPass(delayedR, hpStateR);
            
            if (pingPong)
            {
                delayBufferL[writePos] = inL + feedback * fbR;
                delayBufferR[writePos] = inR + feedback * fbL;
            }
            else
            {
                delayBufferL[writePos] = inL + feedback * fbL;
                delayBufferR[writePos] = inR + feedback * fbR;
            }
            
            // Mix and clamp
            float outL = dryGain * inL + wetGain * delayedL;
            float outR = dryGain * inR + wetGain * delayedR;
            
            stereoBuffer[i * 2]     = static_cast<int16_t>(std::max(-32768.0f, std::min(32767.0f, outL)));
            stereoBuffer[i * 2 + 1] = static_cast<int16_t>(std::max(-32768.0f, std::min(32767.0f, outR)));
            
            writePos++;
            if (writePos >= maxDelaySamples) writePos = 0;
        }
    }

private:
    float applyHighPass(float input, float& state)
    {
        float output = hpCoeff * (state + input - state);
        state = input;
        return output * hpCoeff + input * (1.0f - hpCoeff);  // Simple mix
    }
    
    int sampleRate = 48000;
    int maxDelaySamples = 48000;
    int delaySamples = 12000;  // 250ms at 48kHz
    float delayTimeMs = 250.0f;
    float feedback = 0.3f;
    float mix = 0.3f;
    bool pingPong = false;
    
    std::vector<float> delayBufferL;
    std::vector<float> delayBufferR;
    int writePos = 0;
    
    // High-pass filter state
    float hpFreq = 100.0f;
    float hpCoeff = 0.99f;
    float hpStateL = 0.0f;
    float hpStateR = 0.0f;
};
