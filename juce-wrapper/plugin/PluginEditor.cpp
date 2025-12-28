#include "PluginEditor.h"
#include "JucePluginAdapter.h"
#include "JamulusGui.h"
#include "../jamulus_wrapper.h"

JamulusPluginEditor::JamulusPluginEditor ( JamulusAudioProcessor& p ) : AudioProcessorEditor ( &p ), audioProcessor ( p )
{
    // Get the Jamulus client from the processor
    auto client = audioProcessor.getProcessor().getClient();

    if ( client )
    {
        guiComponent                     = std::make_unique<JamulusGuiComponent> ( client );
        guiComponent->onSidechainChanged = [this] ( bool enabled ) { audioProcessor.setSidechainEnabled ( enabled ); };
        guiComponent->setSidechainAvailable ( audioProcessor.isSidechainAvailable() );
        guiComponent->setTestToneButtonCallback ( [this] ( bool enabled ) { audioProcessor.getProcessor().setTestToneEnabled ( enabled ); } );
        guiComponent->onMonitorModeChanged = [this] ( bool enabled ) { audioProcessor.getProcessor().setMonitorMode ( enabled ); };
        guiComponent->onMainVolumeChanged  = [this] ( float vol ) { audioProcessor.getProcessor().setMainVolume ( vol ); };
        addAndMakeVisible ( guiComponent.get() );
    }

    // Set default size
    setSize ( 900, 600 );
    setResizable ( true, true );
}

JamulusPluginEditor::~JamulusPluginEditor() { guiComponent.reset(); }

void JamulusPluginEditor::paint ( juce::Graphics& g )
{
    if ( !guiComponent )
    {
        g.fillAll ( juce::Colour ( 0xff323232 ) );
        g.setColour ( juce::Colours::white );
        g.setFont ( 16.0f );
        g.drawFittedText ( "Jamulus VST3 Plugin\n\n"
                           "Error: Could not create GUI",
                           getLocalBounds().reduced ( 20 ),
                           juce::Justification::centred,
                           10 );
    }
}

void JamulusPluginEditor::resized()
{
    if ( guiComponent )
    {
        guiComponent->setBounds ( getLocalBounds() );
    }
}
