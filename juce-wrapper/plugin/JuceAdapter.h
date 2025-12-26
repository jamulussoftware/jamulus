// Minimal JUCE AudioProcessor adapter that delegates to the lightweight
// `JamulusPluginProcessor` implemented in PluginProcessor.cpp
#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"

class JamulusAudioProcessor : public juce::AudioProcessor
{
public:
    JamulusAudioProcessor();
    ~JamulusAudioProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    // Minimal required overrides
    const juce::String getName() const override { return "JamulusVST3"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override { return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo(); }
    juce::AudioProcessorEditor* createEditor() override { return nullptr; }
    bool hasEditor() const override { return false; }
    void getStateInformation(juce::MemoryBlock&) override {}
    void setStateInformation(const void*, int) override {}

private:
    std::unique_ptr<JamulusPluginProcessor> impl;
};
