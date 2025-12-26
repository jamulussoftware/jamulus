#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

class JamulusAudioProcessor;

class JamulusPluginEditor : public juce::AudioProcessorEditor,
                            private juce::Timer
{
public:
    explicit JamulusPluginEditor(JamulusAudioProcessor& processor);
    ~JamulusPluginEditor() override;

    void paint(juce::Graphics& g) override;
    void resized() override;
    void focusGained(juce::Component::FocusChangeType cause) override;
    void focusLost(juce::Component::FocusChangeType cause) override;
    void parentHierarchyChanged() override;
    void visibilityChanged() override;

private:
    void timerCallback() override;
    
    JamulusAudioProcessor& audioProcessor;

    // Handle to the Qt GUI widget
    void* qtGui = nullptr;
    
    // Flag to track if embedding worked
    bool guiEmbedded = false;
    
    // Flag to track if Qt window is shown
    bool parentSet = false;
    
    // Last known position/size for change detection
    int lastX = 0, lastY = 0, lastW = 0, lastH = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(JamulusPluginEditor)
};
