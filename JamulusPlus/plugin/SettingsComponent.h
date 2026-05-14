#pragma once

#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include "../jamulus_wrapper.h"
#include <algorithm>
#include <array>
#include <functional>
#include <unordered_map>
#include <vector>

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
            if ( setNameCallback )
            {
                setNameCallback ( aliasEdit.getText() );
            }
            else if ( jamulusClient )
            {
                jamulus_client_set_name ( jamulusClient, aliasEdit.getText().toRawUTF8() );
            }
            if ( onProfileChanged )
                onProfileChanged ();
        };

        // Instrument
        addAndMakeVisible ( instrumentLabel );
        instrumentLabel.setText ( "Instrument:", juce::dontSendNotification );
        instrumentLabel.setColour ( juce::Label::textColourId, juce::Colours::white );

        addAndMakeVisible ( instrumentCombo );
        populateInstruments();
        instrumentCombo.onChange = [this]() {
            int instrumentIndex = instrumentCombo.getSelectedId() - 1;
            if ( setInstrumentCallback )
            {
                setInstrumentCallback ( instrumentIndex );
            }
            else if ( jamulusClient )
            {
                jamulus_client_set_instrument ( jamulusClient, instrumentIndex );
            }
            if ( onProfileChanged )
                onProfileChanged ();
        };

        // Country
        addAndMakeVisible ( countryLabel );
        countryLabel.setText ( "Country/Region:", juce::dontSendNotification );
        countryLabel.setColour ( juce::Label::textColourId, juce::Colours::white );

        addAndMakeVisible ( countryCombo );
        populateCountries();
        countryCombo.onChange = [this]() {
            int countryValue = countryCombo.getSelectedId();
            if ( setCountryCallback )
            {
                setCountryCallback ( countryValue );
            }
            else if ( jamulusClient )
            {
                jamulus_client_set_country ( jamulusClient, countryValue );
            }
            updateCountryFlag();
            if ( onProfileChanged )
                onProfileChanged ();
        };
        addAndMakeVisible ( countryFlag );

        // City
        addAndMakeVisible ( cityLabel );
        cityLabel.setText ( "City:", juce::dontSendNotification );
        cityLabel.setColour ( juce::Label::textColourId, juce::Colours::white );

        addAndMakeVisible ( cityEdit );
        cityEdit.onTextChange = [this]() {
            if ( setCityCallback )
            {
                setCityCallback ( cityEdit.getText() );
            }
            else if ( jamulusClient )
            {
                jamulus_client_set_city ( jamulusClient, cityEdit.getText().toRawUTF8() );
            }
            if ( onProfileChanged )
                onProfileChanged ();
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
            int skillValue = skillCombo.getSelectedId() - 1;
            if ( setSkillCallback )
            {
                setSkillCallback ( skillValue );
            }
            else if ( jamulusClient )
            {
                jamulus_client_set_skill ( jamulusClient, skillValue );
            }
            if ( onProfileChanged )
                onProfileChanged ();
        };

        loadCurrentValues();
    }

    void setProfileChangedCallback ( std::function<void()> cb ) { onProfileChanged = std::move ( cb ); }

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
        auto flagArea = row.removeFromRight ( 30 );
        countryCombo.setBounds ( row );
        countryFlag.setBounds ( flagArea.removeFromLeft ( 22 ).reduced ( 3 ) );

        row = makeRow();
        cityLabel.setBounds ( row.removeFromLeft ( labelWidth ) );
        cityEdit.setBounds ( row );

        row = makeRow();
        skillLabel.setBounds ( row.removeFromLeft ( labelWidth ) );
        skillCombo.setBounds ( row );
    }

