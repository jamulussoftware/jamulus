#include "PluginEditor.h"
#include "JucePluginAdapter.h"
#include "JamulusGui.h"
#include "../jamulus_wrapper.h"

namespace
{
juce::File getLearnMappingsFile()
{
    auto appData = juce::File::getSpecialLocation ( juce::File::userApplicationDataDirectory );
    auto settingsDir = appData.getChildFile ( "JamulusPlus" );
    if ( !settingsDir.exists() )
        settingsDir.createDirectory();
    return settingsDir.getChildFile ( "control_mappings.json" );
}

float getOscNormalizedValue ( const juce::OSCMessage& message, int argIndex )
{
    if ( message.size() <= argIndex )
        return -1.0f;

    const auto& arg = message[argIndex];
    if ( arg.isFloat32() )
        return juce::jlimit ( 0.0f, 1.0f, arg.getFloat32() );
    if ( arg.isInt32() )
    {
        const int value = arg.getInt32();
        return juce::jlimit ( 0.0f, 1.0f, value > 1 ? ( value / 127.0f ) : static_cast<float> ( value ) );
    }

    return -1.0f;
}
}

JamulusPluginEditor::JamulusPluginEditor ( JamulusAudioProcessor& p ) : AudioProcessorEditor ( &p ), audioProcessor ( p )
{
    auto client = audioProcessor.getProcessor().getClient();
    guiComponent     = std::make_unique<JamulusGuiComponent> ( client, &audioProcessor.getProcessor() );
    guiComponent->onConnectToServer = [this] ( const juce::String& addr ) { audioProcessor.getProcessor().connectToServer ( addr.toStdString() ); };
    guiComponent->onDisconnect      = [this]() { audioProcessor.getProcessor().disconnectFromServer(); };
    guiComponent->getIsConnected    = [this]() { return audioProcessor.getProcessor().isConnected(); };
    guiComponent->onSendPing        = [this]() { audioProcessor.getProcessor().sendPing(); };
    guiComponent->getNetworkStats   = [this] ( jamulus_network_stats_t& stats ) { return audioProcessor.getProcessor().getNetworkStats ( stats ); };
    guiComponent->getInputPeakLeft  = [this]() { return audioProcessor.getProcessor().getLastInputPeakLeft(); };
    guiComponent->getInputPeakRight = [this]() { return audioProcessor.getProcessor().getLastInputPeakRight(); };
    guiComponent->getOutputPeakLeft  = [this]() { return audioProcessor.getProcessor().getLastOutputPeakLeft(); };
    guiComponent->getOutputPeakRight = [this]() { return audioProcessor.getProcessor().getLastOutputPeakRight(); };
    guiComponent->getInputFader      = [this]() { return audioProcessor.getProcessor().getInputFader(); };
    guiComponent->onInputFaderChanged = [this] ( int value ) { audioProcessor.getProcessor().setInputFader ( value ); };
    guiComponent->onChannelGainChanged = [this] ( int channelId, float gain ) { audioProcessor.getProcessor().setChannelGain ( channelId, gain ); };
    guiComponent->onChannelReferenceGainChanged = [this] ( int channelId, float gain ) { audioProcessor.getProcessor().setChannelReferenceGain ( channelId, gain ); };
    guiComponent->onChannelPanChanged  = [this] ( int channelId, float pan ) { audioProcessor.getProcessor().setChannelPan ( channelId, pan ); };
    guiComponent->onChannelMuteChanged = [this] ( int channelId, bool muted ) { audioProcessor.getProcessor().setChannelMute ( channelId, muted ); };
    guiComponent->onChannelSoloChanged = [this] ( int channelId, bool solo ) { audioProcessor.getProcessor().setChannelSolo ( channelId, solo ); };
    guiComponent->onChannelOutputRouteChanged = [this] ( int channelId, int routeCode ) { audioProcessor.getProcessor().setChannelOutputRoute ( channelId, routeCode ); };
    guiComponent->getNumChannels       = [this]() { return audioProcessor.getProcessor().getNumChannels(); };
    guiComponent->getMyChannelId       = [this]() { return audioProcessor.getProcessor().getMyChannelId(); };
    guiComponent->getChannelInfo = [this] ( int index, jamulus_channel_info_t& info ) { return audioProcessor.getProcessor().getChannelInfo ( index, info ); };
    guiComponent->onChatSend = [this] ( const juce::String& message ) { audioProcessor.getProcessor().sendChatMessage ( message.toStdString() ); };
    guiComponent->getNextChatMessage = [this]() {
        std::string message;
        if ( audioProcessor.getProcessor().getNextChatMessage ( message ) )
            return juce::String ( message );
        return juce::String();
    };
    guiComponent->getMidiInputDeviceNames = []() {
        juce::StringArray names;
        for ( const auto& device : juce::MidiInput::getAvailableDevices() )
            names.add ( device.name );
        return names;
    };
    guiComponent->getSelectedMidiInputDevice = [this]() { return selectedMidiInputDevice; };
    guiComponent->setSelectedMidiInputDevice = [this] ( const juce::String& name ) {
        selectedMidiInputDevice = name.isNotEmpty() ? name : "None";
        updateMidiDevices();
    };
    guiComponent->getMidiOutputDeviceNames = []() {
        juce::StringArray names;
        for ( const auto& device : juce::MidiOutput::getAvailableDevices() )
            names.add ( device.name );
        return names;
    };
    guiComponent->getSelectedMidiOutputDevice = [this]() { return selectedMidiOutputDevice; };
    guiComponent->setSelectedMidiOutputDevice = [this] ( const juce::String& name ) {
        selectedMidiOutputDevice = name.isNotEmpty() ? name : "None";
        updateMidiDevices();
    };
    guiComponent->getProfileName       = [this]() { return juce::String ( audioProcessor.getProcessor().getProfileName() ); };
    guiComponent->setProfileName       = [this] ( const juce::String& v ) { audioProcessor.getProcessor().setProfileName ( v.toStdString() ); };
    guiComponent->getProfileInstrument = [this]() { return audioProcessor.getProcessor().getProfileInstrument(); };
    guiComponent->setProfileInstrument = [this] ( int v ) { audioProcessor.getProcessor().setProfileInstrument ( v ); };
    guiComponent->getProfileCountry    = [this]() { return audioProcessor.getProcessor().getProfileCountry(); };
    guiComponent->setProfileCountry    = [this] ( int v ) { audioProcessor.getProcessor().setProfileCountry ( v ); };
    guiComponent->getProfileCountryCode = [this]() { return juce::String ( audioProcessor.getProcessor().getProfileCountryCode() ); };
    guiComponent->getProfileCity       = [this]() { return juce::String ( audioProcessor.getProcessor().getProfileCity() ); };
    guiComponent->setProfileCity       = [this] ( const juce::String& v ) { audioProcessor.getProcessor().setProfileCity ( v.toStdString() ); };
    guiComponent->getProfileSkill      = [this]() { return audioProcessor.getProcessor().getProfileSkill(); };
    guiComponent->setProfileSkill      = [this] ( int v ) { audioProcessor.getProcessor().setProfileSkill ( v ); };
    guiComponent->setDirectoryCallbacks ( [this]() { audioProcessor.getProcessor().clearServerList(); },
                                          [this] ( int directoryType, const juce::String& customAddress ) {
                                              audioProcessor.getProcessor().requestServerList ( directoryType, customAddress.toStdString() );
                                          },
                                          [this]() { return audioProcessor.getProcessor().getNumServers(); },
                                          [this] ( int index, jamulus_server_info_t& info ) { return audioProcessor.getProcessor().getServerInfoAt ( index, info ); },
                                          [this] ( const juce::String& address ) {
                                              audioProcessor.getProcessor().requestServerClientList ( address.toStdString() );
                                          },
                                          [this]() { audioProcessor.getProcessor().pingServerList(); } );
    guiComponent->requestInitialServerList();
    guiComponent->onSidechainChanged = [this] ( bool enabled ) { audioProcessor.setSidechainEnabled ( enabled ); };
    guiComponent->setSidechainAvailable ( audioProcessor.isSidechainAvailable() );
    guiComponent->setTestToneButtonCallback ( [this] ( bool enabled ) { audioProcessor.getProcessor().setTestToneEnabled ( enabled ); } );
    guiComponent->onMonitorModeChanged = [this] ( bool enabled ) { audioProcessor.getProcessor().setMonitorMode ( enabled ); };
    guiComponent->onMainVolumeChanged  = [this] ( float vol ) { audioProcessor.getProcessor().setMainVolume ( vol ); };
    guiComponent->onDelayEnableChanged   = [this] ( bool enabled ) { audioProcessor.getProcessor().setDelayEnabled ( enabled ); };
    guiComponent->onDelayMixChanged      = [this] ( float mix ) { audioProcessor.getProcessor().setDelayMix ( mix ); };
    guiComponent->onDelayTimeChanged     = [this] ( float ms ) { audioProcessor.getProcessor().setDelayTime ( ms ); };
    guiComponent->onDelayFeedbackChanged = [this] ( float fb ) { audioProcessor.getProcessor().setDelayFeedback ( fb ); };
    guiComponent->onDelayPingPongChanged = [this] ( bool pp ) { audioProcessor.getProcessor().setDelayPingPong ( pp ); };
    guiComponent->onDelayHPChanged       = [this] ( float freq ) { audioProcessor.getProcessor().setDelayHighPass ( freq ); };
    guiComponent->onReverbEnableChanged  = [this] ( bool enabled ) { audioProcessor.getProcessor().setReverbEnabled ( enabled ); };
    guiComponent->onReverbMixChanged     = [this] ( float mix ) { audioProcessor.getProcessor().setReverbMix ( mix ); };
    guiComponent->onReverbDecayChanged   = [this] ( float decay ) { audioProcessor.getProcessor().setReverbDecay ( decay ); };
    guiComponent->onLimiterEnableChanged = [this] ( bool enabled ) { audioProcessor.getProcessor().setLimiterEnabled ( enabled ); };
    guiComponent->onLimiterThresholdChanged = [this] ( float db ) { audioProcessor.getProcessor().setLimiterThreshold ( db ); };
    guiComponent->onLimiterReleaseChanged   = [this] ( float ms ) { audioProcessor.getProcessor().setLimiterRelease ( ms ); };
    guiComponent->getRecordingSettings   = [this]() { return audioProcessor.getProcessor().getRecordingSettings(); };
    guiComponent->setRecordingSettings   = [this] ( const JamulusPluginProcessor::RecordingSettings& settings ) {
        audioProcessor.getProcessor().setRecordingSettings ( settings );
    };
    guiComponent->onStartRecording = [this]() { return audioProcessor.getProcessor().startRecording(); };
    guiComponent->onStopRecording  = [this]() { audioProcessor.getProcessor().stopRecording(); };
    guiComponent->onResetRequested       = [this]() { audioProcessor.getProcessor().resetAudioState(); };
    guiComponent->syncPersistentStateToCallbacks();
    addAndMakeVisible ( guiComponent.get() );
    guiComponent->addMouseListener ( this, true );

    setSize ( 900, 600 );
    setResizable ( true, true );

    loadLearnMappings();
    updateMidiDevices();
    refreshLearnableControls();
    connect ( oscListenPort );
    addListener ( this );
    startTimerHz ( 20 );
}

