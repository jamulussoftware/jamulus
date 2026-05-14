#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include "../jamulus_wrapper.h"
#include "DebugLogger.h"
#include "SettingsComponent.h"
#include "ThemeSupport.h"
#include "ConnectComponent.h"
#include "ChatComponent.h"
#include "FXPanelComponent.h"
#include "AudioDelay.h"
#include <map>
#include <set>
#include <cmath>
#if JUCE_WINDOWS
#include <shellapi.h>
#endif

static bool openUrlExternal ( const juce::String& urlText )
{
#if JUCE_WINDOWS
    const HINSTANCE result = ShellExecuteW ( nullptr, L"open", urlText.toWideCharPointer(), nullptr, nullptr, SW_SHOWNORMAL );
    if ( reinterpret_cast<std::uintptr_t> ( result ) > 32u )
        return true;
#endif

    if ( juce::URL ( urlText ).launchInDefaultBrowser() )
        return true;

    return juce::Process::openDocument ( urlText, {} );
}

static bool openUrlExternalOnMessageThread ( const juce::String& urlText )
{
#if JUCE_WINDOWS
    return openUrlExternal ( urlText );
#else
    if ( auto* mm = juce::MessageManager::getInstanceWithoutCreating() )
    {
        if ( mm->isThisTheMessageThread() )
            return openUrlExternal ( urlText );

        struct UrlPayload
        {
            juce::String url;
            bool         opened = false;
        } payload{ urlText, false };

        mm->callFunctionOnMessageThread (
            [] ( void* userData ) -> void* {
                auto* p  = static_cast<UrlPayload*> ( userData );
                p->opened = openUrlExternal ( p->url );
                return nullptr;
            },
            &payload );

        return payload.opened;
    }

    return openUrlExternal ( urlText );
#endif
}

// Settings persistence helper
static juce::File getSettingsFile()
{
    auto appData     = juce::File::getSpecialLocation ( juce::File::userApplicationDataDirectory );
    auto settingsDir = appData.getChildFile ( "JamulusPlus" );
    if ( !settingsDir.exists() )
        settingsDir.createDirectory();
    return settingsDir.getChildFile ( "settings.json" );
}

// Helper to load image from embedded resources via wrapper
static juce::Image getEmbeddedImage ( const juce::String& resourcePath )
{
    void* data = nullptr;
    int   size = 0;

    if ( jamulus_load_resource ( resourcePath.toRawUTF8(), &data, &size ) && data && size > 0 )
    {
        // Load image from memory
        juce::Image img = juce::ImageCache::getFromMemory ( data, size );

        // Free the raw buffer allocated by the wrapper
        jamulus_free_resource ( data );

        return img;
    }

    return juce::Image();
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

    juce::String filename = "none.png";
    if ( instrument >= 0 && instrument < (int) ( sizeof ( instrumentFiles ) / sizeof ( instrumentFiles[0] ) ) )
    {
        filename = instrumentFiles[instrument];
    }

    return getEmbeddedImage ( "png/instr/res/instruments/" + filename );
}

static juce::String getInstrumentNameFromIndex ( int instrument )
{
    static const char* instrumentNames[] = {
        "None",        "Drums",      "Djembe",        "Electric Guitar", "Acoustic Guitar", "Bass Guitar", "Keyboard",    "Synthesizer",
        "Grand Piano", "Accordion",  "Vocal",         "Microphone",      "Harmonica",       "Trumpet",     "Trombone",    "French Horn",
        "Tuba",        "Saxophone",  "Clarinet",      "Flute",           "Violin",          "Cello",       "Double Bass", "Recorder",
        "Streamer",    "Listener",   "Guitar/Vocal",  "Keyboard/Vocal",  "Bodhran",         "Bassoon",     "Oboe",        "Harp",
        "Viola",       "Congas",     "Bongo",         "Vocal Bass",      "Vocal Tenor",     "Vocal Alto",  "Vocal Soprano","Banjo",
        "Mandolin",    "Ukulele",    "Bass Ukulele",  "Vocal Baritone",  "Vocal Lead",      "Mountain Dulcimer",
        "Scratching",  "Rapping",    "Vibraphone",    "Conductor"
    };

    if ( instrument >= 0 && instrument < (int) ( sizeof ( instrumentNames ) / sizeof ( instrumentNames[0] ) ) )
        return juce::String ( instrumentNames[instrument] );

    return {};
}

static bool isRealCountryCode ( const juce::String& codeIn )
{
    juce::String code = codeIn;

    if ( code.isEmpty() )
        return false;

    if ( code.equalsIgnoreCase ( "none" ) )
        return false;

    if ( code.equalsIgnoreCase ( "flagnon" ) || code.equalsIgnoreCase ( "flagnone" ) )
        return false;

    return true;
}

static juce::Image getFlagImage ( const char* countryCode )
{
    juce::String code ( countryCode ? countryCode : "" );

    if ( !isRealCountryCode ( code ) )
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
        DebugLogger::instance().log ( std::string ( "[getFlagImage] non-ISO code=" ) + code.toStdString() + " -> flagnone" );
        return getEmbeddedImage ( "png/flags/res/flags/flagnone.png" );
    }

    auto img = getEmbeddedImage ( "png/flags/res/flags/" + code.toLowerCase() + ".png" );
    if ( !img.isValid() )
    {
        DebugLogger::instance().log ( std::string ( "[getFlagImage] missing flag for code=" ) + code.toStdString() + ", using flagnone" );
        auto imgNone = getEmbeddedImage ( "png/flags/res/flags/flagnone.png" );
        return imgNone;
    }
    return img;
}

static juce::String isoFromWireCode ( int wireCode )
{
    if ( wireCode <= 0 )
        return "none";
    int count = jamulus_get_supported_countries_count();
    char nameBuf[128];
    char isoBuf[8];
    int  wire = 0;
    for ( int i = 0; i < count; ++i )
    {
        if ( jamulus_get_supported_country ( i, nameBuf, (int) sizeof ( nameBuf ), isoBuf, (int) sizeof ( isoBuf ), &wire ) )
        {
            if ( wire == wireCode )
                return juce::String::fromUTF8 ( isoBuf );
        }
    }
    return "none";
}

static juce::String countryNameFromIso ( const juce::String& rawCode )
{
    juce::String code = rawCode.toLowerCase();

    if ( code.equalsIgnoreCase ( "uk" ) )
        code = "gb";
    if ( code.equalsIgnoreCase ( "el" ) )
        code = "gr";

    int count = jamulus_get_supported_countries_count();
    char nameBuf[128];
    char isoBuf[8];
    int  wire = 0;
    for ( int i = 0; i < count; ++i )
    {
        if ( jamulus_get_supported_country ( i, nameBuf, (int) sizeof ( nameBuf ), isoBuf, (int) sizeof ( isoBuf ), &wire ) )
        {
            if ( juce::String::fromUTF8 ( isoBuf ).equalsIgnoreCase ( code ) )
                return juce::String::fromUTF8 ( nameBuf );
        }
    }

    return code.toUpperCase();
}

static juce::String makeLearnSafeId ( const juce::String& text )
{
    auto lowered = text.toLowerCase().trim();
    juce::String cleaned;
    for ( auto ch : lowered )
    {
        if ( juce::CharacterFunctions::isLetterOrDigit ( ch ) )
            cleaned << ch;
        else if ( ch == ' ' || ch == '-' || ch == '_' )
            cleaned << '_';
    }

    while ( cleaned.contains ( "__" ) )
        cleaned = cleaned.replace ( "__", "_" );

    return cleaned.isNotEmpty() ? cleaned : "unnamed";
}

