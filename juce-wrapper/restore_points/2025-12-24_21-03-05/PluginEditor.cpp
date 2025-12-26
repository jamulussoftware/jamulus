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
    if (qtGui)
    {
        jamulus_gui_hide(qtGui);
        // Don't destroy - keep the singleton alive
    }
}

void JamulusPluginEditor::paint(juce::Graphics& g)
{
    if (!guiEmbedded)
    {
        // Show placeholder if Qt GUI isn't available
        g.fillAll(juce::Colour(0xff323232));
        g.setColour(juce::Colours::white);
        g.setFont(16.0f);
        g.drawFittedText(
            "Jamulus VST3 Plugin\n\n"
            "Initializing GUI...\n"
            "If you see this message, the Qt GUI may not have loaded correctly.",
            getLocalBounds().reduced(20),
            juce::Justification::centred,
            10);
    }
    // If embedded, the Qt window will paint itself
}

void JamulusPluginEditor::resized()
{
    if (qtGui && guiEmbedded)
    {
#ifdef _WIN32
        // Get our native window handle
        if (auto* peer = getPeer())
        {
            void* parentHwnd = peer->getNativeHandle();
            
            // Set the Qt window as a child of our JUCE window
            jamulus_gui_set_parent(qtGui, parentHwnd);
            
            // Resize to fill our bounds
            jamulus_gui_resize(qtGui, getWidth(), getHeight());
            
            // Show it
            jamulus_gui_show(qtGui);
        }
#endif
    }
}
