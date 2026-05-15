#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../jamulus_wrapper.h"

/**
 * FX Panel Component - Contains Reverb and Delay controls
 *
 * Reverb Controls:
 *  - Level (wet/dry mix)
 *  - Time (T60 decay time)
 *  - Pre-delay
 *  - High-pass filter
 *  - Low-pass filter
 *  - Channel (Left/Right/Both)
 *
 * Delay Controls:
 *  - Level (wet/dry mix)
 *  - Time (delay in ms)
 *  - Feedback
 *  - Ping-pong mode
 *  - High-pass filter
 */
class FXPanelComponent : public juce::Component
{
public:
    FXPanelComponent ( jamulus_client_t client ) : jamulusClient ( client )
    {
        // Reverb Enable - Removed per user request
        /*
        addAndMakeVisible ( reverbEnableButton );
        reverbEnableButton.setButtonText ( "Enable" );
        reverbEnableButton.setClickingTogglesState ( true );
        reverbEnableButton.setToggleState ( false, juce::dontSendNotification ); // Default enabled
        reverbEnableButton.onClick = [this]() {
            bool enabled = reverbEnableButton.getToggleState();
            if ( onReverbEnableChanged )
                onReverbEnableChanged ( enabled );

            // Toggle Logic: 0 if disabled, current slider value if enabled
            int levelToSend = enabled ? static_cast<int> ( reverbLevelSlider.getValue() ) : 0;
            jamulus_client_set_reverb_level ( jamulusClient, levelToSend );

            updateReverbControlsEnabled();
        };
        */
        // === Header ===
        addAndMakeVisible ( titleLabel );
        titleLabel.setText ( "Effects", juce::dontSendNotification );
        titleLabel.setFont ( juce::Font ( 18.0f, juce::Font::bold ) );
        titleLabel.setColour ( juce::Label::textColourId, juce::Colours::white );

        addAndMakeVisible ( closeButton );
        closeButton.setButtonText ( "X" );
        closeButton.onClick = [this]() {
            if ( onClose )
                onClose();
        };

        // === REVERB SECTION (Custom low-latency reverb) ===
        addAndMakeVisible ( reverbSectionLabel );
        reverbSectionLabel.setText ( "REVERB", juce::dontSendNotification );
        reverbSectionLabel.setFont ( juce::Font ( 14.0f, juce::Font::bold ) );
        reverbSectionLabel.setColour ( juce::Label::textColourId, juce::Colour ( 0xff00cc66 ) );

        // Reverb Enable
        addAndMakeVisible ( reverbEnableButton );
        reverbEnableButton.setButtonText ( "Enable" );
        reverbEnableButton.setClickingTogglesState ( true );
        reverbEnableButton.onClick = [this]() {
            if ( onReverbEnableChanged )
                onReverbEnableChanged ( reverbEnableButton.getToggleState() );
            updateReverbControlsEnabled();
        };

        // Reverb Level (wet/dry mix)
        addAndMakeVisible ( reverbLevelLabel );
        reverbLevelLabel.setText ( "Mix", juce::dontSendNotification );
        reverbLevelLabel.setColour ( juce::Label::textColourId, juce::Colours::white );

        addAndMakeVisible ( reverbLevelSlider );
        reverbLevelSlider.setComponentID ( "fx_reverb_mix" );
        reverbLevelSlider.getProperties().set ( "learnLabel", "Reverb Mix" );
        setupSlider ( reverbLevelSlider, 0, 100, 30, "%" );
        reverbLevelSlider.onValueChange = [this]() {
            if ( onReverbMixChanged )
                onReverbMixChanged ( static_cast<float> ( reverbLevelSlider.getValue() ) / 100.0f );
        };

        // Reverb Decay Time
        addAndMakeVisible ( reverbDecayLabel );
        reverbDecayLabel.setText ( "Decay", juce::dontSendNotification );
        reverbDecayLabel.setColour ( juce::Label::textColourId, juce::Colours::white );

        addAndMakeVisible ( reverbDecaySlider );
        reverbDecaySlider.setComponentID ( "fx_reverb_decay" );
        reverbDecaySlider.getProperties().set ( "learnLabel", "Reverb Decay" );
        setupSlider ( reverbDecaySlider, 0.1, 5.0, 1.5, " s" );
        reverbDecaySlider.setNumDecimalPlacesToDisplay ( 1 );
        reverbDecaySlider.onValueChange = [this]() {
            if ( onReverbDecayChanged )
                onReverbDecayChanged ( static_cast<float> ( reverbDecaySlider.getValue() ) );
        };

        // Initial state
        updateReverbControlsEnabled();

        // === DELAY SECTION ===
        addAndMakeVisible ( delaySectionLabel );
        delaySectionLabel.setText ( "DELAY", juce::dontSendNotification );
        delaySectionLabel.setFont ( juce::Font ( 14.0f, juce::Font::bold ) );
        delaySectionLabel.setColour ( juce::Label::textColourId, juce::Colour ( 0xff6699ff ) );

        // Delay Enable
        addAndMakeVisible ( delayEnableButton );
        delayEnableButton.setButtonText ( "Enable" );
        delayEnableButton.setClickingTogglesState ( true );
        delayEnableButton.onClick = [this]() {
            if ( onDelayEnableChanged )
                onDelayEnableChanged ( delayEnableButton.getToggleState() );
            updateDelayControlsEnabled();
        };

        // Delay Level
        addAndMakeVisible ( delayLevelLabel );
        delayLevelLabel.setText ( "Level", juce::dontSendNotification );
        delayLevelLabel.setColour ( juce::Label::textColourId, juce::Colours::white );

        addAndMakeVisible ( delayLevelSlider );
        delayLevelSlider.setComponentID ( "fx_delay_level" );
        delayLevelSlider.getProperties().set ( "learnLabel", "Delay Level" );
        setupSlider ( delayLevelSlider, 0, 100, 30, "%" );
        delayLevelSlider.onValueChange = [this]() {
            if ( onDelayLevelChanged )
                onDelayLevelChanged ( static_cast<float> ( delayLevelSlider.getValue() ) / 100.0f );
        };

        // Delay Time
        addAndMakeVisible ( delayTimeLabel );
        delayTimeLabel.setText ( "Time", juce::dontSendNotification );
        delayTimeLabel.setColour ( juce::Label::textColourId, juce::Colours::white );

        addAndMakeVisible ( delayTimeSlider );
        delayTimeSlider.setComponentID ( "fx_delay_time" );
        delayTimeSlider.getProperties().set ( "learnLabel", "Delay Time" );
        setupSlider ( delayTimeSlider, 1, 1000, 250, " ms" );
        delayTimeSlider.setSkewFactorFromMidPoint ( 200 );
        delayTimeSlider.onValueChange = [this]() {
            if ( onDelayTimeChanged )
                onDelayTimeChanged ( static_cast<float> ( delayTimeSlider.getValue() ) );
        };

        // Feedback
        addAndMakeVisible ( feedbackLabel );
        feedbackLabel.setText ( "Feedback", juce::dontSendNotification );
        feedbackLabel.setColour ( juce::Label::textColourId, juce::Colours::white );

        addAndMakeVisible ( feedbackSlider );
        feedbackSlider.setComponentID ( "fx_delay_feedback" );
        feedbackSlider.getProperties().set ( "learnLabel", "Delay Feedback" );
        setupSlider ( feedbackSlider, 0, 95, 30, "%" );
        feedbackSlider.onValueChange = [this]() {
            if ( onDelayFeedbackChanged )
                onDelayFeedbackChanged ( static_cast<float> ( feedbackSlider.getValue() ) / 100.0f );
        };

        // Ping-pong
        addAndMakeVisible ( pingPongButton );
        pingPongButton.setButtonText ( "Ping-Pong" );
        pingPongButton.setClickingTogglesState ( true );
        pingPongButton.onClick = [this]() {
            if ( onDelayPingPongChanged )
                onDelayPingPongChanged ( pingPongButton.getToggleState() );
        };

        // Delay HP Filter
        addAndMakeVisible ( delayHPLabel );
        delayHPLabel.setText ( "HP Filter", juce::dontSendNotification );
        delayHPLabel.setColour ( juce::Label::textColourId, juce::Colours::white );

        addAndMakeVisible ( delayHPSlider );
        delayHPSlider.setComponentID ( "fx_delay_hp" );
        delayHPSlider.getProperties().set ( "learnLabel", "Delay HP Filter" );
        setupSlider ( delayHPSlider, 20, 1000, 100, " Hz" );
        delayHPSlider.setSkewFactorFromMidPoint ( 150 );
        delayHPSlider.onValueChange = [this]() {
            if ( onDelayHPChanged )
                onDelayHPChanged ( static_cast<float> ( delayHPSlider.getValue() ) );
        };

        // Initial state
        updateDelayControlsEnabled();
    }

