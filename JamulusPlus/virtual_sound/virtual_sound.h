#pragma once

// Minimal virtual sound backend stub for future integration.
class VirtualSoundBackend
{
public:
    VirtualSoundBackend() = default;
    ~VirtualSoundBackend() = default;

    // Called by host audio thread to provide input frames and receive output frames.
    // Frames are interleaved 16-bit PCM. This is a placeholder.
    void process(const short* in, short* out, int numFrames, int numChannels)
    {
        // Default: passthrough (copy input to output)
        int samples = numFrames * numChannels;
        for (int i = 0; i < samples; ++i)
            out[i] = in ? in[i] : 0;
    }

    // Float version for easier integration with JUCE-style floats [-1..1]
    void processFloat(const float* in, float* out, int numFrames, int numChannels)
    {
        int samples = numFrames * numChannels;
        for (int i = 0; i < samples; ++i)
            out[i] = in ? in[i] : 0.0f;
    }
};
