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
        targetLevel = juce::jlimit ( 0.0f, 1.5f, newLevel ); // Allow slightly over 1.0 for clip testing
    }

    void setHorizontal ( bool isHorizontal )
    {
        horizontal = isHorizontal;
        repaint();
    }

    void paint ( juce::Graphics& g ) override
    {
        auto bounds = getLocalBounds().toFloat();

        if ( horizontal )
        {
            // Simple horizontal bar for master or small strips (internal use)
            g.setColour ( juce::Colour ( 0xff212121 ) );
            g.fillRoundedRectangle ( bounds, 1.0f );

            float levelWidth = bounds.getWidth() * juce::jlimit ( 0.0f, 1.0f, currentLevel );
            g.setColour ( currentLevel > 0.95f ? juce::Colours::red : ( currentLevel > 0.7f ? juce::Colours::yellow : juce::Colour ( 0xff00cc66 ) ) );
            g.fillRect ( bounds.getX(), bounds.getY(), levelWidth, bounds.getHeight() );
            return;
        }

        // Pro-style Vertical Stereo Meter with dB Scale (+6 to -60)
        g.fillAll ( juce::Colour ( 0xff121212 ) );

        auto meterArea = bounds.reduced ( 1 );

        // 1. Draw labels and scale lines
        g.setColour ( juce::Colours::grey.withAlpha ( 0.5f ) );
        g.setFont ( 7.0f );

        const float labels[] = { 6.0f, 0.0f, -6.0f, -12.0f, -18.0f, -24.0f, -30.0f, -40.0f, -50.0f, -60.0f };
        for ( float db : labels )
        {
            float y = dbToY ( db, meterArea );
            g.drawLine ( meterArea.getX(), y, meterArea.getRight(), y, 0.4f );

            // Draw text for key values
            if ( (int) db % 6 == 0 || db == 6.0f )
                g.drawText ( juce::String ( (int) std::abs ( db ) ), meterArea.getX(), y - 4, meterArea.getWidth(), 8, juce::Justification::centred );
        }

        // 2. Draw the two bars (stereo look)
        float barWidth = ( meterArea.getWidth() * 0.35f );
        auto  leftBar  = meterArea.removeFromLeft ( barWidth ).reduced ( 1, 0 );
        auto  rightBar = meterArea.removeFromRight ( barWidth ).reduced ( 1, 0 );

        drawBar ( g, leftBar, currentLevel );
        drawBar ( g, rightBar, currentLevel );
    }

private:
    float dbToY ( float db, juce::Rectangle<float> area ) const
    {
        // Scale: +6dB is top, -60dB is bottom
        float normalized = juce::jmap ( db, -60.0f, 6.0f, 0.0f, 1.0f );
        normalized       = juce::jlimit ( 0.0f, 1.0f, normalized );
        return area.getBottom() - ( normalized * area.getHeight() );
    }

    float linearToDb ( float linear ) const { return ( linear <= 0.00001f ) ? -100.0f : 20.0f * std::log10 ( linear ); }

    void drawBar ( juce::Graphics& g, juce::Rectangle<float> area, float level )
    {
        g.setColour ( juce::Colours::black.withAlpha ( 0.6f ) );
        g.fillRect ( area );

        float db = linearToDb ( level );
        float y  = dbToY ( db, area );

        if ( y < area.getBottom() )
        {
            auto levelRect = juce::Rectangle<float> ( area.getX(), y, area.getWidth(), area.getBottom() - y );

            // Gradient: Green up to -12, Yellow to 0, Red above 0
            juce::ColourGradient grad;
            grad.isRadial = false;
            grad.point1   = { 0, area.getBottom() };
            grad.point2   = { 0, area.getY() };

            grad.addColour ( 0.0, juce::Colour ( 0xff00cc66 ) ); // Green at bottom
            grad.addColour ( juce::jmap ( -12.0f, -60.0f, 6.0f, 0.0f, 1.0f ), juce::Colour ( 0xff00cc66 ) );
            grad.addColour ( juce::jmap ( 0.0f, -60.0f, 6.0f, 0.0f, 1.0f ), juce::Colours::yellow );
            grad.addColour ( juce::jmap ( 0.1f, -60.0f, 6.0f, 0.0f, 1.0f ), juce::Colours::red );
            grad.addColour ( 1.0, juce::Colours::red );

            g.setGradientFill ( grad );
            g.fillRect ( levelRect );
        }

        // Clip indicator
        if ( db > 0.0f )
        {
            g.setColour ( juce::Colours::red );
            g.fillRect ( area.getX(), area.getY(), area.getWidth(), 2.0f );
        }
    }

    void timerCallback() override
    {
        const float smoothing = 0.25f;
        float       newLevel  = currentLevel + ( targetLevel - currentLevel ) * smoothing;

        if ( targetLevel < currentLevel )
            newLevel = currentLevel * 0.85f; // Faster drop for "peak" feel

        if ( std::abs ( newLevel - currentLevel ) > 0.0001f )
        {
            currentLevel = newLevel;
            repaint();
        }
    }

    bool  horizontal   = false;
    float currentLevel = 0.0f;
    float targetLevel  = 0.0f;
};