    void paint ( juce::Graphics& g ) override
    {
        // Panel background
        g.setColour ( juce::Colour ( 0xff2d2d2d ) );
        g.fillRoundedRectangle ( getLocalBounds().toFloat(), 8.0f );

        // Border
        g.setColour ( juce::Colour ( 0xff404040 ) );
        g.drawRoundedRectangle ( getLocalBounds().toFloat().reduced ( 1 ), 8.0f, 2.0f );

        // Section divider
        auto bounds   = getLocalBounds().reduced ( 10 );
        int  dividerY = bounds.getY() + 200;
        g.setColour ( juce::Colour ( 0xff404040 ) );
        g.drawLine ( bounds.getX() + 10, dividerY, bounds.getRight() - 10, dividerY, 1.0f );
    }

    void resized() override
    {
        auto bounds = getLocalBounds().reduced ( 10 );

        // Header
        auto header = bounds.removeFromTop ( 30 );
        closeButton.setBounds ( header.removeFromRight ( 30 ) );
        titleLabel.setBounds ( header );

        bounds.removeFromTop ( 10 );

        // === REVERB SECTION ===
        auto reverbHeader = bounds.removeFromTop ( 25 );
        reverbSectionLabel.setBounds ( reverbHeader.removeFromLeft ( 100 ) );
        reverbEnableButton.setBounds ( reverbHeader.removeFromRight ( 80 ) );
        bounds.removeFromTop ( 5 );

        auto reverbRow1 = bounds.removeFromTop ( 30 );
        reverbLevelLabel.setBounds ( reverbRow1.removeFromLeft ( 70 ) );
        reverbLevelSlider.setBounds ( reverbRow1 );

        bounds.removeFromTop ( 5 );

        auto reverbRow2 = bounds.removeFromTop ( 30 );
        reverbDecayLabel.setBounds ( reverbRow2.removeFromLeft ( 70 ) );
        reverbDecaySlider.setBounds ( reverbRow2 );

        bounds.removeFromTop ( 15 );

        // === DELAY SECTION ===
        auto delayHeader = bounds.removeFromTop ( 25 );
        delaySectionLabel.setBounds ( delayHeader.removeFromLeft ( 100 ) );
        delayEnableButton.setBounds ( delayHeader.removeFromRight ( 80 ) );

        bounds.removeFromTop ( 5 );

        auto drow1 = bounds.removeFromTop ( 30 );
        delayLevelLabel.setBounds ( drow1.removeFromLeft ( 70 ) );
        delayLevelSlider.setBounds ( drow1 );

        bounds.removeFromTop ( 5 );

        auto drow2 = bounds.removeFromTop ( 30 );
        delayTimeLabel.setBounds ( drow2.removeFromLeft ( 70 ) );
        delayTimeSlider.setBounds ( drow2 );

        bounds.removeFromTop ( 5 );

        auto drow3 = bounds.removeFromTop ( 30 );
        feedbackLabel.setBounds ( drow3.removeFromLeft ( 70 ) );
        feedbackSlider.setBounds ( drow3 );

        bounds.removeFromTop ( 5 );

        auto drow4 = bounds.removeFromTop ( 30 );
        delayHPLabel.setBounds ( drow4.removeFromLeft ( 70 ) );
        delayHPSlider.setBounds ( drow4 );

        bounds.removeFromTop ( 5 );

        auto drow5 = bounds.removeFromTop ( 30 );
        pingPongButton.setBounds ( drow5.removeFromLeft ( 120 ) );
    }

