#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../jamulus_wrapper.h"

//==============================================================================
// My Profile Tab
//==============================================================================
class ProfileTab : public juce::Component
{
public:
    ProfileTab ( jamulus_client_t client ) : jamulusClient ( client )
    {
        // Alias/Name
        addAndMakeVisible ( aliasLabel );
        aliasLabel.setText ( "Alias/Name:", juce::dontSendNotification );
        aliasLabel.setColour ( juce::Label::textColourId, juce::Colours::white );

        addAndMakeVisible ( aliasEdit );
        aliasEdit.onTextChange = [this]() {
            if ( jamulusClient )
                jamulus_client_set_name ( jamulusClient, aliasEdit.getText().toRawUTF8() );
        };

        // Instrument
        addAndMakeVisible ( instrumentLabel );
        instrumentLabel.setText ( "Instrument:", juce::dontSendNotification );
        instrumentLabel.setColour ( juce::Label::textColourId, juce::Colours::white );

        addAndMakeVisible ( instrumentCombo );
        populateInstruments();
        instrumentCombo.onChange = [this]() {
            if ( jamulusClient )
                jamulus_client_set_instrument ( jamulusClient, instrumentCombo.getSelectedId() - 1 );
        };

        // Country
        addAndMakeVisible ( countryLabel );
        countryLabel.setText ( "Country/Region:", juce::dontSendNotification );
        countryLabel.setColour ( juce::Label::textColourId, juce::Colours::white );

        addAndMakeVisible ( countryCombo );
        populateCountries();
        countryCombo.onChange = [this]() {
            if ( jamulusClient )
                jamulus_client_set_country ( jamulusClient, countryCombo.getSelectedId() - 1 ); // Subtract 1 for QLocale::Country
        };

        // City
        addAndMakeVisible ( cityLabel );
        cityLabel.setText ( "City:", juce::dontSendNotification );
        cityLabel.setColour ( juce::Label::textColourId, juce::Colours::white );

        addAndMakeVisible ( cityEdit );
        cityEdit.onTextChange = [this]() {
            if ( jamulusClient )
                jamulus_client_set_city ( jamulusClient, cityEdit.getText().toRawUTF8() );
        };

        // Skill Level
        addAndMakeVisible ( skillLabel );
        skillLabel.setText ( "Skill Level:", juce::dontSendNotification );
        skillLabel.setColour ( juce::Label::textColourId, juce::Colours::white );

        addAndMakeVisible ( skillCombo );
        skillCombo.addItem ( "None", 1 );
        skillCombo.addItem ( "Beginner", 2 );
        skillCombo.addItem ( "Intermediate", 3 );
        skillCombo.addItem ( "Expert", 4 );
        skillCombo.setSelectedId ( 1 );
        skillCombo.onChange = [this]() {
            if ( jamulusClient )
                jamulus_client_set_skill ( jamulusClient, skillCombo.getSelectedId() - 1 );
        };

        // Load current values
        loadCurrentValues();
    }