JamulusPluginEditor::~JamulusPluginEditor()
{
    stopTimer();
    disconnect();
}

void JamulusPluginEditor::paint ( juce::Graphics& g )
{
    g.fillAll ( juce::Colour ( 0xff323232 ) );
}

void JamulusPluginEditor::resized()
{
    if ( guiComponent )
    {
        guiComponent->setBounds ( getLocalBounds() );
    }
}

void JamulusPluginEditor::timerCallback()
{
    refreshLearnableControls();
    sendMidiFeedbackForBindings();

    JamulusPluginProcessor::MidiLearnEvent midiEvent;
    while ( audioProcessor.getProcessor().popNextMidiLearnEvent ( midiEvent ) )
    {
        handleMidiLearnEvent ( midiEvent );
        applyMidiBinding ( midiEvent );
    }
}

void JamulusPluginEditor::mouseDown ( const juce::MouseEvent& event )
{
    if ( !event.mods.isPopupMenu() )
        return;

    auto* slider = dynamic_cast<juce::Slider*> ( event.eventComponent );
    if ( slider == nullptr && event.eventComponent != nullptr )
        slider = event.eventComponent->findParentComponentOfClass<juce::Slider>();
    if ( slider == nullptr || slider->getComponentID().isEmpty() )
        return;

    showLearnMenuForSlider ( *slider, event.getScreenPosition() );
}