private:
    static juce::Image loadFlagByCode ( const juce::String& rawCode )
    {
        juce::String code = rawCode;

        if ( code.isEmpty() || code.equalsIgnoreCase ( "none" ) )
            return juce::Image();

        if ( code.equalsIgnoreCase ( "uk" ) )
            code = "gb";
        if ( code.equalsIgnoreCase ( "el" ) )
            code = "gr";

        if ( code.containsChar ( '-' ) )
        {
            auto parts = juce::StringArray::fromTokens ( code, "-", "" );
            if ( parts.size() >= 2 )
                code = parts[0];
        }

        if ( code.length() != 2 )
        {
            void*        data = nullptr;
            int          size = 0;
            juce::String path = "png/flags/res/flags/flagnone.png";
            if ( jamulus_load_resource ( path.toRawUTF8(), &data, &size ) && data && size > 0 )
            {
                juce::Image img = juce::ImageCache::getFromMemory ( data, size );
                jamulus_free_resource ( data );
                return img;
            }
            return juce::Image();
        }

        void* data = nullptr;
        int   size = 0;
        juce::String path = "png/flags/res/flags/" + code.toLowerCase() + ".png";
        if ( jamulus_load_resource ( path.toRawUTF8(), &data, &size ) && data && size > 0 )
        {
            juce::Image img = juce::ImageCache::getFromMemory ( data, size );
            jamulus_free_resource ( data );
            return img;
        }

        void*        fallbackData = nullptr;
        int          fallbackSize = 0;
        juce::String fallbackPath = "png/flags/res/flags/flagnone.png";
        if ( jamulus_load_resource ( fallbackPath.toRawUTF8(), &fallbackData, &fallbackSize ) && fallbackData && fallbackSize > 0 )
        {
            juce::Image img = juce::ImageCache::getFromMemory ( fallbackData, fallbackSize );
            jamulus_free_resource ( fallbackData );
            return img;
        }

        return juce::Image();
    }

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
        countryCombo.clear();
        countryIsoCodes.clear();

        struct CountryEntry
        {
            juce::String name;
            juce::String iso;
            int          wire = 0;
        };

        std::vector<CountryEntry> entries;

        int count = jamulus_get_supported_countries_count();
        for ( int i = 0; i < count; ++i )
        {
            char nameBuf[64];
            char codeBuf[8];
            int  wire = 0;
            if ( jamulus_get_supported_country ( i, nameBuf, (int) sizeof ( nameBuf ), codeBuf, (int) sizeof ( codeBuf ), &wire ) )
            {
                juce::String name = juce::String::fromUTF8 ( nameBuf ).trim();
                juce::String iso  = juce::String::fromUTF8 ( codeBuf ).trim().toLowerCase();

                if ( wire == 0 || iso.isEmpty() || iso.equalsIgnoreCase ( "none" ) )
                    continue;

                if ( name.isEmpty() )
                    name = iso.toUpperCase();

                if ( loadFlagByCode ( iso ).isNull() )
                    continue;

                entries.push_back ( { name, iso, wire } );
            }
        }

        std::sort ( entries.begin(),
                    entries.end(),
                    [] ( const CountryEntry& a, const CountryEntry& b ) { return a.name.compareIgnoreCase ( b.name ) < 0; } );

        countryCombo.addItem ( "None", 0 );
        countryIsoCodes[0] = "none";

        for ( const auto& entry : entries )
        {
            countryCombo.addItem ( entry.name, entry.wire );
            countryIsoCodes[entry.wire] = entry.iso;
        }
    }

    void loadCurrentValues()
    {
        if ( !jamulusClient )
            return;

        juce::String nameText;
        if ( getNameCallback )
        {
            nameText = getNameCallback();
        }
        else
        {
            const char* name = jamulus_client_get_name ( jamulusClient );
            if ( name )
                nameText = juce::String::fromUTF8 ( name );
        }
        if ( nameText.isNotEmpty() )
            aliasEdit.setText ( nameText, false );

        int instrumentIndex = 0;
        if ( getInstrumentCallback )
        {
            instrumentIndex = getInstrumentCallback();
        }
        else
        {
            instrumentIndex = jamulus_client_get_instrument ( jamulusClient );
        }
        instrumentCombo.setSelectedId ( instrumentIndex + 1, juce::dontSendNotification );

        int countryVal = 0;
        if ( getCountryCallback )
        {
            countryVal = getCountryCallback();
        }
        else
        {
            countryVal = jamulus_client_get_country ( jamulusClient );
        }
        countryCombo.setSelectedId ( countryVal, juce::dontSendNotification );
        updateCountryFlag();

        juce::String cityText;
        if ( getCityCallback )
        {
            cityText = getCityCallback();
        }
        else
        {
            const char* city = jamulus_client_get_city ( jamulusClient );
            if ( city )
                cityText = juce::String::fromUTF8 ( city );
        }
        if ( cityText.isNotEmpty() )
            cityEdit.setText ( cityText, false );

        int skillIndex = 0;
        if ( getSkillCallback )
        {
            skillIndex = getSkillCallback();
        }
        else
        {
            skillIndex = jamulus_client_get_skill ( jamulusClient );
        }
        skillCombo.setSelectedId ( skillIndex + 1, juce::dontSendNotification );
    }

    void updateCountryFlag()
    {
        juce::String codeText;

        const int selectedCountry = countryCombo.getSelectedId();
        auto      selectedIt      = countryIsoCodes.find ( selectedCountry );
        if ( selectedIt != countryIsoCodes.end() )
            codeText = selectedIt->second;

        if ( codeText.isEmpty() && getCountryCodeCallback )
        {
            codeText = getCountryCodeCallback();
        }
        else if ( codeText.isEmpty() )
        {
            if ( !jamulusClient )
            {
                countryFlag.setImage ( juce::Image() );
                return;
            }
            const char* code = jamulus_client_get_country_code ( jamulusClient );
            codeText         = juce::String::fromUTF8 ( code ? code : "none" );
        }

        if ( codeText.isEmpty() )
            codeText = "none";

        auto img = loadFlagByCode ( codeText );
        countryFlag.setImage ( img, juce::RectanglePlacement::centred );
    }

public:
    void setProfileCallbacks ( std::function<juce::String()> getNameCb,
                               std::function<void ( const juce::String& )> setNameCb,
                               std::function<int()> getInstrumentCb,
                               std::function<void ( int )> setInstrumentCb,
                               std::function<int()> getCountryCb,
                               std::function<void ( int )> setCountryCb,
                               std::function<juce::String()> getCountryCodeCb,
                               std::function<juce::String()> getCityCb,
                               std::function<void ( const juce::String& )> setCityCb,
                               std::function<int()> getSkillCb,
                               std::function<void ( int )> setSkillCb )
    {
        getNameCallback        = std::move ( getNameCb );
        setNameCallback        = std::move ( setNameCb );
        getInstrumentCallback  = std::move ( getInstrumentCb );
        setInstrumentCallback  = std::move ( setInstrumentCb );
        getCountryCallback     = std::move ( getCountryCb );
        setCountryCallback     = std::move ( setCountryCb );
        getCountryCodeCallback = std::move ( getCountryCodeCb );
        getCityCallback        = std::move ( getCityCb );
        setCityCallback        = std::move ( setCityCb );
        getSkillCallback       = std::move ( getSkillCb );
        setSkillCallback       = std::move ( setSkillCb );
    }

private:
    jamulus_client_t jamulusClient;

    juce::Label      aliasLabel;
    juce::TextEditor aliasEdit;
    juce::Label      instrumentLabel;
    juce::ComboBox   instrumentCombo;
    juce::Label      countryLabel;
    juce::ComboBox   countryCombo;
    juce::ImageComponent countryFlag;
    juce::Label      cityLabel;
    juce::TextEditor cityEdit;
    juce::Label      skillLabel;
    juce::ComboBox   skillCombo;

    std::function<juce::String()>                  getNameCallback;
    std::function<void ( const juce::String& )>    setNameCallback;
    std::function<int()>                           getInstrumentCallback;
    std::function<void ( int )>                    setInstrumentCallback;
    std::function<int()>                           getCountryCallback;
    std::function<void ( int )>                    setCountryCallback;
    std::function<juce::String()>                  getCountryCodeCallback;
    std::function<juce::String()>                  getCityCallback;
    std::function<void ( const juce::String& )>    setCityCallback;
    std::function<int()>                           getSkillCallback;
    std::function<void ( int )>                    setSkillCallback;

    std::unordered_map<int, juce::String> countryIsoCodes;
    std::function<void()> onProfileChanged;
};