    void resized() override
    {
        auto      bounds     = getLocalBounds().reduced ( 20 );
        const int rowHeight  = 30;
        const int labelWidth = 120;
        const int spacing    = 10;

        auto makeRow = [&]() {
            auto row = bounds.removeFromTop ( rowHeight );
            bounds.removeFromTop ( spacing );
            return row;
        };

        auto row = makeRow();
        aliasLabel.setBounds ( row.removeFromLeft ( labelWidth ) );
        aliasEdit.setBounds ( row );

        row = makeRow();
        instrumentLabel.setBounds ( row.removeFromLeft ( labelWidth ) );
        instrumentCombo.setBounds ( row );

        row = makeRow();
        countryLabel.setBounds ( row.removeFromLeft ( labelWidth ) );
        countryCombo.setBounds ( row );

        row = makeRow();
        cityLabel.setBounds ( row.removeFromLeft ( labelWidth ) );
        cityEdit.setBounds ( row );

        row = makeRow();
        skillLabel.setBounds ( row.removeFromLeft ( labelWidth ) );
        skillCombo.setBounds ( row );
    }

private:
    void populateInstruments()
    {
        // Standard Jamulus instrument list
        const char* instruments[] = {
            "None",        "Drums",      "Djembe",        "Electric Guitar", "Acoustic Guitar", "Bass Guitar", "Keyboard",    "Synthesizer",
            "Grand Piano", "Accordion",  "Vocal",         "Microphone",      "Harmonica",       "Trumpet",     "Trombone",    "French Horn",
            "Tuba",        "Saxophone",  "Clarinet",      "Flute",           "Violin",          "Cello",       "Double Bass", "Recorder",
            "Banjo",       "Mandolin",   "Ukulele",       "Bass Ukulele",    "Viola",           "Congas",      "Bongo",       "Vocal Bass",
            "Vocal Tenor", "Vocal Alto", "Vocal Soprano", "Bagpipe",         "Theremin",        "Oud",         "Harp",        "Oboe",
            "Bassoon",     "Listener" };

        for ( int i = 0; i < sizeof ( instruments ) / sizeof ( instruments[0] ); ++i )
            instrumentCombo.addItem ( instruments[i], i + 1 );

        instrumentCombo.setSelectedId ( 1 );
    }

    void populateCountries()
    {
        // Country IDs in JUCE ComboBox must be > 0, so we add 1 to QLocale::Country values
        // When reading, we subtract 1 to get the actual QLocale::Country enum
        // QLocale::AnyCountry = 0, so combo ID = 1
        countryCombo.addItem ( "(Not specified)", 1 );  // QLocale::AnyCountry (0) + 1
        countryCombo.addItem ( "Argentina", 11 );       // 10 + 1
        countryCombo.addItem ( "Australia", 14 );       // 13 + 1
        countryCombo.addItem ( "Austria", 15 );         // 14 + 1
        countryCombo.addItem ( "Belgium", 22 );         // 21 + 1
        countryCombo.addItem ( "Brazil", 31 );          // 30 + 1
        countryCombo.addItem ( "Canada", 39 );          // 38 + 1
        countryCombo.addItem ( "Chile", 44 );           // 43 + 1
        countryCombo.addItem ( "China", 45 );           // 44 + 1
        countryCombo.addItem ( "Czech Republic", 57 );  // 56 + 1
        countryCombo.addItem ( "Denmark", 59 );         // 58 + 1
        countryCombo.addItem ( "Finland", 74 );         // 73 + 1
        countryCombo.addItem ( "France", 75 );          // 74 + 1
        countryCombo.addItem ( "Germany", 83 );         // 82 + 1
        countryCombo.addItem ( "Greece", 86 );          // 85 + 1
        countryCombo.addItem ( "Hungary", 98 );         // 97 + 1
        countryCombo.addItem ( "India", 101 );          // 100 + 1
        countryCombo.addItem ( "Ireland", 105 );        // 104 + 1
        countryCombo.addItem ( "Israel", 106 );         // 105 + 1
        countryCombo.addItem ( "Italy", 107 );          // 106 + 1
        countryCombo.addItem ( "Japan", 109 );          // 108 + 1
        countryCombo.addItem ( "Mexico", 140 );         // 139 + 1
        countryCombo.addItem ( "Netherlands", 152 );    // 151 + 1
        countryCombo.addItem ( "New Zealand", 155 );    // 154 + 1
        countryCombo.addItem ( "Norway", 162 );         // 161 + 1
        countryCombo.addItem ( "Poland", 173 );         // 172 + 1
        countryCombo.addItem ( "Portugal", 174 );       // 173 + 1
        countryCombo.addItem ( "Russia", 179 );         // 178 + 1
        countryCombo.addItem ( "South Africa", 196 );   // 195 + 1
        countryCombo.addItem ( "South Korea", 116 );    // 115 + 1
        countryCombo.addItem ( "Spain", 198 );          // 197 + 1
        countryCombo.addItem ( "Sweden", 206 );         // 205 + 1
        countryCombo.addItem ( "Switzerland", 207 );    // 206 + 1
        countryCombo.addItem ( "Taiwan", 209 );         // 208 + 1
        countryCombo.addItem ( "Turkey", 218 );         // 217 + 1
        countryCombo.addItem ( "Ukraine", 223 );        // 222 + 1
        countryCombo.addItem ( "United Kingdom", 225 ); // 224 + 1
        countryCombo.addItem ( "United States", 226 );  // 225 + 1
        countryCombo.setSelectedId ( 1 );
    }

