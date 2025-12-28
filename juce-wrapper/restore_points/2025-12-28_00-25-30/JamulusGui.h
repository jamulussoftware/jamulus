#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include "../jamulus_wrapper.h"
#include "SettingsComponent.h"
#include "ConnectComponent.h"
#include "ChatComponent.h"
#include "FXPanelComponent.h"
#include "AudioDelay.h"

// Settings persistence helper
static juce::File getSettingsFile()
{
    auto appData     = juce::File::getSpecialLocation ( juce::File::userApplicationDataDirectory );
    auto settingsDir = appData.getChildFile ( "JamulusVST" );
    if ( !settingsDir.exists() )
        settingsDir.createDirectory();
    return settingsDir.getChildFile ( "settings.json" );
}

// Helper to get asset paths
static juce::File getAssetDir()
{
    // Attempt to find the res directory relative to the source
    // In a real VST we might bundle these, but for now we'll use the dev path
    return juce::File ( "e:/Web stuff/Jamulus VST Wrapper/jamulus/src/res" );
}

static juce::Image getInstrumentImage ( int instrument )
{
    static const char* instrumentFiles[] = { "none.png",         "drumset.png",
                                             "djembe.png",       "eguitar.png",
                                             "aguitar.png",      "bassguitar.png",
                                             "keyboard.png",     "synthesizer.png",
                                             "grandpiano.png",   "accordeon.png",
                                             "vocal.png",        "microphone.png",
                                             "harmonica.png",    "trumpet.png",
                                             "trombone.png",     "frenchhorn.png",
                                             "tuba.png",         "saxophone.png",
                                             "clarinet.png",     "flute.png",
                                             "violin.png",       "cello.png",
                                             "doublebass.png",   "recorder.png",
                                             "streamer.png",     "listener.png",
                                             "guitarvocal.png",  "keyboardvocal.png",
                                             "bodhran.png",      "bassoon.png",
                                             "oboe.png",         "harp.png",
                                             "viola.png",        "congas.png",
                                             "bongo.png",        "vocalbass.png",
                                             "vocaltenor.png",   "vocalalto.png",
                                             "vocalsoprano.png", "banjo.png",
                                             "mandolin.png",     "ukulele.png",
                                             "bassukulele.png",  "vocalbaritone.png",
                                             "vocallead.png",    "mountaindulcimer.png",
                                             "scratching.png",   "rapping.png",
                                             "vibraphone.png",   "conductor.png" };

    juce::File file;
    if ( instrument >= 0 && instrument < (int) ( sizeof ( instrumentFiles ) / sizeof ( instrumentFiles[0] ) ) )
    {
        file = getAssetDir().getChildFile ( "instruments" ).getChildFile ( instrumentFiles[instrument] );
    }
    else
    {
        file = getAssetDir().getChildFile ( "instruments" ).getChildFile ( "none.png" );
    }

    return juce::ImageCache::getFromFile ( file );
}

static juce::Image getFlagImage ( const char* countryCode )
{
    juce::String code ( countryCode );
    DBG ( "getFlagImage called with code: '" << code << "'" );

    if ( code.isEmpty() || code == "none" )
    {
        DBG ( "  -> Loading flagnone.png" );
        return juce::ImageCache::getFromFile ( getAssetDir().getChildFile ( "flags" ).getChildFile ( "flagnone.png" ) );
    }

    // Some common fixes
    if ( code.equalsIgnoreCase ( "uk" ) )
        code = "gb";

    auto file = getAssetDir().getChildFile ( "flags" ).getChildFile ( code.toLowerCase() + ".png" );
    DBG ( "  -> Trying to load: " << file.getFullPathName() << " exists: " << ( file.existsAsFile() ? "yes" : "no" ) );

    if ( !file.existsAsFile() )
    {
        DBG ( "  -> File not found, loading flagnone.png" );
        return juce::ImageCache::getFromFile ( getAssetDir().getChildFile ( "flags" ).getChildFile ( "flagnone.png" ) );
    }

    return juce::ImageCache::getFromFile ( file );
}

//==============================================================================
// Level Meter Component
//==============================================================================
class LevelMeter : public juce::Component, private juce::Timer
{
public:
    LevelMeter() { startTimerHz ( 30 ); }

    void setLevel ( float newLevel )
    {
        // Smooth the level for nice visual feedback
        targetLevel = juce::jlimit ( 0.0f, 1.0f, newLevel );
    }

    void paint ( juce::Graphics& g ) override
    {
        auto bounds = getLocalBounds().toFloat();

        // Background
        g.setColour ( juce::Colour ( 0xff2a2a2a ) );
        g.fillRoundedRectangle ( bounds, 2.0f );

        // Level bar
        float                  levelHeight = bounds.getHeight() * currentLevel;
        juce::Rectangle<float> levelRect ( bounds.getX() + 1, bounds.getBottom() - levelHeight - 1, bounds.getWidth() - 2, levelHeight );

        // Color gradient based on level
        juce::Colour levelColor;
        if ( currentLevel > 0.9f )
            levelColor = juce::Colours::red;
        else if ( currentLevel > 0.7f )
            levelColor = juce::Colours::yellow;
        else
            levelColor = juce::Colour ( 0xff00cc66 ); // Green

        g.setColour ( levelColor );
        g.fillRoundedRectangle ( levelRect, 1.0f );
    }

private:
    void timerCallback() override
    {
        // Smooth the meter movement
        const float smoothing = 0.3f;
        float       newLevel  = currentLevel + ( targetLevel - currentLevel ) * smoothing;

        // Decay when signal drops
        if ( targetLevel < currentLevel )
            newLevel = currentLevel * 0.9f;

        if ( std::abs ( newLevel - currentLevel ) > 0.001f )
        {
            currentLevel = newLevel;
            repaint();
        }
    }

    float currentLevel = 0.0f;
    float targetLevel  = 0.0f;
};