//==============================================================================
// Audio/Network Tab
//==============================================================================
class AudioNetworkTab : public juce::Component, private juce::Timer
{
public:
    AudioNetworkTab ( jamulus_client_t client, JamulusPluginProcessor* proc ) : jamulusClient ( client ), processor ( proc )
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

        addAndMakeVisible ( testToneToggle );
        testToneToggle.setButtonText ( "Enable Test Tone" );
        testToneToggle.setColour ( juce::ToggleButton::textColourId, juce::Colours::white );
        testToneToggle.setTooltip ( "Play a local test tone" );
        testToneToggle.onClick = [this]() {
            if ( onTestToneChanged )
                onTestToneChanged ( testToneToggle.getToggleState() );
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
            if ( processor )
                processor->setAutoJitter ( autoEnabled );
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
            if ( autoJitterToggle.getToggleState() )
                return;
            if ( processor )
                processor->setJitterBuffer ( static_cast<int> ( jitterSlider.getValue() ) );
            else if ( jamulusClient )
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
            if ( autoJitterToggle.getToggleState() )
                return;
            if ( processor )
                processor->setServerJitterBuffer ( static_cast<int> ( serverJitterSlider.getValue() ) );
            else if ( jamulusClient )
                jamulus_client_set_server_jitter_buffer ( jamulusClient, static_cast<int> ( serverJitterSlider.getValue() ) );
        };

        // New Client Level
        addAndMakeVisible ( newClientLevelLabel );
        newClientLevelLabel.setText ( "New Client Level:", juce::dontSendNotification );
        newClientLevelLabel.setColour ( juce::Label::textColourId, juce::Colours::white );

        addAndMakeVisible ( newClientLevelCombo );
        newClientLevelCombo.addItem ( "1x (0 dB)", 100 );
        newClientLevelCombo.addItem ( "0.71x (-3 dB)", 71 );
        newClientLevelCombo.addItem ( "0.5x (-6 dB)", 50 );
        newClientLevelCombo.addItem ( "0.35x (-9 dB)", 35 );
        newClientLevelCombo.addItem ( "0.25x (-12 dB)", 25 );
        newClientLevelCombo.setSelectedId ( 50 );
        newClientLevelCombo.onChange = [this]() {
            if ( jamulusClient )
                jamulus_client_set_new_client_level ( jamulusClient, newClientLevelCombo.getSelectedId() );
        };

        // Small Network Buffers
        addAndMakeVisible ( smallBuffersToggle );
        smallBuffersToggle.setButtonText ( "Enable Small Network Buffers" );
        smallBuffersToggle.setColour ( juce::ToggleButton::textColourId, juce::Colours::white );
        smallBuffersToggle.setTooltip ( "Use smaller network buffers for lower latency (may cause dropouts)" );
        smallBuffersToggle.onClick = [this]() {
            if ( processor )
                processor->setSmallBuffers ( smallBuffersToggle.getToggleState() );
            else if ( jamulusClient )
                jamulus_client_set_small_buffers ( jamulusClient, smallBuffersToggle.getToggleState() );
        };

        // Auto Level Boosted Channels - Removed per user request
        /*
        addAndMakeVisible ( autoLevelBoostToggle );
        autoLevelBoostToggle.setButtonText ( "Auto Level Boosted Channels" );
        autoLevelBoostToggle.setColour ( juce::ToggleButton::textColourId, juce::Colours::white );
        autoLevelBoostToggle.setTooltip ( "Automatically reduce boosted channels if they become too loud compared to others" );
        autoLevelBoostToggle.onClick = [this]() {
            if ( onAutoLevelBoostChanged )
                onAutoLevelBoostChanged ( autoLevelBoostToggle.getToggleState() );
        };
        */

        // === Status Section ===
        addAndMakeVisible ( statusSectionLabel );
        statusSectionLabel.setText ( "Network Status", juce::dontSendNotification );
        statusSectionLabel.setFont ( juce::Font ( 16.0f, juce::Font::bold ) );
        statusSectionLabel.setColour ( juce::Label::textColourId, juce::Colours::white );

        addAndMakeVisible ( delayLabel );
        delayLabel.setText ( "Overall Delay: --", juce::dontSendNotification );
        delayLabel.setColour ( juce::Label::textColourId, juce::Colours::white );

        addAndMakeVisible ( uploadRateLabel );
        uploadRateLabel.setText ( "Upload Rate: --", juce::dontSendNotification );
        uploadRateLabel.setColour ( juce::Label::textColourId, juce::Colours::grey );

        addAndMakeVisible ( translateSectionLabel );
        translateSectionLabel.setText ( "Chat Translation", juce::dontSendNotification );
        translateSectionLabel.setFont ( juce::Font ( 16.0f, juce::Font::bold ) );
        translateSectionLabel.setColour ( juce::Label::textColourId, juce::Colours::white );

        addAndMakeVisible ( autoTranslateToggle );
        autoTranslateToggle.setButtonText ( "Auto Translate Incoming Chat" );
        autoTranslateToggle.setColour ( juce::ToggleButton::textColourId, juce::Colours::white );
        autoTranslateToggle.onClick = [this]() {
            if ( processor )
                processor->setAutoTranslateEnabled ( autoTranslateToggle.getToggleState() );
        };

        addAndMakeVisible ( translateLanguageLabel );
        translateLanguageLabel.setText ( "Target Language:", juce::dontSendNotification );
        translateLanguageLabel.setColour ( juce::Label::textColourId, juce::Colours::white );