    void loadCurrentValues()
    {
        if ( !jamulusClient )
            return;

        const char* name = jamulus_client_get_name ( jamulusClient );
        if ( name )
            aliasEdit.setText ( name, false );

        instrumentCombo.setSelectedId ( jamulus_client_get_instrument ( jamulusClient ) + 1, juce::dontSendNotification );

        // Restore country - combo ID = QLocale::Country + 1
        int countryVal = jamulus_client_get_country ( jamulusClient );
        countryCombo.setSelectedId ( countryVal + 1, juce::dontSendNotification ); // Add 1 for combo ID

        const char* city = jamulus_client_get_city ( jamulusClient );
        if ( city )
            cityEdit.setText ( city, false );

        skillCombo.setSelectedId ( jamulus_client_get_skill ( jamulusClient ) + 1, juce::dontSendNotification );
    }

    jamulus_client_t jamulusClient;

    juce::Label      aliasLabel;
    juce::TextEditor aliasEdit;
    juce::Label      instrumentLabel;
    juce::ComboBox   instrumentCombo;
    juce::Label      countryLabel;
    juce::ComboBox   countryCombo;
    juce::Label      cityLabel;
    juce::TextEditor cityEdit;
    juce::Label      skillLabel;
    juce::ComboBox   skillCombo;
};