//==============================================================================
// Channel Strip Component (for mixer)
//==============================================================================
class ChannelStrip : public juce::Component
{
public:
    ChannelStrip()
    {
        addAndMakeVisible ( nameLabel );
        nameLabel.setFont ( juce::Font ( 13.0f, juce::Font::bold ) );
        nameLabel.setJustificationType ( juce::Justification::centred );
        nameLabel.setColour ( juce::Label::textColourId, juce::Colours::black );
        nameLabel.setMinimumHorizontalScale ( 0.8f );

        addAndMakeVisible ( faderSlider );
        faderSlider.setSliderStyle ( juce::Slider::LinearVertical );
        faderSlider.setRange ( 0.0, 1.0 );
        faderSlider.setValue ( 0.75 );
        faderSlider.setScrollWheelEnabled ( true );
        faderSlider.setDoubleClickReturnValue ( true, 0.75 );
        faderSlider.setTextBoxStyle ( juce::Slider::NoTextBox, true, 0, 0 );
        // Style fader like original (blue handle)
        faderSlider.setColour ( juce::Slider::thumbColourId, juce::Colour ( 0xff2b93d4 ) );
        faderSlider.setColour ( juce::Slider::trackColourId, juce::Colours::transparentBlack );

        faderSlider.onValueChange = [this]() {
            if ( onGainChanged )
                onGainChanged ( channelId, static_cast<float> ( faderSlider.getValue() ) );
        };

        addAndMakeVisible ( panSlider );
        panSlider.setSliderStyle ( juce::Slider::RotaryHorizontalVerticalDrag );
        panSlider.setRange ( -1.0, 1.0 );
        panSlider.setValue ( 0.0 );
        panSlider.setScrollWheelEnabled ( true );
        panSlider.setDoubleClickReturnValue ( true, 0.0 );
        panSlider.setTextBoxStyle ( juce::Slider::NoTextBox, true, 0, 0 );
        panSlider.onValueChange = [this]() {
            if ( onPanChanged )
                onPanChanged ( channelId, static_cast<float> ( panSlider.getValue() ) );
        };

        addAndMakeVisible ( muteButton );
        muteButton.setButtonText ( "MUTE" );
        muteButton.setClickingTogglesState ( true );
        muteButton.onClick = [this]() {
            updateMuteButtonColor();
            if ( onMuteChanged )
                onMuteChanged ( channelId, muteButton.getToggleState() );
        };

        addAndMakeVisible ( skillLabel );
        skillLabel.setFont ( juce::Font ( 9.0f ) );
        skillLabel.setJustificationType ( juce::Justification::centred );
        skillLabel.setColour ( juce::Label::textColourId, juce::Colours::grey.darker() );

        addAndMakeVisible ( levelMeter );
    }

    void updateMuteButtonColor()
    {
        if ( muteButton.getToggleState() )
        {
            // Bright glowing red when muted
            muteButton.setColour ( juce::TextButton::buttonColourId, juce::Colour ( 0xffee2222 ) );
            muteButton.setColour ( juce::TextButton::buttonOnColourId, juce::Colour ( 0xffee2222 ) );
            muteButton.setColour ( juce::TextButton::textColourOnId, juce::Colours::white );
            muteButton.setColour ( juce::TextButton::textColourOffId, juce::Colours::white );
        }
        else
        {
            // Reset to default colors
            muteButton.removeColour ( juce::TextButton::buttonColourId );
            muteButton.removeColour ( juce::TextButton::buttonOnColourId );
            muteButton.removeColour ( juce::TextButton::textColourOnId );
            muteButton.removeColour ( juce::TextButton::textColourOffId );
        }
        muteButton.repaint();
    }

    void setChannelInfo ( int id, const juce::String& name, int instrument, const char* countryCode, int skill, bool isMe )
    {
        channelId = id;
        nameLabel.setText ( name, juce::dontSendNotification );
        instrumentIcon   = getInstrumentImage ( instrument );
        flagIcon         = getFlagImage ( countryCode );
        this->skillLevel = skill;
        this->isMe       = isMe;

        // Skill level text
        juce::String skillText;
        switch ( skill )
        {
        case 1:
            skillText = "Beginner";
            break;
        case 2:
            skillText = "Intermediate";
            break;
        case 3:
            skillText = "Expert";
            break;
        default:
            skillText = "";
            break;
        }
        skillLabel.setText ( skillText, juce::dontSendNotification );

        repaint();
    }

    void setLevel ( float level ) { levelMeter.setLevel ( level ); }

    float getGain() const { return static_cast<float> ( faderSlider.getValue() ); }
    int   getChannelId() const { return channelId; }
    bool  getIsMe() const { return isMe; }

    void paint ( juce::Graphics& g ) override
    {
        auto bounds = getLocalBounds();

        // Background for the tag part (the "fader tag")
        auto tagArea = bounds.removeFromTop ( 80 ).reduced ( 2 );

        // Skill level colors from Jamulus source
        juce::Colour skillColour = juce::Colours::white;
        switch ( skillLevel )
        {
        case 1:
            skillColour = juce::Colour ( 255, 255, 200 );
            break; // Beginner: Pale Yellow
        case 2:
            skillColour = juce::Colour ( 225, 255, 225 );
            break; // Intermediate: Pale Green
        case 3:
            skillColour = juce::Colour ( 255, 225, 225 );
            break; // Professional: Pale Pink
        default:
            skillColour = juce::Colours::white;
            break;
        }

        // Draw the background
        g.setColour ( skillColour );
        g.fillRoundedRectangle ( tagArea.toFloat(), 4.0f );

        if ( isMe )
        {
            // Stronger border for "Me"
            g.setColour ( juce::Colours::orange.withAlpha ( 0.6f ) );
            g.drawRoundedRectangle ( tagArea.toFloat(), 4.0f, 2.0f );

            // Subtle overlay tint for Me fader area too?
            g.setColour ( juce::Colours::orange.withAlpha ( 0.05f ) );
            g.fillRoundedRectangle ( bounds.reduced ( 2 ).toFloat(), 4.0f );
        }
        else
        {
            g.setColour ( juce::Colours::grey );
            g.drawRoundedRectangle ( tagArea.toFloat(), 4.0f, 1.0f );
        }

        // Reset opacity and resampling for icons
        g.setOpacity ( 1.0f );
        g.setImageResamplingQuality ( juce::Graphics::highResamplingQuality );

        // Icons row at top - instrument on left, flag on right (side by side)
        int iconRowY     = tagArea.getY() + 4;
        int iconSize     = 24;
        int iconRowWidth = tagArea.getWidth() - 8;
        int centerX      = tagArea.getX() + tagArea.getWidth() / 2;

        // Instrument Icon (left side of center)
        if ( instrumentIcon.isValid() )
        {
            g.drawImageWithin ( instrumentIcon, centerX - iconSize - 2, iconRowY, iconSize, iconSize, juce::RectanglePlacement::centred );
        }

        // Flag Icon (right side of center)
        if ( flagIcon.isValid() )
        {
            g.drawImageWithin ( flagIcon, centerX + 2, iconRowY + 3, 22, 16, juce::RectanglePlacement::centred );
        }
    }