void JamulusPluginEditor::handleIncomingMidiMessage ( juce::MidiInput*, const juce::MidiMessage& message )
{
    if ( !message.isController() )
        return;

    const JamulusPluginProcessor::MidiLearnEvent event {
        message.getChannel(),
        message.getControllerNumber(),
        juce::jlimit ( 0.0f, 1.0f, message.getControllerValue() / 127.0f )
    };

    juce::MessageManager::callAsync ( [this, event]() {
        handleMidiLearnEvent ( event );
        applyMidiBinding ( event );
    } );
}

void JamulusPluginEditor::oscMessageReceived ( const juce::OSCMessage& message )
{
    handleOscLearnEvent ( message );
    applyOscBinding ( message );
}

void JamulusPluginEditor::refreshLearnableControls()
{
    learnableControls.clear();
    if ( guiComponent )
        collectLearnableControls ( *guiComponent );
}

void JamulusPluginEditor::collectLearnableControls ( juce::Component& component )
{
    if ( auto* slider = dynamic_cast<juce::Slider*> ( &component ) )
    {
        const auto controlId = slider->getComponentID();
        if ( controlId.isNotEmpty() )
        {
            learnableControls[controlId] = slider;
            observedLearnControls.insert ( slider );
        }
    }

    for ( int i = 0; i < component.getNumChildComponents(); ++i )
        collectLearnableControls ( *component.getChildComponent ( i ) );
}