//==============================================================================
// Audio/Network Tab
//==============================================================================
class AudioNetworkTab : public juce::Component, private juce::Timer
{
public:
    AudioNetworkTab ( jamulus_client_t client ) : jamulusClient ( client )
    {
        // === Audio Section ===
        addAndMakeVisible ( audioSectionLabel );
        audioSectionLabel.setText ( "Audio Settings", juce::dontSendNotification );
        audioSectionLabel.setFont ( juce::Font ( 16.0f, juce::Font::bold ) );
        audioSectionLabel.setColour ( juce::Label::textColourId, juce::Colours::white );

        // Audio Quality
        addAndMakeVisible ( qualityLabel );
        qualityLabel.setText ( "Audio Quality:", juce::dontSendNotification );
        qualityLabel.setColour ( juce::Label::textColourId, juce::Colours::white );

        addAndMakeVisible ( qualityCombo );
        qualityCombo.addItem ( "Low (22 kHz)", 1 );
        qualityCombo.addItem ( "Normal (44.1 kHz)", 2 );
        qualityCombo.addItem ( "High (48 kHz OPUS64)", 3 );
        qualityCombo.setSelectedId ( 2 );
        qualityCombo.onChange = [this]() {
            if ( jamulusClient )
                jamulus_client_set_audio_quality ( jamulusClient, qualityCombo.getSelectedId() - 1 );
        };

        // Audio Channels
        addAndMakeVisible ( channelsLabel );
        channelsLabel.setText ( "Audio Channels:", juce::dontSendNotification );
        channelsLabel.setColour ( juce::Label::textColourId, juce::Colours::white );

        addAndMakeVisible ( channelsCombo );
        channelsCombo.addItem ( "Mono", 1 );
        channelsCombo.addItem ( "Mono-In / Stereo-Out", 2 );
        channelsCombo.addItem ( "Stereo", 3 );
        channelsCombo.setSelectedId ( 3 );
        channelsCombo.onChange = [this]() {
            if ( jamulusClient )
                jamulus_client_set_audio_channels ( jamulusClient, channelsCombo.getSelectedId() - 1 );
        };

        // Input Boost
        addAndMakeVisible ( inputBoostLabel );
        inputBoostLabel.setText ( "Input Boost:", juce::dontSendNotification );
        inputBoostLabel.setColour ( juce::Label::textColourId, juce::Colours::white );

        addAndMakeVisible ( inputBoostCombo );
        inputBoostCombo.addItem ( "1x (0 dB)", 1 );
        inputBoostCombo.addItem ( "2x (+6 dB)", 2 );
        inputBoostCombo.addItem ( "4x (+12 dB)", 3 );
        inputBoostCombo.addItem ( "8x (+18 dB)", 4 );
        inputBoostCombo.setSelectedId ( 1 );
        inputBoostCombo.onChange = [this]() {
            if ( jamulusClient )
                jamulus_client_set_input_boost ( jamulusClient, inputBoostCombo.getSelectedId() );
        };

        // === Network Section ===
        addAndMakeVisible ( networkSectionLabel );
        networkSectionLabel.setText ( "Network Settings", juce::dontSendNotification );
        networkSectionLabel.setFont ( juce::Font ( 16.0f, juce::Font::bold ) );
        networkSectionLabel.setColour ( juce::Label::textColourId, juce::Colours::white );

        // Auto Jitter Checkbox
        addAndMakeVisible ( autoJitterToggle );
        autoJitterToggle.setButtonText ( "Auto Jitter Buffer/Buffers" );
        autoJitterToggle.setToggleState ( true, juce::dontSendNotification );
        autoJitterToggle.onClick = [this]() {
            bool autoEnabled = autoJitterToggle.getToggleState();
            jitterSlider.setEnabled ( !autoEnabled );
            serverJitterSlider.setEnabled ( !autoEnabled );
            if ( jamulusClient )
                jamulus_client_set_auto_jitter ( jamulusClient, autoEnabled );
        };

        // Local Jitter Buffer
        addAndMakeVisible ( jitterLabel );
        jitterLabel.setText ( "Local Jitter Buffer:", juce::dontSendNotification );
        jitterLabel.setColour ( juce::Label::textColourId, juce::Colours::white );

        addAndMakeVisible ( jitterSlider );
        jitterSlider.setRange ( 1, 20, 1 );
        jitterSlider.setValue ( 10 );
        jitterSlider.setTextBoxStyle ( juce::Slider::TextBoxLeft, false, 40, 20 );
        jitterSlider.setEnabled ( false ); // Default auto is true
        jitterSlider.onValueChange = [this]() {
            if ( jamulusClient && !autoJitterToggle.getToggleState() )
                jamulus_client_set_jitter_buffer ( jamulusClient, static_cast<int> ( jitterSlider.getValue() ) );
        };

        // Server Jitter Buffer
        addAndMakeVisible ( serverJitterLabel );
        serverJitterLabel.setText ( "Server Jitter Buffer:", juce::dontSendNotification );
        serverJitterLabel.setColour ( juce::Label::textColourId, juce::Colours::white );

        addAndMakeVisible ( serverJitterSlider );
        serverJitterSlider.setRange ( 1, 20, 1 );
        serverJitterSlider.setValue ( 10 );
        serverJitterSlider.setTextBoxStyle ( juce::Slider::TextBoxLeft, false, 40, 20 );
        serverJitterSlider.setEnabled ( false ); // Default auto is true
        serverJitterSlider.onValueChange = [this]() {
            if ( jamulusClient && !autoJitterToggle.getToggleState() )
                jamulus_client_set_server_jitter_buffer ( jamulusClient, static_cast<int> ( serverJitterSlider.getValue() ) );
        };

        // New Client Level
        addAndMakeVisible ( newClientLevelLabel );
        newClientLevelLabel.setText ( "New Client Level:", juce::dontSendNotification );
        newClientLevelLabel.setColour ( juce::Label::textColourId, juce::Colours::white );

        addAndMakeVisible ( newClientLevelSlider );
        newClientLevelSlider.setRange ( 0, 100, 1 );
        newClientLevelSlider.setValue ( 100 );
        newClientLevelSlider.setTextBoxStyle ( juce::Slider::TextBoxLeft, false, 40, 20 );
        newClientLevelSlider.setTextValueSuffix ( "%" );
        newClientLevelSlider.onValueChange = [this]() {
            if ( jamulusClient )
                jamulus_client_set_new_client_level ( jamulusClient, static_cast<int> ( newClientLevelSlider.getValue() ) );
        };

        // Small Network Buffers
        addAndMakeVisible ( smallBuffersToggle );
        smallBuffersToggle.setButtonText ( "Enable Small Network Buffers" );
        smallBuffersToggle.setColour ( juce::ToggleButton::textColourId, juce::Colours::white );
        smallBuffersToggle.setTooltip ( "Use smaller network buffers for lower latency (may cause dropouts)" );
        smallBuffersToggle.onClick = [this]() {
            if ( jamulusClient )
                jamulus_client_set_small_buffers ( jamulusClient, smallBuffersToggle.getToggleState() );
        };

        // === Status Section ===
        addAndMakeVisible ( statusSectionLabel );
        statusSectionLabel.setText ( "Network Status", juce::dontSendNotification );
        statusSectionLabel.setFont ( juce::Font ( 16.0f, juce::Font::bold ) );
        statusSectionLabel.setColour ( juce::Label::textColourId, juce::Colours::white );

        addAndMakeVisible ( delayLabel );
        delayLabel.setText ( "Overall Delay: --", juce::dontSendNotification );
        delayLabel.setColour ( juce::Label::textColourId, juce::Colours::grey );

        addAndMakeVisible ( uploadRateLabel );
        uploadRateLabel.setText ( "Upload Rate: --", juce::dontSendNotification );
        uploadRateLabel.setColour ( juce::Label::textColourId, juce::Colours::grey );

        loadCurrentValues();
        startTimerHz ( 5 );
    }