    void resized() override
    {
        auto bounds = getLocalBounds().reduced ( 3 );

        // Tag area (Name/Icon box) - fixed height 80
        auto tagArea = bounds.removeFromTop ( 80 ).reduced ( 2 );

        // Icons take up the top 30px (drawn in paint)
        tagArea.removeFromTop ( 30 );

        // Name label below icons
        nameLabel.setBounds ( tagArea.removeFromTop ( 28 ).reduced ( 2, 0 ) );

        // Skill label at the very bottom of tag area
        skillLabel.setBounds ( tagArea.reduced ( 2, 0 ) );

        bounds.removeFromTop ( 6 );

        // Mute at bottom only (solo removed)
        auto bottomArea = bounds.removeFromBottom ( 35 );
        muteButton.setBounds ( bottomArea.reduced ( 10, 2 ) );

        bounds.removeFromBottom ( 6 );

        // Pan knob
        panSlider.setBounds ( bounds.removeFromBottom ( 34 ).reduced ( 15, 0 ) );

        bounds.removeFromTop ( 4 );

        // Fader and meter (main middle area)
        auto faderArea = bounds.reduced ( 2, 0 );
        levelMeter.setBounds ( faderArea.removeFromRight ( 14 ).reduced ( 2, 2 ) );
        faderSlider.setBounds ( faderArea );
    }

    std::function<void ( int, float )> onGainChanged;
    std::function<void ( int, float )> onPanChanged;
    std::function<void ( int, bool )>  onMuteChanged;

private:
    int              channelId  = -1;
    bool             isMe       = false;
    int              skillLevel = 0;
    juce::Label      nameLabel;
    juce::Label      skillLabel;
    juce::Image      instrumentIcon;
    juce::Image      flagIcon;
    juce::Slider     faderSlider;
    juce::Slider     panSlider;
    juce::TextButton muteButton;
    LevelMeter       levelMeter;
};

//==============================================================================
// Main Jamulus GUI Component
//==============================================================================
class JamulusGuiComponent : public juce::Component, private juce::Timer
{
public:
    // Allow editor to set test tone button callback
    void setTestToneButtonCallback ( std::function<void ( bool )> cb )
    {
        testToneButton.onClick = [this, cb]() {
            testToneEnabled = testToneButton.getToggleState();
            if ( cb )
                cb ( testToneEnabled );
        };
    }
    JamulusGuiComponent ( jamulus_client_t client ) : jamulusClient ( client )
    {
        // Title
        addAndMakeVisible ( titleLabel );
        titleLabel.setText ( "Jamulus VST3", juce::dontSendNotification );
        titleLabel.setFont ( juce::Font ( 24.0f, juce::Font::bold ) );
        titleLabel.setColour ( juce::Label::textColourId, juce::Colours::white );

        // === Top Toolbar Buttons ===
        addAndMakeVisible ( connectButton );
        connectButton.setButtonText ( "Connect" );
        connectButton.onClick = [this]() { showConnectDialog(); };

        addAndMakeVisible ( settingsButton );
        settingsButton.setButtonText ( "Settings" );
        settingsButton.onClick = [this]() { showSettingsDialog(); };

        addAndMakeVisible ( chatButton );
        chatButton.setButtonText ( "Chat" );
        chatButton.setClickingTogglesState ( true );
        chatButton.onClick = [this]() { toggleChatPanel(); };

        // Test Tone Button
        addAndMakeVisible ( testToneButton );
        testToneButton.setButtonText ( "Test Tone" );
        testToneButton.setClickingTogglesState ( true );
        testToneButton.onClick = [this]() { testToneEnabled = testToneButton.getToggleState(); };

        // Status
        addAndMakeVisible ( statusLabel );
        statusLabel.setText ( "Disconnected", juce::dontSendNotification );
        statusLabel.setColour ( juce::Label::textColourId, juce::Colours::grey );

        // Delay & Jitter
        addAndMakeVisible ( delayLight );
        delayLight.setText ( "", juce::dontSendNotification );

        addAndMakeVisible ( delayLabel );
        delayLabel.setText ( "Delay: --", juce::dontSendNotification );
        delayLabel.setColour ( juce::Label::textColourId, juce::Colours::white );

        addAndMakeVisible ( jitterLight );
        jitterLight.setText ( "", juce::dontSendNotification );

        addAndMakeVisible ( jitterLabel );
        jitterLabel.setText ( "Jitter", juce::dontSendNotification );
        jitterLabel.setColour ( juce::Label::textColourId, juce::Colours::white );

        // Ping
        addAndMakeVisible ( pingLabel );
        pingLabel.setText ( "Ping: --", juce::dontSendNotification );
        pingLabel.setColour ( juce::Label::textColourId, juce::Colours::white );

        // Input level meters
        addAndMakeVisible ( inputLevelLabel );
        inputLevelLabel.setText ( "Input", juce::dontSendNotification );
        inputLevelLabel.setColour ( juce::Label::textColourId, juce::Colours::white );
        inputLevelLabel.setJustificationType ( juce::Justification::centred );

        addAndMakeVisible ( inputMeterL );
        addAndMakeVisible ( inputMeterR );

        // Input fader
        addAndMakeVisible ( inputFaderLabel );
        inputFaderLabel.setText ( "Level", juce::dontSendNotification );
        inputFaderLabel.setColour ( juce::Label::textColourId, juce::Colours::white );

        addAndMakeVisible ( inputFaderSlider );
        inputFaderSlider.setSliderStyle ( juce::Slider::LinearHorizontal );
        inputFaderSlider.setRange ( 0, 100 );
        inputFaderSlider.setValue ( 50 );
        inputFaderSlider.setTextBoxStyle ( juce::Slider::NoTextBox, true, 0, 0 );
        inputFaderSlider.onValueChange = [this]() {
            jamulus_client_set_input_fader ( jamulusClient, static_cast<int> ( inputFaderSlider.getValue() ) );
        };

        // FX Button (opens FX panel with reverb and delay)
        addAndMakeVisible ( fxButton );
        fxButton.setButtonText ( "FX" );
        fxButton.setClickingTogglesState ( true );
        fxButton.onClick = [this]() { toggleFXPanel(); };

        // Sidechain button
        addAndMakeVisible ( sidechainButton );
        sidechainButton.setButtonText ( "SC" );
        sidechainButton.setClickingTogglesState ( true );
        sidechainButton.setTooltip ( "Use Sidechain Input" );
        sidechainButton.onClick = [this]() {
            if ( onSidechainChanged )
                onSidechainChanged ( sidechainButton.getToggleState() );
            updateSidechainButtonColor();
        };

        // Initialize delay effect
        audioDelay.init ( 48000, 1000 ); // 48kHz, max 1000ms delay

        // Monitor Mode button (Direct Thru vs Server Return)
        addAndMakeVisible ( monitorModeButton );
        monitorModeButton.setButtonText ( "MON" );
        monitorModeButton.setClickingTogglesState ( true );
        monitorModeButton.setTooltip ( "Monitor Mode: OFF = Direct Thru (zero-latency), ON = Server Return (with network delay)" );
        monitorModeButton.onClick = [this]() {
            monitorServerReturn = monitorModeButton.getToggleState();
            updateMonitorButtonColor();
            if ( onMonitorModeChanged )
                onMonitorModeChanged ( monitorServerReturn );
        };
        updateMonitorButtonColor();

        // Mixer viewport
        addAndMakeVisible ( mixerViewport );
        mixerViewport.setViewedComponent ( &mixerContainer, false );

        // Disconnect button (visible when connected)
        addAndMakeVisible ( disconnectButton );
        disconnectButton.setButtonText ( "Disconnect" );
        disconnectButton.setVisible ( false );
        disconnectButton.onClick = [this]() {
            if ( jamulusClient )
                jamulus_client_disconnect ( jamulusClient );
        };

        // Load initial settings
        if ( jamulusClient )
        {
            inputFaderSlider.setValue ( jamulus_client_get_input_fader ( jamulusClient ), juce::dontSendNotification );
        }

        // Create initial Me strip immediately so it's always visible
        updateChannelStrips();

        // Load saved settings from file
        loadSettings();

        startTimerHz ( 30 );
    }

