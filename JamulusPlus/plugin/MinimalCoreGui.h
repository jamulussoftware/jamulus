#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "DebugLogger.h"
#include "PluginProcessor.h"

class JamulusMinimalCoreGui : public juce::Component,
                              private juce::Button::Listener,
                              private juce::Timer
{
public:
    explicit JamulusMinimalCoreGui ( JamulusPluginProcessor& proc ) : processor ( proc )
    {
        addAndMakeVisible ( addressLabel );
        addressLabel.setText ( "Server (host:port)", juce::dontSendNotification );

        addAndMakeVisible ( addressEditor );
        addressEditor.setText ( "127.0.0.1:22124" );

        addAndMakeVisible ( connectButton );
        connectButton.setButtonText ( "Connect" );
        connectButton.addListener ( this );

        addAndMakeVisible ( statusLabel );
        statusLabel.setText ( "Status: Disconnected", juce::dontSendNotification );

        addAndMakeVisible ( testToneToggle );
        testToneToggle.setButtonText ( "Test Tone" );
        testToneToggle.onClick = [this]()
        {
            const bool enabled = testToneToggle.getToggleState();
            processor.setTestToneEnabled ( enabled );
        };

        addAndMakeVisible ( monitorToggle );
        monitorToggle.setButtonText ( "Direct Monitor" );
        monitorToggle.onClick = [this]()
        {
            const bool enabled = monitorToggle.getToggleState();
            processor.setMonitorMode ( enabled );
        };

        addAndMakeVisible ( volumeSlider );
        volumeSlider.setRange ( 0.0, 1.0, 0.01 );
        volumeSlider.setValue ( 1.0 );
        volumeSlider.onValueChange = [this]()
        {
            processor.setMainVolume ( (float) volumeSlider.getValue() );
        };

        addAndMakeVisible ( volumeLabel );
        volumeLabel.setText ( "Main Volume", juce::dontSendNotification );

        addAndMakeVisible ( inputMeterLabel );
        inputMeterLabel.setText ( "Input", juce::dontSendNotification );

        addAndMakeVisible ( outputMeterLabel );
        outputMeterLabel.setText ( "Output", juce::dontSendNotification );

        addAndMakeVisible ( inputMeter );
        addAndMakeVisible ( outputMeter );

        addAndMakeVisible ( reverbEnable );
        reverbEnable.setButtonText ( "Reverb" );
        reverbEnable.onClick = [this]()
        {
            processor.setReverbEnabled ( reverbEnable.getToggleState() );
        };

        addAndMakeVisible ( reverbMix );
        reverbMix.setRange ( 0.0, 1.0, 0.01 );
        reverbMix.setValue ( 0.25 );
        reverbMix.onValueChange = [this]()
        {
            processor.setReverbMix ( (float) reverbMix.getValue() );
        };

        addAndMakeVisible ( reverbLabel );
        reverbLabel.setText ( "Reverb Mix", juce::dontSendNotification );

        addAndMakeVisible ( delayEnable );
        delayEnable.setButtonText ( "Delay" );
        delayEnable.onClick = [this]()
        {
            processor.setDelayEnabled ( delayEnable.getToggleState() );
        };

        addAndMakeVisible ( delayMix );
        delayMix.setRange ( 0.0, 1.0, 0.01 );
        delayMix.setValue ( 0.2 );
        delayMix.onValueChange = [this]()
        {
            processor.setDelayMix ( (float) delayMix.getValue() );
        };

        addAndMakeVisible ( delayTime );
        delayTime.setRange ( 10.0, 1000.0, 1.0 );
        delayTime.setValue ( 250.0 );
        delayTime.onValueChange = [this]()
        {
            processor.setDelayTime ( (float) delayTime.getValue() );
        };

        addAndMakeVisible ( delayFeedback );
        delayFeedback.setRange ( 0.0, 0.95, 0.01 );
        delayFeedback.setValue ( 0.3 );
        delayFeedback.onValueChange = [this]()
        {
            processor.setDelayFeedback ( (float) delayFeedback.getValue() );
        };

        addAndMakeVisible ( delayLabel );
        delayLabel.setText ( "Delay Mix/Time/Feedback", juce::dontSendNotification );

        addAndMakeVisible ( limiterEnable );
        limiterEnable.setButtonText ( "Limiter" );
        limiterEnable.onClick = [this]()
        {
            processor.setLimiterEnabled ( limiterEnable.getToggleState() );
        };

        startTimerHz ( 20 );

        startTimerHz ( 4 );
    }

    void resized() override
    {
        auto area = getLocalBounds().reduced ( 10 );

        auto topRow = area.removeFromTop ( 24 );
        addressLabel.setBounds ( topRow.removeFromLeft ( 150 ) );
        addressEditor.setBounds ( topRow );

        area.removeFromTop ( 10 );
        auto connectRow = area.removeFromTop ( 24 );
        connectButton.setBounds ( connectRow.removeFromLeft ( 100 ) );
        connectRow.removeFromLeft ( 10 );
        statusLabel.setBounds ( connectRow );

        area.removeFromTop ( 10 );
        auto togglesRow = area.removeFromTop ( 24 );
        testToneToggle.setBounds ( togglesRow.removeFromLeft ( 120 ) );
        togglesRow.removeFromLeft ( 10 );
        monitorToggle.setBounds ( togglesRow.removeFromLeft ( 140 ) );

        area.removeFromTop ( 10 );
        auto volumeRow = area.removeFromTop ( 24 );
        volumeLabel.setBounds ( volumeRow.removeFromLeft ( 100 ) );
        volumeRow.removeFromLeft ( 10 );
        volumeSlider.setBounds ( volumeRow );

        area.removeFromTop ( 10 );
        auto metersRow = area.removeFromTop ( 40 );
        inputMeterLabel.setBounds ( metersRow.removeFromLeft ( 50 ) );
        inputMeter.setBounds ( metersRow.removeFromLeft ( 120 ) );
        metersRow.removeFromLeft ( 20 );
        outputMeterLabel.setBounds ( metersRow.removeFromLeft ( 60 ) );
        outputMeter.setBounds ( metersRow.removeFromLeft ( 120 ) );

        area.removeFromTop ( 10 );
        auto reverbRow = area.removeFromTop ( 24 );
        reverbEnable.setBounds ( reverbRow.removeFromLeft ( 80 ) );
        reverbRow.removeFromLeft ( 10 );
        reverbLabel.setBounds ( reverbRow.removeFromLeft ( 120 ) );
        reverbRow.removeFromLeft ( 10 );
        reverbMix.setBounds ( reverbRow );

        area.removeFromTop ( 10 );
        auto delayRow = area.removeFromTop ( 24 );
        delayEnable.setBounds ( delayRow.removeFromLeft ( 80 ) );
        delayRow.removeFromLeft ( 10 );
        delayLabel.setBounds ( delayRow.removeFromLeft ( 180 ) );
        delayRow.removeFromLeft ( 10 );
        delayMix.setBounds ( delayRow.removeFromLeft ( 80 ) );
        delayRow.removeFromLeft ( 10 );
        delayTime.setBounds ( delayRow.removeFromLeft ( 80 ) );
        delayRow.removeFromLeft ( 10 );
        delayFeedback.setBounds ( delayRow.removeFromLeft ( 80 ) );

        area.removeFromTop ( 10 );
        auto limiterRow = area.removeFromTop ( 24 );
        limiterEnable.setBounds ( limiterRow.removeFromLeft ( 80 ) );
    }

private:
    void buttonClicked ( juce::Button* button ) override
    {
        if ( button == &connectButton )
        {
            const auto addr = addressEditor.getText().toStdString();
            if ( processor.isConnected() )
            {
                DebugLogger::instance().log ( "[MinimalCoreGui] Disconnect requested" );
                processor.disconnectFromServer();
            }
            else
            {
                DebugLogger::instance().log ( "[MinimalCoreGui] Connect requested to " + addr );
                processor.connectToServer ( addr );
            }
        }
    }

    void timerCallback() override
    {
        const bool connected = processor.isConnected();
        statusLabel.setText ( connected ? "Status: Connected" : "Status: Disconnected", juce::dontSendNotification );
        connectButton.setButtonText ( connected ? "Disconnect" : "Connect" );

        const float inL  = processor.getLastInputPeakLeft();
        const float inR  = processor.getLastInputPeakRight();
        const float outL = processor.getLastOutputPeakLeft();
        const float outR = processor.getLastOutputPeakRight();

        inputMeter.setLevels ( inL, inR );
        outputMeter.setLevels ( outL, outR );
    }

    JamulusPluginProcessor& processor;

    class SimpleStereoMeter : public juce::Component
    {
    public:
        void setLevels ( float left, float right )
        {
            levelLeft  = left;
            levelRight = right;
            repaint();
        }

        void paint ( juce::Graphics& g ) override
        {
            auto bounds = getLocalBounds().toFloat();
            auto half   = bounds.reduced ( 2 ).withHeight ( bounds.getHeight() / 2.0f );

            auto top    = half;
            auto bottom = half.translated ( 0.0f, half.getHeight() );

            const auto wL = juce::jlimit ( 0.0f, half.getWidth(), half.getWidth() * levelLeft );
            const auto wR = juce::jlimit ( 0.0f, half.getWidth(), half.getWidth() * levelRight );

            g.setColour ( juce::Colours::darkgrey );
            g.fillRect ( top );
            g.fillRect ( bottom );

            g.setColour ( juce::Colours::limegreen );
            g.fillRect ( top.removeFromLeft ( (int) wL ) );

            g.setColour ( juce::Colours::orange );
            g.fillRect ( bottom.removeFromLeft ( (int) wR ) );
        }

    private:
        float levelLeft  = 0.0f;
        float levelRight = 0.0f;
    };

    juce::Label       addressLabel;
    juce::TextEditor  addressEditor;
    juce::TextButton  connectButton;
    juce::Label       statusLabel;
    juce::ToggleButton testToneToggle;
    juce::ToggleButton monitorToggle;
    juce::Slider      volumeSlider;
    juce::Label       volumeLabel;

    juce::Label        inputMeterLabel;
    juce::Label        outputMeterLabel;
    SimpleStereoMeter  inputMeter;
    SimpleStereoMeter  outputMeter;

    juce::ToggleButton reverbEnable;
    juce::Slider       reverbMix;
    juce::Label        reverbLabel;

    juce::ToggleButton delayEnable;
    juce::Slider       delayMix;
    juce::Slider       delayTime;
    juce::Slider       delayFeedback;
    juce::Label        delayLabel;

    juce::ToggleButton limiterEnable;
};