    ~AudioNetworkTab() override { stopTimer(); }

    void resized() override
    {
        auto      bounds     = getLocalBounds().reduced ( 20 );
        const int rowHeight  = 30;
        const int labelWidth = 130;
        const int spacing    = 8;

        auto makeRow = [&]() {
            auto row = bounds.removeFromTop ( rowHeight );
            bounds.removeFromTop ( spacing );
            return row;
        };

        audioSectionLabel.setBounds ( makeRow() );

        auto row = makeRow();
        qualityLabel.setBounds ( row.removeFromLeft ( labelWidth ) );
        qualityCombo.setBounds ( row );

        row = makeRow();
        channelsLabel.setBounds ( row.removeFromLeft ( labelWidth ) );
        channelsCombo.setBounds ( row );

        row = makeRow();
        inputBoostLabel.setBounds ( row.removeFromLeft ( labelWidth ) );
        inputBoostCombo.setBounds ( row );

        bounds.removeFromTop ( 10 );
        networkSectionLabel.setBounds ( makeRow() );

        // Auto Jitter
        autoJitterToggle.setBounds ( makeRow() );

        // Local Buffer
        row = makeRow();
        jitterLabel.setBounds ( row.removeFromLeft ( labelWidth ) );
        jitterSlider.setBounds ( row );

        // Server Buffer
        row = makeRow();
        serverJitterLabel.setBounds ( row.removeFromLeft ( labelWidth ) );
        serverJitterSlider.setBounds ( row );

        row = makeRow();
        newClientLevelLabel.setBounds ( row.removeFromLeft ( labelWidth ) );
        newClientLevelSlider.setBounds ( row );

        smallBuffersToggle.setBounds ( makeRow() );

        bounds.removeFromTop ( 10 );
        statusSectionLabel.setBounds ( makeRow() );
        delayLabel.setBounds ( makeRow() );
        uploadRateLabel.setBounds ( makeRow() );
    }

private:
    void timerCallback() override
    {
        if ( !jamulusClient )
            return;

        // Update network stats
        int delay = jamulus_client_get_overall_delay ( jamulusClient );
        if ( delay >= 0 )
            delayLabel.setText ( "Overall Delay: " + juce::String ( delay ) + " ms", juce::dontSendNotification );
        else
            delayLabel.setText ( "Overall Delay: --", juce::dontSendNotification );

        // Sync Jitter values if in Auto mode
        bool autoJitter = jamulus_client_get_auto_jitter ( jamulusClient );

        // If external change (unlikely but possible), sync checkbox
        if ( autoJitterToggle.getToggleState() != autoJitter )
            autoJitterToggle.setToggleState ( autoJitter, juce::dontSendNotification );

        if ( autoJitter )
        {
            // In auto mode, update sliders to show current actual values
            jitterSlider.setValue ( jamulus_client_get_jitter_buffer ( jamulusClient ), juce::dontSendNotification );
            serverJitterSlider.setValue ( jamulus_client_get_server_jitter_buffer ( jamulusClient ), juce::dontSendNotification );

            // Ensure disabled
            if ( jitterSlider.isEnabled() )
                jitterSlider.setEnabled ( false );
            if ( serverJitterSlider.isEnabled() )
                serverJitterSlider.setEnabled ( false );
        }
        else
        {
            // Ensure enabled
            if ( !jitterSlider.isEnabled() )
                jitterSlider.setEnabled ( true );
            if ( !serverJitterSlider.isEnabled() )
                serverJitterSlider.setEnabled ( true );
        }
    }