        addAndMakeVisible ( translateLanguageCombo );
        translateLanguageCombo.addItem ( "System Default", 1 );
        translateLanguageCombo.addItem ( "English", 2 );
        translateLanguageCombo.addItem ( "French", 3 );
        translateLanguageCombo.addItem ( "German", 4 );
        translateLanguageCombo.addItem ( "Spanish", 5 );
        translateLanguageCombo.addItem ( "Italian", 6 );
        translateLanguageCombo.addItem ( "Portuguese", 7 );
        translateLanguageCombo.addItem ( "Dutch", 8 );
        translateLanguageCombo.addItem ( "Russian", 9 );
        translateLanguageCombo.addItem ( "Japanese", 10 );
        translateLanguageCombo.addItem ( "Korean", 11 );
        translateLanguageCombo.addItem ( "Chinese (Simplified)", 12 );
        translateLanguageCombo.addItem ( "Chinese (Traditional)", 13 );
        translateLanguageCombo.onChange = [this]() {
            if ( !processor )
                return;

            static const std::array<const char*, 13> codes { "system", "en", "fr", "de", "es", "it", "pt", "nl", "ru", "ja", "ko", "zh-cn", "zh-hant" };
            const int selected = juce::jlimit ( 1, static_cast<int> ( codes.size() ), translateLanguageCombo.getSelectedId() ) - 1;
            processor->setTranslateTargetLang ( codes[static_cast<size_t> ( selected )] );
        };

        loadCurrentValues();
        startTimerHz ( 5 );
    }

    ~AudioNetworkTab() override { stopTimer(); }

    void setTestToneEnabled ( bool enabled )
    {
        testToneToggle.setToggleState ( enabled, juce::dontSendNotification );
    }

    void resized() override
    {
        auto      bounds     = getLocalBounds().reduced ( 20 );
        const int rowHeight  = 34;
        const int labelWidth = 150;
        const int spacing    = 10;

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

        row = makeRow();
        newClientLevelLabel.setBounds ( row.removeFromLeft ( labelWidth ) );
        newClientLevelCombo.setBounds ( row );

        testToneToggle.setBounds ( makeRow() );

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

        auto smallBuffersRow = makeRow();
        smallBuffersToggle.setBounds ( smallBuffersRow.withHeight ( 30 ) );
        // autoLevelBoostToggle.setBounds ( makeRow() ); // Removed

        bounds.removeFromTop ( 10 );
        statusSectionLabel.setBounds ( makeRow() );
        delayLabel.setBounds ( makeRow() );
        uploadRateLabel.setBounds ( makeRow() );

        bounds.removeFromTop ( 10 );
        translateSectionLabel.setBounds ( makeRow() );
        autoTranslateToggle.setBounds ( makeRow() );
        row = makeRow();
        translateLanguageLabel.setBounds ( row.removeFromLeft ( labelWidth ) );
        translateLanguageCombo.setBounds ( row );
    }

private:
    void timerCallback() override
    {
        jamulus_network_stats_t stats{};
        bool                    haveStats = false;

        if ( processor )
            haveStats = processor->getNetworkStats ( stats );

        if ( haveStats && stats.total_delay_ms >= 0 )
        {
            delayLabel.setText ( "Overall Delay: " + juce::String ( stats.total_delay_ms ) + " ms", juce::dontSendNotification );
            juce::Colour delayColor;
            if ( stats.total_delay_ms <= 43 )
                delayColor = juce::Colour ( 0xff00cc66 );
            else if ( stats.total_delay_ms <= 68 )
                delayColor = juce::Colour ( 0xffffa500 );
            else
                delayColor = juce::Colours::red;
            delayLabel.setColour ( juce::Label::textColourId, delayColor );
        }
        else
        {
            delayLabel.setText ( "Overall Delay: --", juce::dontSendNotification );
            delayLabel.setColour ( juce::Label::textColourId, juce::Colours::white );
        }

        if ( haveStats && stats.upload_rate_kbps > 0 )
        {
            uploadRateLabel.setText ( "Upload Rate: " + juce::String ( stats.upload_rate_kbps ) + " kbps", juce::dontSendNotification );
            uploadRateLabel.setColour ( juce::Label::textColourId, juce::Colours::white );
        }
        else
        {
            uploadRateLabel.setText ( "Upload Rate: --", juce::dontSendNotification );
            uploadRateLabel.setColour ( juce::Label::textColourId, juce::Colours::grey );
        }

        bool autoJitter = processor ? processor->getAutoJitter() : true;

        if ( autoJitterToggle.getToggleState() != autoJitter )
            autoJitterToggle.setToggleState ( autoJitter, juce::dontSendNotification );

        if ( autoJitter )
        {
            jitterSlider.setValue ( processor ? processor->getJitterBuffer() : jitterSlider.getValue(), juce::dontSendNotification );
            const int serverJitter = processor ? processor->getServerJitterBuffer()
                                               : ( jamulusClient ? jamulus_client_get_server_jitter_buffer ( jamulusClient ) : static_cast<int> ( serverJitterSlider.getValue() ) );
            serverJitterSlider.setValue ( serverJitter, juce::dontSendNotification );

            if ( jitterSlider.isEnabled() )
                jitterSlider.setEnabled ( false );
            if ( serverJitterSlider.isEnabled() )
                serverJitterSlider.setEnabled ( false );
        }
        else
        {
            if ( !jitterSlider.isEnabled() )
                jitterSlider.setEnabled ( true );
            if ( !serverJitterSlider.isEnabled() )
                serverJitterSlider.setEnabled ( true );
        }
    }

    void loadCurrentValues()
    {
        if ( jamulusClient )
        {
            qualityCombo.setSelectedId ( jamulus_client_get_audio_quality ( jamulusClient ) + 1, juce::dontSendNotification );
            channelsCombo.setSelectedId ( jamulus_client_get_audio_channels ( jamulusClient ) + 1, juce::dontSendNotification );
            inputBoostCombo.setSelectedId ( jamulus_client_get_input_boost ( jamulusClient ), juce::dontSendNotification );
        }

        bool autoJitter = processor ? processor->getAutoJitter() : true;
        autoJitterToggle.setToggleState ( autoJitter, juce::dontSendNotification );

        jitterSlider.setValue ( processor ? processor->getJitterBuffer() : jitterSlider.getValue(), juce::dontSendNotification );
        const int serverJitter = processor ? processor->getServerJitterBuffer()
                                           : ( jamulusClient ? jamulus_client_get_server_jitter_buffer ( jamulusClient ) : static_cast<int> ( serverJitterSlider.getValue() ) );
        serverJitterSlider.setValue ( serverJitter, juce::dontSendNotification );

        jitterSlider.setEnabled ( !autoJitter );
        serverJitterSlider.setEnabled ( !autoJitter );

        if ( jamulusClient )
        {
            int cur = jamulus_client_get_new_client_level ( jamulusClient );
            int sel = 50;
            int dif = std::abs ( cur - 50 );
            auto trySel = [&] ( int v ) {
                int d = std::abs ( cur - v );
                if ( d < dif )
                {
                    dif = d;
                    sel = v;
                }
            };
            trySel ( 100 );
            trySel ( 71 );
            trySel ( 35 );
            trySel ( 25 );
            newClientLevelCombo.setSelectedId ( sel, juce::dontSendNotification );
        }

        bool smallBuffersEnabled = false;
        if ( processor )
            smallBuffersEnabled = processor->getSmallBuffers();
        else if ( jamulusClient )
            smallBuffersEnabled = jamulus_client_get_small_buffers ( jamulusClient );
        smallBuffersToggle.setToggleState ( smallBuffersEnabled, juce::dontSendNotification );

        const bool autoTranslateEnabled = processor ? processor->isAutoTranslateEnabled() : false;
        autoTranslateToggle.setToggleState ( autoTranslateEnabled, juce::dontSendNotification );

        const juce::String targetCode = processor ? processor->getTranslateTargetLang().trim().toLowerCase() : "system";
        int targetId                  = 1;
        if ( targetCode == "en" )
            targetId = 2;
        else if ( targetCode == "fr" )
            targetId = 3;
        else if ( targetCode == "de" )
            targetId = 4;
        else if ( targetCode == "es" )
            targetId = 5;
        else if ( targetCode == "it" )
            targetId = 6;
        else if ( targetCode == "pt" )
            targetId = 7;
        else if ( targetCode == "nl" )
            targetId = 8;
        else if ( targetCode == "ru" )
            targetId = 9;
        else if ( targetCode == "ja" )
            targetId = 10;
        else if ( targetCode == "ko" )
            targetId = 11;
        else if ( targetCode == "zh-cn" )
            targetId = 12;
        else if ( targetCode == "zh-hant" )
            targetId = 13;
        translateLanguageCombo.setSelectedId ( targetId, juce::dontSendNotification );
    }

    jamulus_client_t       jamulusClient;
    JamulusPluginProcessor* processor;

    juce::Label    audioSectionLabel;
    juce::Label    qualityLabel;
    juce::ComboBox qualityCombo;
    juce::Label    channelsLabel;
    juce::ComboBox channelsCombo;
    juce::Label    inputBoostLabel;
    juce::ComboBox inputBoostCombo;
    juce::ToggleButton testToneToggle;

    juce::Label        networkSectionLabel;
    juce::Label        jitterLabel;
    juce::ToggleButton autoJitterToggle;
    juce::Slider       jitterSlider;
    juce::Label        serverJitterLabel;
    juce::Slider       serverJitterSlider;

    juce::Label        newClientLevelLabel;
    juce::ComboBox     newClientLevelCombo;
    juce::ToggleButton smallBuffersToggle;
    // juce::ToggleButton autoLevelBoostToggle; // Removed

    juce::Label statusSectionLabel;
    juce::Label delayLabel;
    juce::Label uploadRateLabel;
    juce::Label        translateSectionLabel;
    juce::ToggleButton autoTranslateToggle;
    juce::Label        translateLanguageLabel;
    juce::ComboBox     translateLanguageCombo;

