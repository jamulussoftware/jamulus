#pragma once

#include <vector>
#include <atomic>
#include <cstddef>

class AudioFifo
{
public:
    AudioFifo(size_t capacityFrames, int channels);
    ~AudioFifo();

    // Push frames from interleaved float buffer. Returns true if all frames were pushed.
    bool push(const float* data, size_t frames);

    // Pop up to 'frames' frames into interleaved float buffer. Returns number of frames popped.
    size_t pop(float* out, size_t frames);

    void clear();

    size_t available() const;

private:
    const size_t capacityFrames;
    const int numChannels;
    std::vector<float> buffer; // interleaved
    std::atomic<size_t> writePos{0};
    std::atomic<size_t> readPos{0};
};
