#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"

class JamulusAudioProcessor : public juce::AudioProcessor, private juce::Timer
{
public:
    JamulusAudioProcessor()
        : AudioProcessor ( createBusLayout() )
    {
        startTimer ( 20 ); // 20ms = 50fps event pump
    }
    ~JamulusAudioProcessor() override;

    const juce::String getName() const override { return "JamulusPlus"; }
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    void timerCallback() override;

    bool hasEditor() const override { return true; }
    juce::AudioProcessorEditor* createEditor() override;

    bool isBusesLayoutSupported(const BusesLayout& layouts) const override
    {
        // Main I/O must be stereo
        if (layouts.getMainInputChannelSet() != juce::AudioChannelSet::stereo())
            return false;
        if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
            return false;

        for (int busIndex = 1; busIndex < layouts.outputBuses.size(); ++busIndex)
        {
            const auto& outputSet = layouts.getChannelSet ( false, busIndex );
            if (!outputSet.isDisabled() && outputSet != juce::AudioChannelSet::stereo())
                return false;
        }
        
        // Sidechain can be stereo, mono, or disabled
        auto sidechainSet = layouts.getChannelSet(true, 1);
        if (!sidechainSet.isDisabled() && 
            sidechainSet != juce::AudioChannelSet::stereo() &&
            sidechainSet != juce::AudioChannelSet::mono())
            return false;
        
        return true;
    }

    double getTailLengthSeconds() const override { return 0.0; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return true; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock&) override {}
    void setStateInformation(const void*, int) override {}

    // Access the underlying processor for GUI creation
    JamulusPluginProcessor& getProcessor() { return impl; }
    
    // Sidechain control
    void setSidechainEnabled(bool enabled) { useSidechain = enabled; }
    bool isSidechainEnabled() const { return useSidechain; }
    bool isSidechainAvailable() const { return getBus(true, 1) != nullptr && getBus(true, 1)->isEnabled(); }

private:
    static BusesProperties createBusLayout()
    {
        auto buses = BusesProperties()
                         .withInput ( "Input", juce::AudioChannelSet::stereo(), true )
                         .withOutput ( "Output 1/2", juce::AudioChannelSet::stereo(), true )
                         .withInput ( "Sidechain", juce::AudioChannelSet::stereo(), false );

        for (int pairIndex = 1; pairIndex < 16; ++pairIndex)
        {
            const int firstChannel = pairIndex * 2 + 1;
            const int secondChannel = firstChannel + 1;
            buses = buses.withOutput ( "Output " + juce::String ( firstChannel ) + "/" + juce::String ( secondChannel ),
                                       juce::AudioChannelSet::stereo(),
                                       false );
        }

        return buses;
    }

    JamulusPluginProcessor impl;
    bool useSidechain = false;
};

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter();