    ~JamulusGuiComponent() override
    {
        saveSettings();
        stopTimer();
    }

    void paint ( juce::Graphics& g ) override
    {
        g.fillAll ( juce::Colour ( 0xff323232 ) );

        // Header background
        g.setColour ( juce::Colour ( 0xff252525 ) );
        g.fillRect ( 0, 0, getWidth(), 50 );

        // Mixer area background (starts after header + controls)
        auto mixerBounds = getLocalBounds().removeFromBottom ( getHeight() - 270 ); // 50 (header) + 220 (controls)
        g.setColour ( juce::Colour ( 0xff2a2a2a ) );
        g.fillRect ( mixerBounds );

        // Vertical Separator Line
        g.setColour ( juce::Colour ( 0xff505050 ) );
        g.drawVerticalLine ( 200, 50.0f, 270.0f ); // Draw line separating local controls
    }

    void resized() override
    {
        auto bounds = getLocalBounds();

        // Chat panel on right side (if visible) - limit height to header row only
        const int chatPanelWidth = 250;
        if ( chatPanelVisible && chatPanel )
        {
            auto chatBounds = bounds.removeFromRight ( chatPanelWidth );
            // Limit chat panel to start below header (first 50px)
            chatBounds.removeFromTop ( 50 );
            chatPanel->setBounds ( chatBounds );
        }

        // Header
        auto header = bounds.removeFromTop ( 50 );
        titleLabel.setBounds ( header.removeFromLeft ( 150 ).reduced ( 10 ) );

        // Toolbar buttons
        auto toolbar = header;
        connectButton.setBounds ( toolbar.removeFromLeft ( 90 ).reduced ( 5 ) );
        settingsButton.setBounds ( toolbar.removeFromLeft ( 90 ).reduced ( 5 ) );
        chatButton.setBounds ( toolbar.removeFromLeft ( 70 ).reduced ( 5 ) );
        testToneButton.setBounds ( toolbar.removeFromLeft ( 90 ).reduced ( 5 ) );

        // Status area on right
        disconnectButton.setBounds ( toolbar.removeFromRight ( 90 ).reduced ( 5 ) );
        statusLabel.setBounds ( toolbar.reduced ( 5 ) );

        // Controls area - reduced height since we removed the big input section
        auto controlsArea = bounds.removeFromTop ( 80 );

        // Left side - FX button, MON button, and network stats
        auto leftControls = controlsArea.removeFromLeft ( 200 ).reduced ( 10 );

        // Buttons row
        auto buttonsRow = leftControls.removeFromTop ( 30 );
        fxButton.setBounds ( buttonsRow.removeFromLeft ( 50 ).reduced ( 2 ) );
        // MON button removed - hiding it
        monitorModeButton.setVisible ( false );

        leftControls.removeFromTop ( 5 );

        // Network Stats - now visible!
        int statHeight = 18;

        // Ping
        pingLabel.setBounds ( leftControls.removeFromTop ( statHeight ) );

        // Delay
        auto delayRow = leftControls.removeFromTop ( statHeight );
        delayLight.setBounds ( delayRow.removeFromRight ( 15 ).reduced ( 3 ) );
        delayLabel.setBounds ( delayRow );

        // Jitter
        auto jitterRow = leftControls.removeFromTop ( statHeight );
        jitterLight.setBounds ( jitterRow.removeFromRight ( 15 ).reduced ( 3 ) );
        jitterLabel.setBounds ( jitterRow );

        // Hide the removed elements (they still exist but won't be shown)
        inputLevelLabel.setVisible ( false );
        inputMeterL.setVisible ( false );
        inputMeterR.setVisible ( false );
        inputFaderLabel.setVisible ( false );
        inputFaderSlider.setVisible ( false );
        sidechainButton.setVisible ( false );

        // FX Panel (appears below the FX button when toggled)
        if ( fxPanelVisible && fxPanel )
        {
            fxPanel->setBounds ( 10, 100, 320, 530 ); // Adjusted Y position
        }

        // Right side - Info/Status
        auto rightControls = controlsArea.removeFromRight ( 200 );
        statusLabel.setBounds ( rightControls.removeFromTop ( 30 ).reduced ( 10 ) );

        // Mixer area
        bounds.removeFromTop ( 10 );
        mixerViewport.setBounds ( bounds );

        // Update mixer container width based on number of channels
        updateMixerLayout();
    }

private:
    juce::TextButton testToneButton;
    bool             testToneEnabled = false;
    void             timerCallback() override
    {
        if ( !jamulusClient )
            return;

        // Update input meters
        double levelL = jamulus_client_get_level_left ( jamulusClient );
        double levelR = jamulus_client_get_level_right ( jamulusClient );

        // GetLevelForMeterdBLeft/Right returns 0-8 (NUM_STEPS_LED_BAR), not actual dB
        // Simply normalize to 0-1 range
        float linearL = static_cast<float> ( levelL / 8.0 ); // 8 = NUM_STEPS_LED_BAR
        float linearR = static_cast<float> ( levelR / 8.0 );
        linearL       = juce::jlimit ( 0.0f, 1.0f, linearL );
        linearR       = juce::jlimit ( 0.0f, 1.0f, linearR );

        inputMeterL.setLevel ( linearL );
        inputMeterR.setLevel ( linearR );

        // Update connection status
        bool connected = jamulus_client_is_connected ( jamulusClient );

        if ( connected != wasConnected )
        {
            wasConnected = connected;
            statusLabel.setText ( connected ? "Connected" : "Disconnected", juce::dontSendNotification );
            statusLabel.setColour ( juce::Label::textColourId, connected ? juce::Colour ( 0xff00cc66 ) : juce::Colours::grey );
            connectButton.setButtonText ( connected ? "Connect..." : "Connect" );
            disconnectButton.setVisible ( connected );
        }

        // Update Network Stats
        if ( connected )
        {
            // Send ping periodically (every ~1 second at 30Hz)
            if ( ++pingCounter >= 30 )
            {
                jamulus_client_send_ping ( jamulusClient );
                pingCounter = 0;
            }

            jamulus_network_stats_t stats;
            jamulus_client_get_network_stats ( jamulusClient, &stats );

            if ( stats.ping_time_ms >= 0 )
            {
                pingLabel.setText ( "Ping: " + juce::String ( stats.ping_time_ms ) + " ms", juce::dontSendNotification );
                pingLabel.setColour ( juce::Label::textColourId, juce::Colours::white );

                // Delay
                delayLabel.setText ( "Delay: " + juce::String ( stats.total_delay_ms ) + " ms", juce::dontSendNotification );

                juce::Colour delayColor;
                if ( stats.total_delay_ms <= 43 )
                    delayColor = juce::Colour ( 0xff00cc66 ); // Green
                else if ( stats.total_delay_ms <= 68 )
                    delayColor = juce::Colours::yellow;
                else
                    delayColor = juce::Colours::red;

                delayLight.setColour ( juce::Label::backgroundColourId, delayColor );

                // Jitter
                juce::Colour jitterColor;
                if ( stats.jitter_buffer_status == 0 )
                    jitterColor = juce::Colour ( 0xff00cc66 ); // Green
                else if ( stats.jitter_buffer_status == 1 )
                    jitterColor = juce::Colours::yellow;
                else
                    jitterColor = juce::Colours::red;

                jitterLight.setColour ( juce::Label::backgroundColourId, jitterColor );
            }
        }
        else
        {
            pingLabel.setText ( "Ping: --", juce::dontSendNotification );
            pingLabel.setColour ( juce::Label::textColourId, juce::Colours::grey );

            delayLabel.setText ( "Delay: --", juce::dontSendNotification );
            delayLabel.setColour ( juce::Label::textColourId, juce::Colours::grey );

            delayLight.setColour ( juce::Label::backgroundColourId, juce::Colours::transparentBlack );
            jitterLight.setColour ( juce::Label::backgroundColourId, juce::Colours::transparentBlack );
        }

        // Update channel list if anything changed (count, IDs, names, icons)
        int          numChannels = jamulus_client_get_num_channels ( jamulusClient );
        juce::uint64 currentHash = 0;
        for ( int i = 0; i < numChannels; ++i )
        {
            jamulus_channel_info_t info;
            if ( jamulus_client_get_channel_info ( jamulusClient, i, &info ) )
            {
                currentHash += (juce::uint64) info.id * 131;
                currentHash ^= juce::String ( info.name ).hashCode64();
                currentHash ^= (juce::uint64) info.instrument * 17;
                currentHash ^= (juce::uint64) info.skill_level * 7;
                currentHash ^= juce::String ( info.country_code ).hashCode64();
            }
        }

        // Also include local profile in hash so "Me" strip updates when settings change
        const char* localName    = jamulus_client_get_name ( jamulusClient );
        const char* localCountry = jamulus_client_get_country_code ( jamulusClient );
        currentHash ^= juce::String ( localName ? localName : "" ).hashCode64() * 37;
        currentHash ^= (juce::uint64) jamulus_client_get_instrument ( jamulusClient ) * 41;
        currentHash ^= (juce::uint64) jamulus_client_get_skill ( jamulusClient ) * 43;
        currentHash ^= juce::String ( localCountry ? localCountry : "" ).hashCode64() * 47;

        if ( numChannels != lastNumChannels || currentHash != lastChannelsHash )
        {
            lastNumChannels  = numChannels;
            lastChannelsHash = currentHash;
            updateChannelStrips();
        }

        // Update channel levels - MUST map by ID because strips are sorted
        std::map<int, int> idToLevel;
        for ( int i = 0; i < numChannels; ++i )
        {
            jamulus_channel_info_t info;
            if ( jamulus_client_get_channel_info ( jamulusClient, i, &info ) )
                idToLevel[info.id] = info.level;
        }

        for ( auto& strip : channelStrips )
        {
            int id = strip->getChannelId();

            // PRIORITY: If this is the "Me" strip, always show local input levels
            // even if connected, for better feeling and consistency.
            if ( strip->getIsMe() )
            {
                strip->setLevel ( ( linearL + linearR ) * 0.5f );
            }
            else if ( id != -1 && idToLevel.count ( id ) )
            {
                strip->setLevel ( idToLevel[id] / 65535.0f );
            }
        }

        // Process Qt events to keep the Jamulus client running
        jamulus_process_events();
    }

