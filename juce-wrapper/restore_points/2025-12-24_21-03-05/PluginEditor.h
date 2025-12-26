#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

class JamulusAudioProcessor;

class JamulusPluginEditor : public juce::AudioProcessorEditor
{
public:
    explicit JamulusPluginEditor(JamulusAudioProcessor& processor);
    ~JamulusPluginEditor() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    JamulusAudioProcessor& audioProcessor;

    // Handle to the Qt GUI widget
    void* qtGui = nullptr;
    
    // Flag to track if embedding worked
    bool guiEmbedded = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(JamulusPluginEditor)
};