void JamulusPluginEditor::showLearnMenuForSlider ( juce::Slider& slider, juce::Point<int> screenPosition )
{
    juce::PopupMenu menu;
    const auto controlId = slider.getComponentID();

    if ( controlId.endsWith ( "_pan" ) )
    {
        if ( auto* strip = dynamic_cast<ChannelStrip*> ( slider.getParentComponent() ) )
        {
            juce::PopupMenu::Item stereoItem ( "Stereo" );
            stereoItem.setTicked ( !strip->getMonoMode() );
            stereoItem.action = [strip]() { strip->setMonoMode ( false ); };
            menu.addItem ( std::move ( stereoItem ) );

            juce::PopupMenu::Item monoItem ( "Mono" );
            monoItem.setTicked ( strip->getMonoMode() );
            monoItem.action = [strip]() { strip->setMonoMode ( true ); };
            menu.addItem ( std::move ( monoItem ) );
            menu.addSeparator();
        }
    }

    menu.addItem ( "MIDI Learn", [this, controlId]() { armLearnForControl ( controlId, LearnMode::midi ); } );
    menu.addItem ( "OSC Learn", [this, controlId]() { armLearnForControl ( controlId, LearnMode::osc ); } );
    menu.addSeparator();
    juce::PopupMenu::Item forgetMidiItem ( "Forget MIDI" );
    forgetMidiItem.setEnabled ( midiBindings.count ( controlId ) > 0 );
    forgetMidiItem.action = [this, controlId]() { clearLearnForControl ( controlId, LearnMode::midi ); };
    menu.addItem ( std::move ( forgetMidiItem ) );

    juce::PopupMenu::Item forgetOscItem ( "Forget OSC" );
    forgetOscItem.setEnabled ( oscBindings.count ( controlId ) > 0 );
    forgetOscItem.action = [this, controlId]() { clearLearnForControl ( controlId, LearnMode::osc ); };
    menu.addItem ( std::move ( forgetOscItem ) );
    const juce::Rectangle<int> popupAnchor ( screenPosition.x, screenPosition.y, 1, 1 );
    menu.showMenuAsync ( juce::PopupMenu::Options().withTargetScreenArea ( popupAnchor ) );
}