    // Load current values from client
    void loadCurrentValues()
    {
        if ( !jamulusClient )
            return;

        /*
        reverbLevelSlider.setValue ( jamulus_client_get_reverb_level ( jamulusClient ), juce::dontSendNotification );
        */
        // Default to Stereo
        /*
        reverbChannelCombo.setSelectedId ( 1, juce::dontSendNotification );
        */
    }

    // Callbacks
    std::function<void()> onClose;

    // Set constraint from Mono mode
    void setMonoConstraint ( bool isMono )
    {
        /*
        if ( isMono )
        {
            // Force Stereo (ID 1) and Disable
            reverbChannelCombo.setSelectedId ( 1, juce::dontSendNotification );
            reverbChannelCombo.setEnabled ( false );

            // Should also ensure the client state is updated
            if ( reverbChannelCombo.onChange )
                reverbChannelCombo.onChange();
        }
        else
        {
            // Enable if Reverb is enabled
            reverbChannelCombo.setEnabled ( reverbEnableButton.getToggleState() );
        }
        */
    }

    // Reverb callbacks
    std::function<void ( bool )>  onReverbEnableChanged;
    std::function<void ( float )> onReverbMixChanged;
    std::function<void ( float )> onReverbDecayChanged;

    // Delay callbacks
    std::function<void ( bool )>  onDelayEnableChanged;
    std::function<void ( float )> onDelayLevelChanged;
    std::function<void ( float )> onDelayTimeChanged;
    std::function<void ( float )> onDelayFeedbackChanged;
    std::function<void ( bool )>  onDelayPingPongChanged;
    std::function<void ( float )> onDelayHPChanged;

private:
    void setupSlider ( juce::Slider& slider, double min, double max, double initial, const juce::String& suffix )
    {
        slider.setSliderStyle ( juce::Slider::LinearHorizontal );
        slider.setRange ( min, max );
        slider.setValue ( initial );
        slider.setTextBoxStyle ( juce::Slider::TextBoxRight, false, 60, 20 );
        slider.setTextValueSuffix ( suffix );
    }

private:
    void updateReverbControlsEnabled()
    {
        bool enabled = reverbEnableButton.getToggleState();
        reverbLevelSlider.setEnabled ( enabled );
        reverbDecaySlider.setEnabled ( enabled );
        float alpha = enabled ? 1.0f : 0.5f;
        reverbLevelLabel.setAlpha ( alpha );
        reverbDecayLabel.setAlpha ( alpha );
    }