    void showConnectDialog()
    {
        if ( !connectDialog )
        {
            connectDialog                     = std::make_unique<ConnectComponent> ( jamulusClient );
            connectDialog->onConnectRequested = [this] ( const juce::String& address ) {
                if ( jamulusClient )
                {
                    jamulus_client_set_server_addr ( jamulusClient, address.toRawUTF8() );
                    jamulus_client_start ( jamulusClient );
                }
                hideAllDialogs();
            };
            connectDialog->onCancel = [this]() { hideAllDialogs(); };
        }
        showDialog ( connectDialog.get() );
    }

    void showSettingsDialog()
    {
        if ( !settingsDialog )
        {
            settingsDialog          = std::make_unique<SettingsComponent> ( jamulusClient );
            settingsDialog->onClose = [this]() { hideAllDialogs(); };
        }
        showDialog ( settingsDialog.get() );
    }

    void toggleFXPanel()
    {
        fxPanelVisible = fxButton.getToggleState();

        if ( fxPanelVisible && !fxPanel )
        {
            fxPanel          = std::make_unique<FXPanelComponent> ( jamulusClient );
            fxPanel->onClose = [this]() {
                fxButton.setToggleState ( false, juce::dontSendNotification );
                toggleFXPanel();
            };

            // Reverb callbacks
            fxPanel->onReverbTimeChanged = [this] ( float time ) {
                reverbTime = time;
                // Note: Actual reverb T60 is set in the Qt audio processing
            };
            fxPanel->onReverbPreDelayChanged = [this] ( float ms ) { reverbPreDelay = ms; };
            fxPanel->onReverbHPChanged       = [this] ( float hz ) { reverbHP = hz; };
            fxPanel->onReverbLPChanged       = [this] ( float hz ) { reverbLP = hz; };

            // Delay callbacks
            fxPanel->onDelayEnableChanged   = [this] ( bool enabled ) { delayEnabled = enabled; };
            fxPanel->onDelayLevelChanged    = [this] ( float level ) { audioDelay.setMix ( level ); };
            fxPanel->onDelayTimeChanged     = [this] ( float ms ) { audioDelay.setDelayTime ( ms ); };
            fxPanel->onDelayFeedbackChanged = [this] ( float fb ) { audioDelay.setFeedback ( fb ); };
            fxPanel->onDelayPingPongChanged = [this] ( bool enabled ) { audioDelay.setPingPong ( enabled ); };
            fxPanel->onDelayHPChanged       = [this] ( float hz ) { audioDelay.setHighPassFreq ( hz ); };

            fxPanel->loadCurrentValues();
            addAndMakeVisible ( fxPanel.get() );
        }

        if ( !fxPanelVisible && fxPanel )
        {
            removeChildComponent ( fxPanel.get() );
            fxPanel.reset();
        }

        resized();
    }