void JamulusPluginEditor::armLearnForControl ( const juce::String& controlId, LearnMode mode )
{
    activeLearnControlId = controlId;
    activeLearnMode      = mode;
}

void JamulusPluginEditor::clearLearnForControl ( const juce::String& controlId, LearnMode mode )
{
    if ( mode == LearnMode::midi )
        midiBindings.erase ( controlId );
    else if ( mode == LearnMode::osc )
        oscBindings.erase ( controlId );

    saveLearnMappings();
}

void JamulusPluginEditor::handleMidiLearnEvent ( const JamulusPluginProcessor::MidiLearnEvent& event )
{
    if ( activeLearnMode != LearnMode::midi || activeLearnControlId.isEmpty() )
        return;

    midiBindings[activeLearnControlId] = { event.channel, event.controller };
    activeLearnControlId.clear();
    activeLearnMode = LearnMode::none;
    saveLearnMappings();
}

void JamulusPluginEditor::handleOscLearnEvent ( const juce::OSCMessage& message )
{
    if ( activeLearnMode != LearnMode::osc || activeLearnControlId.isEmpty() )
        return;

    if ( getOscNormalizedValue ( message, 0 ) < 0.0f )
        return;

    oscBindings[activeLearnControlId] = { message.getAddressPattern().toString(), 0 };
    activeLearnControlId.clear();
    activeLearnMode = LearnMode::none;
    saveLearnMappings();
}

void JamulusPluginEditor::applyMidiBinding ( const JamulusPluginProcessor::MidiLearnEvent& event )
{
    for ( const auto& [controlId, binding] : midiBindings )
    {
        if ( binding.channel == event.channel && binding.controller == event.controller )
            applyNormalizedValueToControl ( controlId, event.normalizedValue );
    }
}

void JamulusPluginEditor::applyOscBinding ( const juce::OSCMessage& message )
{
    for ( const auto& [controlId, binding] : oscBindings )
    {
        if ( binding.address != message.getAddressPattern().toString() )
            continue;

        const float normalized = getOscNormalizedValue ( message, binding.argIndex );
        if ( normalized >= 0.0f )
            applyNormalizedValueToControl ( controlId, normalized );
    }
}

void JamulusPluginEditor::applyNormalizedValueToControl ( const juce::String& controlId, float normalizedValue )
{
    const auto it = learnableControls.find ( controlId );
    if ( it == learnableControls.end() || it->second == nullptr )
        return;

    auto* slider = it->second.getComponent();
    const double targetValue = slider->proportionOfLengthToValue ( juce::jlimit ( 0.0f, 1.0f, normalizedValue ) );
    slider->setValue ( targetValue, juce::sendNotificationSync );
}

void JamulusPluginEditor::updateMidiDevices()
{
    midiInputDevice.reset();
    midiOutputDevice.reset();

    if ( selectedMidiInputDevice.isNotEmpty() && !selectedMidiInputDevice.equalsIgnoreCase ( "None" ) )
    {
        for ( const auto& device : juce::MidiInput::getAvailableDevices() )
        {
            if ( device.name == selectedMidiInputDevice )
            {
                midiInputDevice = juce::MidiInput::openDevice ( device.identifier, this );
                if ( midiInputDevice )
                    midiInputDevice->start();
                break;
            }
        }
    }

    if ( selectedMidiOutputDevice.isNotEmpty() && !selectedMidiOutputDevice.equalsIgnoreCase ( "None" ) )
    {
        for ( const auto& device : juce::MidiOutput::getAvailableDevices() )
        {
            if ( device.name == selectedMidiOutputDevice )
            {
                midiOutputDevice = juce::MidiOutput::openDevice ( device.identifier );
                break;
            }
        }
    }
}