static juce::String makeUrlSafeRoomName ( const juce::String& text )
{
    auto lowered = text.toLowerCase().trim();
    juce::String cleaned;
    bool lastWasDash = false;

    for ( auto ch : lowered )
    {
        if ( juce::CharacterFunctions::isLetterOrDigit ( ch ) )
        {
            cleaned << ch;
            lastWasDash = false;
        }
        else if ( !lastWasDash )
        {
            cleaned << '-';
            lastWasDash = true;
        }
    }

    while ( cleaned.startsWithChar ( '-' ) )
        cleaned = cleaned.substring ( 1 );
    while ( cleaned.endsWithChar ( '-' ) )
        cleaned = cleaned.dropLastCharacters ( 1 );

    return cleaned.isNotEmpty() ? cleaned : "jamulus-room";
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
// Private Chat Component (VST to VST)
//==============================================================================
class PrivateChatComponent : public juce::Component
{
public:
    PrivateChatComponent ( int remoteId, const juce::String& remoteName, std::function<void ( const juce::String& )> onSend ) :
        targetId ( remoteId ),
        targetName ( remoteName ),
        onSendCallback ( onSend )
    {
        addAndMakeVisible ( chatDisplay );
        chatDisplay.setMultiLine ( true );
        chatDisplay.setReadOnly ( true );
        chatDisplay.setScrollbarsShown ( true );
        chatDisplay.setColour ( juce::TextEditor::backgroundColourId, juce::Colour ( 0xff212121 ) );

        addAndMakeVisible ( inputEdit );
        inputEdit.setReturnKeyStartsNewLine ( false );
        inputEdit.onReturnKey = [this]() { sendMessage(); };

        addAndMakeVisible ( sendButton );
        sendButton.setButtonText ( "Send" );
        sendButton.onClick = [this]() { sendMessage(); };

        addMessage ( "SYSTEM", "Secure P2P Signaling session started with " + targetName );
    }

    void addMessage ( const juce::String& sender, const juce::String& text )
    {
        juce::String timestamp = juce::Time::getCurrentTime().toString ( false, true, false );
        chatDisplay.moveCaretToEnd();
        chatDisplay.insertTextAtCaret ( "[" + timestamp + "] " + sender + ": " + text + "\n" );
    }

    void resized() override
    {
        auto bounds = getLocalBounds().reduced ( 10 );
        auto bottom = bounds.removeFromBottom ( 30 );
        sendButton.setBounds ( bottom.removeFromRight ( 60 ) );
        bottom.removeFromRight ( 5 );
        inputEdit.setBounds ( bottom );
        chatDisplay.setBounds ( bounds.removeFromTop ( bounds.getHeight() - 10 ) );
    }

private:
    void sendMessage()
    {
        juce::String text = inputEdit.getText().trim();
        if ( text.isEmpty() )
            return;

        addMessage ( "Me", text );
        if ( onSendCallback )
            onSendCallback ( text );

        inputEdit.clear();
    }

    int                                         targetId;
    juce::String                                targetName;
    std::function<void ( const juce::String& )> onSendCallback;
    juce::TextEditor                            chatDisplay;
    juce::TextEditor                            inputEdit;
    juce::TextButton                            sendButton;
};

class PrivateChatWindow : public juce::DocumentWindow
{
public:
    PrivateChatWindow ( int id, const juce::String& name, std::function<void ( const juce::String& )> onSend ) :
        DocumentWindow ( "Private Chat: " + name, juce::Colour ( 0xff323232 ), allButtons )
    {
        auto* comp = new PrivateChatComponent ( id, name, onSend );
        setContentOwned ( comp, true );
        setResizable ( true, true );
        setUsingNativeTitleBar ( true );
        setSize ( 320, 400 );
        setVisible ( true );
    }

    void closeButtonPressed() override
    {
        if ( onWindowClosed )
            onWindowClosed();
    }

    void addMessage ( const juce::String& sender, const juce::String& text )
    {
        if ( auto* comp = dynamic_cast<PrivateChatComponent*> ( getContentComponent() ) )
            comp->addMessage ( sender, text );
    }

    std::function<void()> onWindowClosed;
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
            grad.addColour ( juce::jmap ( -18.0f, -60.0f, 6.0f, 0.0f, 1.0f ), juce::Colour ( 0xff00cc66 ) );
            grad.addColour ( juce::jmap ( -12.0f, -60.0f, 6.0f, 0.0f, 1.0f ), juce::Colours::yellow );
            grad.addColour ( juce::jmap ( -3.0f, -60.0f, 6.0f, 0.0f, 1.0f ), juce::Colours::yellow );
            grad.addColour ( juce::jmap ( 0.0f, -60.0f, 6.0f, 0.0f, 1.0f ), juce::Colours::red );
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
    void setThumbImage ( const juce::Image& image ) { thumbImage = image; }

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
        if ( thumbImage.isValid() )
        {
            auto isVertical  = style == juce::Slider::LinearVertical;
            auto thumbScale  = (float) slider.getProperties().getWithDefault ( "thumbWidthScale", 1.16f );
            const float thumbAspect = thumbImage.getWidth() > 0 ? (float) thumbImage.getHeight() / (float) thumbImage.getWidth() : 1.3f;
            auto thumbWidth  = isVertical ? juce::roundToInt ( width * thumbScale ) : 24;
            auto thumbHeight = isVertical ? juce::roundToInt ( thumbWidth * thumbAspect ) : height;
            auto thumbX      = isVertical ? juce::roundToInt ( x + ( width - thumbWidth ) * 0.5f ) : juce::roundToInt ( sliderPos - thumbWidth * 0.5f );
            auto thumbY      = isVertical ? juce::roundToInt ( sliderPos - thumbHeight * 0.5f ) : y;

            g.setColour ( juce::Colour ( 0xff333333 ) );
            if ( isVertical )
                g.fillRoundedRectangle ( juce::Rectangle<float> ( x + width * 0.5f - 2.0f, (float) y, 4.0f, (float) height ), 2.0f );
            else
                g.fillRoundedRectangle ( juce::Rectangle<float> ( (float) x, y + height * 0.5f - 2.0f, (float) width, 4.0f ), 2.0f );

            g.drawImageWithin ( thumbImage, thumbX, thumbY, thumbWidth, thumbHeight, juce::RectanglePlacement::centred );
            return;
        }

        // Draw track (thin line)
        auto                   trackWidth = 4.0f;
        juce::Rectangle<float> track ( x + width * 0.5f - trackWidth * 0.5f, (float) y, trackWidth, (float) height );
        g.setColour ( juce::Colour ( 0xff333333 ) );
        g.fillRoundedRectangle ( track, 2.0f );

        // Draw thumb (The "Lever")
        auto thumbScale  = (float) slider.getProperties().getWithDefault ( "thumbWidthScale", 0.8f );
        auto thumbWidth  = width * thumbScale;
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

private:
    juce::Image thumbImage;
};

class RotaryKnobLookAndFeel : public juce::LookAndFeel_V4
{
public:
    void setKnobImage ( const juce::Image& image ) { knobImage = image; }
    void setKnobColour ( juce::Colour colour ) { knobColour = colour; }

    void drawRotarySlider ( juce::Graphics& g,
                            int             x,
                            int             y,
                            int             width,
                            int             height,
                            float           sliderPos,
                            const float     rotaryStartAngle,
                            const float     rotaryEndAngle,
                            juce::Slider&   slider ) override
    {
        if ( knobImage.isValid() )
        {
            auto fullBounds = juce::Rectangle<float> ( (float) x, (float) y, (float) width, (float) height ).reduced ( 4.0f );
            const float diameter = juce::jmin ( fullBounds.getWidth(), fullBounds.getHeight() ) * 0.88f;
            auto bounds = juce::Rectangle<float> ( diameter, diameter ).withCentre ( fullBounds.getCentre() );
            const float angle = rotaryStartAngle + sliderPos * ( rotaryEndAngle - rotaryStartAngle );
            auto sourceBounds = juce::Rectangle<float> ( 0.0f, 0.0f, (float) knobImage.getWidth(), (float) knobImage.getHeight() );
            auto fitTransform = juce::RectanglePlacement ( juce::RectanglePlacement::centred ).getTransformToFit ( sourceBounds, bounds );
            auto rotateTransform = juce::AffineTransform::rotation ( angle, bounds.getCentreX(), bounds.getCentreY() );

            g.setImageResamplingQuality ( juce::Graphics::highResamplingQuality );
            g.drawImageTransformed ( knobImage, fitTransform.followedBy ( rotateTransform ), false );
            return;
        }

        auto fullBounds = juce::Rectangle<float> ( (float) x, (float) y, (float) width, (float) height ).reduced ( 4.0f );
        const float diameter = juce::jmin ( fullBounds.getWidth(), fullBounds.getHeight() ) * 0.88f;
        auto bounds = juce::Rectangle<float> ( diameter, diameter ).withCentre ( fullBounds.getCentre() );
        auto radius  = diameter * 0.5f;
        auto centre  = bounds.getCentre();
        auto angle   = rotaryStartAngle + sliderPos * ( rotaryEndAngle - rotaryStartAngle );
        juce::Colour bodyCol = juce::Colour ( 0xffb8bec6 );
        if ( knobColour.getAlpha() > 0 && knobColour.getBrightness() > 0.45f )
            bodyCol = knobColour.interpolatedWith ( juce::Colour ( 0xffc8cdd3 ), 0.55f );

        g.setColour ( juce::Colours::black.withAlpha ( 0.18f ) );
        g.fillEllipse ( bounds.translated ( 0.0f, 1.0f ) );
        g.setGradientFill ( juce::ColourGradient::vertical ( bodyCol.brighter ( 0.22f ), bounds.getY(), bodyCol.darker ( 0.18f ), bounds.getBottom() ) );
        g.fillEllipse ( bounds );
        g.setColour ( juce::Colours::black.withAlpha ( 0.55f ) );
        g.drawEllipse ( bounds, 1.0f );

        auto inner = bounds.reduced ( diameter * 0.18f );
        g.setGradientFill ( juce::ColourGradient::vertical ( juce::Colour ( 0xffd7dce2 ), inner.getY(), bodyCol.darker ( 0.05f ), inner.getBottom() ) );
        g.fillEllipse ( inner );

        juce::Path pointer;
        pointer.addRoundedRectangle ( -1.3f, -radius * 0.72f, 2.6f, radius * 0.4f, 1.0f );
        g.setColour ( juce::Colour ( 0xff1d3557 ) );
        g.fillPath ( pointer, juce::AffineTransform::rotation ( angle ).translated ( centre.x, centre.y ) );
    }

private:
    juce::Image  knobImage;
    juce::Colour knobColour = juce::Colour ( 0xff8a9299 );
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
class ChannelStrip : public juce::Component, public juce::SettableTooltipClient
{
public:
    static constexpr int kRouteMasterStereo = 0;

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
        panSlider.setLookAndFeel ( &rotaryLook );
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

        // 5. VST Peer Indicator (Initially hidden)
        addAndMakeVisible ( vstSupportButton );
        vstSupportButton.setButtonText ( "V" );
        vstSupportButton.setTooltip ( "VST Peer Detected - Click for Private Chat / P2P" );
        vstSupportButton.setColour ( juce::TextButton::buttonColourId, juce::Colour ( 0xff00ffff ).withAlpha ( 0.6f ) );
        vstSupportButton.setColour ( juce::TextButton::textColourOnId, juce::Colours::black );
        vstSupportButton.setColour ( juce::TextButton::textColourOffId, juce::Colours::black );
        vstSupportButton.setVisible ( false );
        vstSupportButton.onClick = [this]() {
            if ( onVstClicked )
                onVstClicked ( channelId );
        };

        addAndMakeVisible ( outputRouteBox );
        outputRouteBox.setTextWhenNothingSelected ( "Out 1/2" );
        outputRouteBox.setTooltip ( "Output Routing" );
        outputRouteBox.onChange = [this]() {
            if ( onOutputRouteChanged )
                onOutputRouteChanged ( channelId, outputRouteBox.getSelectedId() - 1 );
        };
        rebuildOutputRouteMenu();

        addAndMakeVisible ( levelMeter );
    }

    ~ChannelStrip() override
    {
        faderSlider.setLookAndFeel ( nullptr );
        panSlider.setLookAndFeel ( nullptr );
    }

    void applyTheme ( const JamulusTheme& theme )
    {
        faderLook.setThumbImage ( theme.faderKnobImage );
        rotaryLook.setKnobImage ( theme.rotaryKnobImage );
        rotaryLook.setKnobColour ( theme.knobColour );
        faderSlider.setColour ( juce::Slider::thumbColourId, theme.faderColour );
        panSlider.setColour ( juce::Slider::rotarySliderOutlineColourId, theme.outlineColour );
        outputRouteBox.setColour ( juce::ComboBox::backgroundColourId, theme.panelAltColour );
        outputRouteBox.setColour ( juce::ComboBox::outlineColourId, theme.outlineColour );
        outputRouteBox.setColour ( juce::ComboBox::textColourId, theme.textColour );
        faderDbLabel.setColour ( juce::Label::textColourId, theme.textColour );
        faderDbLabel.setColour ( juce::Label::backgroundColourId, theme.headerColour.withAlpha ( 0.7f ) );
        peakDbLabel.setColour ( juce::Label::backgroundColourId, theme.headerColour.withAlpha ( 0.7f ) );
        repaint();
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

    void setChannelInfo ( int id, const juce::String& name, int instrument, const char* countryCode, const char* city, int skill, bool isMe )
    {
        channelId       = id;
        instrumentIndex = instrument;

        nameLabel.setText ( name, juce::dontSendNotification );
        instrumentIcon = getInstrumentImage ( instrument );
        countryCodeStr = juce::String::fromUTF8 ( countryCode ? countryCode : "" );
        flagIcon       = getFlagImage ( countryCode );
        cityStr        = juce::String::fromUTF8 ( city ? city : "" );

        this->skillLevel = skill;
        this->isMe       = isMe;

        const juce::String controlBase = isMe ? "channel_me" : "channel_" + makeLearnSafeId ( name );
        faderSlider.setComponentID ( controlBase + "_gain" );
        faderSlider.getProperties().set ( "learnLabel", ( isMe ? "Me" : name ) + " Fader" );
        panSlider.setComponentID ( controlBase + "_pan" );
        panSlider.getProperties().set ( "learnLabel", ( isMe ? "Me" : name ) + " Pan" );

        auto tooltipText = getTooltip();
        nameLabel.setTooltip ( tooltipText );

        nameLabel.setVisible ( true );
        nameLabel.setColour ( juce::Label::backgroundColourId, juce::Colours::transparentBlack );
        nameLabel.setColour ( juce::Label::textColourId, juce::Colours::black );

        repaint();
    }

    static juce::String getSkillLevelName ( int skill )
    {
        switch ( skill )
        {
        case 1:
            return "Beginner";
        case 2:
            return "Intermediate";
        case 3:
            return "Expert";
        default:
            return {};
        }
    }

    juce::String getTooltip() override
    {
        juce::StringArray lines;

        bool hasCountry = isRealCountryCode ( countryCodeStr );

        if ( hasCountry )
        {
            auto countryName = countryNameFromIso ( countryCodeStr );
            juce::String line = "Country: ";

            if ( countryName.isNotEmpty() )
            {
                line += countryName;
                line << " (" << countryCodeStr.toUpperCase() << ")";
            }
            else
            {
                line += countryCodeStr.toUpperCase();
            }

            lines.add ( line );
        }

        if ( instrumentIndex > 0 )
        {
            auto instName = getInstrumentNameFromIndex ( instrumentIndex );
            if ( instName.isNotEmpty() )
                lines.add ( "Instrument: " + instName );
        }

        auto cityTrimmed = cityStr.trim();
        if ( cityTrimmed.isNotEmpty() && !cityTrimmed.equalsIgnoreCase ( "(v)" ) )
            lines.add ( "City: " + cityTrimmed );

        if ( skillLevel > 0 )
        {
            auto skillName = getSkillLevelName ( skillLevel );
            if ( skillName.isNotEmpty() )
                lines.add ( "Skill Level: " + skillName );
        }

        return lines.joinIntoString ( "\n" );
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

    void setPan ( float p ) { panSlider.setValue ( p, juce::dontSendNotification ); }
    void setOutputRoute ( int routeCode ) { outputRouteBox.setSelectedId ( routeCode + 1, juce::dontSendNotification ); }
    int  getOutputRoute() const { return outputRouteBox.getSelectedId() - 1; }

    int  getChannelId() const { return channelId; }
    bool getIsMe() const { return isMe; }
    bool getMonoMode() const { return isMono; }

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
        if ( flagIcon.isValid() && isRealCountryCode ( countryCodeStr ) )
        {
            auto fBounds = juce::Rectangle<float> ( 0, 0, totalBounds.getWidth(), 34.0f );
            g.drawImage ( flagIcon, fBounds, juce::RectanglePlacement::stretchToFit );
        }
        else if ( isRealCountryCode ( countryCodeStr ) )
        {
            auto fBounds = juce::Rectangle<float> ( 0, 0, totalBounds.getWidth(), 34.0f );
            g.setColour ( juce::Colour ( 0xff202020 ) );
            g.fillRect ( fBounds );
            g.setColour ( juce::Colours::white );
            g.setFont ( juce::Font ( 16.0f, juce::Font::bold ) );
            g.drawText ( countryCodeStr.toUpperCase(), fBounds, juce::Justification::centred );
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
        bounds.removeFromTop ( 68 ); // Slightly tighter so the pan knob sits a little higher
        nameLabel.setBounds ( bounds.removeFromTop ( 20 ) );
        panSlider.setBounds ( bounds.removeFromTop ( 42 ).reduced ( 13, 0 ).translated ( 0, -2 ) );

        // 2. dB Readouts
        auto readoutArea = bounds.removeFromTop ( 18 );
        faderDbLabel.setBounds ( readoutArea.removeFromLeft ( readoutArea.getWidth() / 2 ).reduced ( 1 ) );
        peakDbLabel.setBounds ( readoutArea.reduced ( 1 ) );

        // VST Badge (Top Right of name area)
        if ( vstSupportButton.isVisible() )
        {
            vstSupportButton.setBounds ( getWidth() - 25, 5, 20, 16 );
        }

        // 3. Buttons at bottom (Ordered: M/S above B/G)
        auto outputRow = bounds.removeFromBottom ( 22 );
        outputRouteBox.setBounds ( outputRow.reduced ( 1, 0 ) );

        auto bottomRow = bounds.removeFromBottom ( 22 );
        boostButton.setBounds ( bottomRow.removeFromLeft ( bottomRow.getWidth() / 2 ).reduced ( 1 ) );
        groupButton.setBounds ( bottomRow.reduced ( 1 ) );

        auto subBottom = bounds.removeFromBottom ( 22 );
        const int msButtonWidth  = 24;
        const int msButtonGap    = 4;
        const int msGroupWidth   = msButtonWidth * 2 + msButtonGap;
        const int msCenterX      = subBottom.getCentreX();
        const int msGroupX       = msCenterX - msGroupWidth / 2;
        const int msButtonHeight = juce::jmax ( 1, subBottom.getHeight() - 2 );
        muteButton.setBounds ( msGroupX, subBottom.getY() + 1, msButtonWidth, msButtonHeight );
        soloButton.setBounds ( msGroupX + msButtonWidth + msButtonGap, subBottom.getY() + 1, msButtonWidth, msButtonHeight );

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

    void setVstPeer ( bool isPeer )
    {
        vstSupportButton.setVisible ( isPeer );
        resized();
    }

    std::function<void ( int, float )> onGainChanged;
    std::function<void ( int, float )> onPanChanged;
    std::function<void ( int, bool )>  onMuteChanged;
    std::function<void ( int, bool )>  onSoloChanged;
    std::function<void ( int, bool )>  onBoostChanged;
    std::function<void ( int, int )>   onGroupChanged;
    std::function<void ( int )>        onVstClicked;
    std::function<void ( int, bool )>  onMonoChanged;
    std::function<void ( int, int )>   onOutputRouteChanged;

private:
    void rebuildOutputRouteMenu()
    {
        outputRouteBox.clear ( juce::dontSendNotification );
        outputRouteBox.addItem ( "Master 1/2", 1 );
        outputRouteBox.addItem ( "Out 3/4", 4 );
        outputRouteBox.addItem ( "Out 5/6", 7 );
        outputRouteBox.addItem ( "Out 7/8", 10 );
        outputRouteBox.addItem ( "Out 9/10", 13 );
        outputRouteBox.addItem ( "Out 11/12", 16 );
        outputRouteBox.addItem ( "Out 13/14", 19 );
        outputRouteBox.addItem ( "Out 15/16", 22 );
        outputRouteBox.addItem ( "Out 17/18", 25 );
        outputRouteBox.addItem ( "Out 19/20", 28 );
        outputRouteBox.addItem ( "Out 21/22", 31 );
        outputRouteBox.addItem ( "Out 23/24", 34 );
        outputRouteBox.addItem ( "Out 25/26", 37 );
        outputRouteBox.addItem ( "Out 27/28", 40 );
        outputRouteBox.addItem ( "Out 29/30", 43 );
        outputRouteBox.addItem ( "Out 31/32", 46 );

        outputRouteBox.addSeparator();
        outputRouteBox.addItem ( "Mono 1", 2 );
        outputRouteBox.addItem ( "Mono 2", 3 );

        for ( int busIndex = 1; busIndex <= 15; ++busIndex )
        {
            const int firstChannel  = busIndex * 2 + 1;
            const int secondChannel = firstChannel + 1;
            const int baseRouteCode = 3 + ( ( busIndex - 1 ) * 3 );
            outputRouteBox.addItem ( "Mono " + juce::String ( firstChannel ), baseRouteCode + 2 );
            outputRouteBox.addItem ( "Mono " + juce::String ( secondChannel ), baseRouteCode + 3 );
        }

        outputRouteBox.setSelectedId ( 1, juce::dontSendNotification );
    }

    int              channelId       = -1;
    int              instrumentIndex = -1;
    bool             isMe            = false;
    bool             isMono          = false;
    int              skillLevel      = 0;
    int              groupID         = -1;
    juce::Label      nameLabel;
    juce::Label      faderDbLabel;
    juce::Label      peakDbLabel;
    juce::Image      instrumentIcon;
    juce::Image      flagIcon;
    juce::String     countryCodeStr;
    juce::String     cityStr;
    juce::Slider     faderSlider;
    PanSlider        panSlider; // Use custom PanSlider
    juce::TextButton muteButton;
    juce::TextButton soloButton;
    juce::TextButton boostButton;
    juce::TextButton groupButton;
    juce::TextButton vstSupportButton;
    juce::ComboBox   outputRouteBox;

    LevelMeter       levelMeter;
    FaderLookAndFeel faderLook; // Custom look
    RotaryKnobLookAndFeel rotaryLook;
};

class SettingsWindow : public juce::DocumentWindow
{
public:
    explicit SettingsWindow ( SettingsComponent* settingsComponent ) :
        DocumentWindow ( "Jamulus Settings", juce::Colour ( 0xff323232 ), DocumentWindow::closeButton | DocumentWindow::minimiseButton )
    {
        setUsingNativeTitleBar ( true );
        setContentOwned ( settingsComponent, true );
        setResizable ( true, true );
        setResizeLimits ( 820, 620, 1600, 1200 );
        setSize ( 980, 830 );
        centreWithSize ( getWidth(), getHeight() );
        setVisible ( true );
    }

    void closeButtonPressed() override
    {
        if ( onWindowClosed )
            onWindowClosed();
    }

    std::function<void()> onWindowClosed;
};

//==============================================================================
// Main Jamulus GUI Component
//==============================================================================
class JamulusGuiComponent : public juce::Component, private juce::Timer
{
public:
    std::function<void ( float )> onMainVolumeChanged;
    std::function<void ( bool )>  onTestToneChanged;
    std::function<void()>         onResetRequested;
    std::function<void ( const juce::String& )> onConnectToServer;
    std::function<void()>                       onDisconnect;
    std::function<bool()>                       getIsConnected;
    std::function<float()>                      getInputPeakLeft;
    std::function<float()>                      getInputPeakRight;
    std::function<float()>                      getOutputPeakLeft;
    std::function<float()>                      getOutputPeakRight;
    std::function<int()>                        getInputFader;
    std::function<void()>                       onSendPing;
    std::function<bool ( jamulus_network_stats_t& )> getNetworkStats;
    std::function<void ( int )>                 onInputFaderChanged;
    std::function<void ( int, float )>          onChannelGainChanged;
    std::function<void ( int, float )>          onChannelReferenceGainChanged;
    std::function<void ( int, float )>          onChannelPanChanged;
    std::function<void ( int, bool )>           onChannelMuteChanged;
    std::function<void ( int, bool )>           onChannelSoloChanged;
    std::function<void ( int, int )>            onChannelOutputRouteChanged;
    std::function<int()>                        getNumChannels;
    std::function<int()>                        getMyChannelId;
    std::function<bool ( int, jamulus_channel_info_t& )> getChannelInfo;
    std::function<void ( const juce::String& )>           onChatSend;
    std::function<juce::String()>                         getNextChatMessage;
    std::function<juce::StringArray()>                    getMidiInputDeviceNames;
    std::function<juce::String()>                         getSelectedMidiInputDevice;
    std::function<void ( const juce::String& )>           setSelectedMidiInputDevice;
    std::function<juce::StringArray()>                    getMidiOutputDeviceNames;
    std::function<juce::String()>                         getSelectedMidiOutputDevice;
    std::function<void ( const juce::String& )>           setSelectedMidiOutputDevice;

    std::function<juce::String()>               getProfileName;
    std::function<void ( const juce::String& )> setProfileName;
    std::function<int()>                        getProfileInstrument;
    std::function<void ( int )>                 setProfileInstrument;
    std::function<int()>                        getProfileCountry;
    std::function<void ( int )>                 setProfileCountry;
    std::function<juce::String()>               getProfileCountryCode;
    std::function<juce::String()>               getProfileCity;
    std::function<void ( const juce::String& )> setProfileCity;
    std::function<int()>                        getProfileSkill;
    std::function<void ( int )>                 setProfileSkill;

    void setDirectoryCallbacks ( std::function<void()> clearCb,
                                 std::function<void ( int, const juce::String& )> requestListCb,
                                 std::function<int()> getNumServersCb,
                                 std::function<bool ( int, jamulus_server_info_t& )> getServerInfoCb,
                                 std::function<void ( const juce::String& )> requestPlayersCb,
                                 std::function<void()> pingServerListCb )
    {
        connectComp.setDirectoryCallbacks ( std::move ( clearCb ),
                                            std::move ( requestListCb ),
                                            std::move ( getNumServersCb ),
                                            std::move ( getServerInfoCb ),
                                            std::move ( requestPlayersCb ),
                                            std::move ( pingServerListCb ) );
    }

    void syncPersistentStateToCallbacks()
    {
        if ( onMainVolumeChanged )
            onMainVolumeChanged ( mainVolume );
        if ( onMonitorModeChanged )
            onMonitorModeChanged ( monitorServerReturn );
        if ( onLimiterEnableChanged )
            onLimiterEnableChanged ( limiterButton.getToggleState() );
        if ( onLimiterThresholdChanged )
            onLimiterThresholdChanged ( limiterThresholdDb );
        if ( onLimiterReleaseChanged )
            onLimiterReleaseChanged ( limiterReleaseMs );
        if ( onTestToneChanged )
            onTestToneChanged ( testToneEnabled );
        if ( setSelectedMidiInputDevice && pendingMidiInputDeviceName.isNotEmpty() )
            setSelectedMidiInputDevice ( pendingMidiInputDeviceName );
        if ( setSelectedMidiOutputDevice && pendingMidiOutputDeviceName.isNotEmpty() )
            setSelectedMidiOutputDevice ( pendingMidiOutputDeviceName );
        if ( setRecordingSettings && hasPendingRecordingSettings )
            setRecordingSettings ( pendingRecordingSettings );
    }

    // Allow editor to set test tone button callback
    void setTestToneButtonCallback ( std::function<void ( bool )> cb )
    {
        testToneButton.onClick = [this, cb]() {
            testToneEnabled = testToneButton.getToggleState();
            if ( cb )
                cb ( testToneEnabled );
        };
    }
    void requestInitialServerList()
    {
        connectComp.triggerInitialRequest();
    }
    JamulusGuiComponent ( jamulus_client_t client, JamulusPluginProcessor* proc ) : jamulusClient ( client ), connectComp ( client ), processor ( proc )
    {
        jamulusClient = client;
        if ( processor != nullptr )
        {
            currentConnectedServerName    = juce::String ( processor->getCurrentServerName() );
            currentConnectedServerAddress = juce::String ( processor->getCurrentServerAddress() );
        }
        // Connect Component setup (hidden by default)
        addChildComponent ( connectComp );
        connectComp.onConnectRequestedDetailed = [this] ( const juce::String& addr, const juce::String& serverName ) {
            currentConnectedServerAddress = addr;
            currentConnectedServerName    = serverName.isNotEmpty() ? serverName : addr;
            if ( processor != nullptr )
                processor->setCurrentServerName ( currentConnectedServerName.toStdString() );
        };
        connectComp.onConnectRequested = [this] ( const juce::String& addr ) {
            if ( onResetRequested )
                onResetRequested();

            if ( onConnectToServer )
                onConnectToServer ( addr );
            connectComp.setVisible ( false );
        };
        connectComp.onCancel = [this]() { connectComp.setVisible ( false ); };

        // Title
        addAndMakeVisible ( titleLabel );
        titleLabel.setText ( "JamulusPlus", juce::dontSendNotification );
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
        chatButton.onClick = [this]() {
            toggleChatPanel();
            updateChatButtonColor();
        };

        addAndMakeVisible ( cameraButton );
        cameraButton.setButtonText ( "Cam" );
        cameraButton.setTooltip ( "Open the VDO.Ninja room for the current server" );
        cameraButton.onClick = [this]() { openCurrentServerVideoRoom(); };

        addAndMakeVisible ( recordButton );
        recordButton.setButtonText ( "Rec" );
        recordButton.setClickingTogglesState ( true );
        recordButton.setTooltip ( "Start or stop local recording using the Recording settings tab" );
        recordButton.onClick = [this]() {
            bool shouldRecord = recordButton.getToggleState();
            bool started = false;

            if ( shouldRecord )
            {
                if ( onStartRecording )
                    started = onStartRecording();
                recordButton.setToggleState ( started, juce::dontSendNotification );
            }
            else if ( onStopRecording )
            {
                onStopRecording();
            }

            updateRecordButtonColor();
        };

        // Test Tone Button
        addChildComponent ( testToneButton );
        testToneButton.setButtonText ( "Test Tone" );
        testToneButton.setClickingTogglesState ( true );
        testToneButton.onClick = [this]() { testToneEnabled = testToneButton.getToggleState(); };

        // Main Volume slider (controls all channels except local)
        addAndMakeVisible ( mainVolumeLabel );
        mainVolumeLabel.setText ( "Main", juce::dontSendNotification );
        mainVolumeLabel.setColour ( juce::Label::textColourId, juce::Colours::white );
        mainVolumeLabel.setJustificationType ( juce::Justification::centred );

        addAndMakeVisible ( mainVolumeDbLabel );
        mainVolumeDbLabel.setText ( "0.0 dB", juce::dontSendNotification );
        mainVolumeDbLabel.setColour ( juce::Label::textColourId, juce::Colours::white );
        mainVolumeDbLabel.setJustificationType ( juce::Justification::centred );
        mainVolumeDbLabel.setFont ( juce::Font ( 11.0f ) );

        addAndMakeVisible ( masterPeakDbLabel );
        masterPeakDbLabel.setText ( "-oo dB", juce::dontSendNotification );
        masterPeakDbLabel.setColour ( juce::Label::textColourId, juce::Colours::white );
        masterPeakDbLabel.setJustificationType ( juce::Justification::centred );
        masterPeakDbLabel.setFont ( juce::Font ( 11.0f, juce::Font::bold ) );

        addAndMakeVisible ( mainVolumeSlider );
        mainVolumeSlider.setComponentID ( "main_volume" );
        mainVolumeSlider.getProperties().set ( "learnLabel", "Main Volume" );
        mainVolumeSlider.setSliderStyle ( juce::Slider::LinearVertical );
        // Range: 0 = silence, 100 = 0dB (unity), 106 = +6dB (~2x)
        mainVolumeSlider.setRange ( 0, 106, 1 );
        mainVolumeSlider.setValue ( 100 ); // Default to 0dB
        mainVolumeSlider.setTextBoxStyle ( juce::Slider::NoTextBox, true, 0, 0 );
        mainVolumeSlider.setTooltip ( "Main output volume (0dB default, +6dB max). Double-click to reset." );
        mainVolumeSlider.setScrollWheelEnabled ( true );
        mainVolumeSlider.setDoubleClickReturnValue ( true, 100.0 ); // Double-click resets to 0dB
        mainVolumeSlider.getProperties().set ( "thumbWidthScale", 0.50f );
        mainVolumeSlider.setLookAndFeel ( &masterFaderLook );
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

            refreshMainVolumeDbLabel();
            if ( onMainVolumeChanged )
                onMainVolumeChanged ( mainVolume );
            saveSettings();
        };
        refreshMainVolumeDbLabel();

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
                // Clear auto-level state for non-boosted channels only
                // Boosted channels keep their per-channel auto-level active
                for ( auto it = autoLevelCurrentGains.begin(); it != autoLevelCurrentGains.end(); )
                {
                    int id = it->first;
                    if ( !channelBoosts.count ( id ) || !channelBoosts[id] )
                    {
                        autoLevelPeakLevels.erase ( id );
                        autoLevelPeakHoldCounters.erase ( id );
                        autoLevelChannelActiveTicks.erase ( id );
                        it = autoLevelCurrentGains.erase ( it );
                    }
                    else
                    {
                        ++it;
                    }
                }
                saveSettings(); // Ensure the mode change is persisted
            }
        };

        // Limiter button (replaces auto-level settings cog)
        addAndMakeVisible ( limiterButton );
        limiterButton.setButtonText ( "Limiter" );
        limiterButton.setClickingTogglesState ( true );
        limiterButton.setTooltip ( "Enable soft limiter on master output to prevent clipping" );
        limiterButton.onClick = [this]() {
            bool enabled = limiterButton.getToggleState();
            if ( onLimiterEnableChanged )
                onLimiterEnableChanged ( enabled );
            updateLimiterButtonColor();
            saveSettings();
        };

        addAndMakeVisible ( limiterThresholdSlider );
        limiterThresholdSlider.setComponentID ( "limiter_threshold" );
        limiterThresholdSlider.getProperties().set ( "learnLabel", "Limiter Threshold" );
        limiterThresholdSlider.setSliderStyle ( juce::Slider::LinearVertical );
        limiterThresholdSlider.setRange ( -18.0, 0.0, 0.1 );
        limiterThresholdSlider.setValue ( 0.0 );
        limiterThresholdSlider.setTextBoxStyle ( juce::Slider::NoTextBox, true, 0, 0 );
        limiterThresholdSlider.setDoubleClickReturnValue ( true, 0.0 );
        limiterThresholdSlider.getProperties().set ( "thumbWidthScale", 0.50f );
        limiterThresholdSlider.setLookAndFeel ( &masterFaderLook );
        limiterThresholdSlider.onValueChange = [this]() {
            limiterThresholdDb = static_cast<float> ( limiterThresholdSlider.getValue() );
            limiterThresholdDbLabel.setText ( juce::String ( limiterThresholdDb, 1 ) + " dB", juce::dontSendNotification );
            if ( onLimiterThresholdChanged )
                onLimiterThresholdChanged ( limiterThresholdDb );
            saveSettings();
        };

        addAndMakeVisible ( limiterThresholdDbLabel );
        limiterThresholdDbLabel.setText ( "0.0 dB", juce::dontSendNotification );
        limiterThresholdDbLabel.setColour ( juce::Label::textColourId, juce::Colours::white );
        limiterThresholdDbLabel.setJustificationType ( juce::Justification::centred );
        limiterThresholdDbLabel.setFont ( juce::Font ( 11.0f ) );

        addAndMakeVisible ( limiterReleaseLabel );
        limiterReleaseLabel.setText ( "Release", juce::dontSendNotification );
        limiterReleaseLabel.setColour ( juce::Label::textColourId, juce::Colours::white );
        limiterReleaseLabel.setJustificationType ( juce::Justification::centred );
        limiterReleaseLabel.setFont ( juce::Font ( 11.0f ) );

        addAndMakeVisible ( limiterReleaseSlider );
        limiterReleaseSlider.setComponentID ( "limiter_release" );
        limiterReleaseSlider.getProperties().set ( "learnLabel", "Limiter Release" );
        limiterReleaseSlider.setSliderStyle ( juce::Slider::RotaryHorizontalVerticalDrag );
        limiterReleaseSlider.setRange ( 10.0, 1000.0, 1.0 );
        limiterReleaseSlider.setValue ( 120.0 );
        limiterReleaseSlider.setTextBoxStyle ( juce::Slider::NoTextBox, true, 0, 0 );
        limiterReleaseSlider.setDoubleClickReturnValue ( true, 120.0 );
        limiterReleaseSlider.setLookAndFeel ( &masterKnobLook );
        limiterReleaseSlider.onValueChange = [this]() {
            limiterReleaseMs = static_cast<float> ( limiterReleaseSlider.getValue() );
            if ( onLimiterReleaseChanged )
                onLimiterReleaseChanged ( limiterReleaseMs );
            saveSettings();
        };

        addAndMakeVisible ( themeHeaderLabel );
        themeHeaderLabel.setText ( "Theme", juce::dontSendNotification );
        themeHeaderLabel.setColour ( juce::Label::textColourId, juce::Colours::white );
        themeHeaderLabel.setJustificationType ( juce::Justification::centred );
        themeHeaderLabel.setFont ( juce::Font ( 11.0f, juce::Font::plain ) );

        addAndMakeVisible ( themeHeaderCombo );
        themeHeaderCombo.setTextWhenNothingSelected ( "Default" );
        themeHeaderCombo.setJustificationType ( juce::Justification::centredLeft );
        themeHeaderCombo.onChange = [this]() {
            const auto name = themeHeaderCombo.getText();
            if ( name.isNotEmpty() && name != selectedThemeName )
                setThemeByName ( name );
        };

        // Master Output Meters
        addAndMakeVisible ( masterMeterL );
        addAndMakeVisible ( masterMeterR );
        masterMeterL.setHorizontal ( false );
        masterMeterR.setHorizontal ( false );

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
        inputFaderSlider.setComponentID ( "input_fader" );
        inputFaderSlider.getProperties().set ( "learnLabel", "Input Fader" );
        inputFaderSlider.setSliderStyle ( juce::Slider::LinearHorizontal );
        inputFaderSlider.setRange ( 0, 100 );
        inputFaderSlider.setValue ( 50 );
        inputFaderSlider.setTextBoxStyle ( juce::Slider::NoTextBox, true, 0, 0 );
        inputFaderSlider.onValueChange = [this]() {
            const int value = static_cast<int> ( inputFaderSlider.getValue() );
            if ( onInputFaderChanged )
            {
                onInputFaderChanged ( value );
            }
            else if ( jamulusClient )
            {
                jamulus_client_set_input_fader ( jamulusClient, value );
            }
            saveSettings();
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

        engineModeButton.setVisible ( false );

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
            if ( onResetRequested )
                onResetRequested();
            if ( onDisconnect )
                onDisconnect();
            statusLabel.setText ( "Disconnected", juce::dontSendNotification );
            currentConnectedServerName.clear();
            currentConnectedServerAddress.clear();
        };

        int initialFader = 50;
        if ( getInputFader )
            initialFader = getInputFader();
        else if ( jamulusClient )
            initialFader = jamulus_client_get_input_fader ( jamulusClient );
        inputFaderSlider.setValue ( initialFader, juce::dontSendNotification );

        // Create initial Me strip immediately so it's always visible
        updateChannelStrips();

        refreshAvailableThemes();

        // Load saved settings from file
        loadSettings();

        applyTheme();

        if ( chatPanelVisible )
        {
            chatButton.setToggleState ( true, juce::dontSendNotification );
            toggleChatPanel();
        }
        if ( fxPanelVisible )
        {
            fxButton.setToggleState ( true, juce::dontSendNotification );
            toggleFXPanel();
        }

        refreshThemeVideo();
        updateRecordButtonColor();

        startTimerHz ( 30 );
    }

    ~JamulusGuiComponent() override
    {
        // saveSettings(); // DISABLED: Potential crash if client is already destroyed
        stopTimer();
        mainVolumeSlider.setLookAndFeel ( nullptr );
        limiterThresholdSlider.setLookAndFeel ( nullptr );
        limiterReleaseSlider.setLookAndFeel ( nullptr );
        hideAllDialogs();
        if ( chatPanel )
            removeChildComponent ( chatPanel.get() );
        if ( fxPanel )
            removeChildComponent ( fxPanel.get() );
        chatPanel.reset();
        fxPanel.reset();
        settingsWindow.reset();
        chatWindow.reset();
    }

    void paint ( juce::Graphics& g ) override
    {
        constexpr int kHeaderHeight = 50;
        constexpr int kMixerTop = kHeaderHeight;
        constexpr int kLeftSidebarWidth = 132;

        if ( animatedThemeBackground.isValid() )
            g.drawImageWithin ( animatedThemeBackground, 0, 0, getWidth(), getHeight(), juce::RectanglePlacement::fillDestination );
        else if ( currentTheme.backgroundImage.isValid() )
            g.drawImageWithin ( currentTheme.backgroundImage, 0, 0, getWidth(), getHeight(), juce::RectanglePlacement::fillDestination );
        else
            g.fillAll ( currentTheme.windowColour );

        // Header background
        g.setColour ( currentTheme.headerColour.withAlpha ( currentTheme.backgroundImage.isValid() ? 0.88f : 1.0f ) );
        g.fillRect ( 0, 0, getWidth(), kHeaderHeight );

        // Mixer area background (starts after header)
        auto mixerBounds = juce::Rectangle<int> ( 0, kMixerTop, getWidth(), juce::jmax ( 0, getHeight() - kMixerTop ) );
        g.setColour ( currentTheme.backgroundImage.isValid()
                          ? currentTheme.mixerColour.darker ( 0.45f ).withAlpha ( 0.52f )
                          : currentTheme.mixerColour );
        g.fillRect ( mixerBounds );

        g.setColour ( currentTheme.outlineColour );
        g.drawVerticalLine ( (float) kLeftSidebarWidth, (float) kHeaderHeight, (float) getHeight() );
    }

    void paintOverChildren ( juce::Graphics& g ) override
    {
        auto drawGlow = [&](juce::Button& btn, juce::Colour onColour, juce::Colour offColour)
        {
            if ( !btn.isVisible() )
                return;

            const bool isOn = btn.getToggleState();
            if ( !isOn )
                return;
            auto bounds = btn.getBounds().toFloat();
            auto centre = bounds.getCentre();
            const float gap = 5.0f;
            const float radius = bounds.getWidth() * 0.55f + gap;
            auto glowColour = isOn ? onColour : offColour;
            juce::ColourGradient grad ( glowColour,
                                        centre.x,
                                        centre.y,
                                        juce::Colours::transparentBlack,
                                        centre.x + radius,
                                        centre.y,
                                        true );
            g.setGradientFill ( grad );
            g.fillEllipse ( centre.x - radius, centre.y - radius, radius * 2.0f, radius * 2.0f );
        };

        drawGlow ( monitorModeButton, juce::Colour ( 0x55ff3232 ), juce::Colour ( 0x22441515 ) );
        drawGlow ( autoLevelButton, juce::Colour ( 0x55ffdd20 ), juce::Colour ( 0x22443a10 ) );
        drawGlow ( limiterButton, juce::Colour ( 0x55ff9820 ), juce::Colour ( 0x22301808 ) );
        drawGlow ( chatButton, juce::Colour ( 0x5550c8ff ), juce::Colour ( 0x220a2840 ) );
        drawGlow ( recordButton, juce::Colour ( 0x55ff4040 ), juce::Colour ( 0x22281212 ) );
        drawGlow ( fxButton, juce::Colour ( 0x5520c8e8 ), juce::Colour ( 0x220a3240 ) );
    }

    void resized() override
    {
        auto bounds = getLocalBounds();
        constexpr int kHeaderHeight = 50;
        constexpr int kLeftSidebarWidth = 132;
        constexpr int kRightControlColumnWidth = 164;

        const int chatPanelWidth = 250;
        if ( chatPanelVisible && chatPanel )
        {
            auto chatBounds = getLocalBounds().removeFromRight ( chatPanelWidth );
            chatBounds.removeFromTop ( kHeaderHeight );
            chatPanel->setBounds ( chatBounds );
            bounds.removeFromRight ( chatPanelWidth );
        }

        // Header
        auto header = bounds.removeFromTop ( kHeaderHeight );
        titleLabel.setBounds ( header.removeFromLeft ( 150 ).reduced ( 10 ) );

        // Toolbar buttons
        auto toolbar = header;
        connectButton.setBounds ( toolbar.removeFromLeft ( 90 ).reduced ( 5 ) );
        chatButton.setBounds ( toolbar.removeFromLeft ( 70 ).reduced ( 5 ) );
        cameraButton.setBounds ( toolbar.removeFromLeft ( 62 ).reduced ( 5 ) );
        recordButton.setBounds ( toolbar.removeFromLeft ( 58 ).reduced ( 5 ) );
        settingsButton.setBounds ( toolbar.removeFromLeft ( 90 ).reduced ( 5 ) );
        testToneButton.setBounds ( 0, 0, 0, 0 );

        // Header controls
        auto headerControls = toolbar.removeFromLeft ( 240 );
        autoLevelButton.setBounds ( headerControls.removeFromLeft ( 82 ).reduced ( 3, 8 ) );
        auto themeArea = headerControls.removeFromLeft ( 128 ).reduced ( 3, 5 );
        themeHeaderLabel.setBounds ( themeArea.removeFromTop ( 12 ) );
        themeHeaderCombo.setBounds ( themeArea.removeFromTop ( 22 ) );

        // Header right
        disconnectButton.setBounds ( toolbar.removeFromLeft ( 90 ).reduced ( 5 ) );
        statusLabel.setBounds ( 0, 0, 0, 0 );

        // Hide the removed elements (they still exist but won't be shown)
        inputLevelLabel.setVisible ( false );
        inputMeterL.setVisible ( false );
        inputMeterR.setVisible ( false );
        inputFaderLabel.setVisible ( false );
        inputFaderSlider.setVisible ( false );
        sidechainButton.setVisible ( false );

        // FX Panel
        if ( fxPanelVisible && fxPanel )
        {
            fxPanel->setBounds ( 12, 140, 320, juce::jmin ( 530, getHeight() - 160 ) );
        }

        auto contentArea = bounds;
        auto leftSidebar = contentArea.removeFromLeft ( kLeftSidebarWidth ).reduced ( 6, 6 );
        auto rightControlColumn = contentArea.removeFromRight ( kRightControlColumnWidth ).reduced ( 8, 6 );
        const int rightColumnRight = rightControlColumn.getRight();
        constexpr int kMasterMeterWidth = 22;
        constexpr int kFaderGapToMeter = 8;
        constexpr int kFaderSlotWidth = 54;
        constexpr int kFaderLabelWidth = 76;
        const int sharedFaderCenterX = rightColumnRight - kMasterMeterWidth - kFaderGapToMeter - ( kFaderSlotWidth / 2 );

        const int arrowWidth = 18;
        auto leftArrowBounds = contentArea.removeFromLeft ( arrowWidth );
        scrollLeftArrow.setBounds ( leftArrowBounds.reduced ( 1, 48 ) );
        auto rightArrowBounds = contentArea.removeFromRight ( arrowWidth );
        scrollRightArrow.setBounds ( rightArrowBounds.reduced ( 1, 48 ) );
        mixerViewport.setBounds ( contentArea.reduced ( 0, 4 ) );

        auto statsArea = leftSidebar.removeFromTop ( 82 );
        statusLabel.setBounds ( statsArea.removeFromTop ( 20 ) );
        const int statHeight = 17;
        const int lightWidth = 14;
        const int gap = 6;
        auto pingRow = statsArea.removeFromTop ( statHeight );
        pingRow.removeFromLeft ( lightWidth + gap );
        pingLabel.setBounds ( pingRow );
        auto delayRow = statsArea.removeFromTop ( statHeight );
        delayLight.setBounds ( delayRow.removeFromLeft ( lightWidth ).reduced ( 2 ) );
        delayRow.removeFromLeft ( gap );
        delayLabel.setBounds ( delayRow );
        auto jitterRow = statsArea.removeFromTop ( statHeight );
        jitterLight.setBounds ( jitterRow.removeFromLeft ( lightWidth ).reduced ( 2 ) );
        jitterRow.removeFromLeft ( gap );
        jitterLabel.setBounds ( jitterRow );

        auto leftButtons = leftSidebar.removeFromTop ( 28 );
        const int localButtonGap = 6;
        const int localButtonWidth = ( leftButtons.getWidth() - localButtonGap ) / 2;
        fxButton.setBounds ( leftButtons.removeFromLeft ( localButtonWidth ).reduced ( 1 ) );
        leftButtons.removeFromLeft ( localButtonGap );
        monitorModeButton.setBounds ( leftButtons.removeFromLeft ( localButtonWidth ).reduced ( 1 ) );
        leftSidebar.removeFromTop ( 4 );
        localStripContainer.setBounds ( leftSidebar );

        auto limiterArea = rightControlColumn.removeFromTop ( juce::jmax ( 282, ( rightControlColumn.getHeight() * 10 ) / 19 ) );
        auto limiterButtonRow = limiterArea.removeFromTop ( 24 );
        const int limiterButtonWidth = 74;
        limiterButton.setBounds ( sharedFaderCenterX - limiterButtonWidth / 2, limiterButtonRow.getY(), limiterButtonWidth, 22 );
        limiterArea.removeFromTop ( 4 );
        auto limiterInner = limiterArea.reduced ( 2, 0 );
        auto limiterDbRow = limiterInner.removeFromTop ( 18 );
        limiterThresholdDbLabel.setBounds ( sharedFaderCenterX - kFaderLabelWidth / 2, limiterDbRow.getY(), kFaderLabelWidth, limiterDbRow.getHeight() );
        auto limiterBody = limiterInner.removeFromTop ( juce::jmax ( 206, limiterInner.getHeight() - 46 ) );
        auto limiterSliderArea = limiterBody.removeFromTop ( limiterBody.getHeight() - 70 );
        limiterThresholdSlider.setBounds ( sharedFaderCenterX - kFaderSlotWidth / 2, limiterSliderArea.getY(), kFaderSlotWidth, limiterSliderArea.getHeight() );
        auto releaseLabelRow = limiterBody.removeFromTop ( 18 );
        limiterReleaseLabel.setBounds ( sharedFaderCenterX - kFaderLabelWidth / 2, releaseLabelRow.getY(), kFaderLabelWidth, releaseLabelRow.getHeight() );
        auto releaseBounds = juce::Rectangle<int> ( sharedFaderCenterX - 26, limiterBody.getCentreY() - 26 - 2, 52, 52 );
        limiterReleaseSlider.setBounds ( releaseBounds );

        rightControlColumn.removeFromTop ( 4 );
        auto masterArea = rightControlColumn;
        auto masterBody = masterArea.removeFromTop ( juce::jmax ( 210, masterArea.getHeight() ) );
        auto meterArea = masterBody.removeFromRight ( kMasterMeterWidth );
        auto meterPeakRow = meterArea.removeFromTop ( 18 );
        masterPeakDbLabel.setBounds ( meterPeakRow.getCentreX() - 24, meterPeakRow.getY(), 48, meterPeakRow.getHeight() );
        auto sliderArea = masterBody.reduced ( 0, 0 );
        auto mainLabelRow = sliderArea.removeFromTop ( 18 );
        mainVolumeLabel.setBounds ( sharedFaderCenterX - kFaderLabelWidth / 2, mainLabelRow.getY(), kFaderLabelWidth, mainLabelRow.getHeight() );
        auto mainDbRow = sliderArea.removeFromTop ( 18 );
        mainVolumeDbLabel.setBounds ( sharedFaderCenterX - kFaderLabelWidth / 2, mainDbRow.getY(), kFaderLabelWidth, mainDbRow.getHeight() );
        sliderArea.removeFromTop ( 2 );
        masterMeterL.setBounds ( meterArea.reduced ( 3, 0 ) );
        masterMeterR.setBounds ( 0, 0, 0, 0 );
        mainVolumeSlider.setBounds ( sharedFaderCenterX - kFaderSlotWidth / 2, sliderArea.getY(), kFaderSlotWidth, sliderArea.getHeight() );

        // Update mixer container width based on number of channels
        updateMixerLayout();
        updateScrollArrows();

        // Connect Dialog Overlay (On top if visible)
        if ( connectComp.isVisible() )
        {
            auto dialogBounds = getLocalBounds().reduced ( 40 );
            // Limit max size
            if ( dialogBounds.getWidth() > 800 )
                dialogBounds = dialogBounds.withSizeKeepingCentre ( 800, dialogBounds.getHeight() );
            if ( dialogBounds.getHeight() > 600 )
                dialogBounds = dialogBounds.withSizeKeepingCentre ( dialogBounds.getWidth(), 600 );

            connectComp.setBounds ( dialogBounds );
        }
    }

private:
    juce::TextButton testToneButton;
    bool             testToneEnabled = false;
    void             timerCallback() override
    {
        jamulus_process_events();
        if ( !isShowing() )
            return;

#if JUCE_WINDOWS
        if ( themeVideoReader )
        {
            auto frame = themeVideoReader->getLatestFrame();
            if ( frame.isValid() )
            {
                animatedThemeBackground = std::move ( frame );
                repaint();
            }
        }
#endif

        float inL = getInputPeakLeft ? getInputPeakLeft() : 0.0f;
        float inR = getInputPeakRight ? getInputPeakRight() : 0.0f;
        inL       = juce::jlimit ( 0.0f, 1.0f, inL );
        inR       = juce::jlimit ( 0.0f, 1.0f, inR );

        inputMeterL.setLevel ( inL );
        inputMeterR.setLevel ( inR );

        float outL = 0.0f;
        float outR = 0.0f;

        if ( jamulusClient )
        {
            outL = jamulus_client_get_output_level_left ( jamulusClient );
            outR = jamulus_client_get_output_level_right ( jamulusClient );
            if ( outL <= 0.0f && outR <= 0.0f )
            {
                if ( getOutputPeakLeft )
                    outL = getOutputPeakLeft();
                if ( getOutputPeakRight )
                    outR = getOutputPeakRight();
            }
        }
        else
        {
            if ( getOutputPeakLeft )
                outL = getOutputPeakLeft();
            if ( getOutputPeakRight )
                outR = getOutputPeakRight();
        }

        outL = juce::jlimit ( 0.0f, 1.0f, outL );
        outR = juce::jlimit ( 0.0f, 1.0f, outR );

        masterMeterL.setLevel ( outL * mainVolume );
        masterMeterR.setLevel ( outR * mainVolume );
        const float masterPeak = juce::jmax ( outL, outR ) * mainVolume;
        if ( masterPeak <= 0.00001f )
            masterPeakDbLabel.setText ( "-oo dB", juce::dontSendNotification );
        else
            masterPeakDbLabel.setText ( juce::String ( 20.0f * std::log10 ( masterPeak ), 1 ) + " dB", juce::dontSendNotification );

        bool connected = false;
        if ( getIsConnected )
            connected = getIsConnected();
        else if ( jamulusClient )
            connected = jamulus_client_is_connected ( jamulusClient );

        if ( connected != wasConnected )
        {
            if ( connected )
            {
                int n = getNumChannels ? getNumChannels() : jamulus_client_get_num_channels ( jamulusClient );
                int activeCount = 0;
                for ( int i = 0; i < n; ++i )
                {
                    jamulus_channel_info_t info;
                    bool ok = false;
                    if ( getChannelInfo )
                        ok = getChannelInfo ( i, info );
                    else
                        ok = jamulus_client_get_channel_info ( jamulusClient, i, &info );
                    if ( ok )
                    {
                        autoLevelChannelActiveTicks[info.id] = 1000; // Mark as established
                        int myId = getMyChannelId ? getMyChannelId() : jamulus_client_get_my_channel_id ( jamulusClient );
                        if ( info.id != myId )
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
                localChannelSlots.clear();
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
            if ( ++pingCounter >= 30 )
            {
                if ( onSendPing )
                    onSendPing();
                else if ( jamulusClient )
                    jamulus_client_send_ping ( jamulusClient );
                pingCounter = 0;
            }

            jamulus_network_stats_t stats;
            bool                    haveStats = false;
            if ( getNetworkStats )
                haveStats = getNetworkStats ( stats );
            else if ( jamulusClient )
            {
                jamulus_client_get_network_stats ( jamulusClient, &stats );
                haveStats = true;
            }

            if ( haveStats && stats.ping_time_ms >= 0 )
            {
                pingLabel.setText ( "Ping: " + juce::String ( stats.ping_time_ms ) + " ms", juce::dontSendNotification );
                // Color code ping based on latency
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
            delayLight.setColour ( juce::Label::backgroundColourId, juce::Colours::transparentBlack );
            jitterLight.setColour ( juce::Label::backgroundColourId, juce::Colours::transparentBlack );
        }

        // --- VST Peer Discovery & Signaling ---
        if ( connected )
        {
            juce::String cityStr;
            if ( getProfileCity )
                cityStr = getProfileCity();
            else if ( jamulusClient )
            {
                const char* myCity = jamulus_client_get_city ( jamulusClient );
                cityStr            = juce::String ( myCity ? myCity : "" );
            }
            if ( !cityStr.endsWith ( "(V)" ) )
            {
                cityStr = cityStr.trim() + " (V)";
                if ( setProfileCity )
                    setProfileCity ( cityStr );
                else if ( jamulusClient )
                    jamulus_client_set_city ( jamulusClient, cityStr.toRawUTF8() );
            }

            // Cleanup stale peers (not seen for 15 seconds) - still useful if we get chat signals
            auto             now = juce::Time::getCurrentTime();
            juce::Array<int> staleIds;
            for ( auto const& [id, lastSeen] : vstPeers )
            {
                if ( ( now - lastSeen ).inSeconds() > 15 )
                    staleIds.add ( id );
            }
            for ( int id : staleIds )
            {
                vstPeers.erase ( id );
                vstPeerNames.erase ( id );
                lastChannelsHash = 0; // Force UI refresh to remove V icons
            }
        }

        // Update channel list if anything changed (count, IDs, names, icons)
        int          numChannels = getNumChannels ? getNumChannels() : jamulus_client_get_num_channels ( jamulusClient );
        juce::uint64 currentHash = 0;
        for ( int i = 0; i < numChannels; ++i )
        {
            jamulus_channel_info_t info;
            bool                   ok = false;
            if ( getChannelInfo )
                ok = getChannelInfo ( i, info );
            else
                ok = jamulus_client_get_channel_info ( jamulusClient, i, &info );
            if ( ok )
            {
                currentHash += (juce::uint64) info.id * 131;
                currentHash ^= juce::String ( info.name ).hashCode64();
                currentHash ^= (juce::uint64) info.instrument * 17;
                currentHash ^= (juce::uint64) info.skill_level * 7;
                currentHash ^= juce::String ( info.country_code ).hashCode64();
            }
        }

        juce::String localNameStr;
        if ( getProfileName )
            localNameStr = getProfileName();
        else if ( jamulusClient )
            localNameStr = juce::String::fromUTF8 ( jamulus_client_get_name ( jamulusClient ) );

        int localInstrument = 0;
        if ( getProfileInstrument )
            localInstrument = getProfileInstrument();
        else if ( jamulusClient )
            localInstrument = jamulus_client_get_instrument ( jamulusClient );

        int localSkill = 0;
        if ( getProfileSkill )
            localSkill = getProfileSkill();
        else if ( jamulusClient )
            localSkill = jamulus_client_get_skill ( jamulusClient );

        juce::String localCountryStr;
        if ( getProfileCountryCode )
            localCountryStr = getProfileCountryCode();
        else if ( jamulusClient )
        {
            const char* localCountry = jamulus_client_get_country_code ( jamulusClient );
            localCountryStr          = juce::String ( localCountry ? localCountry : "" );
        }

        currentHash ^= localNameStr.hashCode64() * 37;
        currentHash ^= (juce::uint64) localInstrument * 41;
        currentHash ^= (juce::uint64) localSkill * 43;
        currentHash ^= localCountryStr.hashCode64() * 47;

        if ( numChannels != lastNumChannels || currentHash != lastChannelsHash )
        {
            lastNumChannels  = numChannels;
            lastChannelsHash = currentHash;
            updateChannelStrips();
        }

        int myChannelId = getMyChannelId ? getMyChannelId() : jamulus_client_get_my_channel_id ( jamulusClient );

        // 2. Identify global Solo state (from map, only for active channels)
        bool anySolo = false;
        for ( int i = 0; i < numChannels; ++i )
        {
            jamulus_channel_info_t info;
            bool                   ok = false;
            if ( getChannelInfo )
                ok = getChannelInfo ( i, info );
            else
                ok = jamulus_client_get_channel_info ( jamulusClient, i, &info );
            if ( ok )
            {
                int mapId = ( info.id == myChannelId && myChannelId != -1 ) ? -1 : info.id;
                if ( mapId != -1 && channelSolos.count ( mapId ) && channelSolos[mapId] )
                {
                    anySolo = true;
                    break;
                }
            }
        }

        // 3. Update raw input levels map
        std::map<int, float> rawInputLevels;
        for ( int i = 0; i < numChannels; ++i )
        {
            jamulus_channel_info_t info;
            bool                   ok = false;
            if ( getChannelInfo )
                ok = getChannelInfo ( i, info );
            else if ( jamulusClient )
                ok = jamulus_client_get_channel_info ( jamulusClient, i, &info );

            if ( ok )
            {
                const float lvl = ( info.level / 65535.0f ) * 1.07f;
                rawInputLevels[info.id] = lvl;
                channelLevels[info.id]  = lvl;
            }
        }

        // 4. Update Auto Level "Brain" Calculation
        // This runs for:
        // - All non-grouped remote channels when global auto-level is enabled
        // - Individual channels with boost enabled (per-channel auto-level)
        bool anyBoostedChannels = false;
        for ( auto& strip : channelStrips )
        {
            if ( !strip->getIsMe() && channelBoosts.count ( strip->getChannelId() ) && channelBoosts[strip->getChannelId()] )
            {
                anyBoostedChannels = true;
                break;
            }
        }

        if ( ( autoLevelEnabled || anyBoostedChannels ) && !channelStrips.empty() )
        {
            const float timerIntervalMs    = 50.0f;
            const float noiseFloor         = 0.02f;
            const float baseTargetLevel    = 0.5f;
            const int   holdTicks          = 100;
            const float attackCoeff        = 1.0f - std::exp ( -timerIntervalMs / autoLevelAttackMs );
            const float releaseCoeff       = 1.0f - std::exp ( -timerIntervalMs / autoLevelReleaseMs );
            const float longTermDecayCoeff = 1.0f - std::exp ( -timerIntervalMs / 10000.0f );

            // When a channel is boosted under auto-level, duck the others slightly so it stands out.
            const float duckingFactor = ( autoLevelEnabled && anyBoostedChannels ) ? 0.75f : 1.0f;

            for ( auto& strip : channelStrips )
            {
                const int id = strip->getChannelId();
                if ( id == -1 || strip->getIsMe() )
                    continue;

                const bool globalAutoLevel = autoLevelEnabled && ( !channelGroups.count ( id ) || channelGroups[id] == -1 );
                const bool boostAutoLevel  = channelBoosts.count ( id ) && channelBoosts[id];

                if ( !globalAutoLevel && !boostAutoLevel )
                    continue;

                if ( !channelLevels.count ( id ) )
                    continue;
                const float currentLevel = channelLevels[id];

                if ( !autoLevelCurrentGains.count ( id ) )
                {
                    const float faderVolume         = channelGains.count ( id ) ? channelGains[id] : 1.0f;
                    const float boostMult           = ( channelBoosts.count ( id ) && channelBoosts[id] ) ? 2.0f : 1.0f;
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

                const bool isNew = autoLevelChannelActiveTicks[id] < 40;
                float&     longTermPeak = autoLevelPeakLevels[id];
                int&       holdCounter  = autoLevelPeakHoldCounters[id];

                if ( currentLevel >= noiseFloor )
                {
                    if ( currentLevel > longTermPeak )
                    {
                        longTermPeak = currentLevel;
                        holdCounter  = holdTicks;
                    }
                    else if ( holdCounter > 0 )
                    {
                        --holdCounter;
                    }
                    else
                    {
                        longTermPeak -= ( longTermPeak - currentLevel ) * longTermDecayCoeff;
                    }
                }
                else
                {
                    if ( holdCounter > 0 )
                    {
                        --holdCounter;
                    }
                    else if ( longTermPeak > 0.0f )
                    {
                        longTermPeak -= longTermPeak * ( longTermDecayCoeff * 0.5f );
                    }
                }

                if ( longTermPeak < noiseFloor )
                {
                    autoLevelCurrentGains[id] += ( 1.0f - autoLevelCurrentGains[id] ) * releaseCoeff;
                    continue;
                }

                const bool  isBoosted     = channelBoosts.count ( id ) && channelBoosts[id];
                const float boostFactor   = isBoosted ? 1.85f : duckingFactor;
                const float currentTarget = baseTargetLevel * boostFactor;

                float targetGain = juce::jlimit ( 0.1f, 2.0f, currentTarget / longTermPeak );

                const float estimatedOutput = currentLevel * targetGain;
                if ( estimatedOutput > 0.95f && currentLevel > noiseFloor )
                {
                    targetGain = 0.95f / currentLevel;
                    targetGain = juce::jlimit ( 0.1f, 2.0f, targetGain );
                }

                float smoothingCoeff =
                    ( currentLevel > ( longTermPeak * 1.3f ) && ( targetGain < autoLevelCurrentGains[id] ) ) ? attackCoeff : releaseCoeff;
                if ( isNew )
                    smoothingCoeff *= 0.5f;

                autoLevelCurrentGains[id] += ( targetGain - autoLevelCurrentGains[id] ) * smoothingCoeff;
                autoLevelCurrentGains[id] = juce::jlimit ( 0.0f, 0.95f, autoLevelCurrentGains[id] );
            }
        }

        // 5. Final Gain Dispatch and Meter Update pass
        for ( auto& strip : channelStrips )
        {
            int  id   = strip->getChannelId();
            bool isMe = strip->getIsMe();

            // Determine if the channel should be silent
            bool isMutedLocally  = channelMutes.count ( id ) && channelMutes[id];
            bool isSoloedLocally = channelSolos.count ( id ) && channelSolos[id];
            bool isSoloedOut     = anySolo && !isSoloedLocally && !isMe;
            bool silenced        = isMutedLocally || isSoloedOut;

            float finalGain = 0.0f;
            if ( !silenced )
            {
                // Is this channel controlled by auto-level (global or per-channel via boost)?
                bool globalAutoLevel = autoLevelEnabled && !isMe && ( !channelGroups.count ( id ) || channelGroups[id] == -1 );
                bool boostAutoLevel  = !isMe && channelBoosts.count ( id ) && channelBoosts[id];

                if ( ( globalAutoLevel || boostAutoLevel ) && autoLevelCurrentGains.count ( id ) )
                {
                    // Use the auto-level calculated gain
                    finalGain = autoLevelCurrentGains[id];
                }
                else
                {
                    // Manual mode: just use fader (no boost multiplier since boost now means auto-level)
                    float baseFader = channelGains.count ( id ) ? channelGains[id] : 1.0f;
                    finalGain       = baseFader;
                }
            }

            const bool autoControlled = !isMe && ( ( autoLevelEnabled && ( !channelGroups.count ( id ) || channelGroups[id] == -1 ) ) ||
                                                   ( channelBoosts.count ( id ) && channelBoosts[id] ) );
            if ( autoControlled )
            {
                suppressManualGainPersistence = true;
                strip->setGain ( finalGain );
                suppressManualGainPersistence = false;
            }
            else if ( channelGains.count ( id ) )
                strip->setGain ( channelGains[id] );

            if ( isMe )
            {
                strip->setLevel ( ( ( inL + inR ) * 0.5f ) * finalGain );
            }
            else if ( id != -1 && rawInputLevels.count ( id ) )
            {
                strip->setLevel ( rawInputLevels[id] * finalGain );
            }

            if ( !dispatcherLastSentGains.count ( id ) || std::abs ( finalGain - dispatcherLastSentGains[id] ) > 0.0001f )
            {
                int targetId = isMe ? -1 : id;
                if ( onChannelGainChanged )
                    onChannelGainChanged ( targetId, finalGain );
                else if ( jamulusClient )
                    jamulus_client_set_channel_gain ( jamulusClient, targetId, finalGain );
                dispatcherLastSentGains[id] = finalGain;
            }
        }

        // Process event pump so gain/pan timers can fire
        jamulus_process_events();
    }

    void showSettingsDialog()
    {
        refreshAvailableThemes();
        if ( !settingsWindow )
        {
            auto* settings = new SettingsComponent ( jamulusClient, processor );
            configureSettingsComponent ( *settings );
            settingsWindow = std::make_unique<SettingsWindow> ( settings );
            settingsWindow->onWindowClosed = [this]() {
                saveSettings();
                settingsWindow.reset();
            };
        }

        if ( auto* settings = getSettingsComponent() )
        {
            settings->setThemeCallbacks ( [this]() { return getAvailableThemeNames(); },
                                          [this]() { return getCurrentThemeName(); },
                                          [this] ( const juce::String& name ) { setThemeByName ( name ); } );
            settings->setTestToneEnabled ( testToneEnabled );
            settings->setMixerRows ( mixerRows );
            settings->setMixerSortMode ( mixerSortMode );
        }

        settingsWindow->setVisible ( true );
        settingsWindow->toFront ( true );
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
            // Save messages and color before closing window
            if ( auto* chatComp = dynamic_cast<ChatComponent*> ( chatWindow->getContentComponent() ) )
            {
                sharedChatMessages = chatComp->getMessages();
                sharedChatColor    = chatComp->getTextColor();
            }
            chatWindow.reset();
        }

        chatPanelVisible = chatButton.getToggleState();

        if ( chatPanelVisible && !chatPanel )
        {
            chatPanel          = std::make_unique<ChatComponent> ( jamulusClient, true );
            if ( getNextChatMessage )
                chatPanel->setChatMessagePoller ( [this]() { return getNextChatMessage(); } );
            chatPanel->onClose = [this]() {
                // Save messages and color before closing
                sharedChatMessages = chatPanel->getMessages();
                sharedChatColor    = chatPanel->getTextColor();
                chatButton.setToggleState ( false, juce::dontSendNotification );
                toggleChatPanel();
            };
            chatPanel->onPopOut                   = [this]() { popOutChat(); };
            chatPanel->onSignalingMessageReceived = [this] ( const juce::String& sender, const juce::String& payload ) {
                handleVstSignal ( sender, payload );
            };
            chatPanel->onSendRaw = [this] ( const juce::String& text ) { sendChatToEngine ( text ); };
            chatPanel->onAutoTranslateToggled = [this] ( bool enabled ) {
                if ( processor )
                    processor->setAutoTranslateEnabled ( enabled );
                saveSettings();
            };
            chatPanel->setTranslationConfigGetters ( [this]() { return processor ? processor->isAutoTranslateEnabled() : false; },
                                                     [this]() { return processor ? processor->getTranslateTargetLang() : juce::String ( "system" ); } );

            // Restore previous messages and color
            chatPanel->setTextColor ( sharedChatColor );
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
            chatPanel->grabKeyboardFocus();
        }

        if ( !chatPanelVisible && chatPanel )
        {
            // Save messages and color before hiding
            sharedChatMessages = chatPanel->getMessages();
            sharedChatColor    = chatPanel->getTextColor();
            removeChildComponent ( chatPanel.get() );
            chatPanel.reset();
        }

        saveSettings();
        updateChatButtonColor();
        resized();
    }

    void popOutChat()
    {
        // Save messages from panel before closing
        if ( chatPanel )
        {
            sharedChatMessages = chatPanel->getMessages();
            sharedChatColor    = chatPanel->getTextColor();
        }

        // Close the panel
        chatButton.setToggleState ( false, juce::dontSendNotification );
        chatPanelVisible = false;
        if ( chatPanel )
        {
            removeChildComponent ( chatPanel.get() );
            chatPanel.reset();
        }

        // Create pop-out window
        chatWindow = std::make_unique<ChatWindow> ( jamulusClient );
        if ( auto* chatComp = dynamic_cast<ChatComponent*> ( chatWindow->getContentComponent() ) )
        {
            chatComp->setTextColor ( sharedChatColor );
            chatComp->onSignalingMessageReceived = [this] ( const juce::String& sender, const juce::String& payload ) {
                handleVstSignal ( sender, payload );
            };
            chatComp->onSendRaw = [this] ( const juce::String& text ) { sendChatToEngine ( text ); };
            chatComp->onAutoTranslateToggled = [this] ( bool enabled ) {
                if ( processor )
                    processor->setAutoTranslateEnabled ( enabled );
                saveSettings();
            };
            chatComp->setTranslationConfigGetters ( [this]() { return processor ? processor->isAutoTranslateEnabled() : false; },
                                                    [this]() { return processor ? processor->getTranslateTargetLang() : juce::String ( "system" ); } );
            if ( getNextChatMessage )
                chatComp->setChatMessagePoller ( [this]() { return getNextChatMessage(); } );
        }
        chatWindow->onWindowClosed = [this]() {
            // Save messages and color before closing
            if ( auto* chatComp = dynamic_cast<ChatComponent*> ( chatWindow->getContentComponent() ) )
            {
                sharedChatMessages = chatComp->getMessages();
                sharedChatColor    = chatComp->getTextColor();
            }
            chatWindow.reset();
            saveSettings();
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
        // Save messages and color from popup window
        if ( chatWindow )
        {
            if ( auto* chatComp = dynamic_cast<ChatComponent*> ( chatWindow->getContentComponent() ) )
            {
                sharedChatMessages = chatComp->getMessages();
                sharedChatColor    = chatComp->getTextColor();
            }
        }

        // Close the pop-out window
        chatWindow.reset();

        // Show the panel
        chatButton.setToggleState ( true, juce::dontSendNotification );
        toggleChatPanel();
        saveSettings();
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
        connectComp.setVisible ( false );
        // Note: chatPanel is handled separately via toggle
        currentDialog = nullptr;
    }

    SettingsComponent* getSettingsComponent() const
    {
        if ( !settingsWindow )
            return nullptr;
        return dynamic_cast<SettingsComponent*> ( settingsWindow->getContentComponent() );
    }

    void configureSettingsComponent ( SettingsComponent& settings )
    {
        settings.onClose = [this]() {
            if ( settingsWindow )
                settingsWindow->closeButtonPressed();
        };
        settings.onMixerRowsChanged = [this] ( int rows ) { setMixerRows ( rows ); };
        settings.onMixerSortModeChanged = [this] ( int mode ) { setMixerSortMode ( mode ); };
        settings.setMixerRows ( mixerRows );
        settings.setMixerSortMode ( mixerSortMode );

        settings.setProfileCallbacks ( getProfileName,
                                       setProfileName,
                                       getProfileInstrument,
                                       setProfileInstrument,
                                       getProfileCountry,
                                       setProfileCountry,
                                       getProfileCountryCode,
                                       getProfileCity,
                                       setProfileCity,
                                       getProfileSkill,
                                       setProfileSkill );

        settings.setProfileChangedCallback ( [this]() { saveSettings(); } );
        settings.setMidiDeviceCallbacks ( getMidiInputDeviceNames,
                                          getSelectedMidiInputDevice,
                                          [this] ( const juce::String& name ) {
                                              if ( setSelectedMidiInputDevice )
                                                  setSelectedMidiInputDevice ( name );
                                              saveSettings();
                                          },
                                          getMidiOutputDeviceNames,
                                          getSelectedMidiOutputDevice,
                                          [this] ( const juce::String& name ) {
                                              if ( setSelectedMidiOutputDevice )
                                                  setSelectedMidiOutputDevice ( name );
                                              saveSettings();
                                          } );
        if ( getRecordingSettings )
            settings.setRecordingSettings ( getRecordingSettings() );
        settings.setRecordingSettingsCallback ( [this] ( const JamulusPluginProcessor::RecordingSettings& recordingSettings ) {
            if ( setRecordingSettings )
                setRecordingSettings ( recordingSettings );
            saveSettings();
        } );
        settings.setTestToneEnabled ( testToneEnabled );
        settings.setTestToneCallback ( [this] ( bool enabled ) {
            testToneEnabled = enabled;
            testToneButton.setToggleState ( enabled, juce::dontSendNotification );
            if ( onTestToneChanged )
                onTestToneChanged ( enabled );
            saveSettings();
        } );
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

        int  numChannels = getNumChannels ? getNumChannels() : jamulus_client_get_num_channels ( jamulusClient );
        int  myId        = getMyChannelId ? getMyChannelId() : jamulus_client_get_my_channel_id ( jamulusClient );
        bool meIncluded  = false;

        for ( int i = 0; i < numChannels; ++i )
        {
            jamulus_channel_info_t info;
            bool                   ok = false;
            if ( getChannelInfo )
                ok = getChannelInfo ( i, info );
            else
                ok = jamulus_client_get_channel_info ( jamulusClient, i, &info );
            if ( ok )
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
            meInfo.id = -1;

            juce::String profileName;
            if ( getProfileName )
                profileName = getProfileName();
            const char* name = nullptr;
            if ( profileName.isNotEmpty() )
                name = profileName.toRawUTF8();
            else
                name = jamulus_client_get_name ( jamulusClient );
            strncpy ( meInfo.name, name ? name : "Me", sizeof ( meInfo.name ) - 1 );
            meInfo.name[sizeof ( meInfo.name ) - 1] = '\0';

            int instrument = 0;
            if ( getProfileInstrument )
                instrument = getProfileInstrument();
            else
                instrument = jamulus_client_get_instrument ( jamulusClient );
            meInfo.instrument = instrument;

            juce::String profileCode;
            if ( getProfileCountryCode )
                profileCode = getProfileCountryCode();
            const char* country = nullptr;
            const char* useCode = nullptr;
            if ( profileCode.isNotEmpty() )
                useCode = profileCode.toRawUTF8();
            else
            {
                country = jamulus_client_get_country_code ( jamulusClient );
                useCode = country ? country : "none";
                if ( std::strcmp ( useCode, "none" ) == 0 )
                {
                    int wire = 0;
                    if ( getProfileCountry )
                        wire = getProfileCountry();
                    else
                        wire = jamulus_client_get_country ( jamulusClient );
                    juce::String iso = isoFromWireCode ( wire );
                    useCode          = iso.toRawUTF8();
                }
            }
            strncpy ( meInfo.country_code, useCode, sizeof ( meInfo.country_code ) - 1 );
            meInfo.country_code[sizeof ( meInfo.country_code ) - 1] = '\0';

            juce::String profileCity;
            if ( getProfileCity )
                profileCity = getProfileCity();
            const char* city = nullptr;
            if ( profileCity.isNotEmpty() )
                city = profileCity.toRawUTF8();
            else
                city = jamulus_client_get_city ( jamulusClient );
            strncpy ( meInfo.city, city ? city : "", sizeof ( meInfo.city ) - 1 );
            meInfo.city[sizeof ( meInfo.city ) - 1] = '\0';

            int skill = 0;
            if ( getProfileSkill )
                skill = getProfileSkill();
            else
                skill = jamulus_client_get_skill ( jamulusClient );
            meInfo.skill_level = skill;

            stripsToCreate.insert ( stripsToCreate.begin(), { -1, meInfo, true } );
        }

        syncLocalChannelSlots ( stripsToCreate, myId );

        std::sort ( stripsToCreate.begin(), stripsToCreate.end(), [this] ( const ChannelInfo& a, const ChannelInfo& b ) {
            if ( a.isMe != b.isMe )
                return a.isMe;

            if ( mixerSortMode == 1 )
            {
                const auto aIt = localChannelSlots.find ( a.info.id );
                const auto bIt = localChannelSlots.find ( b.info.id );
                const int aSlot = a.isMe ? 0 : ( aIt != localChannelSlots.end() ? aIt->second : a.info.id + 1 );
                const int bSlot = b.isMe ? 0 : ( bIt != localChannelSlots.end() ? bIt->second : b.info.id + 1 );
                if ( aSlot != bSlot )
                    return aSlot < bSlot;
            }

            return a.info.id < b.info.id;
        } );

        // Fully clear existing strips and UI components before rebuild to prevent ghost offsets
        mixerContainer.removeAllChildren();
        channelStrips.clear();

        for ( const auto& ci : stripsToCreate )
        {
            juce::String userName = juce::String::fromUTF8 ( ci.info.name );
            DBG ( "Channel " << ci.info.id << ": " << userName << " Inst: " << ci.info.instrument << " Country: " << ci.info.country_code );
            auto strip = std::make_unique<ChannelStrip>();
            // Force the "Me" track ID to -1 so its local settings map entries are stable
            int faderId = ci.isMe ? -1 : ci.info.id;
            int skillForStrip = ci.info.skill_level;
            if ( ci.isMe )
            {
                if ( getProfileSkill )
                    skillForStrip = getProfileSkill();
                else
                    skillForStrip = jamulus_client_get_skill ( jamulusClient );
            }
            strip->setChannelInfo ( faderId,
                                    userName,
                                    ci.info.instrument,
                                    ci.info.country_code,
                                    ci.info.city,
                                    skillForStrip,
                                    ci.isMe );
            auto stripPtr        = strip.get();
            strip->onGainChanged = [this, stripPtr, isPersonalTrack = ci.isMe, userName] ( int id, float gain ) {
                float oldGain = channelGains.count ( id ) ? channelGains[id] : 1.0f;

                // Update persistence
                if ( !suppressManualGainPersistence && !userName.isEmpty() )
                {
                    userPersistenceMap[userName].gain = gain;
                    if ( onChannelReferenceGainChanged )
                        onChannelReferenceGainChanged ( id, gain );
                    saveSettings();
                }

                // Use the previous non-zero gain as reference if the current gain is 0
                float refOldGain = ( oldGain > 0.001f ) ? oldGain : ( previousGains.count ( id ) ? previousGains[id] : 1.0f );

                channelGains[id] = gain;
                if ( gain > 0.001f )
                    previousGains[id] = gain;

                if ( isPersonalTrack )
                {
                    if ( onChannelGainChanged )
                        onChannelGainChanged ( id, gain );
                    else if ( jamulusClient )
                        jamulus_client_set_channel_gain ( jamulusClient, id, gain );
                }
                else
                {
                    bool globalAutoLevel = autoLevelEnabled && ( !channelGroups.count ( id ) || channelGroups[id] == -1 );
                    bool boostAutoLevel  = channelBoosts.count ( id ) && channelBoosts[id];

                    if ( !globalAutoLevel && !boostAutoLevel )
                    {
                        if ( onChannelGainChanged )
                            onChannelGainChanged ( id, gain );
                        else if ( jamulusClient )
                            jamulus_client_set_channel_gain ( jamulusClient, id, gain );
                        dispatcherLastSentGains[id] = gain;
                    }
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
                            float otherG   = channelGains.count ( otherId ) ? channelGains[otherId] : 1.0f;
                            float otherRef = ( otherG > 0.001f ) ? otherG : ( previousGains.count ( otherId ) ? previousGains[otherId] : 1.0f );

                            float otherNewGain = juce::jlimit ( 0.0f, 1.0f, otherRef * ratio );

                            if ( std::abs ( otherNewGain - otherG ) > 0.0001f )
                            {
                                channelGains[otherId] = otherNewGain;
                                if ( otherNewGain > 0.001f )
                                    previousGains[otherId] = otherNewGain;

                                s->setGain ( otherNewGain );
                                float otherBoost = channelBoosts.count ( otherId ) && channelBoosts[otherId] ? 2.0f : 1.0f;
                                float sendGain   = otherNewGain * otherBoost;
                                if ( onChannelGainChanged )
                                    onChannelGainChanged ( otherId, sendGain );
                                else if ( jamulusClient )
                                    jamulus_client_set_channel_gain ( jamulusClient, otherId, sendGain );
                            }
                        }
                    }
                }
            };
            strip->onPanChanged = [this, userName] ( int id, float pan ) {
                if ( onChannelPanChanged )
                    onChannelPanChanged ( id, pan );
                else if ( jamulusClient )
                    jamulus_client_set_channel_pan ( jamulusClient, id, pan );
                if ( !userName.isEmpty() )
                {
                    userPersistenceMap[userName].pan = pan;
                    saveSettings();
                }
            };
            strip->onMuteChanged = [this] ( int id, bool muted ) {
                channelMutes[id] = muted;
                if ( muted )
                {
                    if ( onChannelMuteChanged )
                        onChannelMuteChanged ( id, true );
                    else if ( onChannelGainChanged )
                        onChannelGainChanged ( id, 0.0f );
                    else if ( jamulusClient )
                        jamulus_client_set_channel_gain ( jamulusClient, id, 0.0f );
                    dispatcherLastSentGains[id] = 0.0f;
                }
                else
                {
                    float gain = channelGains.count ( id ) ? channelGains[id] : 1.0f;
                    if ( onChannelMuteChanged )
                        onChannelMuteChanged ( id, false );
                    else if ( onChannelGainChanged )
                        onChannelGainChanged ( id, gain );
                    else if ( jamulusClient )
                        jamulus_client_set_channel_gain ( jamulusClient, id, gain );
                    dispatcherLastSentGains[id] = gain;
                }
            };
            strip->onSoloChanged = [this] ( int id, bool solo ) {
                channelSolos[id] = solo;
                if ( onChannelSoloChanged )
                    onChannelSoloChanged ( id, solo );
                dispatcherLastSentGains.clear();
            };
            strip->onBoostChanged = [this, isPersonalTrack = ci.isMe] ( int id, bool boosted ) {
                channelBoosts[id] = boosted;

                if ( isPersonalTrack )
                {
                    // For the local track, we use the native Jamulus input boost (1=0dB, 2=6dB)
                    jamulus_client_set_input_boost ( jamulusClient, boosted ? 2 : 1 );

                    float gain = channelGains.count ( id ) ? channelGains[id] : 1.0f;
                    if ( onChannelGainChanged )
                        onChannelGainChanged ( id, gain );
                    else if ( jamulusClient )
                        jamulus_client_set_channel_gain ( jamulusClient, id, gain );
                    saveSettings();
                }
                else
                {
                    // Boost on remote tracks enables per-channel auto-level
                    if ( boosted )
                    {
                        // Initialize auto-level tracking from current fader position
                        float faderVolume         = channelGains.count ( id ) ? channelGains[id] : 1.0f;
                        autoLevelCurrentGains[id] = faderVolume;
                        // Peak tracking will be initialized on next timer tick
                    }
                    else
                    {
                        float gain = channelGains.count ( id ) ? channelGains[id] : 1.0f;
                        if ( onChannelGainChanged )
                            onChannelGainChanged ( id, gain );
                        else if ( jamulusClient )
                            jamulus_client_set_channel_gain ( jamulusClient, id, gain );
                        dispatcherLastSentGains[id] = gain;

                        // Clear auto-level tracking for this channel
                        autoLevelCurrentGains.erase ( id );
                        autoLevelPeakLevels.erase ( id );
                        autoLevelPeakHoldCounters.erase ( id );
                        autoLevelChannelActiveTicks.erase ( id );
                    }
                }
            };
            strip->onGroupChanged = [this] ( int id, int groupId ) { channelGroups[id] = groupId; };
            strip->onMonoChanged  = [this, isPersonalTrack = ci.isMe, userName] ( int id, bool mono ) {
                if ( isPersonalTrack )
                {
                    isMeMono = mono;
                    if ( fxPanel )
                        fxPanel->setMonoConstraint ( mono );
                }

                if ( !userName.isEmpty() )
                {
                    userPersistenceMap[userName].isMono = mono;
                    saveSettings();
                }
            };
            strip->onOutputRouteChanged = [this, userName] ( int id, int routeCode ) {
                if ( !userName.isEmpty() )
                {
                    userPersistenceMap[userName].outputRoute = routeCode;
                    saveSettings();
                }

                if ( onChannelOutputRouteChanged )
                    onChannelOutputRouteChanged ( id, routeCode );
            };
            strip->onVstClicked = [this] ( int id ) { openPrivateChat ( id ); };

            // VST Peer Detection (Silent via City Marker OR via previous Signals)
            bool isVstPeer = false;
            if ( !ci.isMe )
            {
                // check if we have a signal-based record
                if ( vstPeers.count ( ci.info.id ) )
                    isVstPeer = true;
                else
                {
                    // check for silent city marker "(V)"
                    juce::String city ( ci.info.city );
                    if ( city.endsWith ( "(V)" ) )
                    {
                        isVstPeer = true;
                        // Cache it so we recognize them for signaling too
                        vstPeers[ci.info.id]     = juce::Time::getCurrentTime();
                        vstPeerNames[ci.info.id] = juce::String ( ci.info.name );
                    }
                }
            }
            strip->setVstPeer ( isVstPeer );
            strip->applyTheme ( currentTheme );

            // Restore states (Session Priority -> Persistence -> Defaults)
            if ( ci.info.id != -1 )
            {
                if ( channelMutes.count ( ci.info.id ) )
                    strip->setMute ( channelMutes[ci.info.id] );
                else
                {
                    strip->setMute ( ci.info.muted );
                    channelMutes[ci.info.id] = ci.info.muted;
                }

                if ( channelSolos.count ( ci.info.id ) )
                    strip->setSolo ( channelSolos[ci.info.id] );
                else
                {
                    strip->setSolo ( ci.info.solo );
                    channelSolos[ci.info.id] = ci.info.solo;
                }

                if ( channelBoosts.count ( ci.info.id ) )
                    strip->setBoost ( channelBoosts[ci.info.id] );
                if ( channelGroups.count ( ci.info.id ) )
                    strip->setGroup ( channelGroups[ci.info.id] );
            }

            if ( channelGains.count ( ci.info.id ) )
            {
                strip->setGain ( channelGains[ci.info.id] );
            }
            else if ( !userName.isEmpty() && userPersistenceMap.count ( userName ) )
            {
                auto& p = userPersistenceMap[userName];
                strip->setGain ( p.gain );
                strip->setPan ( p.pan );
                strip->setMonoMode ( p.isMono ); // Note: this will trigger onMonoChanged callback
                strip->setOutputRoute ( p.outputRoute );

                channelGains[ci.info.id] = p.gain;

                // Sync with Jamulus client for the new channel
                if ( ci.info.id != -1 )
                {
                    // jamulus_client_set_channel_gain is deferred to timerCallback
                    // to ensure solo/mute logic is applied correctly.
                    jamulus_client_set_channel_pan ( jamulusClient, ci.info.id, p.pan );
                    if ( onChannelOutputRouteChanged )
                        onChannelOutputRouteChanged ( ci.info.id, p.outputRoute );
                }
            }
            else
            {
                // Default from Jamulus info (or unity if not available)
                const float defaultRemoteGain = std::pow ( 10.0f, -6.0f / 20.0f );
                float g = 1.0f;
                if ( ci.info.id != -1 && ci.info.gain > 0.001f )
                    g = ci.info.gain;
                else if ( !ci.isMe )
                    g = defaultRemoteGain;
                channelGains[ci.info.id] = g;
                strip->setGain ( g );
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

    void handleVstSignal ( const juce::String& sender, const juce::String& payload )
    {
        // Payload format: "CHANNEL_ID;COMMAND;DATA"
        int firstSemi = payload.indexOf ( ";" );
        if ( firstSemi <= 0 )
            return;

        int          senderId = payload.substring ( 0, firstSemi ).getIntValue();
        juce::String cmdData  = payload.substring ( firstSemi + 1 );

        int          secondSemi = cmdData.indexOf ( ";" );
        juce::String cmd        = ( secondSemi > 0 ) ? cmdData.substring ( 0, secondSemi ) : cmdData;
        juce::String data       = ( secondSemi > 0 ) ? cmdData.substring ( secondSemi + 1 ) : "";

        if ( cmd == "HELO" )
        {
            // Peer discovery
            bool alreadyPeer       = vstPeers.count ( senderId ) > 0;
            vstPeers[senderId]     = juce::Time::getCurrentTime();
            vstPeerNames[senderId] = sender;

            if ( !alreadyPeer )
            {
                lastChannelsHash = 0; // Refresh UI
            }
        }
        else if ( cmd == "CHAT" )
        {
            // Private chat message
            // The 'data' part of the payload is "RECIPIENT_ID;MESSAGE"
            int thirdSemi = data.indexOf ( ";" );
            if ( thirdSemi <= 0 )
                return; // Malformed private chat message

            int          recipientId = data.substring ( 0, thirdSemi ).getIntValue();
            juce::String message     = data.substring ( thirdSemi + 1 );

            int myId = jamulus_client_get_my_channel_id ( jamulusClient );
            if ( myId == -1 )
                return; // Not fully joined yet

            if ( recipientId == myId )
            {
                onPrivateMessageReceived ( senderId, message );
            }
        }
    }

    void sendChatToEngine ( const juce::String& text )
    {
        bool connected = false;
        if ( getIsConnected )
            connected = getIsConnected();
        else if ( jamulusClient )
            connected = jamulus_client_is_connected ( jamulusClient );

        if ( !connected )
            return;

        if ( onChatSend )
        {
            onChatSend ( text );
        }
        else if ( jamulusClient )
        {
            jamulus_client_send_chat_message ( jamulusClient, text.toRawUTF8() );
        }
    }

    void sendVstSignal ( const juce::String& cmd, const juce::String& data = "" )
    {
        int myId = -1;
        if ( getMyChannelId )
            myId = getMyChannelId();
        else if ( jamulusClient )
            myId = jamulus_client_get_my_channel_id ( jamulusClient );
        if ( myId == -1 )
            return;

        juce::String payload = juce::String ( myId ) + ";" + cmd;
        if ( data.isNotEmpty() )
            payload += ";" + data;

        juce::String fullMsg = juce::String ( ChatComponent::SIG_PREFIX ) + payload;
        sendChatToEngine ( fullMsg );
    }

    void onPrivateMessageReceived ( int senderId, const juce::String& text )
    {
        if ( privateChatWindows.count ( senderId ) )
        {
            privateChatWindows[senderId]->addMessage ( vstPeerNames[senderId], text );
        }
        else
        {
            // For now, auto-open window if message received
            openPrivateChat ( senderId );
            if ( privateChatWindows.count ( senderId ) ) // Check if it was successfully opened
                privateChatWindows[senderId]->addMessage ( vstPeerNames[senderId], text );
        }
    }

    // --- Private Chat ---
    void openPrivateChat ( int channelId )
    {
        if ( vstPeerNames.count ( channelId ) )
        {
            if ( privateChatWindows.count ( channelId ) )
            {
                privateChatWindows[channelId]->toFront ( true );
                return;
            }

            auto name = vstPeerNames[channelId];
            auto win  = std::make_unique<PrivateChatWindow> ( channelId, name, [this, channelId] ( const juce::String& msg ) {
                int myId = -1;
                if ( getMyChannelId )
                    myId = getMyChannelId();
                else if ( jamulusClient )
                    myId = jamulus_client_get_my_channel_id ( jamulusClient );
                if ( myId != -1 )
                {
                    // Format for private chat: "RECIPIENT_ID;MESSAGE"
                    juce::String privateData = juce::String ( channelId ) + ";" + msg;
                    sendVstSignal ( "CHAT", privateData );
                }
            } );

            win->onWindowClosed = [this, channelId]() { privateChatWindows.erase ( channelId ); };

            privateChatWindows[channelId] = std::move ( win );
        }
    }

    void setMixerRows ( int rows )
    {
        rows      = juce::jlimit ( 1, 8, rows );
        mixerRows = rows;
        updateMixerLayout();
        saveSettings();
    }

    void setMixerSortMode ( int mode )
    {
        mode = juce::jlimit ( 0, 1, mode );
        if ( mixerSortMode == mode )
            return;
        mixerSortMode = mode;
        updateChannelStrips();
        updateMixerLayout();
        saveSettings();
    }

    template <typename ChannelInfoList>
    void syncLocalChannelSlots ( const ChannelInfoList& stripsToCreate, int myId )
    {
        std::set<int> activeRemoteIds;
        for ( const auto& ci : stripsToCreate )
        {
            if ( ci.info.id != myId && !ci.isMe )
                activeRemoteIds.insert ( ci.info.id );
        }

        for ( auto it = localChannelSlots.begin(); it != localChannelSlots.end(); )
        {
            if ( activeRemoteIds.count ( it->first ) == 0 )
                it = localChannelSlots.erase ( it );
            else
                ++it;
        }

        std::set<int> usedSlots;
        for ( const auto& [channelId, slot] : localChannelSlots )
            usedSlots.insert ( slot );

        for ( int channelId : activeRemoteIds )
        {
            if ( localChannelSlots.count ( channelId ) != 0 )
                continue;

            int slot = 1;
            while ( usedSlots.count ( slot ) != 0 )
                ++slot;
            localChannelSlots[channelId] = slot;
            usedSlots.insert ( slot );
        }
    }

    void updateMixerLayout()
    {
        const int standardStripWidth = 90;
        const int localStripWidth    = localStripContainer.getWidth();
        const int stripHeight        = mixerViewport.getHeight() - 20;
        const int localStripHeight   = juce::jmax ( 120, localStripContainer.getHeight() - 10 );

        for ( auto& strip : channelStrips )
        {
            if ( strip->getIsMe() )
            {
                strip->setBounds ( 0, 4, juce::jmax ( 1, localStripWidth ), localStripHeight );
                break;
            }
        }

        int numRemote = 0;
        for ( auto& strip : channelStrips )
        {
            if ( !strip->getIsMe() )
                numRemote++;
        }

        int rows = juce::jlimit ( 1, 8, mixerRows );
        if ( rows < 1 )
            rows = 1;

        if ( rows == 1 && numRemote > 1 )
        {
            const int oneRowWidth = numRemote * standardStripWidth;
            const int visibleWidth = juce::jmax ( 0, mixerViewport.getWidth() );
            if ( visibleWidth > 0 && oneRowWidth > ( visibleWidth + standardStripWidth ) )
                rows = 2;
        }

        int cols = rows > 0 ? ( ( numRemote + rows - 1 ) / rows ) : 0;
        if ( cols < 1 && numRemote > 0 )
            cols = 1;

        int totalWidth = cols * standardStripWidth;
        if ( totalWidth < mixerViewport.getWidth() )
            totalWidth = mixerViewport.getWidth();

        mixerContainer.setSize ( totalWidth, stripHeight );

        int rowHeight = rows > 0 ? stripHeight / rows : stripHeight;

        int index = 0;
        for ( auto& strip : channelStrips )
        {
            if ( !strip->getIsMe() )
            {
                int rowIndex = rows > 0 ? ( index % rows ) : 0;
                int colIndex = rows > 0 ? ( index / rows ) : index;

                int x = colIndex * standardStripWidth + 5;
                int y = rowIndex * rowHeight + 10;

                strip->setBounds ( x, y, standardStripWidth - 10, rowHeight - 20 );
                ++index;
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

    // Connect Dialog
    void showConnectDialog()
    {
        connectComp.setVisible ( true );
        connectComp.toFront ( true );
        resized();
    }

    void openCurrentServerVideoRoom()
    {
        if ( processor != nullptr )
        {
            if ( currentConnectedServerName.trim().isEmpty() )
                currentConnectedServerName = juce::String ( processor->getCurrentServerName() );
            if ( currentConnectedServerAddress.trim().isEmpty() )
                currentConnectedServerAddress = juce::String ( processor->getCurrentServerAddress() );
        }

        auto roomSource = currentConnectedServerName.trim();

        if ( roomSource.isEmpty() )
            roomSource = currentConnectedServerAddress.trim();

        if ( roomSource.isEmpty() )
            return;

        const auto roomName = makeUrlSafeRoomName ( roomSource );
        const auto roomUrl  = juce::String ( "https://vdo.ninja/?room=" ) + roomName;
        const bool opened = openUrlExternalOnMessageThread ( roomUrl );
        if ( !opened )
            juce::AlertWindow::showMessageBoxAsync ( juce::AlertWindow::WarningIcon, "Cam", "Failed to open browser for: " + roomUrl );
    }

    jamulus_client_t       jamulusClient;
    ConnectComponent connectComp;
    bool             wasConnected     = false;
    int              lastNumChannels  = 0;
    juce::uint64     lastChannelsHash = 0;
    bool             chatPanelVisible = false;
    bool             fxPanelVisible   = false;
    bool             isMeMono         = false;
    int              mixerRows        = 1;
    int              mixerSortMode    = 0;
    std::unordered_map<int, int> localChannelSlots;

    // FX parameters
    float reverbTime     = 1.0f;
    float reverbPreDelay = 0.0f;
    float reverbHP       = 20.0f;
    float reverbLP       = 20000.0f;
    int   pingCounter    = 0;
    // AudioDelay removed - moved to Processor

    JamulusPluginProcessor* processor = nullptr;
    juce::StringArray       availableThemeNames;
    juce::String            selectedThemeName = "Default";
    juce::String            currentConnectedServerName;
    juce::String            currentConnectedServerAddress;
    juce::String            pendingMidiInputDeviceName = "None";
    juce::String            pendingMidiOutputDeviceName = "None";
    JamulusPluginProcessor::RecordingSettings pendingRecordingSettings;
    bool                    hasPendingRecordingSettings = false;
    JamulusTheme            currentTheme;
    juce::Image             animatedThemeBackground;
#if JUCE_WINDOWS
    std::unique_ptr<WinVideoReader> themeVideoReader;
#endif

    // Header
    juce::Label      titleLabel;
    juce::TextButton connectButton;
    juce::TextButton settingsButton;
    juce::TextButton chatButton;
    juce::TextButton cameraButton;
    juce::TextButton recordButton;
    juce::TextButton disconnectButton;
    juce::Label      statusLabel;
    juce::Label      pingLabel;

    // Main volume control
    juce::Label  mainVolumeLabel;
    juce::Label  mainVolumeDbLabel;
    juce::Label  masterPeakDbLabel;
    juce::Slider mainVolumeSlider;
    juce::Slider limiterThresholdSlider;
    juce::Label  limiterThresholdDbLabel;
    juce::Label  limiterReleaseLabel;
    juce::Slider limiterReleaseSlider;
    float        mainVolume = 1.0f;
    float        limiterThresholdDb = 0.0f;
    float        limiterReleaseMs   = 120.0f;

    // Auto Level controls
    juce::TextButton     autoLevelButton;
    juce::TextButton     limiterButton;
    juce::Label          themeHeaderLabel;
    juce::ComboBox       themeHeaderCombo;
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
    juce::TextButton engineModeButton;
    bool             monitorServerReturn = false;
    bool             coreEngineEnabled   = false;

    // Settings
    // (Consolidated into Settings dialog)

    LevelMeter masterMeterL;
    LevelMeter masterMeterR;
    FaderLookAndFeel  masterFaderLook;
    RotaryKnobLookAndFeel masterKnobLook;

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
    std::map<int, float>                       channelGains;            // Track fader values per channel
    std::map<int, float>                       previousGains;           // Track last non-zero gain for sync
    std::map<int, bool>                        channelBoosts;           // Track boost state per channel
    std::map<int, bool>                        channelMutes;            // Track mute state per channel
    std::map<int, bool>                        channelSolos;            // Track solo state per channel
    std::map<int, int>                         channelGroups;           // Track group state (ID) per channel
    std::map<int, float>                       channelLevels;           // Track current levels for auto-leveling
    std::map<int, float>                       dispatcherLastSentGains; // Track last sent to Jamulus
    bool                                       autoLevelBoostEnabled = false;
    bool                                       suppressManualGainPersistence = false;

    // Persisted User Settings
    struct UserPersistence
    {
        float gain   = 1.0f;
        float pan    = 0.0f;
        bool  isMono = false;
        int   outputRoute = 0;
    };
    std::map<juce::String, UserPersistence> userPersistenceMap;

    // Dialogs
    // Dialogs
    // std::unique_ptr<ConnectComponent>  connectDialog; // Removed in favor of member connectComp
    std::unique_ptr<SettingsWindow> settingsWindow;
    juce::Component*                currentDialog = nullptr;

    // Chat panel and window
    std::unique_ptr<ChatComponent> chatPanel;
    std::unique_ptr<ChatWindow>    chatWindow;
    // VST Peer Discovery
    int                                               vstHeartbeatCounter = 0;
    std::map<int, juce::Time>                         vstPeers;     // channelId -> lastSeen
    std::map<int, juce::String>                       vstPeerNames; // channelId -> name (for chat matching)
    std::map<int, std::unique_ptr<PrivateChatWindow>> privateChatWindows;

    // Saved chat state for sharing between panel/window
    std::vector<ChatMessage> sharedChatMessages;
    juce::Colour             sharedChatColor = juce::Colours::white;
    // Persistent chat storage

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

    void setIlluminatedButtonColour ( juce::TextButton& button, juce::Colour onColour, bool isOn )
    {
        auto offColour = onColour.darker ( 1.4f ).withAlpha ( 0.52f );
        auto textOn = onColour.getPerceivedBrightness() > 0.6f ? juce::Colours::black : juce::Colours::white;
        button.setColour ( juce::TextButton::buttonColourId, isOn ? onColour : offColour );
        button.setColour ( juce::TextButton::buttonOnColourId, isOn ? onColour : offColour );
        button.setColour ( juce::TextButton::textColourOnId, isOn ? textOn : juce::Colours::white );
        button.setColour ( juce::TextButton::textColourOffId, isOn ? textOn : juce::Colours::white );
    }

    void updateMonitorButtonColor()
    {
        setIlluminatedButtonColour ( monitorModeButton, juce::Colour ( 0xffffcc00 ), monitorModeButton.getToggleState() );
    }

    void updateEngineButtonColor()
    {
        if ( engineModeButton.getToggleState() )
        {
            juce::Colour greenGlow ( 0xff00cc66 );
            engineModeButton.setColour ( juce::TextButton::buttonColourId, greenGlow );
            engineModeButton.setColour ( juce::TextButton::buttonOnColourId, greenGlow );
            engineModeButton.setColour ( juce::TextButton::textColourOnId, juce::Colours::black );
            engineModeButton.setColour ( juce::TextButton::textColourOffId, juce::Colours::black );
        }
        else
        {
            juce::Colour defaultColor = juce::LookAndFeel::getDefaultLookAndFeel().findColour ( juce::TextButton::buttonColourId );
            engineModeButton.setColour ( juce::TextButton::buttonColourId, defaultColor );
            engineModeButton.setColour ( juce::TextButton::buttonOnColourId, defaultColor );
            engineModeButton.setColour ( juce::TextButton::textColourOnId, juce::Colours::white );
            engineModeButton.setColour ( juce::TextButton::textColourOffId, juce::Colours::white );
        }
    }

    void updateAutoLevelButtonColor()
    {
        setIlluminatedButtonColour ( autoLevelButton, juce::Colour ( 0xff00cc66 ), autoLevelButton.getToggleState() );
    }

    void updateLimiterButtonColor()
    {
        setIlluminatedButtonColour ( limiterButton, juce::Colour ( 0xffff9900 ), limiterButton.getToggleState() );
    }

    void updateRecordButtonColor()
    {
        setIlluminatedButtonColour ( recordButton, juce::Colour ( 0xffd64541 ), recordButton.getToggleState() );
    }

    void updateChatButtonColor()
    {
        setIlluminatedButtonColour ( chatButton, juce::Colour ( 0xff3598ff ), chatButton.getToggleState() );
    }

    void refreshMainVolumeDbLabel()
    {
        if ( mainVolume <= 0.0f )
        {
            mainVolumeDbLabel.setText ( "-oo dB", juce::dontSendNotification );
            return;
        }

        const auto gainDb = 20.0f * std::log10 ( mainVolume );
        mainVolumeDbLabel.setText ( juce::String ( gainDb, 1 ) + " dB", juce::dontSendNotification );
    }

public:
    // Limiter Callbacks
    std::function<void ( bool )> onLimiterEnableChanged;
    std::function<void ( float )> onLimiterThresholdChanged;
    std::function<void ( float )> onLimiterReleaseChanged;
    std::function<bool()>        onStartRecording;
    std::function<void()>        onStopRecording;
    std::function<JamulusPluginProcessor::RecordingSettings()> getRecordingSettings;
    std::function<void ( const JamulusPluginProcessor::RecordingSettings& )> setRecordingSettings;

    // Callbacks that the editor can set
    std::function<void ( bool )> onSidechainChanged;
    std::function<void ( bool )> onMonitorModeChanged; // true = server return, false = direct thru
    std::function<void ( bool )> onCoreEngineChanged;

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

    void setCoreEngineMode ( bool enabled )
    {
        coreEngineEnabled = enabled;
        engineModeButton.setToggleState ( enabled, juce::dontSendNotification );
        updateEngineButtonColor();
    }

    juce::StringArray getAvailableThemeNames() const { return availableThemeNames; }
    juce::String      getCurrentThemeName() const { return selectedThemeName; }

    void setThemeByName ( const juce::String& name )
    {
        selectedThemeName = name.isNotEmpty() ? name : "Default";
        currentTheme      = loadJamulusTheme ( selectedThemeName );
        refreshAvailableThemes();
        refreshThemeVideo();
        applyTheme();
        if ( auto* settings = getSettingsComponent() )
            settings->setThemeCallbacks ( [this]() { return getAvailableThemeNames(); },
                                          [this]() { return getCurrentThemeName(); },
                                          [this] ( const juce::String& nextName ) { setThemeByName ( nextName ); } );
        saveSettings();
    }

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR ( JamulusGuiComponent )

    void refreshAvailableThemes()
    {
        availableThemeNames = findJamulusThemeNames();
        if ( availableThemeNames.isEmpty() )
            availableThemeNames.add ( "Default" );

        themeHeaderCombo.clear ( juce::dontSendNotification );
        int itemId = 1;
        int selectedId = 1;
        for ( const auto& name : availableThemeNames )
        {
            themeHeaderCombo.addItem ( name, itemId );
            if ( name == selectedThemeName )
                selectedId = itemId;
            ++itemId;
        }
        themeHeaderCombo.setSelectedId ( selectedId, juce::dontSendNotification );
    }

    void refreshThemeVideo()
    {
        animatedThemeBackground = {};
#if JUCE_WINDOWS
        themeVideoReader.reset();
        if ( currentTheme.backgroundVideoFile.existsAsFile() )
        {
            auto reader = std::make_unique<WinVideoReader>();
            if ( reader->open ( currentTheme.backgroundVideoFile ) )
                themeVideoReader = std::move ( reader );
        }
#endif
    }

    void applyThemeToButton ( juce::TextButton& button )
    {
        button.setColour ( juce::TextButton::buttonColourId, currentTheme.buttonColour );
        button.setColour ( juce::TextButton::buttonOnColourId, currentTheme.buttonColour.brighter ( 0.15f ) );
        button.setColour ( juce::TextButton::textColourOffId, currentTheme.textColour );
        button.setColour ( juce::TextButton::textColourOnId, currentTheme.textColour );
    }

    void applyTheme()
    {
        titleLabel.setColour ( juce::Label::textColourId, currentTheme.textColour );
        mainVolumeLabel.setColour ( juce::Label::textColourId, currentTheme.textColour );
        mainVolumeDbLabel.setColour ( juce::Label::textColourId, currentTheme.textColour );
        masterPeakDbLabel.setColour ( juce::Label::textColourId, currentTheme.textColour );
        themeHeaderLabel.setColour ( juce::Label::textColourId, currentTheme.textColour );
        statusLabel.setColour ( juce::Label::textColourId, currentTheme.subTextColour );
        delayLabel.setColour ( juce::Label::textColourId, currentTheme.textColour );
        jitterLabel.setColour ( juce::Label::textColourId, currentTheme.textColour );
        pingLabel.setColour ( juce::Label::textColourId, currentTheme.textColour );
        inputLevelLabel.setColour ( juce::Label::textColourId, currentTheme.textColour );
        inputFaderLabel.setColour ( juce::Label::textColourId, currentTheme.textColour );

        applyThemeToButton ( connectButton );
        applyThemeToButton ( settingsButton );
        applyThemeToButton ( cameraButton );
        applyThemeToButton ( testToneButton );
        applyThemeToButton ( disconnectButton );
        applyThemeToButton ( scrollLeftArrow );
        applyThemeToButton ( scrollRightArrow );
        applyThemeToButton ( fxButton );
        themeHeaderCombo.setColour ( juce::ComboBox::backgroundColourId, currentTheme.panelAltColour );
        themeHeaderCombo.setColour ( juce::ComboBox::outlineColourId, currentTheme.outlineColour );
        themeHeaderCombo.setColour ( juce::ComboBox::textColourId, currentTheme.textColour );
        themeHeaderCombo.setColour ( juce::ComboBox::arrowColourId, currentTheme.textColour );

        mainVolumeSlider.setColour ( juce::Slider::thumbColourId, currentTheme.faderColour );
        limiterThresholdSlider.setColour ( juce::Slider::thumbColourId, currentTheme.faderColour );
        masterKnobLook.setKnobColour ( currentTheme.knobColour );
        masterFaderLook.setThumbImage ( currentTheme.faderKnobImage );
        masterKnobLook.setKnobImage ( currentTheme.rotaryKnobImage );
        inputFaderSlider.setColour ( juce::Slider::thumbColourId, currentTheme.faderColour );
        sidechainButton.setColour ( juce::TextButton::buttonColourId, currentTheme.buttonColour );

        for ( auto& strip : channelStrips )
            if ( strip )
                strip->applyTheme ( currentTheme );

        updateMonitorButtonColor();
        updateAutoLevelButtonColor();
        updateLimiterButtonColor();
        updateRecordButtonColor();
        updateChatButtonColor();
        repaint();
    }

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

        if ( obj->hasProperty ( "autoJitter" ) )
        {
            const bool autoJitter = static_cast<bool> ( obj->getProperty ( "autoJitter" ) );
            if ( processor )
                processor->setAutoJitter ( autoJitter );
            else
                jamulus_client_set_auto_jitter ( jamulusClient, autoJitter );
        }
        if ( obj->hasProperty ( "jitterBuffer" ) )
        {
            const int jitterBuffer = static_cast<int> ( obj->getProperty ( "jitterBuffer" ) );
            if ( processor )
                processor->setJitterBuffer ( jitterBuffer );
            else
                jamulus_client_set_jitter_buffer ( jamulusClient, jitterBuffer );
        }
        if ( obj->hasProperty ( "serverJitterBuffer" ) )
        {
            const int serverJitterBuffer = static_cast<int> ( obj->getProperty ( "serverJitterBuffer" ) );
            if ( processor )
                processor->setServerJitterBuffer ( serverJitterBuffer );
            else
                jamulus_client_set_server_jitter_buffer ( jamulusClient, serverJitterBuffer );
        }
        if ( obj->hasProperty ( "newClientLevel" ) )
            jamulus_client_set_new_client_level ( jamulusClient, static_cast<int> ( obj->getProperty ( "newClientLevel" ) ) );
        if ( obj->hasProperty ( "smallBuffers" ) )
        {
            const bool smallBuffers = static_cast<bool> ( obj->getProperty ( "smallBuffers" ) );
            if ( processor )
                processor->setSmallBuffers ( smallBuffers );
            else
                jamulus_client_set_small_buffers ( jamulusClient, smallBuffers );
        }

        if ( obj->hasProperty ( "mixerRows" ) )
        {
            int rows = static_cast<int> ( obj->getProperty ( "mixerRows" ) );
            mixerRows = juce::jlimit ( 1, 8, rows );
        }
        if ( obj->hasProperty ( "mixerSortMode" ) )
            mixerSortMode = juce::jlimit ( 0, 1, static_cast<int> ( obj->getProperty ( "mixerSortMode" ) ) );
        if ( obj->hasProperty ( "mainVolumeSlider" ) )
        {
            mainVolumeSlider.setValue ( static_cast<double> ( obj->getProperty ( "mainVolumeSlider" ) ), juce::dontSendNotification );
            const double sliderVal = mainVolumeSlider.getValue();
            if ( sliderVal <= 0.0 )
                mainVolume = 0.0f;
            else if ( sliderVal <= 100.0 )
                mainVolume = static_cast<float> ( sliderVal / 100.0 );
            else
                mainVolume = static_cast<float> ( std::pow ( 10.0, ( sliderVal - 100.0 ) / 20.0 ) );
            refreshMainVolumeDbLabel();
            if ( onMainVolumeChanged )
                onMainVolumeChanged ( mainVolume );
        }
        if ( obj->hasProperty ( "monitorMode" ) )
        {
            monitorServerReturn = static_cast<bool> ( obj->getProperty ( "monitorMode" ) );
            monitorModeButton.setToggleState ( monitorServerReturn, juce::dontSendNotification );
            updateMonitorButtonColor();
            if ( onMonitorModeChanged )
                onMonitorModeChanged ( monitorServerReturn );
        }
        if ( obj->hasProperty ( "fxPanelVisible" ) )
        {
            fxPanelVisible = static_cast<bool> ( obj->getProperty ( "fxPanelVisible" ) );
            fxButton.setToggleState ( fxPanelVisible, juce::dontSendNotification );
        }

        // Feature Toggles (Auto Level, Limiter)
        if ( obj->hasProperty ( "autoLevelEnabled" ) )
        {
            autoLevelEnabled = static_cast<bool> ( obj->getProperty ( "autoLevelEnabled" ) );
            autoLevelButton.setToggleState ( autoLevelEnabled, juce::dontSendNotification );
            updateAutoLevelButtonColor();
        }
        if ( obj->hasProperty ( "limiterEnabled" ) )
        {
            bool lim = static_cast<bool> ( obj->getProperty ( "limiterEnabled" ) );
            limiterButton.setToggleState ( lim, juce::dontSendNotification );
            if ( onLimiterEnableChanged )
                onLimiterEnableChanged ( lim );
            updateLimiterButtonColor();
        }
        if ( obj->hasProperty ( "midiInputDevice" ) )
        {
            pendingMidiInputDeviceName = obj->getProperty ( "midiInputDevice" ).toString();
            if ( setSelectedMidiInputDevice )
                setSelectedMidiInputDevice ( pendingMidiInputDeviceName );
        }
        if ( obj->hasProperty ( "midiOutputDevice" ) )
        {
            pendingMidiOutputDeviceName = obj->getProperty ( "midiOutputDevice" ).toString();
            if ( setSelectedMidiOutputDevice )
                setSelectedMidiOutputDevice ( pendingMidiOutputDeviceName );
        }
        if ( obj->hasProperty ( "limiterThresholdDb" ) )
        {
            limiterThresholdDb = static_cast<float> ( static_cast<double> ( obj->getProperty ( "limiterThresholdDb" ) ) );
            limiterThresholdSlider.setValue ( limiterThresholdDb, juce::dontSendNotification );
            limiterThresholdDbLabel.setText ( juce::String ( limiterThresholdDb, 1 ) + " dB", juce::dontSendNotification );
            if ( onLimiterThresholdChanged )
                onLimiterThresholdChanged ( limiterThresholdDb );
        }
        if ( obj->hasProperty ( "limiterReleaseMs" ) )
        {
            limiterReleaseMs = static_cast<float> ( static_cast<double> ( obj->getProperty ( "limiterReleaseMs" ) ) );
            limiterReleaseSlider.setValue ( limiterReleaseMs, juce::dontSendNotification );
            if ( onLimiterReleaseChanged )
                onLimiterReleaseChanged ( limiterReleaseMs );
        }
        if ( obj->hasProperty ( "testToneEnabled" ) )
        {
            testToneEnabled = static_cast<bool> ( obj->getProperty ( "testToneEnabled" ) );
            testToneButton.setToggleState ( testToneEnabled, juce::dontSendNotification );
            if ( onTestToneChanged )
                onTestToneChanged ( testToneEnabled );
        }
        if ( obj->hasProperty ( "themeName" ) )
        {
            selectedThemeName = obj->getProperty ( "themeName" ).toString();
            if ( selectedThemeName.isEmpty() )
                selectedThemeName = "Default";
            currentTheme = loadJamulusTheme ( selectedThemeName );
        }
        if ( obj->hasProperty ( "chatPanelVisible" ) )
            chatPanelVisible = static_cast<bool> ( obj->getProperty ( "chatPanelVisible" ) );
        if ( obj->hasProperty ( "chatTextColour" ) )
        {
            auto colourString = obj->getProperty ( "chatTextColour" ).toString();
            auto parsed       = juce::Colour::fromString ( colourString );
            if ( parsed.getARGB() != 0 )
                sharedChatColor = parsed;
        }
        if ( obj->hasProperty ( "chatAutoTranslate" ) && processor )
            processor->setAutoTranslateEnabled ( static_cast<bool> ( obj->getProperty ( "chatAutoTranslate" ) ) );
        if ( obj->hasProperty ( "chatTranslateTarget" ) && processor )
            processor->setTranslateTargetLang ( obj->getProperty ( "chatTranslateTarget" ).toString() );
        JamulusPluginProcessor::RecordingSettings recording;
        if ( obj->hasProperty ( "recordingMode" ) )
            recording.mode = static_cast<int> ( obj->getProperty ( "recordingMode" ) );
        if ( obj->hasProperty ( "recordingFolder" ) )
            recording.folderPath = obj->getProperty ( "recordingFolder" ).toString();
        if ( obj->hasProperty ( "recordStereoUseLiveAutoLevel" ) )
            recording.stereoUseLiveAutoLevel = static_cast<bool> ( obj->getProperty ( "recordStereoUseLiveAutoLevel" ) );
        if ( obj->hasProperty ( "recordStereoApplyLimiter" ) )
            recording.stereoApplyLimiter = static_cast<bool> ( obj->getProperty ( "recordStereoApplyLimiter" ) );
        if ( obj->hasProperty ( "recordStemsUseLiveAutoLevel" ) )
            recording.stemsUseLiveAutoLevel = static_cast<bool> ( obj->getProperty ( "recordStemsUseLiveAutoLevel" ) );
        pendingRecordingSettings = recording;
        hasPendingRecordingSettings = true;
        if ( setRecordingSettings )
            setRecordingSettings ( pendingRecordingSettings );

        // Remote User Persistence
        userPersistenceMap.clear();
            if ( obj->hasProperty ( "userSettings" ) )
        {
            auto* userSettingsArr = obj->getProperty ( "userSettings" ).getArray();
            if ( userSettingsArr )
            {
                for ( const auto& v : *userSettingsArr )
                {
                    auto* uobj = v.getDynamicObject();
                    if ( uobj )
                    {
                        juce::String    name = uobj->getProperty ( "name" );
                        UserPersistence p;
                        p.gain                   = (float) uobj->getProperty ( "gain" );
                        p.pan                    = (float) uobj->getProperty ( "pan" );
                        p.isMono                 = (bool) uobj->getProperty ( "isMono" );
                        p.outputRoute            = uobj->hasProperty ( "outputRoute" ) ? static_cast<int> ( uobj->getProperty ( "outputRoute" ) ) : 0;
                        userPersistenceMap[name] = p;
                    }
                }
            }
        }

        updateMixerLayout();
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

        obj->setProperty ( "audioQuality", jamulus_client_get_audio_quality ( jamulusClient ) );
        obj->setProperty ( "audioChannels", jamulus_client_get_audio_channels ( jamulusClient ) );
        obj->setProperty ( "inputBoost", jamulus_client_get_input_boost ( jamulusClient ) );

        obj->setProperty ( "autoJitter", processor ? processor->getAutoJitter() : jamulus_client_get_auto_jitter ( jamulusClient ) );
        obj->setProperty ( "jitterBuffer", processor ? processor->getJitterBuffer() : jamulus_client_get_jitter_buffer ( jamulusClient ) );
        obj->setProperty ( "serverJitterBuffer", processor ? processor->getServerJitterBuffer() : jamulus_client_get_server_jitter_buffer ( jamulusClient ) );
        obj->setProperty ( "newClientLevel", jamulus_client_get_new_client_level ( jamulusClient ) );
        obj->setProperty ( "smallBuffers", processor ? processor->getSmallBuffers() : jamulus_client_get_small_buffers ( jamulusClient ) );
        obj->setProperty ( "mixerRows", mixerRows );
        obj->setProperty ( "mixerSortMode", mixerSortMode );
        obj->setProperty ( "mainVolumeSlider", mainVolumeSlider.getValue() );
        obj->setProperty ( "monitorMode", monitorServerReturn );
        obj->setProperty ( "fxPanelVisible", fxPanelVisible );

        // Feature Toggles
        obj->setProperty ( "autoLevelEnabled", autoLevelEnabled );
        bool lim = limiterButton.getToggleState();
        obj->setProperty ( "limiterEnabled", lim );
        obj->setProperty ( "limiterThresholdDb", limiterThresholdDb );
        obj->setProperty ( "limiterReleaseMs", limiterReleaseMs );
        obj->setProperty ( "testToneEnabled", testToneEnabled );
        if ( getSelectedMidiInputDevice )
            obj->setProperty ( "midiInputDevice", getSelectedMidiInputDevice() );
        if ( getSelectedMidiOutputDevice )
            obj->setProperty ( "midiOutputDevice", getSelectedMidiOutputDevice() );
        obj->setProperty ( "themeName", selectedThemeName.isNotEmpty() ? selectedThemeName : "Default" );
        obj->setProperty ( "chatPanelVisible", chatPanelVisible );
        obj->setProperty ( "chatTextColour", sharedChatColor.toString() );
        obj->setProperty ( "chatAutoTranslate", processor ? processor->isAutoTranslateEnabled() : false );
        obj->setProperty ( "chatTranslateTarget", processor ? processor->getTranslateTargetLang() : juce::String ( "system" ) );
        if ( getRecordingSettings )
        {
            const auto recording = getRecordingSettings();
            obj->setProperty ( "recordingMode", recording.mode );
            obj->setProperty ( "recordingFolder", recording.folderPath );
            obj->setProperty ( "recordStereoUseLiveAutoLevel", recording.stereoUseLiveAutoLevel );
            obj->setProperty ( "recordStereoApplyLimiter", recording.stereoApplyLimiter );
            obj->setProperty ( "recordStemsUseLiveAutoLevel", recording.stemsUseLiveAutoLevel );
        }

        // Remote User Persistence
        juce::Array<juce::var> userSettingsArr;
        for ( auto const& [name, p] : userPersistenceMap )
        {
            juce::DynamicObject::Ptr uobj = new juce::DynamicObject();
            uobj->setProperty ( "name", name );
            uobj->setProperty ( "gain", p.gain );
            uobj->setProperty ( "pan", p.pan );
            uobj->setProperty ( "isMono", p.isMono );
            uobj->setProperty ( "outputRoute", p.outputRoute );
            userSettingsArr.add ( juce::var ( uobj.get() ) );
        }
        obj->setProperty ( "userSettings", userSettingsArr );

        auto file = getSettingsFile();
        file.replaceWithText ( juce::JSON::toString ( juce::var ( obj.get() ) ) );
    }
};