    void toggleChatPanel()
    {
        // If chat window is open, close it and show panel instead
        if ( chatWindow )
        {
            // Save messages before closing window
            if ( auto* chatComp = dynamic_cast<ChatComponent*> ( chatWindow->getContentComponent() ) )
                sharedChatMessages = chatComp->getMessages();
            chatWindow.reset();
        }

        chatPanelVisible = chatButton.getToggleState();

        if ( chatPanelVisible && !chatPanel )
        {
            chatPanel          = std::make_unique<ChatComponent> ( jamulusClient, true );
            chatPanel->onClose = [this]() {
                // Save messages before closing
                sharedChatMessages = chatPanel->getMessages();
                chatButton.setToggleState ( false, juce::dontSendNotification );
                toggleChatPanel();
            };
            chatPanel->onPopOut = [this]() { popOutChat(); };

            // Restore previous messages
            if ( !sharedChatMessages.empty() )
                chatPanel->setMessages ( sharedChatMessages );
            else
            {
                // Add welcome message for first open
                ChatMessage welcome;
                welcome.text            = "Welcome to Jamulus Chat. Be respectful to other musicians!";
                welcome.isSystemMessage = true;
                welcome.timestamp       = juce::Time::getCurrentTime();
                sharedChatMessages.push_back ( welcome );
                chatPanel->setMessages ( sharedChatMessages );
            }

            addAndMakeVisible ( chatPanel.get() );
        }

        if ( !chatPanelVisible && chatPanel )
        {
            // Save messages before hiding
            sharedChatMessages = chatPanel->getMessages();
            removeChildComponent ( chatPanel.get() );
            chatPanel.reset();
        }

        resized();
    }

    void popOutChat()
    {
        // Save messages from panel before closing
        if ( chatPanel )
            sharedChatMessages = chatPanel->getMessages();

        // Close the panel
        chatButton.setToggleState ( false, juce::dontSendNotification );
        chatPanelVisible = false;
        if ( chatPanel )
        {
            removeChildComponent ( chatPanel.get() );
            chatPanel.reset();
        }

        // Create pop-out window
        chatWindow                 = std::make_unique<ChatWindow> ( jamulusClient );
        chatWindow->onWindowClosed = [this]() {
            // Save messages before closing
            if ( auto* chatComp = dynamic_cast<ChatComponent*> ( chatWindow->getContentComponent() ) )
                sharedChatMessages = chatComp->getMessages();
            chatWindow.reset();
        };
        chatWindow->onPopIn = [this]() { popInChat(); };

        // Restore messages to popup window
        if ( auto* chatComp = dynamic_cast<ChatComponent*> ( chatWindow->getContentComponent() ) )
        {
            if ( !sharedChatMessages.empty() )
                chatComp->setMessages ( sharedChatMessages );
        }

        resized();
    }

    void popInChat()
    {
        // Save messages from popup window
        if ( chatWindow )
        {
            if ( auto* chatComp = dynamic_cast<ChatComponent*> ( chatWindow->getContentComponent() ) )
                sharedChatMessages = chatComp->getMessages();
        }

        // Close the pop-out window
        chatWindow.reset();

        // Show the panel
        chatButton.setToggleState ( true, juce::dontSendNotification );
        toggleChatPanel();
    }

    void showDialog ( juce::Component* dialog )
    {
        hideAllDialogs();
        if ( dialog )
        {
            addAndMakeVisible ( dialog );
            dialog->setBounds ( getLocalBounds().reduced ( 20 ) );
            dialog->toFront ( true );
            currentDialog = dialog;
        }
    }

    void hideAllDialogs()
    {
        if ( connectDialog )
            removeChildComponent ( connectDialog.get() );
        if ( settingsDialog )
            removeChildComponent ( settingsDialog.get() );
        // Note: chatPanel is handled separately via toggle
        currentDialog = nullptr;
    }

    void onConnectClicked()
    {
        // Show connect dialog instead of direct connect
        showConnectDialog();
    }