public:
    std::function<void ( bool )> onTestToneChanged;
    // Callback for auto level boost setting - Removed
    // std::function<void ( bool )> onAutoLevelBoostChanged;
};

//==============================================================================
// Advanced Tab
//==============================================================================
class AdvancedTab : public juce::Component
{
public:
    AdvancedTab ( jamulus_client_t client ) : jamulusClient ( client )
    {
        addAndMakeVisible ( feedbackToggle );
        feedbackToggle.setButtonText ( "Enable Feedback Detection" );
        feedbackToggle.setColour ( juce::ToggleButton::textColourId, juce::Colours::white );
        feedbackToggle.onClick = [this]() {
            if ( jamulusClient )
                jamulus_client_set_feedback_detection ( jamulusClient, feedbackToggle.getToggleState() );
        };

        addAndMakeVisible ( audioAlertsToggle );
        audioAlertsToggle.setButtonText ( "Enable Audio Alerts" );
        audioAlertsToggle.setColour ( juce::ToggleButton::textColourId, juce::Colours::white );
        audioAlertsToggle.onClick = [this]() {
            if ( jamulusClient )
                jamulus_client_set_audio_alerts ( jamulusClient, audioAlertsToggle.getToggleState() );
        };

        addAndMakeVisible ( mixerRowsLabel );
        mixerRowsLabel.setText ( "Mixer Rows:", juce::dontSendNotification );
        mixerRowsLabel.setColour ( juce::Label::textColourId, juce::Colours::white );

        addAndMakeVisible ( mixerRowsCombo );
        for ( int i = 1; i <= 8; ++i )
            mixerRowsCombo.addItem ( juce::String ( i ), i );
        mixerRowsCombo.setSelectedId ( 1 );
        mixerRowsCombo.onChange = [this]() {
            int rows = mixerRowsCombo.getSelectedId();
            if ( rows < 1 )
                rows = 1;
            if ( rows > 8 )
                rows = 8;
            if ( onMixerRowsChanged )
                onMixerRowsChanged ( rows );
        };

        addAndMakeVisible ( mixerSortLabel );
        mixerSortLabel.setText ( "Mixer Order:", juce::dontSendNotification );
        mixerSortLabel.setColour ( juce::Label::textColourId, juce::Colours::white );

        addAndMakeVisible ( mixerSortCombo );
        mixerSortCombo.addItem ( "Stable", 1 );
        mixerSortCombo.addItem ( "Sort by Channel", 2 );
        mixerSortCombo.setSelectedId ( 1 );
        mixerSortCombo.onChange = [this]() {
            if ( onMixerSortModeChanged )
                onMixerSortModeChanged ( mixerSortCombo.getSelectedId() - 1 );
        };

        addAndMakeVisible ( midiInputLabel );
        midiInputLabel.setText ( "MIDI Input Device:", juce::dontSendNotification );
        midiInputLabel.setColour ( juce::Label::textColourId, juce::Colours::white );

        addAndMakeVisible ( midiInputCombo );
        midiInputCombo.onChange = [this]() {
            if ( onMidiInputDeviceChanged )
                onMidiInputDeviceChanged ( midiInputCombo.getText() );
        };

        addAndMakeVisible ( midiOutputLabel );
        midiOutputLabel.setText ( "MIDI Output Device:", juce::dontSendNotification );
        midiOutputLabel.setColour ( juce::Label::textColourId, juce::Colours::white );

        addAndMakeVisible ( midiOutputCombo );
        midiOutputCombo.onChange = [this]() {
            if ( onMidiOutputDeviceChanged )
                onMidiOutputDeviceChanged ( midiOutputCombo.getText() );
        };

        addAndMakeVisible ( themeLabel );
        themeLabel.setText ( "Theme:", juce::dontSendNotification );
        themeLabel.setColour ( juce::Label::textColourId, juce::Colours::white );

        addAndMakeVisible ( themeCombo );
        themeCombo.onChange = [this]() {
            if ( onThemeChanged )
                onThemeChanged ( themeCombo.getText() );
        };

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
        bounds.removeFromTop ( spacing );

        auto row = bounds.removeFromTop ( rowHeight );
        bounds.removeFromTop ( spacing * 2 );
        mixerRowsLabel.setBounds ( row.removeFromLeft ( 120 ) );
        mixerRowsCombo.setBounds ( row );

        row = bounds.removeFromTop ( rowHeight );
        bounds.removeFromTop ( spacing * 2 );
        mixerSortLabel.setBounds ( row.removeFromLeft ( 120 ) );
        mixerSortCombo.setBounds ( row );

        row = bounds.removeFromTop ( rowHeight );
        bounds.removeFromTop ( spacing );
        midiInputLabel.setBounds ( row.removeFromLeft ( 120 ) );
        midiInputCombo.setBounds ( row );

        row = bounds.removeFromTop ( rowHeight );
        bounds.removeFromTop ( spacing * 2 );
        midiOutputLabel.setBounds ( row.removeFromLeft ( 120 ) );
        midiOutputCombo.setBounds ( row );

        row = bounds.removeFromTop ( rowHeight );
        bounds.removeFromTop ( spacing * 2 );
        themeLabel.setBounds ( row.removeFromLeft ( 120 ) );
        themeCombo.setBounds ( row );

        bufferInfoLabel.setBounds ( bounds.removeFromTop ( 40 ) );
    }