// 1. Custom LookAndFeel for the Fader "Lever" Knob
class FaderLookAndFeel : public juce::LookAndFeel_V4
{
public:
    void drawLinearSlider ( juce::Graphics&                 g,
                            int                             x,
                            int                             y,
                            int                             width,
                            int                             height,
                            float                           sliderPos,
                            float                           minSliderPos,
                            float                           maxSliderPos,
                            const juce::Slider::SliderStyle style,
                            juce::Slider&                   slider ) override
    {
        // Draw track (thin line)
        auto                   trackWidth = 4.0f;
        juce::Rectangle<float> track ( x + width * 0.5f - trackWidth * 0.5f, (float) y, trackWidth, (float) height );
        g.setColour ( juce::Colour ( 0xff333333 ) );
        g.fillRoundedRectangle ( track, 2.0f );

        // Draw thumb (The "Lever")
        auto thumbWidth  = width * 0.8f; // Wide
        auto thumbHeight = 16.0f;        // Tall lever
        auto thumbY      = sliderPos - ( thumbHeight * 0.5f );

        juce::Rectangle<float> thumbBounds ( x + ( width - thumbWidth ) * 0.5f, thumbY, thumbWidth, thumbHeight );

        // Draw shadow
        g.setColour ( juce::Colours::black.withAlpha ( 0.5f ) );
        g.fillRect ( thumbBounds.translated ( 1, 2 ) );

        // Draw main body gradient
        juce::Colour baseColor = slider.findColour ( juce::Slider::thumbColourId );
        g.setGradientFill (
            juce::ColourGradient::vertical ( baseColor.brighter ( 0.1f ), thumbBounds.getY(), baseColor.darker ( 0.3f ), thumbBounds.getBottom() ) );
        g.fillRect ( thumbBounds );

        // Draw outline
        g.setColour ( juce::Colours::black.withAlpha ( 0.6f ) );
        g.drawRect ( thumbBounds, 1.0f );

        // Draw grip line
        g.setColour ( juce::Colours::black.withAlpha ( 0.3f ) );
        float gripY = thumbBounds.getCentreY();
        g.drawLine ( thumbBounds.getX() + 4, gripY, thumbBounds.getRight() - 4, gripY, 1.0f );
        g.setColour ( juce::Colours::white.withAlpha ( 0.2f ) );
        g.drawLine ( thumbBounds.getX() + 4, gripY + 1, thumbBounds.getRight() - 4, gripY + 1, 1.0f );
    }
};

// 2. Custom Slider for Pan to handle Right-Click Context Menu
class PanSlider : public juce::Slider
{
public:
    std::function<void()> onRightClick;

    void mouseDown ( const juce::MouseEvent& e ) override
    {
        if ( e.mods.isPopupMenu() )
        {
            if ( onRightClick )
                onRightClick();
            return;
        }
        juce::Slider::mouseDown ( e );
    }
};

