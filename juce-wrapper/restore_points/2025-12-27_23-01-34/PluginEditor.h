#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

class JamulusAudioProcessor;
class JamulusGuiComponent;

class JamulusPluginEditor : public juce::AudioProcessorEditor
{
public:
    explicit JamulusPluginEditor(JamulusAudioProcessor& processor);
    ~JamulusPluginEditor() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    JamulusAudioProcessor& audioProcessor;

    // Native JUCE GUI component
    std::unique_ptr<JamulusGuiComponent> guiComponent;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(JamulusPluginEditor)
};