void JamulusPluginEditor::sendMidiFeedbackForBindings()
{
    for ( const auto& [controlId, binding] : midiBindings )
    {
        const auto it = learnableControls.find ( controlId );
        if ( it == learnableControls.end() || it->second == nullptr )
            continue;

        auto* slider = it->second.getComponent();
        const float normalized = juce::jlimit ( 0.0f, 1.0f, static_cast<float> ( slider->valueToProportionOfLength ( slider->getValue() ) ) );
        const auto lastIt = lastControlNormalizedValues.find ( controlId );
        if ( lastIt != lastControlNormalizedValues.end() && std::abs ( lastIt->second - normalized ) < ( 1.0f / 127.0f ) )
            continue;

        lastControlNormalizedValues[controlId] = normalized;
        const int ccValue = juce::roundToInt ( normalized * 127.0f );
        const auto msg = juce::MidiMessage::controllerEvent ( binding.channel, binding.controller, ccValue );
        if ( midiOutputDevice )
            midiOutputDevice->sendMessageNow ( msg );
        audioProcessor.getProcessor().enqueueMidiOutputCc ( binding.channel, binding.controller, ccValue );
    }
}

void JamulusPluginEditor::saveLearnMappings() const
{
    juce::DynamicObject::Ptr root = new juce::DynamicObject();
    root->setProperty ( "oscPort", oscListenPort );
    root->setProperty ( "selectedMidiInputDevice", selectedMidiInputDevice );
    root->setProperty ( "selectedMidiOutputDevice", selectedMidiOutputDevice );

    juce::Array<juce::var> midiArray;
    for ( const auto& [controlId, binding] : midiBindings )
    {
        juce::DynamicObject::Ptr item = new juce::DynamicObject();
        item->setProperty ( "id", controlId );
        item->setProperty ( "channel", binding.channel );
        item->setProperty ( "controller", binding.controller );
        midiArray.add ( juce::var ( item.get() ) );
    }
    root->setProperty ( "midi", midiArray );

    juce::Array<juce::var> oscArray;
    for ( const auto& [controlId, binding] : oscBindings )
    {
        juce::DynamicObject::Ptr item = new juce::DynamicObject();
        item->setProperty ( "id", controlId );
        item->setProperty ( "address", binding.address );
        item->setProperty ( "argIndex", binding.argIndex );
        oscArray.add ( juce::var ( item.get() ) );
    }
    root->setProperty ( "osc", oscArray );

    getLearnMappingsFile().replaceWithText ( juce::JSON::toString ( juce::var ( root.get() ) ) );
}

void JamulusPluginEditor::loadLearnMappings()
{
    auto file = getLearnMappingsFile();
    if ( !file.existsAsFile() )
        return;

    auto json = juce::JSON::parse ( file );
    if ( !json.isObject() )
        return;

    auto* root = json.getDynamicObject();
    if ( root == nullptr )
        return;

    if ( root->hasProperty ( "oscPort" ) )
        oscListenPort = static_cast<int> ( root->getProperty ( "oscPort" ) );
    if ( root->hasProperty ( "selectedMidiInputDevice" ) )
        selectedMidiInputDevice = root->getProperty ( "selectedMidiInputDevice" ).toString();
    if ( root->hasProperty ( "selectedMidiOutputDevice" ) )
        selectedMidiOutputDevice = root->getProperty ( "selectedMidiOutputDevice" ).toString();

    midiBindings.clear();
    if ( auto* midiArray = root->getProperty ( "midi" ).getArray() )
    {
        for ( const auto& item : *midiArray )
        {
            if ( auto* object = item.getDynamicObject() )
            {
                midiBindings[object->getProperty ( "id" ).toString()] =
                    { static_cast<int> ( object->getProperty ( "channel" ) ), static_cast<int> ( object->getProperty ( "controller" ) ) };
            }
        }
    }

    oscBindings.clear();
    if ( auto* oscArray = root->getProperty ( "osc" ).getArray() )
    {
        for ( const auto& item : *oscArray )
        {
            if ( auto* object = item.getDynamicObject() )
            {
                OscBinding binding;
                binding.address = object->getProperty ( "address" ).toString();
                binding.argIndex = static_cast<int> ( object->getProperty ( "argIndex" ) );
                oscBindings[object->getProperty ( "id" ).toString()] = binding;
            }
        }
    }
}