    void updateChannelStrips()
    {
        // Gather channel info
        struct ChannelInfo
        {
            int                    index;
            jamulus_channel_info_t info;
            bool                   isMe;
        };
        std::vector<ChannelInfo> stripsToCreate;

        int  numChannels = jamulus_client_get_num_channels ( jamulusClient );
        int  myId        = jamulus_client_get_my_channel_id ( jamulusClient );
        bool meIncluded  = false;

        for ( int i = 0; i < numChannels; ++i )
        {
            jamulus_channel_info_t info;
            if ( jamulus_client_get_channel_info ( jamulusClient, i, &info ) )
            {
                bool isMe = ( info.id == myId && myId != -1 );
                if ( isMe )
                    meIncluded = true;
                stripsToCreate.push_back ( { i, info, isMe } );
            }
        }

        // If not connected or not in list, add local fader
        if ( !meIncluded )
        {
            jamulus_channel_info_t meInfo;
            memset ( &meInfo, 0, sizeof ( meInfo ) );
            meInfo.id = -1; // Special ID for local-only

            // Get current profile info
            const char* name = jamulus_client_get_name ( jamulusClient );
            strncpy ( meInfo.name, name ? name : "Me", sizeof ( meInfo.name ) - 1 );
            meInfo.instrument   = jamulus_client_get_instrument ( jamulusClient );
            const char* country = jamulus_client_get_country_code ( jamulusClient );
            strncpy ( meInfo.country_code, country ? country : "none", sizeof ( meInfo.country_code ) - 1 );
            meInfo.skill_level = jamulus_client_get_skill ( jamulusClient );

            stripsToCreate.insert ( stripsToCreate.begin(), { -1, meInfo, true } );
        }

        // Sort: Me first, then by ID
        std::sort ( stripsToCreate.begin(), stripsToCreate.end(), [] ( const ChannelInfo& a, const ChannelInfo& b ) {
            if ( a.isMe != b.isMe )
                return a.isMe;
            return a.info.id < b.info.id;
        } );

        // Fully clear existing strips and UI components before rebuild to prevent ghost offsets
        mixerContainer.removeAllChildren();
        channelStrips.clear();

        for ( const auto& ci : stripsToCreate )
        {
            DBG ( "Channel " << ci.info.id << ": " << ci.info.name << " Inst: " << ci.info.instrument << " Country: " << ci.info.country_code );
            auto strip = std::make_unique<ChannelStrip>();
            strip->setChannelInfo ( ci.info.id,
                                    juce::String ( ci.info.name ),
                                    ci.info.instrument,
                                    ci.info.country_code,
                                    ci.info.skill_level,
                                    ci.isMe );

            auto stripPtr        = strip.get();
            strip->onGainChanged = [this] ( int id, float gain ) { jamulus_client_set_channel_gain ( jamulusClient, id, gain ); };
            strip->onPanChanged  = [this] ( int id, float pan ) { jamulus_client_set_channel_pan ( jamulusClient, id, pan ); };
            strip->onMuteChanged = [this, stripPtr] ( int id, bool muted ) {
                if ( muted )
                {
                    jamulus_client_set_channel_mute ( jamulusClient, id, true );
                }
                else
                {
                    jamulus_client_set_channel_gain ( jamulusClient, id, stripPtr->getGain() );
                }
            };

            mixerContainer.addAndMakeVisible ( strip.get() );
            channelStrips.push_back ( std::move ( strip ) );
        }

        updateMixerLayout();
    }

    void updateMixerLayout()
    {
        const int stripWidth  = 80;
        const int stripHeight = mixerViewport.getHeight() - 20;

        int totalWidth = static_cast<int> ( channelStrips.size() ) * stripWidth;
        if ( totalWidth < mixerViewport.getWidth() )
            totalWidth = mixerViewport.getWidth();

        mixerContainer.setSize ( totalWidth, stripHeight );

        int x = 10;
        for ( auto& strip : channelStrips )
        {
            strip->setBounds ( x, 10, stripWidth - 10, stripHeight - 20 );
            x += stripWidth;
        }
    }

    jamulus_client_t jamulusClient;
    bool             wasConnected     = false;
    int              lastNumChannels  = 0;
    juce::uint64     lastChannelsHash = 0;
    bool             chatPanelVisible = false;
    bool             fxPanelVisible   = false;

    // FX parameters
    float      reverbTime     = 1.0f;
    float      reverbPreDelay = 0.0f;
    float      reverbHP       = 20.0f;
    float      reverbLP       = 20000.0f;
    bool       delayEnabled   = false;
    int        pingCounter    = 0;
    AudioDelay audioDelay;

    // Header
    juce::Label      titleLabel;
    juce::TextButton connectButton;
    juce::TextButton settingsButton;
    juce::TextButton chatButton;
    juce::TextButton disconnectButton;
    juce::Label      statusLabel;
    juce::Label      pingLabel;

    // Network stats
    juce::Label delayLabel;
    juce::Label jitterLabel;
    juce::Label delayLight;  // Using Label as colored box
    juce::Label jitterLight; // Using Label as colored box

    // Input controls
    juce::Label      inputLevelLabel;
    LevelMeter       inputMeterL;
    LevelMeter       inputMeterR;
    juce::Label      inputFaderLabel;
    juce::Slider     inputFaderSlider;
    juce::TextButton fxButton;
    juce::TextButton sidechainButton;
    juce::TextButton monitorModeButton;
    bool             monitorServerReturn = false; // false = direct thru, true = server return

    // Settings
    // (Consolidated into Settings dialog)

    // Mixer
    juce::Viewport mixerViewport;
    struct MixerContainer : public juce::Component
    {
        void paint ( juce::Graphics& g ) override
        {
            // Draw separator line after the first strip (local user)
            if ( getNumChildComponents() > 1 )
            {
                g.setColour ( juce::Colour ( 0xff505050 ) );
                g.drawVerticalLine ( 80, 5.0f, (float) getHeight() - 5.0f );
            }
        }
    } mixerContainer;
    std::vector<std::unique_ptr<ChannelStrip>> channelStrips;

    // Dialogs
    std::unique_ptr<ConnectComponent>  connectDialog;
    std::unique_ptr<SettingsComponent> settingsDialog;
    juce::Component*                   currentDialog = nullptr;

    // Chat panel and window
    std::unique_ptr<ChatComponent> chatPanel;
    std::unique_ptr<ChatWindow>    chatWindow;
    std::vector<ChatMessage>       sharedChatMessages; // Persistent chat storage