    void setMixerRows ( int rows )
    {
        rows = juce::jlimit ( 1, 8, rows );
        mixerRowsCombo.setSelectedId ( rows, juce::dontSendNotification );
    }

    void setMixerSortMode ( int mode )
    {
        mixerSortCombo.setSelectedId ( juce::jlimit ( 1, 2, mode + 1 ), juce::dontSendNotification );
    }

    void setMidiDevices ( const juce::StringArray& inputNames,
                          const juce::String&      selectedInput,
                          const juce::StringArray& outputNames,
                          const juce::String&      selectedOutput )
    {
        midiInputCombo.clear ( juce::dontSendNotification );
        midiInputCombo.addItem ( "None", 1 );
        int itemId = 2;
        int selectedId = 1;
        for ( const auto& name : inputNames )
        {
            midiInputCombo.addItem ( name, itemId );
            if ( name == selectedInput )
                selectedId = itemId;
            ++itemId;
        }
        midiInputCombo.setSelectedId ( selectedId, juce::dontSendNotification );

        midiOutputCombo.clear ( juce::dontSendNotification );
        midiOutputCombo.addItem ( "None", 1 );
        itemId = 2;
        selectedId = 1;
        for ( const auto& name : outputNames )
        {
            midiOutputCombo.addItem ( name, itemId );
            if ( name == selectedOutput )
                selectedId = itemId;
            ++itemId;
        }
        midiOutputCombo.setSelectedId ( selectedId, juce::dontSendNotification );
    }

    void setThemes ( const juce::StringArray& themeNames, const juce::String& selectedTheme )
    {
        themeCombo.clear ( juce::dontSendNotification );

        int itemId = 1;
        int selectedId = 1;
        for ( const auto& name : themeNames )
        {
            themeCombo.addItem ( name, itemId );
            if ( name == selectedTheme )
                selectedId = itemId;
            ++itemId;
        }

        if ( themeCombo.getNumItems() == 0 )
            themeCombo.addItem ( "Default", 1 );

        themeCombo.setSelectedId ( selectedId, juce::dontSendNotification );
    }

    std::function<void ( int )> onMixerRowsChanged;
    std::function<void ( int )> onMixerSortModeChanged;
    std::function<void ( const juce::String& )> onMidiInputDeviceChanged;
    std::function<void ( const juce::String& )> onMidiOutputDeviceChanged;
    std::function<void ( const juce::String& )> onThemeChanged;

private:
    jamulus_client_t jamulusClient;

    juce::ToggleButton feedbackToggle;
    juce::ToggleButton audioAlertsToggle;
    juce::Label        mixerRowsLabel;
    juce::ComboBox     mixerRowsCombo;
    juce::Label        mixerSortLabel;
    juce::ComboBox     mixerSortCombo;
    juce::Label        midiInputLabel;
    juce::ComboBox     midiInputCombo;
    juce::Label        midiOutputLabel;
    juce::ComboBox     midiOutputCombo;
    juce::Label        themeLabel;
    juce::ComboBox     themeCombo;
    juce::Label        bufferInfoLabel;
};

