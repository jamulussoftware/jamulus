#include "audio_fifo.h"
#include <algorithm>

AudioFifo::AudioFifo(size_t capacityFrames, int channels)
    : capacityFrames(capacityFrames), numChannels(channels), buffer(capacityFrames * channels, 0.0f)
{
}

AudioFifo::~AudioFifo() = default;

bool AudioFifo::push(const float* data, size_t frames)
{
    if (!data || frames == 0) return false;

    size_t w = writePos.load(std::memory_order_relaxed);
    size_t r = readPos.load(std::memory_order_acquire);
    size_t freeFrames = (r + capacityFrames - w - 1) % capacityFrames;
    if (frames > freeFrames) return false;

    for (size_t f = 0; f < frames; ++f)
    {
        size_t idx = ((w + f) % capacityFrames) * numChannels;
        for (int c = 0; c < numChannels; ++c)
            buffer[idx + c] = data[f * numChannels + c];
    }

    writePos.store((w + frames) % capacityFrames, std::memory_order_release);
    return true;
}

size_t AudioFifo::pop(float* out, size_t frames)
{
    if (!out || frames == 0) return 0;

    size_t w = writePos.load(std::memory_order_acquire);
    size_t r = readPos.load(std::memory_order_relaxed);
    size_t availableFrames = (w + capacityFrames - r) % capacityFrames;
    size_t toRead = std::min(frames, availableFrames);

    for (size_t f = 0; f < toRead; ++f)
    {
        size_t idx = ((r + f) % capacityFrames) * numChannels;
        for (int c = 0; c < numChannels; ++c)
            out[f * numChannels + c] = buffer[idx + c];
    }

    readPos.store((r + toRead) % capacityFrames, std::memory_order_release);
    return toRead;
}

void AudioFifo::clear()
{
    writePos.store(0);
    readPos.store(0);
    std::fill(buffer.begin(), buffer.end(), 0.0f);
}

size_t AudioFifo::available() const
{
    size_t w = writePos.load(std::memory_order_acquire);
    size_t r = readPos.load(std::memory_order_acquire);
    return (w + capacityFrames - r) % capacityFrames;
}
