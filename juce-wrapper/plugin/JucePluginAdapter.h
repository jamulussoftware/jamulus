#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"

class JamulusAudioProcessor : public juce::AudioProcessor
{
public:
    JamulusAudioProcessor();
    ~JamulusAudioProcessor() override;

    const juce::String getName() const override { return "JamulusVST3"; }
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    bool hasEditor() const override { return true; }
    juce::AudioProcessorEditor* createEditor() override;

    bool isBusesLayoutSupported(const BusesLayout& layouts) const override { return true; }

    double getTailLengthSeconds() const override { return 0.0; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock&) override {}
    void setStateInformation(const void*, int) override {}

    // Access the underlying processor for GUI creation
    JamulusPluginProcessor& getProcessor() { return impl; }

private:
    JamulusPluginProcessor impl;
};

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter();
