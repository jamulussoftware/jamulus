#include "PluginEditor.h"
#include "JucePluginAdapter.h"
#include "../jamulus_wrapper.h"

#ifdef _WIN32
#include <Windows.h>
#endif

JamulusPluginEditor::JamulusPluginEditor(JamulusAudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p)
{
    // Get the Jamulus client from the processor
    auto client = audioProcessor.getProcessor().getClient();
    
    if (client)
    {
        // Create the Qt GUI
        qtGui = jamulus_gui_create(client);
        
        if (qtGui)
        {
            // Get preferred size from the Qt dialog
            int prefWidth = 800, prefHeight = 600;
            jamulus_gui_get_preferred_size(qtGui, &prefWidth, &prefHeight);
            
            // Make it a bit wider for better initial view
            prefWidth = juce::jmax(prefWidth, 900);
            
            // Set our size to match
            setSize(prefWidth, prefHeight);
            setResizable(true, true);
            
            guiEmbedded = true;
            
            // Start a timer to embed the Qt window once JUCE window is ready
            startTimer(50);
        }
        else
        {
            setSize(800, 600);
        }
    }
    else
    {
        setSize(800, 600);
    }
}

JamulusPluginEditor::~JamulusPluginEditor()
{
    stopTimer();
    if (qtGui)
    {
        jamulus_gui_hide(qtGui);
    }
}

void JamulusPluginEditor::timerCallback()
{
    if (!parentSet && qtGui && guiEmbedded)
    {
#ifdef _WIN32
        if (auto* peer = getPeer())
        {
            void* parentHwnd = peer->getNativeHandle();
            if (parentHwnd)
            {
                // Set up as child window
                jamulus_gui_set_parent(qtGui, parentHwnd);
                jamulus_gui_resize(qtGui, getWidth(), getHeight());
                jamulus_gui_show(qtGui);
                parentSet = true;
                
                // Stop the fast timer, switch to slow monitoring
                stopTimer();
                startTimer(200);
            }
        }
#endif
    }
}

void JamulusPluginEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff323232));
    
    if (!guiEmbedded || !parentSet)
    {
        g.setColour(juce::Colours::white);
        g.setFont(16.0f);
        g.drawFittedText(
            "Jamulus VST3 Plugin\n\n"
            "Loading...",
            getLocalBounds().reduced(20),
            juce::Justification::centred,
            10);
    }
}

void JamulusPluginEditor::resized()
{
    if (qtGui && guiEmbedded && parentSet)
    {
        jamulus_gui_resize(qtGui, getWidth(), getHeight());
    }
}

void JamulusPluginEditor::focusGained(juce::Component::FocusChangeType)
{
    // Nothing - don't touch Qt from focus callbacks
}

void JamulusPluginEditor::focusLost(juce::Component::FocusChangeType)
{
    // Nothing - don't touch Qt from focus callbacks
}

void JamulusPluginEditor::visibilityChanged()
{
    // Nothing - let timer handle it
}

void JamulusPluginEditor::parentHierarchyChanged()
{
    // Nothing - let timer handle it
}