//==============================================================================
class RecordingTab : public juce::Component
{
public:
    RecordingTab()
    {
        addAndMakeVisible ( modeLabel );
        modeLabel.setText ( "Record Format:", juce::dontSendNotification );
        modeLabel.setColour ( juce::Label::textColourId, juce::Colours::white );

        addAndMakeVisible ( modeCombo );
        modeCombo.addItem ( "Off", JamulusPluginProcessor::RecordingSettings::recordingOff + 1 );
        modeCombo.addItem ( "Stereo WAV", JamulusPluginProcessor::RecordingSettings::stereoWav + 1 );
        modeCombo.addItem ( "Stereo OGG", JamulusPluginProcessor::RecordingSettings::stereoOgg + 1 );
        modeCombo.addItem ( "Stereo MP3", JamulusPluginProcessor::RecordingSettings::stereoMp3 + 1 );
        modeCombo.addItem ( "Multi-Stem MOGG", JamulusPluginProcessor::RecordingSettings::multitrackMogg + 1 );
        modeCombo.onChange = [this]() { emitSettingsChanged(); };

        addAndMakeVisible ( folderLabel );
        folderLabel.setText ( "Folder:", juce::dontSendNotification );
        folderLabel.setColour ( juce::Label::textColourId, juce::Colours::white );

        addAndMakeVisible ( folderEditor );
        folderEditor.onTextChange = [this]() { emitSettingsChanged(); };

        addAndMakeVisible ( browseButton );
        browseButton.setButtonText ( "Browse" );
        browseButton.onClick = [this]() {
            folderChooser = std::make_unique<juce::FileChooser> ( "Select recording folder",
                                                                  juce::File ( folderEditor.getText().isNotEmpty() ? folderEditor.getText()
                                                                                                                    : juce::File::getSpecialLocation ( juce::File::userDocumentsDirectory ).getFullPathName() ),
                                                                  juce::String() );
            folderChooser->launchAsync ( juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectDirectories,
                                         [this] ( const juce::FileChooser& chooser ) {
                                             const auto result = chooser.getResult();
                                             if ( result.exists() )
                                             {
                                                 folderEditor.setText ( result.getFullPathName(), juce::sendNotification );
                                                 emitSettingsChanged();
                                             }
                                             folderChooser.reset();
                                         } );
        };

        addAndMakeVisible ( stereoAutoLevelToggle );
        stereoAutoLevelToggle.setButtonText ( "Stereo record follows live auto level" );
        stereoAutoLevelToggle.setColour ( juce::ToggleButton::textColourId, juce::Colours::white );
        stereoAutoLevelToggle.onClick = [this]() { emitSettingsChanged(); };

        addAndMakeVisible ( stereoLimiterToggle );
        stereoLimiterToggle.setButtonText ( "Stereo record applies limiter" );
        stereoLimiterToggle.setColour ( juce::ToggleButton::textColourId, juce::Colours::white );
        stereoLimiterToggle.onClick = [this]() { emitSettingsChanged(); };

        addAndMakeVisible ( stemsAutoLevelToggle );
        stemsAutoLevelToggle.setButtonText ( "Stem record follows live auto level/faders" );
        stemsAutoLevelToggle.setColour ( juce::ToggleButton::textColourId, juce::Colours::white );
        stemsAutoLevelToggle.onClick = [this]() { emitSettingsChanged(); };

        addAndMakeVisible ( noteLabel );
        noteLabel.setText ( "MP3 needs lame.exe available to the plugin. MOGG writes one stereo pair per remote user at record start.", juce::dontSendNotification );
        noteLabel.setColour ( juce::Label::textColourId, juce::Colours::grey );
        noteLabel.setFont ( juce::Font ( 12.0f ) );
    }

    void resized() override
    {
        auto bounds = getLocalBounds().reduced ( 20 );
        const int rowHeight = 30;
        const int spacing = 10;

        auto row = bounds.removeFromTop ( rowHeight );
        modeLabel.setBounds ( row.removeFromLeft ( 130 ) );
        modeCombo.setBounds ( row );
        bounds.removeFromTop ( spacing );

        row = bounds.removeFromTop ( rowHeight );
        folderLabel.setBounds ( row.removeFromLeft ( 130 ) );
        browseButton.setBounds ( row.removeFromRight ( 90 ) );
        row.removeFromRight ( 8 );
        folderEditor.setBounds ( row );
        bounds.removeFromTop ( spacing * 2 );

        stereoAutoLevelToggle.setBounds ( bounds.removeFromTop ( rowHeight ) );
        bounds.removeFromTop ( spacing );
        stereoLimiterToggle.setBounds ( bounds.removeFromTop ( rowHeight ) );
        bounds.removeFromTop ( spacing );
        stemsAutoLevelToggle.setBounds ( bounds.removeFromTop ( rowHeight ) );
        bounds.removeFromTop ( spacing * 2 );
        noteLabel.setBounds ( bounds.removeFromTop ( 44 ) );
    }

    void setRecordingSettings ( const JamulusPluginProcessor::RecordingSettings& settings )
    {
        modeCombo.setSelectedId ( settings.mode + 1, juce::dontSendNotification );
        folderEditor.setText ( settings.folderPath, juce::dontSendNotification );
        stereoAutoLevelToggle.setToggleState ( settings.stereoUseLiveAutoLevel, juce::dontSendNotification );
        stereoLimiterToggle.setToggleState ( settings.stereoApplyLimiter, juce::dontSendNotification );
        stemsAutoLevelToggle.setToggleState ( settings.stemsUseLiveAutoLevel, juce::dontSendNotification );
    }

    JamulusPluginProcessor::RecordingSettings getRecordingSettings() const
    {
        JamulusPluginProcessor::RecordingSettings settings;
        settings.mode = juce::jmax ( 0, modeCombo.getSelectedId() - 1 );
        settings.folderPath = folderEditor.getText().trim();
        settings.stereoUseLiveAutoLevel = stereoAutoLevelToggle.getToggleState();
        settings.stereoApplyLimiter = stereoLimiterToggle.getToggleState();
        settings.stemsUseLiveAutoLevel = stemsAutoLevelToggle.getToggleState();
        return settings;
    }