    // FX Panel
    std::unique_ptr<FXPanelComponent> fxPanel;

    // Sidechain callback
    void updateSidechainButtonColor()
    {
        if ( sidechainButton.getToggleState() )
            sidechainButton.setColour ( juce::TextButton::buttonColourId, juce::Colour ( 0xff00aa66 ) );
        else
            sidechainButton.setColour ( juce::TextButton::buttonColourId,
                                        juce::LookAndFeel::getDefaultLookAndFeel().findColour ( juce::TextButton::buttonColourId ) );
    }

    void updateMonitorButtonColor()
    {
        if ( monitorModeButton.getToggleState() )
            monitorModeButton.setColour ( juce::TextButton::buttonColourId, juce::Colour ( 0xffcc6600 ) ); // Orange when ON
        else
            monitorModeButton.setColour ( juce::TextButton::buttonColourId,
                                          juce::LookAndFeel::getDefaultLookAndFeel().findColour ( juce::TextButton::buttonColourId ) );
    }

public:
    // Callbacks that the editor can set
    std::function<void ( bool )> onSidechainChanged;
    std::function<void ( bool )> onMonitorModeChanged; // true = server return, false = direct thru

    // Set sidechain availability (called by editor)
    void setSidechainAvailable ( bool available )
    {
        sidechainButton.setEnabled ( available );
        if ( !available )
        {
            sidechainButton.setToggleState ( false, juce::dontSendNotification );
            sidechainButton.setTooltip ( "Sidechain not available - enable sidechain bus in DAW" );
        }
        else
        {
            sidechainButton.setTooltip ( "Use Sidechain Input" );
        }
        updateSidechainButtonColor();
    }

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR ( JamulusGuiComponent )

    void loadSettings()
    {
        auto file = getSettingsFile();
        if ( !file.existsAsFile() )
            return;

        auto json = juce::JSON::parse ( file );
        if ( !json.isObject() )
            return;

        auto* obj = json.getDynamicObject();
        if ( !obj || !jamulusClient )
            return;

        // Profile settings
        if ( obj->hasProperty ( "name" ) )
            jamulus_client_set_name ( jamulusClient, obj->getProperty ( "name" ).toString().toRawUTF8() );
        if ( obj->hasProperty ( "instrument" ) )
            jamulus_client_set_instrument ( jamulusClient, static_cast<int> ( obj->getProperty ( "instrument" ) ) );
        if ( obj->hasProperty ( "country" ) )
            jamulus_client_set_country ( jamulusClient, static_cast<int> ( obj->getProperty ( "country" ) ) );
        if ( obj->hasProperty ( "city" ) )
            jamulus_client_set_city ( jamulusClient, obj->getProperty ( "city" ).toString().toRawUTF8() );
        if ( obj->hasProperty ( "skill" ) )
            jamulus_client_set_skill ( jamulusClient, static_cast<int> ( obj->getProperty ( "skill" ) ) );

        // Audio settings
        if ( obj->hasProperty ( "audioQuality" ) )
            jamulus_client_set_audio_quality ( jamulusClient, static_cast<int> ( obj->getProperty ( "audioQuality" ) ) );
        if ( obj->hasProperty ( "audioChannels" ) )
            jamulus_client_set_audio_channels ( jamulusClient, static_cast<int> ( obj->getProperty ( "audioChannels" ) ) );
        if ( obj->hasProperty ( "inputBoost" ) )
            jamulus_client_set_input_boost ( jamulusClient, static_cast<int> ( obj->getProperty ( "inputBoost" ) ) );

        // Network settings
        if ( obj->hasProperty ( "autoJitter" ) )
            jamulus_client_set_auto_jitter ( jamulusClient, static_cast<bool> ( obj->getProperty ( "autoJitter" ) ) );
        if ( obj->hasProperty ( "jitterBuffer" ) )
            jamulus_client_set_jitter_buffer ( jamulusClient, static_cast<int> ( obj->getProperty ( "jitterBuffer" ) ) );
        if ( obj->hasProperty ( "serverJitterBuffer" ) )
            jamulus_client_set_server_jitter_buffer ( jamulusClient, static_cast<int> ( obj->getProperty ( "serverJitterBuffer" ) ) );
        if ( obj->hasProperty ( "newClientLevel" ) )
            jamulus_client_set_new_client_level ( jamulusClient, static_cast<int> ( obj->getProperty ( "newClientLevel" ) ) );
        if ( obj->hasProperty ( "smallBuffers" ) )
            jamulus_client_set_small_buffers ( jamulusClient, static_cast<bool> ( obj->getProperty ( "smallBuffers" ) ) );
    }

public:
    void saveSettings()
    {
        if ( !jamulusClient )
            return;

        juce::DynamicObject::Ptr obj = new juce::DynamicObject();

        // Profile settings
        obj->setProperty ( "name", juce::String ( jamulus_client_get_name ( jamulusClient ) ) );
        obj->setProperty ( "instrument", jamulus_client_get_instrument ( jamulusClient ) );
        obj->setProperty ( "country", jamulus_client_get_country ( jamulusClient ) );
        obj->setProperty ( "city", juce::String ( jamulus_client_get_city ( jamulusClient ) ) );
        obj->setProperty ( "skill", jamulus_client_get_skill ( jamulusClient ) );

        // Audio settings
        obj->setProperty ( "audioQuality", jamulus_client_get_audio_quality ( jamulusClient ) );
        obj->setProperty ( "audioChannels", jamulus_client_get_audio_channels ( jamulusClient ) );
        obj->setProperty ( "inputBoost", jamulus_client_get_input_boost ( jamulusClient ) );

        // Network settings
        obj->setProperty ( "autoJitter", jamulus_client_get_auto_jitter ( jamulusClient ) );
        obj->setProperty ( "jitterBuffer", jamulus_client_get_jitter_buffer ( jamulusClient ) );
        obj->setProperty ( "serverJitterBuffer", jamulus_client_get_server_jitter_buffer ( jamulusClient ) );
        obj->setProperty ( "newClientLevel", jamulus_client_get_new_client_level ( jamulusClient ) );
        obj->setProperty ( "smallBuffers", jamulus_client_get_small_buffers ( jamulusClient ) );

        auto file = getSettingsFile();
        file.replaceWithText ( juce::JSON::toString ( juce::var ( obj.get() ) ) );
    }
};
