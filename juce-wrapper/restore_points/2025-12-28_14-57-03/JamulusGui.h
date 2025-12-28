#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include "../jamulus_wrapper.h"
#include "SettingsComponent.h"
#include "ConnectComponent.h"
#include "ChatComponent.h"
#include "FXPanelComponent.h"
#include "AudioDelay.h"
#include <map>
#include <cmath>

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
// Status Light Component (Glowing)
//==============================================================================
class StatusLight : public juce::Component
{
public:
    void setText ( const juce::String&, juce::NotificationType ) {} // Compatibility dummy

    void setColour ( int /*id*/, juce::Colour c )
    {
        if ( colour != c )
        {
            colour = c;
            repaint();
        }
    }

    void paint ( juce::Graphics& g ) override
    {
        if ( colour.isTransparent() )
            return;

        auto  bounds = getLocalBounds().toFloat();
        float radius = std::min ( bounds.getWidth(), bounds.getHeight() ) * 0.45f;
        auto  center = bounds.getCentre();

        // Gradient glow halo
        juce::ColourGradient grad ( colour, center.x, center.y, colour.withAlpha ( 0.0f ), center.x, center.y - radius * 2.0f, true );
        g.setGradientFill ( grad );
        g.fillEllipse ( center.x - radius * 2.0f, center.y - radius * 2.0f, radius * 4.0f, radius * 4.0f );

        // Core light
        g.setColour ( colour );
        g.fillEllipse ( center.x - radius * 0.7f, center.y - radius * 0.7f, radius * 1.4f, radius * 1.4f );

        // Shine (specular highlight)
        g.setColour ( juce::Colours::white.withAlpha ( 0.4f ) );
        g.fillEllipse ( center.x - radius * 0.4f, center.y - radius * 0.4f, radius * 0.4f, radius * 0.4f );
    }

private:
    juce::Colour colour = juce::Colours::transparentBlack;
};

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

    void setHorizontal ( bool isHorizontal )
    {
        horizontal = isHorizontal;
        repaint();
    }

    void paint ( juce::Graphics& g ) override
    {
        auto bounds = getLocalBounds().toFloat();

        // Background
        g.setColour ( juce::Colour ( 0xff2a2a2a ) );
        g.fillRoundedRectangle ( bounds, horizontal ? 1.0f : 2.0f );

        // Level bar
        juce::Rectangle<float> levelRect;
        if ( horizontal )
        {
            float levelWidth = bounds.getWidth() * currentLevel;
            levelRect        = juce::Rectangle<float> ( bounds.getX(), bounds.getY(), levelWidth, bounds.getHeight() );
        }
        else
        {
            float levelHeight = bounds.getHeight() * currentLevel;
            levelRect = juce::Rectangle<float> ( bounds.getX() + 1, bounds.getBottom() - levelHeight - 1, bounds.getWidth() - 2, levelHeight );
        }

        // Color gradient based on level
        juce::Colour levelColor;
        if ( currentLevel > 0.9f )
            levelColor = juce::Colours::red;
        else if ( currentLevel > 0.7f )
            levelColor = juce::Colours::yellow;
        else
            levelColor = juce::Colour ( 0xff00cc66 ); // Green

        g.setColour ( levelColor );
        if ( horizontal )
            g.fillRect ( levelRect );
        else
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

    juce::Colour getGroupColor ( int gid )
    {
        switch ( gid % 8 )
        {
        case 0:
            return juce::Colour ( 0xff00ff00 ); // Green
        case 1:
            return juce::Colour ( 0xff32cd32 ); // LimeGreen
        case 2:
            return juce::Colour ( 0xff00fa9a ); // MediumSpringGreen
        case 3:
            return juce::Colour ( 0xff9acd32 ); // YellowGreen
        case 4:
            return juce::Colour ( 0xff228b22 ); // ForestGreen
        case 5:
            return juce::Colour ( 0xff2e8b57 ); // SeaGreen
        case 6:
            return juce::Colour ( 0xff6b8e23 ); // OliveDrab
        case 7:
            return juce::Colour ( 0xff8fbc8f ); // DarkSeaGreen
        default:
            return juce::Colours::transparentBlack;
        }
    }

    bool  horizontal   = false;
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

        // Mute Button
        addAndMakeVisible ( muteButton );
        muteButton.setButtonText ( "M" );
        muteButton.setClickingTogglesState ( true );
        muteButton.setTooltip ( "Mute" );
        muteButton.onClick = [this]() {
            updateMuteButtonColor();
            if ( onMuteChanged )
                onMuteChanged ( channelId, muteButton.getToggleState() );
        };

        // Solo Button
        addAndMakeVisible ( soloButton );
        soloButton.setButtonText ( "S" );
        soloButton.setClickingTogglesState ( true );
        soloButton.setTooltip ( "Solo" );
        soloButton.onClick = [this]() {
            updateSoloButtonColor();
            if ( onSoloChanged )
                onSoloChanged ( channelId, soloButton.getToggleState() );
        };

        // Boost Button (+6dB)
        addAndMakeVisible ( boostButton );
        boostButton.setButtonText ( "B" );
        boostButton.setClickingTogglesState ( true );
        boostButton.setTooltip ( "Boost +6dB" );
        boostButton.onClick = [this]() {
            updateBoostButtonColor();
            if ( onBoostChanged )
                onBoostChanged ( channelId, boostButton.getToggleState() );
        };

        // Group Button
        addAndMakeVisible ( groupButton );
        groupButton.setButtonText ( "G" );
        groupButton.setClickingTogglesState ( false ); // We'll handle state manually via popup/menu
        groupButton.setTooltip ( "Group" );
        groupButton.onClick = [this]() {
            juce::PopupMenu m;
            m.addItem ( 1, "No Grouping", true, groupID == -1 );
            m.addSeparator();
            for ( int i = 1; i <= 8; ++i )
                m.addItem ( i + 1, "Group " + juce::String ( i ), true, groupID == ( i - 1 ) );

            m.showMenuAsync ( juce::PopupMenu::Options().withTargetComponent ( &groupButton ), [this] ( int result ) {
                if ( result == 1 )
                {
                    setGroup ( -1 );
                    if ( onGroupChanged )
                        onGroupChanged ( channelId, -1 );
                }
                else if ( result > 1 )
                {
                    int newGid = result - 2;
                    setGroup ( newGid );
                    if ( onGroupChanged )
                        onGroupChanged ( channelId, newGid );
                }
            } );
        };
        groupButton.addMouseListener ( this, false );

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

    void updateSoloButtonColor()
    {
        if ( soloButton.getToggleState() )
        {
            // Yellow Glow (0xFFFFCC00)
            juce::Colour yellowGlow ( 0xffffcc00 );
            soloButton.setColour ( juce::TextButton::buttonColourId, yellowGlow );
            soloButton.setColour ( juce::TextButton::buttonOnColourId, yellowGlow );
            soloButton.setColour ( juce::TextButton::textColourOnId, juce::Colours::black );
            soloButton.setColour ( juce::TextButton::textColourOffId, juce::Colours::black );
        }
        else
        {
            // Reset
            soloButton.removeColour ( juce::TextButton::buttonColourId );
            soloButton.removeColour ( juce::TextButton::buttonOnColourId );
            soloButton.removeColour ( juce::TextButton::textColourOnId );
            soloButton.removeColour ( juce::TextButton::textColourOffId );
        }
        soloButton.repaint();
    }

    void updateBoostButtonColor()
    {
        if ( boostButton.getToggleState() )
        {
            // Cyan glow for boost
            juce::Colour cyanGlow ( 0xff00cccc );
            boostButton.setColour ( juce::TextButton::buttonColourId, cyanGlow );
            boostButton.setColour ( juce::TextButton::buttonOnColourId, cyanGlow );
            boostButton.setColour ( juce::TextButton::textColourOnId, juce::Colours::black );
            boostButton.setColour ( juce::TextButton::textColourOffId, juce::Colours::black );
        }
        else
        {
            // Reset
            boostButton.removeColour ( juce::TextButton::buttonColourId );
            boostButton.removeColour ( juce::TextButton::buttonOnColourId );
            boostButton.removeColour ( juce::TextButton::textColourOnId );
            boostButton.removeColour ( juce::TextButton::textColourOffId );
        }
        boostButton.repaint();
    }

    void updateGroupButtonColor()
    {
        if ( groupID != -1 )
        {
            // Green variants for Group
            juce::Colour groupColor = getGroupColor ( groupID );
            groupButton.setColour ( juce::TextButton::buttonColourId, groupColor );
            groupButton.setColour ( juce::TextButton::buttonOnColourId, groupColor.brighter() );

            // Contrast check for text color
            bool darkBg = ( groupID == 4 || groupID == 6 ); // Hunter Green and Sea Green
            groupButton.setColour ( juce::TextButton::textColourOnId, darkBg ? juce::Colours::white : juce::Colours::black );
            groupButton.setColour ( juce::TextButton::textColourOffId, darkBg ? juce::Colours::white : juce::Colours::black );

            groupButton.setButtonText ( "G" + juce::String ( groupID + 1 ) );
        }
        else
        {
            // Reset
            groupButton.removeColour ( juce::TextButton::buttonColourId );
            groupButton.removeColour ( juce::TextButton::buttonOnColourId );
            groupButton.removeColour ( juce::TextButton::textColourOnId );
            groupButton.removeColour ( juce::TextButton::textColourOffId );
            groupButton.setButtonText ( "G" );
        }
        groupButton.repaint();
    }

    void mouseWheelMove ( const juce::MouseEvent& event, const juce::MouseWheelDetails& wheel ) override
    {
        if ( event.eventComponent == &groupButton )
        {
            float d = wheel.deltaY;
            if ( std::abs ( d ) > 0.01f )
            {
                int delta     = ( d > 0 ) ? 1 : -1;
                int nextGroup = groupID + delta;
                if ( nextGroup < -1 )
                    nextGroup = 7;
                else if ( nextGroup > 7 )
                    nextGroup = -1;

                setGroup ( nextGroup );
                if ( onGroupChanged )
                    onGroupChanged ( channelId, nextGroup );
            }
        }
    }

    juce::Colour getGroupColor ( int gid )
    {
        switch ( gid % 8 )
        {
        case 0:
            return juce::Colour ( 0xff00ff00 ); // Group 1: Pure Neon Green
        case 1:
            return juce::Colour ( 0xff32cd32 ); // Group 2: Lime Green
        case 2:
            return juce::Colour ( 0xff00fa9a ); // Group 3: Spring Green
        case 3:
            return juce::Colour ( 0xffadff2f ); // Group 4: Green Yellow
        case 4:
            return juce::Colour ( 0xff006400 ); // Group 5: Hunter Green (Very Dark)
        case 5:
            return juce::Colour ( 0xff50c878 ); // Group 6: Emerald Green
        case 6:
            return juce::Colour ( 0xff2e8b57 ); // Group 7: Sea Green (Deep)
        case 7:
            return juce::Colour ( 0xff76ff7a ); // Group 8: Screamin Green (Distinct Green)
        default:
            return juce::Colours::transparentBlack;
        }
    }

    void setChannelInfo ( int id, const juce::String& name, int instrument, const char* countryCode, int skill, bool isMe )
    {
        channelId = id;
        nameLabel.setText ( name, juce::dontSendNotification );
        instrumentIcon   = getInstrumentImage ( instrument );
        flagIcon         = getFlagImage ( countryCode );
        this->skillLevel = skill;
        this->isMe       = isMe;

        // Skill text removed as requested
        skillLabel.setVisible ( false );
        nameLabel.setVisible ( true );
        nameLabel.setColour ( juce::Label::backgroundColourId, juce::Colours::transparentBlack );
        nameLabel.setColour ( juce::Label::textColourId, juce::Colours::black );

        repaint();
    }

    void setLevel ( float level ) { levelMeter.setLevel ( level ); }

    void setMute ( bool m )
    {
        muteButton.setToggleState ( m, juce::dontSendNotification );
        updateMuteButtonColor();
    }
    void setSolo ( bool s )
    {
        soloButton.setToggleState ( s, juce::dontSendNotification );
        updateSoloButtonColor();
    }
    void setBoost ( bool b )
    {
        boostButton.setToggleState ( b, juce::dontSendNotification );
        updateBoostButtonColor();
    }
    void setGroup ( int gid )
    {
        groupID = gid;
        updateGroupButtonColor();
    }
    void setGain ( float g ) { faderSlider.setValue ( g, juce::dontSendNotification ); }

    int  getChannelId() const { return channelId; }
    bool getIsMe() const { return isMe; }

    void paint ( juce::Graphics& g ) override
    {
        auto bounds = getLocalBounds().toFloat();

        // Define shape
        juce::Path shape;
        shape.addRoundedRectangle ( bounds, 6.0f );

        // Clip everything to the rounded shape to prevent corners sticking out
        g.saveState();
        g.reduceClipRegion ( shape );

        // 1. Fill Background (Skill Color)
        juce::Colour bgColour;
        switch ( skillLevel )
        {
        case 1:
            bgColour = juce::Colour ( 255, 255, 200 );
            break; // Beginner: Pale Yellow
        case 2:
            bgColour = juce::Colour ( 225, 255, 225 );
            break; // Intermediate: Pale Green
        case 3:
            bgColour = juce::Colour ( 255, 225, 225 );
            break; // Expert: Pale Pink
        default:
            bgColour = isMe ? juce::Colour ( 0xffd0d0d0 ) : juce::Colour ( 0xffe0e0e0 );
            break;
        }
        g.setColour ( bgColour );
        g.fillAll(); // Fills clipped region

        // 2. Draw Header Elements
        auto headerBounds = bounds;
        auto flagBounds   = headerBounds.removeFromTop ( 30 ); // Adjusted to 30
        auto instBounds   = headerBounds.removeFromTop ( 25 ); // Adjusted to 25

        // Flag
        if ( flagIcon.isValid() )
        {
            g.setOpacity ( 1.0f );
            // Draw filling the area
            g.drawImage ( flagIcon, flagBounds, juce::RectanglePlacement::stretchToFit );
        }

        // Instrument Icon
        if ( instrumentIcon.isValid() )
        {
            g.setOpacity ( 1.0f );
            g.drawImage ( instrumentIcon, instBounds.reduced ( 2 ), juce::RectanglePlacement::centred );
        }

        g.restoreState(); // Pop clip

        // 3. Draw Border (on top)
        g.setColour ( isMe ? juce::Colours::orange : juce::Colours::grey );
        g.drawRoundedRectangle ( bounds, 6.0f, isMe ? 2.0f : 1.0f );
    }

    void resized() override
    {
        auto bounds = getLocalBounds().reduced ( 3 );

        // Tag area (Icon box + Name) - fixed height 80
        auto tagArea = bounds.removeFromTop ( 80 ).reduced ( 2 );

        // Icons space (Flag 30 + Inst 25 + Gap 5)
        tagArea.removeFromTop ( 30 );
        tagArea.removeFromTop ( 25 );
        tagArea.removeFromTop ( 5 );

        // Name label below icons
        nameLabel.setBounds ( tagArea );

        // Skill label hidden
        skillLabel.setBounds ( 0, 0, 0, 0 );

        bounds.removeFromTop ( 6 );

        // Mute, Solo, Boost, and Group buttons area (2x2 grid)
        auto bottomArea = bounds.removeFromBottom ( 45 ); // Increased height for 2 rows
        auto buttonGrid = bottomArea.reduced ( 2, 0 );

        int halfWidth  = buttonGrid.getWidth() / 2;
        int halfHeight = buttonGrid.getHeight() / 2;

        auto topRow = buttonGrid.removeFromTop ( halfHeight );
        muteButton.setBounds ( topRow.removeFromLeft ( halfWidth ).reduced ( 1, 1 ) );
        soloButton.setBounds ( topRow.reduced ( 1, 1 ) );

        auto bottomRow = buttonGrid;
        boostButton.setBounds ( bottomRow.removeFromLeft ( halfWidth ).reduced ( 1, 1 ) );
        groupButton.setBounds ( bottomRow.reduced ( 1, 1 ) );

        bounds.removeFromBottom ( 4 );

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
    std::function<void ( int, bool )>  onSoloChanged;
    std::function<void ( int, bool )>  onBoostChanged;
    std::function<void ( int, int )>   onGroupChanged;

private:
    int              channelId  = -1;
    bool             isMe       = false;
    int              skillLevel = 0;
    int              groupID    = -1;
    juce::Label      nameLabel;
    juce::Label      skillLabel;
    juce::Image      instrumentIcon;
    juce::Image      flagIcon;
    juce::Slider     faderSlider;
    juce::Slider     panSlider;
    juce::TextButton muteButton;
    juce::TextButton soloButton;
    juce::TextButton boostButton;
    juce::TextButton groupButton;
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

        // Main Volume slider (controls all channels except local)
        addAndMakeVisible ( mainVolumeLabel );
        mainVolumeLabel.setText ( "Main", juce::dontSendNotification );
        mainVolumeLabel.setColour ( juce::Label::textColourId, juce::Colours::white );
        mainVolumeLabel.setJustificationType ( juce::Justification::centred );

        addAndMakeVisible ( mainVolumeSlider );
        mainVolumeSlider.setSliderStyle ( juce::Slider::LinearHorizontal );
        // Range: 0 = silence, 100 = 0dB (unity), 106 = +6dB (~2x)
        mainVolumeSlider.setRange ( 0, 106, 1 );
        mainVolumeSlider.setValue ( 100 ); // Default to 0dB
        mainVolumeSlider.setTextBoxStyle ( juce::Slider::NoTextBox, true, 0, 0 );
        mainVolumeSlider.setTooltip ( "Main output volume (0dB default, +6dB max). Double-click to reset." );
        mainVolumeSlider.setScrollWheelEnabled ( true );
        mainVolumeSlider.setDoubleClickReturnValue ( true, 100.0 ); // Double-click resets to 0dB
        mainVolumeSlider.onValueChange = [this]() {
            // Convert slider value to gain: 100 = 1.0 (0dB), 106 = ~2.0 (+6dB)
            double sliderVal = mainVolumeSlider.getValue();
            if ( sliderVal <= 0 )
                mainVolume = 0.0f;
            else if ( sliderVal <= 100 )
                mainVolume = static_cast<float> ( sliderVal / 100.0 );
            else
                // Above 100: convert to dB gain (+1 to +6 dB)
                mainVolume = static_cast<float> ( std::pow ( 10.0, ( sliderVal - 100.0 ) / 20.0 ) );
            if ( onMainVolumeChanged )
                onMainVolumeChanged ( mainVolume );
        };

        // Auto Level button
        addAndMakeVisible ( autoLevelButton );
        autoLevelButton.setButtonText ( "Auto Level" );
        autoLevelButton.setClickingTogglesState ( true );
        autoLevelButton.setTooltip ( "Auto-level all channels to -6dB average" );
        autoLevelButton.onClick = [this]() {
            autoLevelEnabled = autoLevelButton.getToggleState();
            updateAutoLevelButtonColor();
            if ( autoLevelEnabled )
            {
                // Turn off all boost buttons when auto level is enabled
                for ( auto& strip : channelStrips )
                {
                    int id = strip->getChannelId();
                    if ( channelBoosts.count ( id ) && channelBoosts[id] )
                    {
                        channelBoosts[id] = false;
                        // Reset gain to non-boosted value
                        float gain = channelGains.count ( id ) ? channelGains[id] : 0.75f;
                        jamulus_client_set_channel_gain ( jamulusClient, id, gain );
                    }
                }
                // Clear target gains so they get recalculated
                autoLevelTargetGains.clear();
            }
            else
            {
                // Restore gains to current fader settings when disabled
                for ( auto& strip : channelStrips )
                {
                    int id = strip->getChannelId();
                    if ( id != -1 )
                    {
                        float faderGain = channelGains.count ( id ) ? channelGains[id] : 0.75f;
                        float boost     = ( channelBoosts.count ( id ) && channelBoosts[id] ) ? 2.0f : 1.0f;
                        jamulus_client_set_channel_gain ( jamulusClient, id, faderGain * boost );
                    }
                }

                // Clear auto-level state
                autoLevelTargetGains.clear();
                autoLevelCurrentGains.clear();
                autoLevelPeakLevels.clear();
                autoLevelPeakHoldCounters.clear();
            }
        };

        // Auto Level settings (cog) button
        addAndMakeVisible ( autoLevelSettingsButton );
        autoLevelSettingsButton.setButtonText ( juce::String::charToString ( 0x2699 ) ); // Gear unicode
        autoLevelSettingsButton.setTooltip ( "Auto Level Settings" );
        autoLevelSettingsButton.onClick = [this]() { showAutoLevelSettingsPopup(); };

        // Master Output Meters
        addAndMakeVisible ( masterMeterL );
        addAndMakeVisible ( masterMeterR );
        masterMeterL.setHorizontal ( true );
        masterMeterR.setHorizontal ( true );

        // Status
        addAndMakeVisible ( statusLabel );
        statusLabel.setText ( "Disconnected", juce::dontSendNotification );
        statusLabel.setColour ( juce::Label::textColourId, juce::Colours::grey );

        // Delay & Jitter
        addAndMakeVisible ( delayLight );
        // delayLight.setText ( "", juce::dontSendNotification ); // Not needed for StatusLight

        addAndMakeVisible ( delayLabel );
        delayLabel.setText ( "Delay: --", juce::dontSendNotification );
        delayLabel.setColour ( juce::Label::textColourId, juce::Colours::white );

        addAndMakeVisible ( jitterLight );
        // jitterLight.setText ( "", juce::dontSendNotification );

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
        // saveSettings(); // DISABLED: Potential crash if client is already destroyed
        stopTimer();
    }

    void paint ( juce::Graphics& g ) override
    {
        g.fillAll ( juce::Colour ( 0xff323232 ) );

        // Header background
        g.setColour ( juce::Colour ( 0xff252525 ) );
        g.fillRect ( 0, 0, getWidth(), 50 );

        // Mixer area background (starts after header + controls)
        // Header (50) + Controls (140) = 190
        auto mixerBounds = getLocalBounds().removeFromBottom ( getHeight() - 190 );
        g.setColour ( juce::Colour ( 0xff2a2a2a ) );
        g.fillRect ( mixerBounds );

        // Vertical Separator Line
        g.setColour ( juce::Colour ( 0xff505050 ) );
        g.drawVerticalLine ( 200, 50.0f, 190.0f ); // Draw line separating local controls
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

        // Main Volume (label + slider + meters)
        auto mainVolArea = toolbar.removeFromLeft ( 160 );
        mainVolumeLabel.setBounds ( mainVolArea.removeFromLeft ( 35 ).removeFromTop ( 35 ).reduced ( 2 ) );
        auto sliderMeterArea = mainVolArea.reduced ( 5, 5 );
        mainVolumeSlider.setBounds ( sliderMeterArea.removeFromTop ( 20 ) );
        sliderMeterArea.removeFromTop ( 2 );
        masterMeterL.setBounds ( sliderMeterArea.removeFromTop ( 4 ) );
        sliderMeterArea.removeFromTop ( 1 );
        masterMeterR.setBounds ( sliderMeterArea.removeFromTop ( 4 ) );

        // Auto Level button + cog
        auto autoLevelArea = toolbar.removeFromLeft ( 100 );
        autoLevelButton.setBounds ( autoLevelArea.removeFromLeft ( 75 ).reduced ( 3, 8 ) );
        autoLevelSettingsButton.setBounds ( autoLevelArea.reduced ( 2, 10 ) );

        // Status area on right
        disconnectButton.setBounds ( toolbar.removeFromRight ( 90 ).reduced ( 5 ) );
        statusLabel.setBounds ( toolbar.reduced ( 5 ) );

        // Controls area - reduced height since we removed the big input section
        auto controlsArea = bounds.removeFromTop ( 140 );

        // Left side - FX button, MON button, and network stats
        auto leftControls = controlsArea.removeFromLeft ( 140 ).reduced ( 10 );

        // Network Stats
        int statHeight = 20;
        int lightWidth = 14;
        int gap        = 6;

        // Ping (Align text with others)
        auto pingRow = leftControls.removeFromTop ( statHeight );
        pingRow.removeFromLeft ( lightWidth + gap ); // Indent
        pingLabel.setBounds ( pingRow );

        // Delay (Light on Left)
        auto delayRow = leftControls.removeFromTop ( statHeight );
        delayLight.setBounds ( delayRow.removeFromLeft ( lightWidth ).reduced ( 2 ) );
        delayRow.removeFromLeft ( gap );
        delayLabel.setBounds ( delayRow );

        // Jitter (Light on Left)
        auto jitterRow = leftControls.removeFromTop ( statHeight );
        jitterLight.setBounds ( jitterRow.removeFromLeft ( lightWidth ).reduced ( 2 ) );
        jitterRow.removeFromLeft ( gap );
        jitterLabel.setBounds ( jitterRow );

        // Spacer
        leftControls.removeFromTop ( 10 );

        // Buttons row (FX, MON) - Moved below stats
        auto buttonsRow = leftControls.removeFromTop ( 30 );
        fxButton.setBounds ( buttonsRow.removeFromLeft ( 50 ).reduced ( 2 ) );
        buttonsRow.removeFromLeft ( 5 ); // Gap
        monitorModeButton.setBounds ( buttonsRow.removeFromLeft ( 50 ).reduced ( 2 ) );

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

        // Master output meters
        masterMeterL.setLevel ( jamulus_client_get_output_level_left ( jamulusClient ) );
        masterMeterR.setLevel ( jamulus_client_get_output_level_right ( jamulusClient ) );

        // Update connection status
        bool connected = jamulus_client_is_connected ( jamulusClient );

        if ( connected != wasConnected )
        {
            if ( connected )
            {
                // We just connected. Mark everyone already here as "established"
                // so they don't get the "new client" gradual level-in.
                int n           = jamulus_client_get_num_channels ( jamulusClient );
                int activeCount = 0;
                for ( int i = 0; i < n; ++i )
                {
                    jamulus_channel_info_t info;
                    if ( jamulus_client_get_channel_info ( jamulusClient, i, &info ) )
                    {
                        autoLevelChannelActiveTicks[info.id] = 1000; // Mark as established
                        if ( info.id != jamulus_client_get_my_channel_id ( jamulusClient ) )
                            activeCount++;
                    }
                }
                // Initialize the mix count based on current population
                autoLevelSmoothedChannelCount = (float) std::max ( 1, activeCount );
            }
            else
            {
                // Clean up tracking on disconnect
                autoLevelChannelActiveTicks.clear();
                autoLevelPeakLevels.clear();
                autoLevelPeakHoldCounters.clear();
                autoLevelCurrentGains.clear();
            }

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
                // Color code ping based on latency (like original Qt GUI)
                juce::Colour pingColor;
                if ( stats.ping_time_ms <= 25 )
                    pingColor = juce::Colour ( 0xff00cc66 ); // Green - excellent
                else if ( stats.ping_time_ms <= 50 )
                    pingColor = juce::Colour ( 0xffffa500 ); // Orange - warning
                else
                    pingColor = juce::Colours::red; // Red - bad
                pingLabel.setColour ( juce::Label::textColourId, pingColor );

                // Delay - color both the light AND the text
                delayLabel.setText ( "Delay: " + juce::String ( stats.total_delay_ms ) + " ms", juce::dontSendNotification );

                juce::Colour delayColor;
                if ( stats.total_delay_ms <= 43 )
                    delayColor = juce::Colour ( 0xff00cc66 ); // Green
                else if ( stats.total_delay_ms <= 68 )
                    delayColor = juce::Colour ( 0xffffa500 ); // Orange
                else
                    delayColor = juce::Colours::red;

                delayLight.setColour ( juce::Label::backgroundColourId, delayColor );
                delayLabel.setColour ( juce::Label::textColourId, delayColor );

                // Jitter
                juce::Colour jitterColor;
                if ( stats.jitter_buffer_status == 0 )
                    jitterColor = juce::Colour ( 0xff00cc66 ); // Green
                else if ( stats.jitter_buffer_status == 1 )
                    jitterColor = juce::Colour ( 0xffffa500 ); // Orange
                else
                    jitterColor = juce::Colours::red;

                jitterLight.setColour ( juce::Label::backgroundColourId, jitterColor );
            }
        }
        else
        {
            pingLabel.setText ( "Ping: --", juce::dontSendNotification );
            pingLabel.setColour ( juce::Label::textColourId, juce::Colours::white );

            delayLabel.setText ( "Delay: --", juce::dontSendNotification );
            delayLabel.setColour ( juce::Label::textColourId, juce::Colours::white );

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
                float level = idToLevel[id] / 65535.0f;
                strip->setLevel ( level );
                channelLevels[id] = level; // Track for auto-leveling
            }
        }

        // Auto-level boosted channels if enabled
        if ( autoLevelBoostEnabled && !channelStrips.empty() )
        {
            // Calculate average level of non-boosted, non-local channels
            float avgLevel    = 0.0f;
            int   normalCount = 0;
            for ( auto& strip : channelStrips )
            {
                int id = strip->getChannelId();
                if ( id != -1 && !strip->getIsMe() && channelLevels.count ( id ) )
                {
                    // Skip grouped channels for auto-level
                    if ( channelGroups.count ( id ) && channelGroups[id] != -1 )
                        continue;

                    bool isBoosted = channelBoosts.count ( id ) && channelBoosts[id];
                    if ( !isBoosted )
                    {
                        avgLevel += channelLevels[id];
                        normalCount++;
                    }
                }
            }

            if ( normalCount > 0 )
            {
                avgLevel /= normalCount;

                // For each boosted channel, check if it's too loud
                for ( auto& strip : channelStrips )
                {
                    int id = strip->getChannelId();
                    if ( id != -1 && !strip->getIsMe() && channelLevels.count ( id ) )
                    {
                        // Skip grouped channels for auto-level
                        if ( channelGroups.count ( id ) && channelGroups[id] != -1 )
                            continue;

                        bool isBoosted = channelBoosts.count ( id ) && channelBoosts[id];
                        if ( isBoosted )
                        {
                            float boostedLevel = channelLevels[id];
                            // If boosted channel is more than 6dB louder than average
                            float threshold = avgLevel * 2.0f; // +6dB = 2x
                            if ( boostedLevel > threshold && threshold > 0.0f )
                            {
                                // Calculate attenuation needed
                                float ratio       = boostedLevel / threshold;
                                float attenuation = 1.0f / ratio;
                                // Apply attenuated gain (fader * boost * attenuation)
                                float baseGain = channelGains.count ( id ) ? channelGains[id] : 0.75f;
                                float newGain  = baseGain * 2.0f * attenuation; // 2.0 is boost
                                jamulus_client_set_channel_gain ( jamulusClient, id, newGain );
                            }
                        }
                    }
                }
            }
        }

        // Auto Level ALL channels to target average (if enabled)
        if ( autoLevelEnabled && !channelStrips.empty() )
        {
            const float timerIntervalMs = 50.0f; // Timer runs at ~20Hz
            const float noiseFloor      = 0.02f; // -34dB threshold (ignore very low background noise)

            // Count active auto-leveled channels
            int actualActiveCount = 0;
            for ( auto& strip : channelStrips )
            {
                int id = strip->getChannelId();
                if ( id != -1 && !strip->getIsMe() && ( !channelGroups.count ( id ) || channelGroups[id] == -1 ) )
                    actualActiveCount++;
            }

            // Slowly adjust the "seen" channel count to prevent sudden mix shifts
            // Adjust over ~3 seconds (at 20Hz, 60 ticks)
            autoLevelSmoothedChannelCount += ( (float) actualActiveCount - autoLevelSmoothedChannelCount ) * 0.05f;

            // Calculate a dynamic target level based on number of participants
            // This prevents the total mix from clipping as more people join.
            // Formula: 0.5 / sqrt(N) gives 0.5 for 1 person, 0.25 for 4 people, 0.125 for 16.
            // We use a slightly more generous log-based or sqrt approach.
            const float baseTarget        = 0.6f;
            float       targetLevelLinear = baseTarget / std::sqrt ( std::max ( 1.0f, autoLevelSmoothedChannelCount ) );
            targetLevelLinear             = juce::jlimit ( 0.1f, 0.6f, targetLevelLinear );

            const int   holdTicks    = 100; // 5 seconds hold at 20Hz
            const float attackCoeff  = 1.0f - std::exp ( -timerIntervalMs / autoLevelAttackMs );
            const float releaseCoeff = 1.0f - std::exp ( -timerIntervalMs / autoLevelReleaseMs );

            // Even longer long-term peak tracking: decay over ~10 seconds
            const float longTermPeakDecayMs = 10000.0f;
            const float longTermDecayCoeff  = 1.0f - std::exp ( -timerIntervalMs / longTermPeakDecayMs );

            for ( auto& strip : channelStrips )
            {
                int id = strip->getChannelId();
                if ( id == -1 || strip->getIsMe() )
                    continue; // Skip local channel

                // Skip grouped channels for auto-level
                if ( channelGroups.count ( id ) && channelGroups[id] != -1 )
                    continue;

                if ( !channelLevels.count ( id ) )
                    continue;

                float currentLevel = channelLevels[id];

                // Initialize tracking if not yet done
                if ( !autoLevelCurrentGains.count ( id ) )
                {
                    // For new channels, start at their current gain rather than forcing 1.0
                    float startGain           = channelGains.count ( id ) ? channelGains[id] : 0.75f;
                    autoLevelCurrentGains[id] = startGain;
                }
                if ( !autoLevelPeakLevels.count ( id ) )
                    autoLevelPeakLevels[id] = 0.0f;
                if ( !autoLevelPeakHoldCounters.count ( id ) )
                    autoLevelPeakHoldCounters[id] = 0;

                // Track how long they've been in the auto-level mix
                if ( !autoLevelChannelActiveTicks.count ( id ) )
                    autoLevelChannelActiveTicks[id] = 0;
                else
                    autoLevelChannelActiveTicks[id]++;

                bool isNew = autoLevelChannelActiveTicks[id] < 60; // First 3 seconds are "new"

                // Handle peak hold and decay logic
                float& longTermPeak = autoLevelPeakLevels[id];
                int&   holdCounter  = autoLevelPeakHoldCounters[id];

                if ( currentLevel >= noiseFloor )
                {
                    if ( currentLevel > longTermPeak )
                    {
                        // New peak - update immediately and reset hold timer
                        longTermPeak = currentLevel;
                        holdCounter  = holdTicks;
                    }
                    else if ( holdCounter > 0 )
                    {
                        // Level is below current peak, but we are still in hold period
                        --holdCounter;
                    }
                    else
                    {
                        // Hold finished, decay slowly to re-evaluate
                        longTermPeak -= ( longTermPeak - currentLevel ) * longTermDecayCoeff;
                    }
                }
                else
                {
                    // Level is below noise floor - just decay slowly if hold is over
                    // This allows it to eventually reset if they stop playing,
                    // but won't jump up immediately just because they paused.
                    if ( holdCounter > 0 )
                        --holdCounter;
                    else if ( longTermPeak > 0.0f )
                        longTermPeak -= longTermPeak * ( longTermDecayCoeff * 0.5f ); // Slower decay for silence
                }

                // Skip gain calculation if signal or peak is too low
                if ( longTermPeak < noiseFloor )
                {
                    // If everything is quiet, gradually return to default gain 1.0 if not already there
                    float& currentGain = autoLevelCurrentGains[id];
                    if ( std::abs ( currentGain - 1.0f ) > 0.01f )
                    {
                        currentGain += ( 1.0f - currentGain ) * releaseCoeff;
                        jamulus_client_set_channel_gain ( jamulusClient, id, currentGain );
                    }
                    continue;
                }

                // Calculate target gain based on long-term peak trend
                float targetGain = targetLevelLinear / longTermPeak;
                targetGain       = juce::jlimit ( 0.25f, 2.0f, targetGain ); // Conservative gain range

                float& currentGain = autoLevelCurrentGains[id];

                // Determine smoothing based on attack/release
                bool isSuddenSpike = currentLevel > ( longTermPeak * 1.3f ) && ( targetGain < currentGain );

                float smoothingCoeff = isSuddenSpike ? attackCoeff : releaseCoeff;

                // If it's a new channel, adjust a bit slower to "gradually level them in"
                if ( isNew )
                    smoothingCoeff *= 0.5f;

                // Apply smoothed gain change
                float oldGain = currentGain;
                currentGain += ( targetGain - currentGain ) * smoothingCoeff;

                // Only send to client if the change is noticeable (> 0.2%)
                if ( std::abs ( currentGain - oldGain ) > ( oldGain * 0.002f ) )
                {
                    jamulus_client_set_channel_gain ( jamulusClient, id, currentGain );
                }
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
            settingsDialog->setAutoLevelBoostCallback ( [this] ( bool enabled ) { autoLevelBoostEnabled = enabled; } );
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
            strip->onGainChanged = [this, stripPtr] ( int id, float gain ) {
                float oldGain = channelGains.count ( id ) ? channelGains[id] : 0.75f;

                // Use the previous non-zero gain as reference if the current gain is 0
                float refOldGain = ( oldGain > 0.001f ) ? oldGain : ( previousGains.count ( id ) ? previousGains[id] : 0.75f );

                channelGains[id] = gain;
                if ( gain > 0.001f )
                    previousGains[id] = gain;

                float boost = channelBoosts.count ( id ) && channelBoosts[id] ? 2.0f : 1.0f;
                jamulus_client_set_channel_gain ( jamulusClient, id, gain * boost );

                // Proportional Group Sync
                int gid = channelGroups.count ( id ) ? channelGroups[id] : -1;
                if ( gid != -1 && refOldGain > 0.001f )
                {
                    float ratio = gain / refOldGain;
                    for ( auto& s : channelStrips )
                    {
                        int otherId = s->getChannelId();
                        if ( otherId != id && channelGroups.count ( otherId ) && channelGroups[otherId] == gid )
                        {
                            float otherG   = channelGains.count ( otherId ) ? channelGains[otherId] : 0.75f;
                            float otherRef = ( otherG > 0.001f ) ? otherG : ( previousGains.count ( otherId ) ? previousGains[otherId] : 0.75f );

                            float otherNewGain = juce::jlimit ( 0.0f, 1.0f, otherRef * ratio );

                            if ( std::abs ( otherNewGain - otherG ) > 0.0001f )
                            {
                                channelGains[otherId] = otherNewGain;
                                if ( otherNewGain > 0.001f )
                                    previousGains[otherId] = otherNewGain;

                                s->setGain ( otherNewGain );
                                float otherBoost = channelBoosts.count ( otherId ) && channelBoosts[otherId] ? 2.0f : 1.0f;
                                jamulus_client_set_channel_gain ( jamulusClient, otherId, otherNewGain * otherBoost );
                            }
                        }
                    }
                }
            };
            strip->onPanChanged   = [this] ( int id, float pan ) { jamulus_client_set_channel_pan ( jamulusClient, id, pan ); };
            strip->onMuteChanged  = [this] ( int id, bool muted ) { jamulus_client_set_channel_mute ( jamulusClient, id, muted ); };
            strip->onSoloChanged  = [this] ( int id, bool solo ) { jamulus_client_set_channel_solo ( jamulusClient, id, solo ); };
            strip->onBoostChanged = [this] ( int id, bool boosted ) {
                channelBoosts[id] = boosted;
                float gain        = channelGains.count ( id ) ? channelGains[id] : 0.75f;
                float boost       = boosted ? 2.0f : 1.0f; // +6dB
                jamulus_client_set_channel_gain ( jamulusClient, id, gain * boost );
            };
            strip->onGroupChanged = [this] ( int id, int groupId ) { channelGroups[id] = groupId; };

            // Restore states
            if ( ci.info.id != -1 )
            {
                strip->setMute ( ci.info.muted );
                strip->setSolo ( ci.info.solo );
                if ( channelBoosts.count ( ci.info.id ) )
                    strip->setBoost ( channelBoosts[ci.info.id] );
                if ( channelGroups.count ( ci.info.id ) )
                    strip->setGroup ( channelGroups[ci.info.id] );
                if ( channelGains.count ( ci.info.id ) )
                    strip->setGain ( channelGains[ci.info.id] );
                else
                    channelGains[ci.info.id] = ci.info.gain;
            }

            mixerContainer.addAndMakeVisible ( strip.get() );
            channelStrips.push_back ( std::move ( strip ) );
        }

        updateMixerLayout();
    }

    void updateMixerLayout()
    {
        const int standardStripWidth = 90;
        const int localStripWidth    = 105;
        const int stripHeight        = mixerViewport.getHeight() - 20;

        int totalWidth = 0;
        if ( !channelStrips.empty() )
        {
            // First strip is "Me" - wider
            totalWidth += localStripWidth;
            // Others
            totalWidth += static_cast<int> ( channelStrips.size() - 1 ) * standardStripWidth;
        }

        if ( totalWidth < mixerViewport.getWidth() )
            totalWidth = mixerViewport.getWidth();

        mixerContainer.setSize ( totalWidth, stripHeight );

        int x = 10;
        for ( size_t i = 0; i < channelStrips.size(); ++i )
        {
            int width = ( i == 0 ) ? localStripWidth : standardStripWidth;
            channelStrips[i]->setBounds ( x, 10, width - 10, stripHeight - 20 );
            x += width;
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

    // Main volume control
    juce::Label  mainVolumeLabel;
    juce::Slider mainVolumeSlider;
    float        mainVolume = 1.0f;

    // Auto Level controls
    juce::TextButton     autoLevelButton;
    juce::TextButton     autoLevelSettingsButton;
    bool                 autoLevelEnabled   = false;
    float                autoLevelAttackMs  = 50.0f;           // Fast attack for limiting loud signals
    float                autoLevelReleaseMs = 600.0f;          // Slow release for smooth recovery
    std::map<int, float> autoLevelTargetGains;                 // Smoothed target gains per channel
    std::map<int, float> autoLevelCurrentGains;                // Current applied gains per channel
    std::map<int, float> autoLevelPeakLevels;                  // Peak hold for attack detection
    std::map<int, int>   autoLevelPeakHoldCounters;            // Hold duration before decay starts
    std::map<int, int>   autoLevelChannelActiveTicks;          // How many ticks a channel has been auto-leveled
    float                autoLevelSmoothedChannelCount = 1.0f; // Smoothed count for global scaling

    // Network stats
    juce::Label delayLabel;
    juce::Label jitterLabel;
    StatusLight delayLight;  // glowing
    StatusLight jitterLight; // glowing

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

    LevelMeter masterMeterL;
    LevelMeter masterMeterR;

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
                g.drawVerticalLine ( 93, 5.0f, (float) getHeight() - 5.0f ); // Adjusted for wider strips
            }
        }
    } mixerContainer;
    std::vector<std::unique_ptr<ChannelStrip>> channelStrips;
    std::map<int, float>                       channelGains;  // Track fader values per channel
    std::map<int, float>                       previousGains; // Track last non-zero gain for sync
    std::map<int, bool>                        channelBoosts; // Track boost state per channel
    std::map<int, int>                         channelGroups; // Track group state (ID) per channel
    std::map<int, float>                       channelLevels; // Track current levels for auto-leveling
    bool                                       autoLevelBoostEnabled = false;

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
        {
            // Warm Yellow Glow (0xFFFFCC00) with Black text for contrast
            juce::Colour yellowGlow ( 0xffffcc00 );
            monitorModeButton.setColour ( juce::TextButton::buttonColourId, yellowGlow );
            monitorModeButton.setColour ( juce::TextButton::buttonOnColourId, yellowGlow );
            monitorModeButton.setColour ( juce::TextButton::textColourOnId, juce::Colours::black );
            monitorModeButton.setColour ( juce::TextButton::textColourOffId, juce::Colours::black );
        }
        else
        {
            juce::Colour defaultColor = juce::LookAndFeel::getDefaultLookAndFeel().findColour ( juce::TextButton::buttonColourId );
            monitorModeButton.setColour ( juce::TextButton::buttonColourId, defaultColor );
            monitorModeButton.setColour ( juce::TextButton::buttonOnColourId, defaultColor );
            monitorModeButton.setColour ( juce::TextButton::textColourOnId, juce::Colours::white );
            monitorModeButton.setColour ( juce::TextButton::textColourOffId, juce::Colours::white );
        }
    }

    void updateAutoLevelButtonColor()
    {
        if ( autoLevelButton.getToggleState() )
        {
            // Green glow when auto level is active
            juce::Colour greenGlow ( 0xff00cc66 );
            autoLevelButton.setColour ( juce::TextButton::buttonColourId, greenGlow );
            autoLevelButton.setColour ( juce::TextButton::buttonOnColourId, greenGlow );
            autoLevelButton.setColour ( juce::TextButton::textColourOnId, juce::Colours::black );
            autoLevelButton.setColour ( juce::TextButton::textColourOffId, juce::Colours::black );
        }
        else
        {
            juce::Colour defaultColor = juce::LookAndFeel::getDefaultLookAndFeel().findColour ( juce::TextButton::buttonColourId );
            autoLevelButton.setColour ( juce::TextButton::buttonColourId, defaultColor );
            autoLevelButton.setColour ( juce::TextButton::buttonOnColourId, defaultColor );
            autoLevelButton.setColour ( juce::TextButton::textColourOnId, juce::Colours::white );
            autoLevelButton.setColour ( juce::TextButton::textColourOffId, juce::Colours::white );
        }
    }

    void showAutoLevelSettingsPopup()
    {
        auto popup = std::make_unique<juce::Component>();
        popup->setSize ( 220, 130 );

        // Attack Time
        auto* attackLabel = new juce::Label ( "attackLabel", "Attack Time:" );
        attackLabel->setColour ( juce::Label::textColourId, juce::Colours::white );
        attackLabel->setBounds ( 10, 10, 100, 25 );
        popup->addAndMakeVisible ( attackLabel );

        auto* attackSlider = new juce::Slider();
        attackSlider->setRange ( 10.0, 500.0, 5.0 );
        attackSlider->setValue ( autoLevelAttackMs );
        attackSlider->setTextBoxStyle ( juce::Slider::TextBoxRight, false, 50, 20 );
        attackSlider->setTextValueSuffix ( " ms" );
        attackSlider->setBounds ( 10, 35, 200, 25 );
        attackSlider->onValueChange = [this, attackSlider]() { autoLevelAttackMs = static_cast<float> ( attackSlider->getValue() ); };
        popup->addAndMakeVisible ( attackSlider );

        // Release Time
        auto* releaseLabel = new juce::Label ( "releaseLabel", "Release Time:" );
        releaseLabel->setColour ( juce::Label::textColourId, juce::Colours::white );
        releaseLabel->setBounds ( 10, 65, 100, 25 );
        popup->addAndMakeVisible ( releaseLabel );

        auto* releaseSlider = new juce::Slider();
        releaseSlider->setRange ( 100.0, 3000.0, 50.0 );
        releaseSlider->setValue ( autoLevelReleaseMs );
        releaseSlider->setTextBoxStyle ( juce::Slider::TextBoxRight, false, 50, 20 );
        releaseSlider->setTextValueSuffix ( " ms" );
        releaseSlider->setBounds ( 10, 90, 200, 25 );
        releaseSlider->onValueChange = [this, releaseSlider]() { autoLevelReleaseMs = static_cast<float> ( releaseSlider->getValue() ); };
        popup->addAndMakeVisible ( releaseSlider );

        juce::CallOutBox::launchAsynchronously ( std::move ( popup ), autoLevelSettingsButton.getScreenBounds(), nullptr );
    }

public:
    // Callbacks that the editor can set
    std::function<void ( bool )>  onSidechainChanged;
    std::function<void ( bool )>  onMonitorModeChanged; // true = server return, false = direct thru
    std::function<void ( float )> onMainVolumeChanged;  // 0.0 - 1.0

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