//==============================================================================
// Channel Strip Component (for mixer)
//==============================================================================
class ChannelStrip : public juce::Component
{
public:
    ChannelStrip()
    {
        // 1. Labels
        addAndMakeVisible ( nameLabel );
        nameLabel.setFont ( juce::Font ( 12.0f, juce::Font::bold ) );
        nameLabel.setJustificationType ( juce::Justification::centred );
        nameLabel.setColour ( juce::Label::textColourId, juce::Colours::white );
        nameLabel.setMinimumHorizontalScale ( 0.8f );

        // 2. dB Readouts
        addAndMakeVisible ( faderDbLabel );
        faderDbLabel.setFont ( juce::Font ( 11.0f ) );
        faderDbLabel.setJustificationType ( juce::Justification::centred );
        faderDbLabel.setColour ( juce::Label::backgroundColourId, juce::Colours::black.withAlpha ( 0.5f ) );
        faderDbLabel.setColour ( juce::Label::textColourId, juce::Colours::white );
        faderDbLabel.setText ( "0.0", juce::dontSendNotification );

        addAndMakeVisible ( peakDbLabel );
        peakDbLabel.setFont ( juce::Font ( 11.0f, juce::Font::bold ) );
        peakDbLabel.setJustificationType ( juce::Justification::centred );
        peakDbLabel.setColour ( juce::Label::backgroundColourId, juce::Colours::black.withAlpha ( 0.5f ) );
        peakDbLabel.setColour ( juce::Label::textColourId, juce::Colour ( 0xff00cc66 ) );
        peakDbLabel.setText ( "-oo", juce::dontSendNotification );

        // 3. Sliders
        addAndMakeVisible ( faderSlider );
        faderSlider.setLookAndFeel ( &faderLook ); // Apply custom "Lever" look
        faderSlider.setSliderStyle ( juce::Slider::LinearVertical );
        faderSlider.setRange ( -60.0, 6.0 ); // dB scale
        faderSlider.setValue ( 0.0 );        // 0dB
        faderSlider.setScrollWheelEnabled ( true );
        faderSlider.setDoubleClickReturnValue ( true, 0.0 );
        faderSlider.setTextBoxStyle ( juce::Slider::NoTextBox, true, 0, 0 );
        faderSlider.setColour ( juce::Slider::thumbColourId, juce::Colour ( 0xffe0e0e0 ) ); // Lighter thumb
        faderSlider.setColour ( juce::Slider::trackColourId, juce::Colour ( 0x313131 ) );

        faderSlider.onValueChange = [this]() {
            float db = (float) faderSlider.getValue();
            // Display "-oo" at minimum
            faderDbLabel.setText ( ( db <= -59.5f ) ? "-oo" : juce::String ( db, 1 ), juce::dontSendNotification );

            if ( onGainChanged )
            {
                // Convert back to linear for Jamulus
                float lin = ( db <= -59.5f ) ? 0.0f : std::pow ( 10.0f, db / 20.0f );
                onGainChanged ( channelId, lin );
            }
        };

        addAndMakeVisible ( panSlider );
        panSlider.setSliderStyle ( juce::Slider::RotaryHorizontalVerticalDrag );
        panSlider.setRange ( -1.0, 1.0 );
        panSlider.setValue ( 0.0 );
        panSlider.setScrollWheelEnabled ( true );
        panSlider.setDoubleClickReturnValue ( true, 0.0 );
        panSlider.setTextBoxStyle ( juce::Slider::NoTextBox, true, 0, 0 );
        // Style as a small silver knob
        panSlider.setColour ( juce::Slider::rotarySliderFillColourId, juce::Colours::transparentBlack );
        panSlider.setColour ( juce::Slider::rotarySliderOutlineColourId, juce::Colour ( 0xff444444 ) );

        // Handle Pan Changes
        panSlider.onValueChange = [this]() {
            if ( isMono )
            {
                // Force center if mono
                if ( std::abs ( panSlider.getValue() ) > 0.001 )
                    panSlider.setValue ( 0.0, juce::dontSendNotification );
                return;
            }
            if ( onPanChanged )
                onPanChanged ( channelId, static_cast<float> ( panSlider.getValue() ) );
        };

        // Handle Right-Click context menu
        panSlider.onRightClick = [this]() {
            juce::PopupMenu m;
            m.addSectionHeader ( "Channel Mode" );
            m.addItem ( "Stereo", true, !isMono, [this]() { setMonoMode ( false ); } );
            m.addItem ( "Mono", true, isMono, [this]() { setMonoMode ( true ); } );
            m.showMenuAsync ( juce::PopupMenu::Options() );
        };

        // 4. Buttons
        addAndMakeVisible ( muteButton );
        muteButton.setButtonText ( "M" );
        muteButton.setClickingTogglesState ( true );
        muteButton.setTooltip ( "Mute" );
        muteButton.onClick = [this]() {
            updateMuteButtonColor();
            if ( onMuteChanged )
                onMuteChanged ( channelId, muteButton.getToggleState() );
        };

        addAndMakeVisible ( soloButton );
        soloButton.setButtonText ( "S" );
        soloButton.setClickingTogglesState ( true );
        soloButton.setTooltip ( "Solo" );
        soloButton.onClick = [this]() {
            updateSoloButtonColor();
            if ( onSoloChanged )
                onSoloChanged ( channelId, soloButton.getToggleState() );
        };

        addAndMakeVisible ( boostButton );
        boostButton.setButtonText ( "B" );
        boostButton.setClickingTogglesState ( true );
        boostButton.setTooltip ( "Boost +6dB" );
        boostButton.onClick = [this]() {
            updateBoostButtonColor();
            if ( onBoostChanged )
                onBoostChanged ( channelId, boostButton.getToggleState() );
        };

        addAndMakeVisible ( groupButton );
        groupButton.setButtonText ( "G" );
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
                    int gid = result - 2;
                    setGroup ( gid );
                    if ( onGroupChanged )
                        onGroupChanged ( channelId, gid );
                }
            } );
        };
        groupButton.addMouseListener ( this, false );

        addAndMakeVisible ( levelMeter );
    }

    ~ChannelStrip() override { faderSlider.setLookAndFeel ( nullptr ); }

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

    bool isMuted() const { return muteButton.getToggleState(); }
    bool isSoloed() const { return soloButton.getToggleState(); }
    bool getBoost() const { return boostButton.getToggleState(); }

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

        nameLabel.setVisible ( true );
        nameLabel.setColour ( juce::Label::backgroundColourId, juce::Colours::transparentBlack );
        nameLabel.setColour ( juce::Label::textColourId, juce::Colours::black );

        repaint();
    }

    void setLevel ( float level )
    {
        levelMeter.setLevel ( level );

        // Update peak dB readout
        float db = ( level <= 0.0001f ) ? -100.0f : 20.0f * std::log10 ( level );
        if ( db < -90.0f )
            peakDbLabel.setText ( "-oo", juce::dontSendNotification );
        else
            peakDbLabel.setText ( juce::String ( (int) db ), juce::dontSendNotification );

        // Color peak box based on level
        if ( level > 1.0f )
            peakDbLabel.setColour ( juce::Label::textColourId, juce::Colours::red );
        else
            peakDbLabel.setColour ( juce::Label::textColourId, juce::Colour ( 0xff006633 ) ); // Darker Green for light backgrounds
    }

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

    // New: Mono Mode
    void setMonoMode ( bool mono )
    {
        isMono = mono;
        if ( isMono )
        {
            panSlider.setValue ( 0.0, juce::sendNotification ); // Force center
            panSlider.setEnabled ( false );
            panSlider.setAlpha ( 0.5f );
        }
        else
        {
            panSlider.setEnabled ( true );
            panSlider.setAlpha ( 1.0f );
        }
        if ( onMonoChanged )
            onMonoChanged ( channelId, isMono );
    }
    float getGain() const
    {
        float db = (float) faderSlider.getValue();
        return ( db <= -59.5f ) ? 0.0f : std::pow ( 10.0f, db / 20.0f );
    }

    void setGain ( float g )
    {
        float db = ( g <= 0.00001f ) ? -60.0f : 20.0f * std::log10 ( g );
        faderSlider.setValue ( db, juce::dontSendNotification );
        faderDbLabel.setText ( ( db <= -59.5f ) ? "-oo" : juce::String ( db, 1 ), juce::dontSendNotification );
    }

    int  getChannelId() const { return channelId; }
    bool getIsMe() const { return isMe; }

    void paint ( juce::Graphics& g ) override
    {
        auto totalBounds = getLocalBounds().toFloat();

        // Define shape: Rounded on top (radius 8), straight on bottom
        juce::Path shape;
        float      cornerRadius = 8.0f;
        shape.addRoundedRectangle ( totalBounds.getX(),
                                    totalBounds.getY(),
                                    totalBounds.getWidth(),
                                    totalBounds.getHeight(),
                                    cornerRadius,
                                    cornerRadius,
                                    true,
                                    true,
                                    false,
                                    false ); // Top-Left, Top-Right, Bottom-Left, Bottom-Right

        g.saveState();
        g.reduceClipRegion ( shape );

        // 1. Background (Exact Pale Skill colors from image)
        juce::Colour bgCol = juce::Colours::white; // Default (None/White)

        if ( skillLevel == 1 )
            bgCol = juce::Colour ( 0xffffffea ); // Beginner: Pale Yellow
        else if ( skillLevel == 2 )
            bgCol = juce::Colour ( 0xffeaffea ); // Intermediate: Pale Green
        else if ( skillLevel == 3 )
            bgCol = juce::Colour ( 0xffffeaea ); // Expert: Pale Red

        // Blend 'me' highlight into the base color once
        if ( isMe )
            bgCol = bgCol.interpolatedWith ( juce::Colours::orange, 0.12f );

        g.setColour ( bgCol );
        g.fillAll();

        // 2. Draw Header Area
        // Flag overlay: Top full width, 34px height (taller as requested)
        if ( flagIcon.isValid() )
        {
            auto fBounds = juce::Rectangle<float> ( 0, 0, totalBounds.getWidth(), 34.0f );
            g.drawImage ( flagIcon, fBounds, juce::RectanglePlacement::stretchToFit );
        }

        // Instrument Icon: Below flag, centered, moved down a touch
        if ( instrumentIcon.isValid() )
        {
            auto iBounds = juce::Rectangle<float> ( 0, 38.0f, totalBounds.getWidth(), 36.0f );
            g.drawImage ( instrumentIcon, iBounds, juce::RectanglePlacement::centred );
        }

        g.restoreState();

        // 3. Draw Fader Ticks (Darker and aligned with fader)
        auto faderBounds = faderSlider.getBounds().toFloat();
        g.setColour ( juce::Colours::black.withAlpha ( 0.6f ) );

        // Scale: +6dB to -60dB (matched to LevelMeter)
        float tickDbs[] = { 6.0f, 0.0f, -6.0f, -12.0f, -18.0f, -24.0f, -30.0f, -36.0f, -42.0f, -48.0f, -54.0f, -60.0f };
        for ( float db : tickDbs )
        {
            float normalized = juce::jmap ( db, -60.0f, 6.0f, 0.0f, 1.0f );
            float y          = faderBounds.getBottom() - ( normalized * faderBounds.getHeight() );

            // Draw ticks to the left of the centered fader track
            float tickWidth = ( db == 0.0f ) ? 5.0f : 3.0f;
            g.drawLine ( faderBounds.getX() - tickWidth - 2, y, faderBounds.getX() - 2, y, 1.2f );
        }

        // 4. Draw Border around the ENTIRE strip (using the same shape)
        g.setColour ( isMe ? juce::Colours::orange : juce::Colours::black.withAlpha ( 0.2f ) );
        g.strokePath ( shape, juce::PathStrokeType ( isMe ? 2.0f : 1.0f ) );
    }

    void resized() override
    {
        auto bounds = getLocalBounds().reduced ( 4 );

        // 1. Header Area
        bounds.removeFromTop ( 72 ); // Reduced to 72 to tighten space between icon and label
        nameLabel.setBounds ( bounds.removeFromTop ( 20 ) );
        panSlider.setBounds ( bounds.removeFromTop ( 40 ).reduced ( 15, 0 ) );

        // 2. dB Readouts
        auto readoutArea  = bounds.removeFromTop ( 18 );
        auto leftReadout  = readoutArea.removeFromLeft ( readoutArea.getWidth() / 2 );
        auto rightReadout = readoutArea;

        faderDbLabel.setBounds ( leftReadout.reduced ( 1 ) );
        peakDbLabel.setBounds ( rightReadout.reduced ( 1 ) );

        // 3. Buttons at bottom (Ordered: M/S above B/G)
        auto bottomRow = bounds.removeFromBottom ( 22 );
        boostButton.setBounds ( bottomRow.removeFromLeft ( bottomRow.getWidth() / 2 ).reduced ( 1 ) );
        groupButton.setBounds ( bottomRow.reduced ( 1 ) );

        auto subBottom = bounds.removeFromBottom ( 22 );
        muteButton.setBounds ( subBottom.removeFromLeft ( subBottom.getWidth() / 2 ).reduced ( 1 ) );
        soloButton.setBounds ( subBottom.reduced ( 1 ) );

        // 4. Fader and Meter (Centered under labels)
        bounds.removeFromTop ( 4 );
        bounds.removeFromBottom ( 4 );

        auto faderArea = bounds;
        auto leftCol   = faderArea.removeFromLeft ( faderArea.getWidth() / 2 );
        auto rightCol  = faderArea;

        // Peak Meter: Centered in right column
        levelMeter.setBounds ( rightCol.withSizeKeepingCentre ( 14, rightCol.getHeight() ) );

        // Volume Fader: Centered in left column, matching meter height
        faderSlider.setBounds ( leftCol.withSizeKeepingCentre ( 24, leftCol.getHeight() ) ); // Wider slot for lever knob
    }

    std::function<void ( int, float )> onGainChanged;
    std::function<void ( int, float )> onPanChanged;
    std::function<void ( int, bool )>  onMuteChanged;
    std::function<void ( int, bool )>  onSoloChanged;
    std::function<void ( int, bool )>  onBoostChanged;
    std::function<void ( int, int )>   onGroupChanged;
    std::function<void ( int, bool )>  onMonoChanged;