    void loadCurrentValues()
    {
        if ( !jamulusClient )
            return;

        qualityCombo.setSelectedId ( jamulus_client_get_audio_quality ( jamulusClient ) + 1, juce::dontSendNotification );
        channelsCombo.setSelectedId ( jamulus_client_get_audio_channels ( jamulusClient ) + 1, juce::dontSendNotification );
        inputBoostCombo.setSelectedId ( jamulus_client_get_input_boost ( jamulusClient ), juce::dontSendNotification );

        // Network
        bool autoJitter = jamulus_client_get_auto_jitter ( jamulusClient );
        autoJitterToggle.setToggleState ( autoJitter, juce::dontSendNotification );

        jitterSlider.setValue ( jamulus_client_get_jitter_buffer ( jamulusClient ), juce::dontSendNotification );
        serverJitterSlider.setValue ( jamulus_client_get_server_jitter_buffer ( jamulusClient ), juce::dontSendNotification );

        jitterSlider.setEnabled ( !autoJitter );
        serverJitterSlider.setEnabled ( !autoJitter );

        newClientLevelSlider.setValue ( jamulus_client_get_new_client_level ( jamulusClient ), juce::dontSendNotification );
        smallBuffersToggle.setToggleState ( jamulus_client_get_small_buffers ( jamulusClient ), juce::dontSendNotification );
    }

    jamulus_client_t jamulusClient;

    juce::Label    audioSectionLabel;
    juce::Label    qualityLabel;
    juce::ComboBox qualityCombo;
    juce::Label    channelsLabel;
    juce::ComboBox channelsCombo;
    juce::Label    inputBoostLabel;
    juce::ComboBox inputBoostCombo;

    juce::Label        networkSectionLabel;
    juce::Label        jitterLabel;
    juce::ToggleButton autoJitterToggle;
    juce::Slider       jitterSlider;
    juce::Label        serverJitterLabel;
    juce::Slider       serverJitterSlider;

    juce::Label        newClientLevelLabel;
    juce::Slider       newClientLevelSlider;
    juce::ToggleButton smallBuffersToggle;

    juce::Label statusSectionLabel;
    juce::Label delayLabel;
    juce::Label uploadRateLabel;
};

//==============================================================================
// Advanced Tab
//==============================================================================
class AdvancedTab : public juce::Component
{
public:
    AdvancedTab ( jamulus_client_t client ) : jamulusClient ( client )
    {
        // Feedback Detection
        addAndMakeVisible ( feedbackToggle );
        feedbackToggle.setButtonText ( "Enable Feedback Detection" );
        feedbackToggle.setColour ( juce::ToggleButton::textColourId, juce::Colours::white );
        feedbackToggle.onClick = [this]() {
            if ( jamulusClient )
                jamulus_client_set_feedback_detection ( jamulusClient, feedbackToggle.getToggleState() );
        };

        // Audio Alerts
        addAndMakeVisible ( audioAlertsToggle );
        audioAlertsToggle.setButtonText ( "Enable Audio Alerts" );
        audioAlertsToggle.setColour ( juce::ToggleButton::textColourId, juce::Colours::white );
        audioAlertsToggle.onClick = [this]() {
            if ( jamulusClient )
                jamulus_client_set_audio_alerts ( jamulusClient, audioAlertsToggle.getToggleState() );
        };

        // Buffer Size Info
        addAndMakeVisible ( bufferInfoLabel );
        bufferInfoLabel.setText ( "VST buffer size is determined by your DAW settings.", juce::dontSendNotification );
        bufferInfoLabel.setColour ( juce::Label::textColourId, juce::Colours::grey );
        bufferInfoLabel.setFont ( juce::Font ( 12.0f ) );
    }

