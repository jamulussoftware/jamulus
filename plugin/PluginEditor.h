#pragma once

#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_osc/juce_osc.h>
#include "PluginProcessor.h"
#include <map>
#include <set>

class JamulusAudioProcessor;
class JamulusGuiComponent;

class JamulusPluginEditor : public juce::AudioProcessorEditor,
                            private juce::Timer,
                            private juce::MidiInputCallback,
                            private juce::OSCReceiver,
                            private juce::OSCReceiver::Listener<juce::OSCReceiver::MessageLoopCallback>
{
public:
    explicit JamulusPluginEditor ( JamulusAudioProcessor& processor );
    ~JamulusPluginEditor() override;

    void paint ( juce::Graphics& g ) override;
    void resized() override;

private:
    struct MidiBinding
    {
        int channel    = 1;
        int controller = 0;
    };

    struct OscBinding
    {
        juce::String address;
        int          argIndex = 0;
    };

    enum class LearnMode
    {
        none,
        midi,
        osc
    };

    void timerCallback() override;
    void mouseDown ( const juce::MouseEvent& event ) override;
    void handleIncomingMidiMessage ( juce::MidiInput* source, const juce::MidiMessage& message ) override;
    void oscMessageReceived ( const juce::OSCMessage& message ) override;
    void refreshLearnableControls();
    void collectLearnableControls ( juce::Component& component );
    void showLearnMenuForSlider ( juce::Slider& slider, juce::Point<int> screenPosition );
    void armLearnForControl ( const juce::String& controlId, LearnMode mode );
    void clearLearnForControl ( const juce::String& controlId, LearnMode mode );
    void handleMidiLearnEvent ( const class JamulusPluginProcessor::MidiLearnEvent& event );
    void handleOscLearnEvent ( const juce::OSCMessage& message );
    void applyMidiBinding ( const class JamulusPluginProcessor::MidiLearnEvent& event );
    void applyOscBinding ( const juce::OSCMessage& message );
    void applyNormalizedValueToControl ( const juce::String& controlId, float normalizedValue );
    void updateMidiDevices();
    void sendMidiFeedbackForBindings();
    void saveLearnMappings() const;
    void loadLearnMappings();

    JamulusAudioProcessor&                audioProcessor;
    std::unique_ptr<JamulusGuiComponent>  guiComponent;
    std::map<juce::String, juce::Component::SafePointer<juce::Slider>> learnableControls;
    std::set<juce::Component*> observedLearnControls;
    std::map<juce::String, MidiBinding> midiBindings;
    std::map<juce::String, OscBinding>  oscBindings;
    std::map<juce::String, float> lastControlNormalizedValues;
    juce::String activeLearnControlId;
    LearnMode    activeLearnMode = LearnMode::none;
    int          oscListenPort   = 9001;
    juce::String selectedMidiInputDevice = "None";
    juce::String selectedMidiOutputDevice = "None";
    std::unique_ptr<juce::MidiInput>  midiInputDevice;
    std::unique_ptr<juce::MidiOutput> midiOutputDevice;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR ( JamulusPluginEditor )
};