    std::function<void ( const JamulusPluginProcessor::RecordingSettings& )> onRecordingSettingsChanged;

private:
    void emitSettingsChanged()
    {
        if ( onRecordingSettingsChanged )
            onRecordingSettingsChanged ( getRecordingSettings() );
    }

    juce::Label modeLabel;
    juce::ComboBox modeCombo;
    juce::Label folderLabel;
    juce::TextEditor folderEditor;
    juce::TextButton browseButton;
    juce::ToggleButton stereoAutoLevelToggle;
    juce::ToggleButton stereoLimiterToggle;
    juce::ToggleButton stemsAutoLevelToggle;
    juce::Label noteLabel;
    std::unique_ptr<juce::FileChooser> folderChooser;
};

//==============================================================================
// Main Settings Component
//==============================================================================
class SettingsComponent : public juce::Component
{
public:
    SettingsComponent ( jamulus_client_t client, JamulusPluginProcessor* proc ) :
        jamulusClient ( client ),
        profileTab ( client ),
        audioNetworkTab ( client, proc ),
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
        tabbedComponent.addTab ( "Recording", juce::Colour ( 0xff3a3a3a ), &recordingTab, false );

        advancedTab.onMixerRowsChanged = [this] ( int rows ) {
            if ( onMixerRowsChanged )
                onMixerRowsChanged ( rows );
        };
        advancedTab.onMixerSortModeChanged = [this] ( int mode ) {
            if ( onMixerSortModeChanged )
                onMixerSortModeChanged ( mode );
        };

        addAndMakeVisible ( closeButton );
        closeButton.setButtonText ( "Close" );
        closeButton.onClick = [this]() {
            if ( onClose )
                onClose();
        };
    }

    void setProfileCallbacks ( std::function<juce::String()> getNameCb,
                               std::function<void ( const juce::String& )> setNameCb,
                               std::function<int()> getInstrumentCb,
                               std::function<void ( int )> setInstrumentCb,
                               std::function<int()> getCountryCb,
                               std::function<void ( int )> setCountryCb,
                               std::function<juce::String()> getCountryCodeCb,
                               std::function<juce::String()> getCityCb,
                               std::function<void ( const juce::String& )> setCityCb,
                               std::function<int()> getSkillCb,
                               std::function<void ( int )> setSkillCb )
    {
        profileTab.setProfileCallbacks ( std::move ( getNameCb ),
                                         std::move ( setNameCb ),
                                         std::move ( getInstrumentCb ),
                                         std::move ( setInstrumentCb ),
                                         std::move ( getCountryCb ),
                                         std::move ( setCountryCb ),
                                         std::move ( getCountryCodeCb ),
                                         std::move ( getCityCb ),
                                         std::move ( setCityCb ),
                                         std::move ( getSkillCb ),
                                         std::move ( setSkillCb ) );
    }

    void setProfileChangedCallback ( std::function<void()> cb ) { profileTab.setProfileChangedCallback ( std::move ( cb ) ); }
    void setMidiDeviceCallbacks ( std::function<juce::StringArray()> getMidiInputNamesCb,
                                  std::function<juce::String()>      getSelectedMidiInputCb,
                                  std::function<void ( const juce::String& )> setSelectedMidiInputCb,
                                  std::function<juce::StringArray()> getMidiOutputNamesCb,
                                  std::function<juce::String()>      getSelectedMidiOutputCb,
                                  std::function<void ( const juce::String& )> setSelectedMidiOutputCb )
    {
        if ( getMidiInputNamesCb && getSelectedMidiInputCb && getMidiOutputNamesCb && getSelectedMidiOutputCb )
        {
            advancedTab.setMidiDevices ( getMidiInputNamesCb(),
                                         getSelectedMidiInputCb(),
                                         getMidiOutputNamesCb(),
                                         getSelectedMidiOutputCb() );
        }

        advancedTab.onMidiInputDeviceChanged  = std::move ( setSelectedMidiInputCb );
        advancedTab.onMidiOutputDeviceChanged = std::move ( setSelectedMidiOutputCb );
    }

    void setThemeCallbacks ( std::function<juce::StringArray()> getThemeNamesCb,
                             std::function<juce::String()>      getSelectedThemeCb,
                             std::function<void ( const juce::String& )> setSelectedThemeCb )
    {
        if ( getThemeNamesCb && getSelectedThemeCb )
            advancedTab.setThemes ( getThemeNamesCb(), getSelectedThemeCb() );

        advancedTab.onThemeChanged = std::move ( setSelectedThemeCb );
    }

    void setRecordingSettings ( const JamulusPluginProcessor::RecordingSettings& settings )
    {
        recordingTab.setRecordingSettings ( settings );
    }

    void setRecordingSettingsCallback ( std::function<void ( const JamulusPluginProcessor::RecordingSettings& )> cb )
    {
        recordingTab.onRecordingSettingsChanged = std::move ( cb );
    }

    void setTestToneCallback ( std::function<void ( bool )> cb )
    {
        audioNetworkTab.onTestToneChanged = std::move ( cb );
    }

    void setTestToneEnabled ( bool enabled )
    {
        audioNetworkTab.setTestToneEnabled ( enabled );
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

    std::function<void()>       onClose;
    std::function<void ( int )> onMixerRowsChanged;
    std::function<void ( int )> onMixerSortModeChanged;

    void setMixerRows ( int rows ) { advancedTab.setMixerRows ( rows ); }
    void setMixerSortMode ( int mode ) { advancedTab.setMixerSortMode ( mode ); }

    // Pass through callback for auto level boost setting - Removed
    // void setAutoLevelBoostCallback ( std::function<void ( bool )> cb ) { audioNetworkTab.onAutoLevelBoostChanged = cb; }

private:
    jamulus_client_t jamulusClient;

    juce::Label           titleLabel;
    juce::TabbedComponent tabbedComponent{ juce::TabbedButtonBar::TabsAtTop };
    ProfileTab            profileTab;
    AudioNetworkTab       audioNetworkTab;
    AdvancedTab           advancedTab;
    RecordingTab          recordingTab;
    juce::TextButton      closeButton;
};
