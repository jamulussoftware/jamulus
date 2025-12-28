#pragma once

#include <vector>
#include <cmath>

// Simple linear interpolation resampler for converting between DAW and Jamulus sample rates.
// Jamulus requires 48kHz; DAWs may run at 44.1kHz, 96kHz, etc.
class LinearResampler
{
public:
    LinearResampler() = default;

    void prepare(double srcRate, double dstRate, int numChannels)
    {
        this->srcRate = srcRate;
        this->dstRate = dstRate;
        this->numChannels = numChannels;
        ratio = srcRate / dstRate;
        lastSamples.resize(numChannels, 0.0f);
        position = 0.0;
    }

    // Resample from src (srcFrames interleaved samples) to dst buffer.
    // Returns the number of output frames written.
    // This version is for converting TO Jamulus rate (e.g., 44.1k -> 48k)
    int resampleTo48k(const float* src, int srcFrames, float* dst, int maxDstFrames)
    {
        if (std::abs(srcRate - dstRate) < 1.0)
        {
            // No resampling needed
            int frames = std::min(srcFrames, maxDstFrames);
            for (int i = 0; i < frames * numChannels; ++i)
                dst[i] = src[i];
            return frames;
        }

        int dstFrames = 0;
        double srcPos = position;

        while (dstFrames < maxDstFrames)
        {
            int srcIndex = static_cast<int>(srcPos);
            if (srcIndex + 1 >= srcFrames)
                break;

            double frac = srcPos - srcIndex;

            for (int ch = 0; ch < numChannels; ++ch)
            {
                float s0 = src[srcIndex * numChannels + ch];
                float s1 = src[(srcIndex + 1) * numChannels + ch];
                dst[dstFrames * numChannels + ch] = static_cast<float>(s0 + (s1 - s0) * frac);
            }

            dstFrames++;
            srcPos += ratio;
        }

        // Store fractional position for next call
        position = srcPos - static_cast<int>(srcPos);
        
        // Store last samples for continuity
        if (srcFrames > 0)
        {
            for (int ch = 0; ch < numChannels; ++ch)
                lastSamples[ch] = src[(srcFrames - 1) * numChannels + ch];
        }

        return dstFrames;
    }

    // Resample from Jamulus rate to DAW rate (e.g., 48k -> 44.1k)
    int resampleFrom48k(const float* src, int srcFrames, float* dst, int maxDstFrames)
    {
        if (std::abs(srcRate - dstRate) < 1.0)
        {
            // No resampling needed
            int frames = std::min(srcFrames, maxDstFrames);
            for (int i = 0; i < frames * numChannels; ++i)
                dst[i] = src[i];
            return frames;
        }

        // For output direction (48kHz -> host rate):
        // If srcRate=48000 and dstRate=44100, we have MORE source samples than output.
        // For each output sample, we advance srcRate/dstRate ≈ 1.088 samples in source.
        // This is the same ratio calculation as resampleTo48k uses.
        double outRatio = srcRate / dstRate;
        int dstFrames = 0;
        double srcPos = outPosition;

        while (dstFrames < maxDstFrames)
        {
            int srcIndex = static_cast<int>(srcPos);
            if (srcIndex + 1 >= srcFrames)
                break;

            double frac = srcPos - srcIndex;

            for (int ch = 0; ch < numChannels; ++ch)
            {
                float s0 = src[srcIndex * numChannels + ch];
                float s1 = src[(srcIndex + 1) * numChannels + ch];
                dst[dstFrames * numChannels + ch] = static_cast<float>(s0 + (s1 - s0) * frac);
            }

            dstFrames++;
            srcPos += outRatio;
        }

        outPosition = srcPos - static_cast<int>(srcPos);
        return dstFrames;
    }

    // Calculate how many output frames we'd get from srcFrames input
    int calcOutputFrames(int srcFrames, bool toJamulus) const
    {
        if (std::abs(srcRate - dstRate) < 1.0)
            return srcFrames;
        if (toJamulus)
            return static_cast<int>(srcFrames / ratio);
        else
            return static_cast<int>(srcFrames * ratio / (dstRate / srcRate));
    }

    bool needsResampling() const { return std::abs(srcRate - dstRate) >= 1.0; }

    void reset()
    {
        position = 0.0;
        outPosition = 0.0;
        for (auto& s : lastSamples) s = 0.0f;
    }

private:
    double srcRate = 48000.0;
    double dstRate = 48000.0;
    double ratio = 1.0;
    int numChannels = 2;
    double position = 0.0;      // Fractional position for input resampling
    double outPosition = 0.0;   // Fractional position for output resampling
    std::vector<float> lastSamples;
};