    void updateDelayControlsEnabled()
    {
        bool enabled = delayEnableButton.getToggleState();
        delayLevelSlider.setEnabled ( enabled );
        delayTimeSlider.setEnabled ( enabled );
        feedbackSlider.setEnabled ( enabled );
        pingPongButton.setEnabled ( enabled );
        delayHPSlider.setEnabled ( enabled );
        float alpha = enabled ? 1.0f : 0.5f;
        delayLevelLabel.setAlpha ( alpha );
        delayTimeLabel.setAlpha ( alpha );
        feedbackLabel.setAlpha ( alpha );
        delayHPLabel.setAlpha ( alpha );
    }

    jamulus_client_t jamulusClient;

    // Header
    juce::Label      titleLabel;
    juce::TextButton closeButton;

    // Reverb controls (Custom reverb)
    juce::Label        reverbSectionLabel;
    juce::ToggleButton reverbEnableButton;
    juce::Label        reverbLevelLabel;
    juce::Slider       reverbLevelSlider;
    juce::Label        reverbDecayLabel;
    juce::Slider       reverbDecaySlider;

    // Delay controls
    juce::Label        delaySectionLabel;
    juce::ToggleButton delayEnableButton;
    juce::Label        delayLevelLabel;
    juce::Slider       delayLevelSlider;
    juce::Label        delayTimeLabel;
    juce::Slider       delayTimeSlider;
    juce::Label        feedbackLabel;
    juce::Slider       feedbackSlider;
    juce::ToggleButton pingPongButton;
    juce::Label        delayHPLabel;
    juce::Slider       delayHPSlider;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR ( FXPanelComponent )
};