    void resized() override
    {
        auto      bounds    = getLocalBounds().reduced ( 20 );
        const int rowHeight = 30;
        const int spacing   = 10;

        feedbackToggle.setBounds ( bounds.removeFromTop ( rowHeight ) );
        bounds.removeFromTop ( spacing );

        audioAlertsToggle.setBounds ( bounds.removeFromTop ( rowHeight ) );
        bounds.removeFromTop ( spacing * 2 );

        bufferInfoLabel.setBounds ( bounds.removeFromTop ( 40 ) );
    }

private:
    jamulus_client_t jamulusClient;

    juce::ToggleButton feedbackToggle;
    juce::ToggleButton audioAlertsToggle;
    juce::Label        bufferInfoLabel;
};

//==============================================================================
// Main Settings Component
//==============================================================================
class SettingsComponent : public juce::Component
{
public:
    SettingsComponent ( jamulus_client_t client ) :
        jamulusClient ( client ),
        profileTab ( client ),
        audioNetworkTab ( client ),
        advancedTab ( client )
    {
        addAndMakeVisible ( titleLabel );
        titleLabel.setText ( "Settings", juce::dontSendNotification );
        titleLabel.setFont ( juce::Font ( 20.0f, juce::Font::bold ) );
        titleLabel.setColour ( juce::Label::textColourId, juce::Colours::white );
        titleLabel.setJustificationType ( juce::Justification::centred );

        addAndMakeVisible ( tabbedComponent );
        tabbedComponent.setTabBarDepth ( 30 );
        tabbedComponent.setColour ( juce::TabbedComponent::backgroundColourId, juce::Colour ( 0xff323232 ) );
        tabbedComponent.setColour ( juce::TabbedComponent::outlineColourId, juce::Colour ( 0xff505050 ) );

        tabbedComponent.addTab ( "My Profile", juce::Colour ( 0xff3a3a3a ), &profileTab, false );
        tabbedComponent.addTab ( "Audio/Network", juce::Colour ( 0xff3a3a3a ), &audioNetworkTab, false );
        tabbedComponent.addTab ( "Advanced", juce::Colour ( 0xff3a3a3a ), &advancedTab, false );

        addAndMakeVisible ( closeButton );
        closeButton.setButtonText ( "Close" );
        closeButton.onClick = [this]() {
            if ( onClose )
                onClose();
        };
    }

    void paint ( juce::Graphics& g ) override
    {
        g.fillAll ( juce::Colour ( 0xff323232 ) );

        // Header
        g.setColour ( juce::Colour ( 0xff252525 ) );
        g.fillRect ( 0, 0, getWidth(), 40 );
    }

    void resized() override
    {
        auto bounds = getLocalBounds();

        auto header = bounds.removeFromTop ( 40 );
        titleLabel.setBounds ( header );

        auto footer = bounds.removeFromBottom ( 50 );
        closeButton.setBounds ( footer.reduced ( 20, 10 ) );

        tabbedComponent.setBounds ( bounds );
    }

    std::function<void()> onClose;

private:
    jamulus_client_t jamulusClient;

    juce::Label           titleLabel;
    juce::TabbedComponent tabbedComponent{ juce::TabbedButtonBar::TabsAtTop };
    ProfileTab            profileTab;
    AudioNetworkTab       audioNetworkTab;
    AdvancedTab           advancedTab;
    juce::TextButton      closeButton;
};