private:
    int              channelId  = -1;
    bool             isMe       = false;
    bool             isMono     = false;
    int              skillLevel = 0;
    int              groupID    = -1;
    juce::Label      nameLabel;
    juce::Label      faderDbLabel;
    juce::Label      peakDbLabel;
    juce::Image      instrumentIcon;
    juce::Image      flagIcon;
    juce::Slider     faderSlider;
    PanSlider        panSlider; // Use custom PanSlider
    juce::TextButton muteButton;
    juce::TextButton soloButton;
    juce::TextButton boostButton;
    juce::TextButton groupButton;
    LevelMeter       levelMeter;
    FaderLookAndFeel faderLook; // Custom look
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
            if ( !autoLevelEnabled )
            {
                // Restore gains to manual settings when disabled
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
        statusLabel.setJustificationType ( juce::Justification::centred );

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

        // Initialize delay effect - Handled in Processor
        // audioDelay.init ( 48000, 1000 );

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

        // Local (Me) strip container - fixed, not scrollable
        addAndMakeVisible ( localStripContainer );

        // Mixer viewport - only for remote channels
        addAndMakeVisible ( mixerViewport );
        mixerViewport.setViewedComponent ( &mixerContainer, false );

        // Scroll indicator arrows
        addAndMakeVisible ( scrollLeftArrow );
        scrollLeftArrow.setButtonText ( "<" );
        scrollLeftArrow.setColour ( juce::TextButton::buttonColourId, juce::Colour ( 0xff505050 ) );
        scrollLeftArrow.setColour ( juce::TextButton::textColourOnId, juce::Colours::white );
        scrollLeftArrow.setColour ( juce::TextButton::textColourOffId, juce::Colours::white );
        scrollLeftArrow.onClick = [this]() {
            auto pos  = mixerViewport.getViewPosition();
            int  newX = juce::jmax ( 0, pos.x - 90 ); // Scroll one channel width
            mixerViewport.setViewPosition ( newX, pos.y );
            updateScrollArrows();
        };

        addAndMakeVisible ( scrollRightArrow );
        scrollRightArrow.setButtonText ( ">" );
        scrollRightArrow.setColour ( juce::TextButton::buttonColourId, juce::Colour ( 0xff505050 ) );
        scrollRightArrow.setColour ( juce::TextButton::textColourOnId, juce::Colours::white );
        scrollRightArrow.setColour ( juce::TextButton::textColourOffId, juce::Colours::white );
        scrollRightArrow.onClick = [this]() {
            auto pos  = mixerViewport.getViewPosition();
            int  maxX = mixerContainer.getWidth() - mixerViewport.getWidth();
            int  newX = juce::jmin ( maxX, pos.x + 90 ); // Scroll one channel width
            mixerViewport.setViewPosition ( newX, pos.y );
            updateScrollArrows();
        };

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
        g.drawVerticalLine ( 142, 50.0f, 190.0f ); // Draw line separating local controls (moved closer)
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

        // Toolbar buttons - Chat next to Connect
        auto toolbar = header;
        connectButton.setBounds ( toolbar.removeFromLeft ( 90 ).reduced ( 5 ) );
        chatButton.setBounds ( toolbar.removeFromLeft ( 70 ).reduced ( 5 ) );
        settingsButton.setBounds ( toolbar.removeFromLeft ( 90 ).reduced ( 5 ) );
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

        // Status Label (Connected...) - Moved here above Ping
        auto statusRow = leftControls.removeFromTop ( 20 );
        statusLabel.setBounds ( statusRow );

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
        leftControls.removeFromTop ( 5 ); // Reduced spacer

        // Buttons row (FX, MON) - Moved below stats, Centered
        auto buttonsRow  = leftControls.removeFromTop ( 30 );
        auto centerGroup = buttonsRow.withSizeKeepingCentre ( 105, buttonsRow.getHeight() ); // 50 + 5 + 50 = 105
        fxButton.setBounds ( centerGroup.removeFromLeft ( 50 ).reduced ( 2 ) );
        centerGroup.removeFromLeft ( 5 ); // Gap
        monitorModeButton.setBounds ( centerGroup.removeFromLeft ( 50 ).reduced ( 2 ) );

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

        // Right side - Info/Status (Status label moved to left)
        auto rightControls = controlsArea.removeFromRight ( 200 );
        // statusLabel removed from here

        // Mixer area - local strip fixed on left, remote channels scroll
        bounds.removeFromTop ( 10 );

        // Local (Me) strip on the left - fixed, no scrolling
        const int localStripWidth = 105;
        auto      localBounds     = bounds.removeFromLeft ( localStripWidth );
        localStripContainer.setBounds ( localBounds );

        // Remote channels viewport (scrollable) with scroll arrows
        const int arrowWidth = 20;

        // Left scroll arrow (between local strip and viewport)
        auto leftArrowBounds = bounds.removeFromLeft ( arrowWidth );
        scrollLeftArrow.setBounds ( leftArrowBounds.reduced ( 2, 30 ) );

        // Main viewport for remote channels
        auto viewportBounds = bounds.removeFromRight ( bounds.getWidth() - arrowWidth );
        mixerViewport.setBounds ( viewportBounds );

        // Right scroll arrow
        scrollRightArrow.setBounds ( bounds.reduced ( 2, 30 ) );

        // Update mixer container width based on number of channels
        updateMixerLayout();
        updateScrollArrows();
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

        // 2. Identify global Solo state
        bool anySolo = false;
        for ( auto& s : channelStrips )
        {
            if ( !s->getIsMe() && s->isSoloed() )
            {
                anySolo = true;
                break;
            }
        }

        // 3. Update raw input levels map
        std::map<int, float> rawInputLevels;
        for ( int i = 0; i < numChannels; ++i )
        {
            jamulus_channel_info_t info;
            if ( jamulus_client_get_channel_info ( jamulusClient, i, &info ) )
            {
                float lvl               = info.level / 65535.0f;
                rawInputLevels[info.id] = lvl;
                channelLevels[info.id]  = lvl; // Store for auto-level brain
            }
        }

        // 4. Update Auto Level "Brain" Calculation
        if ( autoLevelEnabled && !channelStrips.empty() )
        {
            const float timerIntervalMs    = 50.0f;
            const float noiseFloor         = 0.02f;
            const float targetLevelLinear  = 0.5f;
            const int   holdTicks          = 100;
            const float attackCoeff        = 1.0f - std::exp ( -timerIntervalMs / autoLevelAttackMs );
            const float releaseCoeff       = 1.0f - std::exp ( -timerIntervalMs / autoLevelReleaseMs );
            const float longTermDecayCoeff = 1.0f - std::exp ( -timerIntervalMs / 10000.0f );

            for ( auto& strip : channelStrips )
            {
                int id = strip->getChannelId();
                if ( id == -1 || strip->getIsMe() )
                    continue;

                // Skip grouped channels from the auto-level brain
                if ( channelGroups.count ( id ) && channelGroups[id] != -1 )
                    continue;

                if ( !channelLevels.count ( id ) )
                    continue;
                float currentLevel = channelLevels[id];

                // Initialize tracking if not yet done
                if ( !autoLevelCurrentGains.count ( id ) )
                {
                    float faderVolume         = channelGains.count ( id ) ? channelGains[id] : 0.75f;
                    float boostMult           = ( channelBoosts.count ( id ) && channelBoosts[id] ) ? 2.0f : 1.0f;
                    autoLevelCurrentGains[id] = faderVolume * boostMult;
                }
                if ( !autoLevelPeakLevels.count ( id ) )
                    autoLevelPeakLevels[id] = 0.0f;
                if ( !autoLevelPeakHoldCounters.count ( id ) )
                    autoLevelPeakHoldCounters[id] = 0;
                if ( !autoLevelChannelActiveTicks.count ( id ) )
                    autoLevelChannelActiveTicks[id] = 0;
                else
                    autoLevelChannelActiveTicks[id]++;

                bool   isNew        = autoLevelChannelActiveTicks[id] < 40;
                float& longTermPeak = autoLevelPeakLevels[id];
                int&   holdCounter  = autoLevelPeakHoldCounters[id];

                if ( currentLevel >= noiseFloor )
                {
                    if ( currentLevel > longTermPeak )
                    {
                        longTermPeak = currentLevel;
                        holdCounter  = holdTicks;
                    }
                    else if ( holdCounter > 0 )
                        --holdCounter;
                    else
                        longTermPeak -= ( longTermPeak - currentLevel ) * longTermDecayCoeff;
                }
                else
                {
                    if ( holdCounter > 0 )
                        --holdCounter;
                    else if ( longTermPeak > 0.0f )
                        longTermPeak -= longTermPeak * ( longTermDecayCoeff * 0.5f );
                }

                if ( longTermPeak < noiseFloor )
                {
                    autoLevelCurrentGains[id] += ( 1.0f - autoLevelCurrentGains[id] ) * releaseCoeff;
                    continue;
                }

                float targetGain = juce::jlimit ( 0.1f, 4.0f, targetLevelLinear / longTermPeak );
                float smoothingCoeff =
                    ( currentLevel > ( longTermPeak * 1.3f ) && ( targetGain < autoLevelCurrentGains[id] ) ) ? attackCoeff : releaseCoeff;
                if ( isNew )
                    smoothingCoeff *= 0.5f;

                autoLevelCurrentGains[id] += ( targetGain - autoLevelCurrentGains[id] ) * smoothingCoeff;
            }
        }

        // 5. Final Gain Dispatch and Meter Update pass
        for ( auto& strip : channelStrips )
        {
            int  id   = strip->getChannelId();
            bool isMe = strip->getIsMe();

            // Determine if the channel should be silent
            bool isMutedLocally = strip->isMuted();
            bool isSoloedOut    = anySolo && !strip->isSoloed() && !isMe;
            bool silenced       = isMutedLocally || isSoloedOut;

            float finalGain = 0.0f;
            if ( !silenced )
            {
                // Is this channel controlled by auto-level?
                bool autoLeveled = autoLevelEnabled && !isMe && ( !channelGroups.count ( id ) || channelGroups[id] == -1 );

                if ( autoLeveled && autoLevelCurrentGains.count ( id ) )
                {
                    finalGain = autoLevelCurrentGains[id];
                }
                else
                {
                    float baseFader = channelGains.count ( id ) ? channelGains[id] : 0.75f;
                    // Boost applies to remote tracks. Local track boost is native in Jamulus.
                    float boost = ( !isMe && channelBoosts.count ( id ) && channelBoosts[id] ) ? 2.0f : 1.0f;
                    finalGain   = baseFader * boost;
                }
            }

            // Update meter with produced volume (input * gain)
            if ( isMe )
            {
                strip->setLevel ( ( ( linearL + linearR ) * 0.5f ) * finalGain );
            }
            else if ( id != -1 && rawInputLevels.count ( id ) )
            {
                strip->setLevel ( rawInputLevels[id] * finalGain );
            }

            static std::map<int, float> dispatcherLastSentGains;
            if ( std::abs ( finalGain - dispatcherLastSentGains[id] ) > 0.0001f )
            {
                jamulus_client_set_channel_gain ( jamulusClient, id, finalGain );
                dispatcherLastSentGains[id] = finalGain;
            }
        }

        // Process Qt events to keep the Jamulus client running
        jamulus_process_events();

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
            // settingsDialog->setAutoLevelBoostCallback ( [this] ( bool enabled ) { autoLevelBoostEnabled = enabled; } ); // Removed
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

            // Apply Mono Constraint if current local channel is mono
            fxPanel->setMonoConstraint ( isMeMono );

            // Reverb callbacks - Proxy to Editor
            fxPanel->onReverbEnableChanged = [this] ( bool enabled ) {
                if ( onReverbEnableChanged )
                    onReverbEnableChanged ( enabled );
            };
            fxPanel->onReverbMixChanged = [this] ( float mix ) {
                if ( onReverbMixChanged )
                    onReverbMixChanged ( mix );
            };
            fxPanel->onReverbDecayChanged = [this] ( float decay ) {
                if ( onReverbDecayChanged )
                    onReverbDecayChanged ( decay );
            };

            // Delay callbacks - Proxy to Editor
            fxPanel->onDelayEnableChanged = [this] ( bool enabled ) {
                if ( onDelayEnableChanged )
                    onDelayEnableChanged ( enabled );
            };
            fxPanel->onDelayLevelChanged = [this] ( float level ) {
                if ( onDelayMixChanged )
                    onDelayMixChanged ( level );
            };
            fxPanel->onDelayTimeChanged = [this] ( float ms ) {
                if ( onDelayTimeChanged )
                    onDelayTimeChanged ( ms );
            };
            fxPanel->onDelayFeedbackChanged = [this] ( float fb ) {
                if ( onDelayFeedbackChanged )
                    onDelayFeedbackChanged ( fb );
            };
            fxPanel->onDelayPingPongChanged = [this] ( bool enabled ) {
                if ( onDelayPingPongChanged )
                    onDelayPingPongChanged ( enabled );
            };
            fxPanel->onDelayHPChanged = [this] ( float hz ) {
                if ( onDelayHPChanged )
                    onDelayHPChanged ( hz );
            };

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
            strip->onGainChanged = [this, stripPtr, isPersonalTrack = ci.isMe] ( int id, float gain ) {
                float oldGain = channelGains.count ( id ) ? channelGains[id] : 0.75f;

                // Use the previous non-zero gain as reference if the current gain is 0
                float refOldGain = ( oldGain > 0.001f ) ? oldGain : ( previousGains.count ( id ) ? previousGains[id] : 0.75f );

                channelGains[id] = gain;
                if ( gain > 0.001f )
                    previousGains[id] = gain;

                // For the local track, we don't multiply gain. The boost button is
                // mapped to the native Jamulus input boost elsewhere.
                if ( isPersonalTrack )
                {
                    jamulus_client_set_channel_gain ( jamulusClient, id, gain );
                }
                else
                {
                    float boost = channelBoosts.count ( id ) && channelBoosts[id] ? 2.0f : 1.0f;
                    jamulus_client_set_channel_gain ( jamulusClient, id, gain * boost );
                }

                // Proportional Group Sync
                int gid = channelGroups.count ( id ) ? channelGroups[id] : -1;
                if ( gid != -1 && refOldGain > 0.001f )
                {
                    float ratio = gain / refOldGain;
                    for ( auto& s : channelStrips )
                    {
                        int otherId = s->getChannelId();
                        // Only sync external tracks
                        if ( otherId != id && !s->getIsMe() && channelGroups.count ( otherId ) && channelGroups[otherId] == gid )
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
            strip->onPanChanged  = [this] ( int id, float pan ) { jamulus_client_set_channel_pan ( jamulusClient, id, pan ); };
            strip->onMuteChanged = [this] ( int id, bool muted ) {
                // We handle muting in timerCallback's gain update loop to avoid conflicts with auto-level
            };
            strip->onSoloChanged = [this] ( int id, bool solo ) {
                // We handle solo in timerCallback's gain update loop
            };
            strip->onBoostChanged = [this, isPersonalTrack = ci.isMe] ( int id, bool boosted ) {
                channelBoosts[id] = boosted;

                // User track ("Me") or manual tracks should update immediately.
                // Tracks under Auto-Level wait for the timer pass (unless grouped).
                bool autoLeveled = autoLevelEnabled && !isPersonalTrack && ( !channelGroups.count ( id ) || channelGroups[id] == -1 );

                if ( isPersonalTrack )
                {
                    // For the local track, we use the native Jamulus input boost (1=0dB, 2=6dB)
                    // This is much safer than multiplying the input fader.
                    jamulus_client_set_input_boost ( jamulusClient, boosted ? 2 : 1 );

                    // Also update monitoring gain (return level)
                    float gain = channelGains.count ( id ) ? channelGains[id] : 0.75f;
                    jamulus_client_set_channel_gain ( jamulusClient, id, gain );
                }
                else if ( !autoLeveled )
                {
                    // Regular remote track - apply UI boost multiplier
                    float gain  = channelGains.count ( id ) ? channelGains[id] : 0.75f;
                    float boost = boosted ? 2.0f : 1.0f;
                    jamulus_client_set_channel_gain ( jamulusClient, id, gain * boost );
                }
            };
            strip->onGroupChanged = [this] ( int id, int groupId ) { channelGroups[id] = groupId; };
            strip->onMonoChanged  = [this, isPersonalTrack = ci.isMe] ( int id, bool mono ) {
                if ( isPersonalTrack )
                {
                    isMeMono = mono;
                    if ( fxPanel )
                        fxPanel->setMonoConstraint ( mono );
                }
            };

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

            // Local "Me" strip goes in fixed container, remote strips go in scrollable mixer
            if ( ci.isMe )
                localStripContainer.addAndMakeVisible ( strip.get() );
            else
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

        // Position local (Me) strip in the fixed container
        for ( auto& strip : channelStrips )
        {
            if ( strip->getIsMe() )
            {
                strip->setBounds ( 5, 10, localStripWidth - 10, stripHeight - 20 );
                break;
            }
        }

        // Calculate width for remote channels only
        int numRemote = 0;
        for ( auto& strip : channelStrips )
        {
            if ( !strip->getIsMe() )
                numRemote++;
        }

        int totalWidth = numRemote * standardStripWidth;
        if ( totalWidth < mixerViewport.getWidth() )
            totalWidth = mixerViewport.getWidth();

        mixerContainer.setSize ( totalWidth, stripHeight );

        // Position remote channel strips in the scrollable container
        int x = 5;
        for ( auto& strip : channelStrips )
        {
            if ( !strip->getIsMe() )
            {
                strip->setBounds ( x, 10, standardStripWidth - 10, stripHeight - 20 );
                x += standardStripWidth;
            }
        }

        updateScrollArrows();
    }

    void updateScrollArrows()
    {
        // Count remote channels
        int numRemote = 0;
        for ( auto& strip : channelStrips )
        {
            if ( !strip->getIsMe() )
                numRemote++;
        }

        // Show/hide arrows based on scroll position and content
        int scrollX      = mixerViewport.getViewPosition().x;
        int contentWidth = mixerContainer.getWidth();
        int viewWidth    = mixerViewport.getWidth();

        // Left arrow: show if scrolled to the right (content hidden on left)
        bool showLeft = scrollX > 0;
        scrollLeftArrow.setVisible ( showLeft );
        scrollLeftArrow.setAlpha ( showLeft ? 1.0f : 0.3f );

        // Right arrow: show if there's more content to the right
        bool showRight = ( scrollX + viewWidth ) < contentWidth && numRemote > 0;
        scrollRightArrow.setVisible ( showRight );
        scrollRightArrow.setAlpha ( showRight ? 1.0f : 0.3f );
    }

    jamulus_client_t jamulusClient;
    bool             wasConnected     = false;
    int              lastNumChannels  = 0;
    juce::uint64     lastChannelsHash = 0;
    bool             chatPanelVisible = false;
    bool             fxPanelVisible   = false;
    bool             isMeMono         = false;

    // FX parameters
    float reverbTime     = 1.0f;
    float reverbPreDelay = 0.0f;
    float reverbHP       = 20.0f;
    float reverbLP       = 20000.0f;
    int   pingCounter    = 0;
    // AudioDelay removed - moved to Processor

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

    // Mixer - Local strip in fixed container, remote channels in scrollable viewport
    juce::Component  localStripContainer; // Fixed container for "Me" strip
    juce::Viewport   mixerViewport;       // Scrollable for remote channels
    juce::TextButton scrollLeftArrow;     // Shows when channels off-screen left
    juce::TextButton scrollRightArrow;    // Shows when channels off-screen right
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

    // Delay Callbacks
    std::function<void ( bool )>  onDelayEnableChanged;
    std::function<void ( float )> onDelayMixChanged;
    std::function<void ( float )> onDelayTimeChanged;
    std::function<void ( float )> onDelayFeedbackChanged;
    std::function<void ( bool )>  onDelayPingPongChanged;
    std::function<void ( float )> onDelayHPChanged;

    // Reverb Callbacks
    std::function<void ( bool )>  onReverbEnableChanged;
    std::function<void ( float )> onReverbMixChanged;
    std::function<void ( float )> onReverbDecayChanged;

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
