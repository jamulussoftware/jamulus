#pragma once

#include <juce_core/juce_core.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include "../jamulus_wrapper.h"
#include <atomic>
#include <deque>
#include <set>

#if JUCE_WINDOWS
#include <shellapi.h>
#endif

static bool openChatUrlExternal ( const juce::String& urlText )
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

//==============================================================================
// Rainbow Color Button
//==============================================================================
class RainbowButton : public juce::TextButton
{
public:
    RainbowButton() = default;

    void paintButton ( juce::Graphics& g, bool isMouseOver, bool isButtonDown ) override
    {
        auto r = getLocalBounds().toFloat().reduced ( 1.0f );

        juce::ColourGradient grad ( juce::Colours::red, r.getX(), r.getY(), juce::Colours::blue, r.getRight(), r.getBottom(), false );
        grad.addColour ( 0.25, juce::Colours::yellow );
        grad.addColour ( 0.5, juce::Colours::green );
        grad.addColour ( 0.75, juce::Colours::magenta );

        g.setGradientFill ( grad );
        g.fillRoundedRectangle ( r, 3.0f );

        if ( isMouseOver || isButtonDown )
        {
            g.setColour ( juce::Colours::white.withAlpha ( isButtonDown ? 0.3f : 0.15f ) );
            g.fillRoundedRectangle ( r, 3.0f );
        }
    }
};

//==============================================================================
// Chat Message Structure
//==============================================================================
struct ChatMessage
{
    juce::String sender;
    juce::String text;
    juce::Time   timestamp;
    bool         isSystemMessage = false;
    juce::Colour textColor       = juce::Colours::transparentBlack; // transparent = use default
};

//==============================================================================
// Chat Message Display Component
//==============================================================================
class ChatMessageList : public juce::Component
{
public:
    struct Span
    {
        juce::String text;
        juce::Font   font;
        juce::Colour fg;
        juce::Colour bg;
        juce::String url;
        bool         isHr = false;

        Span ( const juce::String& t, const juce::Font& f, const juce::Colour& c1, const juce::Colour& c2, const juce::String& u ) :
            text ( t ),
            font ( f ),
            fg ( c1 ),
            bg ( c2 ),
            url ( u )
        {}

        static Span Hr()
        {
            Span s ( "", juce::Font(), juce::Colours::transparentBlack, juce::Colours::transparentBlack, "" );
            s.isHr = true;
            return s;
        }
    };

    struct CachedLayout
    {
        struct BgRun
        {
            juce::Rectangle<float> bounds;
            juce::Colour           color;
            juce::String           url;
        };

        std::unique_ptr<juce::TextLayout>   layout;
        std::vector<BgRun>                  bgBounds;
        std::vector<juce::Rectangle<float>> hrLines;
        std::vector<int>                    hrLineIndices; // Which layout line indices have HRs
        int                                 height        = 0;
        bool                                isCentered    = false; // Manual centering flag
        juce::Justification                 justification = juce::Justification::left;
    };

    ChatMessageList()
    {
        setOpaque ( true );
        setMouseCursor ( juce::MouseCursor::NormalCursor );
    }

    void addMessage ( const ChatMessage& msg )
    {
        messages.push_back ( msg );
        rebuildLayouts();
        repaint();

        if ( auto* viewport = findParentComponentOfClass<juce::Viewport>() )
            viewport->setViewPosition ( 0, getHeight() - viewport->getViewHeight() );
    }

    void clearMessages()
    {
        messages.clear();
        layoutCache.clear();
        setSize ( getWidth(), 10 );
        repaint();
    }

    const std::vector<ChatMessage>& getMessages() const { return messages; }

    void setMessages ( const std::vector<ChatMessage>& msgs )
    {
        messages = msgs;
        rebuildLayouts();
        repaint();
    }

    void resized() override { rebuildLayouts(); }

    juce::String exportToPlainText() const
    {
        juce::String result;
        for ( const auto& msg : messages )
        {
            juce::String timeStr = msg.timestamp.toString ( false, true, false );
            result += "[" + timeStr + "] " + ( msg.isSystemMessage ? "* " : ( msg.sender + ": " ) ) + stripHtml ( msg.text ) + "\n";
        }
        return result;
    }

    void paint ( juce::Graphics& g ) override
    {
        g.fillAll ( juce::Colour ( 0xfffcfcfc ) ); // Soft white background

        int       y              = 12;
        const int padding        = 8;
        const int messageSpacing = 6; // Harmonized spacing

        for ( const auto& cached : layoutCache )
        {
            if ( !cached.layout )
                continue;

            for ( const auto& rb : cached.bgBounds )
            {
                juce::Rectangle<float> screenRect = rb.bounds.translated ( (float) padding, (float) y );
                bool                   isHovered  = rb.url.isNotEmpty() && screenRect.contains ( mousePos.toFloat() );

                if ( rb.color.getAlpha() > 0 )
                {
                    g.setColour ( rb.color );
                    g.fillRect ( screenRect );
                }

                if ( isHovered )
                {
                    g.setColour ( juce::Colours::white.withAlpha ( 0.1f ) );
                    g.fillRect ( screenRect );
                    g.setColour ( juce::Colour ( 0xff55aaff ) );
                    g.drawHorizontalLine ( (int) screenRect.getBottom() - 1, screenRect.getX(), screenRect.getRight() );
                }
            }

            // Draw text layout - always use padded area for consistent margins
            // JUCE's justification setting (horizontallyCentred) handles centering within this area
            float xOffset   = (float) padding;
            float drawWidth = (float) getWidth() - padding * 2;
            cached.layout->draw ( g, juce::Rectangle<float> ( xOffset, (float) y, drawWidth, 1000.0f ) );

            // Draw Horizontal Rules (always starting at padding for consistent margins)
            g.setColour ( juce::Colours::darkgrey.withAlpha ( 0.5f ) );
            for ( const auto& hr : cached.hrLines )
                g.fillRect ( hr.translated ( (float) padding, (float) y ) );

            y += cached.height + messageSpacing;
        }
    }

    void mouseMove ( const juce::MouseEvent& e ) override
    {
        mousePos                 = e.position.toInt();
        bool      overUrl        = false;
        int       y              = 5;
        const int padding        = 8;
        const int messageSpacing = 6; // Harmonized spacing

        for ( const auto& cached : layoutCache )
        {
            juce::Rectangle<float> msgBounds ( (float) padding, (float) y, (float) getWidth() - padding * 2, (float) cached.height );
            if ( msgBounds.contains ( e.position ) )
            {
                juce::Point<float> localP = e.position.translated ( -(float) padding, -(float) y );
                for ( const auto& rb : cached.bgBounds )
                {
                    if ( rb.url.isNotEmpty() && rb.bounds.contains ( localP ) )
                    {
                        overUrl = true;
                        break;
                    }
                }
            }
            if ( overUrl )
                break;
            y += cached.height + messageSpacing;
        }

        setMouseCursor ( overUrl ? juce::MouseCursor::PointingHandCursor : juce::MouseCursor::NormalCursor );
        repaint();
    }

    void mouseExit ( const juce::MouseEvent& e ) override
    {
        mousePos = { -1, -1 };
        setMouseCursor ( juce::MouseCursor::NormalCursor );
        repaint();
    }

    void mouseDown ( const juce::MouseEvent& e ) override
    {
        int                y              = 5;
        const int          padding        = 8;
        const int          messageSpacing = 6; // Harmonized spacing
        juce::Point<float> p              = e.position;

        for ( const auto& cached : layoutCache )
        {
            if ( p.y >= y && p.y < y + cached.height )
            {
                juce::Point<float> localP = p.translated ( -(float) padding, -(float) y );
                for ( const auto& rb : cached.bgBounds )
                {
                    if ( rb.url.isNotEmpty() && rb.bounds.contains ( localP ) )
                    {
                        openChatUrlExternal ( rb.url );
                        return;
                    }
                }
            }
            y += cached.height + messageSpacing;
        }
    }

private:
    void rebuildLayouts()
    {
        if ( isRebuilding )
            return;
        juce::ScopedValueSetter<bool> svs ( isRebuilding, true );

        layoutCache.clear();
        int width       = getWidth();
        int totalHeight = 12;
        if ( width <= 20 )
            width = 300;

        const int padding        = 8;
        const int messageSpacing = 6; // Harmonized spacing

        for ( const auto& msg : messages )
        {
            CachedLayout cached;
            cached.layout = std::make_unique<juce::TextLayout>();

            juce::AttributedString attrStr;
            attrStr.setLineSpacing ( 1.0f );
            attrStr.setWordWrap ( juce::AttributedString::WordWrap::byWord );
            attrStr.setJustification ( juce::Justification::left );

            std::vector<Span> spans;
            juce::Font        baseFont ( 13.0f );
            juce::Colour      baseFg = msg.isSystemMessage ? juce::Colour ( 0xff334455 ) : juce::Colour ( 0xff112233 );
            juce::Colour      baseBg = juce::Colours::transparentBlack;

            // If a custom text color is set for this message (e.g. local user), use it for everything
            if ( msg.textColor.getAlpha() > 0 )
            {
                baseFg = msg.textColor;
                // If it was light (intended for dark mode), darken it for visibility on white
                if ( baseFg.getPerceivedBrightness() > 0.8f )
                    baseFg = baseFg.darker ( 0.5f );
            }

            juce::String timeStr = msg.timestamp.toString ( false, true, false );

            // Special handling: if parsing discovers it's a centered/formatted message,
            // we will suppress the '* ' and timestamp to allow clean layout.
            // We'll peek at the message text for alignment tags or <hr>.
            bool hasLayoutTags = msg.text.containsIgnoreCase ( "<center>" ) || msg.text.containsIgnoreCase ( "text-align: center" ) ||
                                 msg.text.containsIgnoreCase ( "text-align:center" ) || msg.text.containsIgnoreCase ( "<hr" ) ||
                                 msg.text.containsIgnoreCase ( "<div" ) || msg.text.containsIgnoreCase ( "<table" ) ||
                                 msg.text.containsIgnoreCase ( "<body" );

            if ( msg.isSystemMessage )
            {
                if ( !hasLayoutTags )
                    spans.push_back ( Span ( "* ", baseFont.withStyle ( juce::Font::italic ).withHeight ( 12.0f ), baseFg, baseBg, "" ) );
            }
            else
            {
                // For regular messages, if we have a custom color, use it for the name too.
                // Otherwise use a professional dark shade for the name.
                juce::Colour nameColor = ( msg.textColor.getAlpha() > 0 ) ? baseFg : juce::Colour ( 0xff445566 );

                spans.push_back ( Span ( "[" + timeStr + "] " + msg.sender + ": ",
                                         baseFont.withStyle ( juce::Font::bold ).withHeight ( 11.0f ),
                                         nameColor,
                                         baseBg,
                                         "" ) );
            }

            parseMessage ( msg.text, baseFont, baseFg, baseBg, spans, cached.justification );

            struct SpanRange
            {
                int start, len, index;
            };
            std::vector<SpanRange> spanRanges;
            bool                   hasHadContent = false; // Track if we've had any actual content

            for ( size_t i = 0; i < spans.size(); ++i )
            {
                if ( spans[i].isHr )
                {
                    // HR adds a special placeholder - use a thin space that won't be visible
                    // but will create a line for us to draw the HR on
                    if ( !attrStr.getText().isEmpty() && !attrStr.getText().endsWith ( "\n" ) )
                        attrStr.append ( "\n", baseFont, baseFg );
                    int pre = attrStr.getText().length();
                    attrStr.append ( " \n", baseFont, baseFg ); // Space + newline ensures the line exists
                    spanRanges.push_back ( { pre, (int) attrStr.getText().length() - pre, (int) i } );
                    hasHadContent = true;
                    continue;
                }

                if ( spans[i].text.isEmpty() )
                    continue;

                // Collapse multiple consecutive newlines into single newlines
                juce::String spanText = spans[i].text;
                while ( spanText.contains ( "\n\n" ) )
                    spanText = spanText.replace ( "\n\n", "\n" );

                // Don't add newlines at the very beginning of the content (before any actual text)
                if ( !hasHadContent && spanText.trim().isEmpty() )
                    continue;

                if ( spanText.isEmpty() )
                    continue;

                int pre = attrStr.getText().length();
                attrStr.append ( spanText, spans[i].font, spans[i].fg );
                spanRanges.push_back ( { pre, (int) attrStr.getText().length() - pre, (int) i } );
                hasHadContent = true;
            }

            // Use the actual justification from parsing for JUCE's TextLayout
            attrStr.setJustification ( cached.justification );
            cached.isCentered = ( cached.justification == juce::Justification::horizontallyCentred );

            // Always use padded width - JUCE's justification handles centering within this area
            float layoutWidth = (float) ( width - padding * 2 );
            cached.layout->createLayout ( attrStr, layoutWidth );

            // Ensure height for the layout is correct and accounts for all lines
            float layoutHeight = 0.0f;
            int   numLines     = cached.layout->getNumLines();
            if ( numLines > 0 )
                layoutHeight = cached.layout->getLine ( numLines - 1 ).getLineBoundsY().getEnd();
            else
                layoutHeight = (float) cached.layout->getHeight();

            cached.height = (int) std::ceil ( layoutHeight ) + 6; // Safety padding at bottom

            // Process HR lines - find which layout lines they fall on
            for ( size_t si = 0; si < spans.size(); ++si )
            {
                if ( spans[si].isHr )
                {
                    // Find the corresponding span range
                    for ( const auto& sr : spanRanges )
                    {
                        if ( sr.index == (int) si )
                        {
                            // Find which layout line this span is on
                            for ( int li = 0; li < cached.layout->getNumLines(); ++li )
                            {
                                auto& line = cached.layout->getLine ( li );
                                for ( auto* run : line.runs )
                                {
                                    if ( !run )
                                        continue;
                                    int rs = run->stringRange.getStart();
                                    int re = run->stringRange.getEnd();
                                    if ( sr.start >= rs && sr.start < re )
                                    {
                                        // This is the line for this HR
                                        float lineY = ( line.getLineBoundsY().getStart() + line.getLineBoundsY().getEnd() ) * 0.5f;
                                        cached.hrLines.push_back ( { 0.0f, lineY - 0.5f, (float) width - padding * 2, 1.0f } );
                                        goto found_hr_line;
                                    }
                                }
                            }
                            // HR not found in any line - put it at the very bottom
                            cached.hrLines.push_back ( { 0.0f, (float) cached.height - 2.0f, (float) width - padding * 2, 1.0f } );
                        found_hr_line:;
                            break;
                        }
                    }
                }
            }

            for ( int i = 0; i < cached.layout->getNumLines(); ++i )
            {
                auto& line = cached.layout->getLine ( i );
                for ( auto* run : line.runs )
                {
                    if ( !run )
                        continue;
                    int rs = run->stringRange.getStart();
                    int re = run->stringRange.getEnd();
                    for ( const auto& sr : spanRanges )
                    {
                        int is = std::max ( rs, sr.start );
                        int ie = std::min ( re, sr.start + sr.len );
                        if ( ie > is )
                        {
                            const auto& s = spans[(size_t) sr.index];
                            // Skip HR spans - they're handled by the dedicated HR loop above
                            if ( s.isHr )
                            {
                                break;
                            }
                            else if ( s.bg.getAlpha() > 0 || s.url.isNotEmpty() )
                            {
                                juce::Range<float> rx = run->getRunBoundsX();
                                juce::Range<float> ly = line.getLineBoundsY();
                                cached.bgBounds.push_back ( { { rx.getStart(), ly.getStart(), rx.getLength(), ly.getLength() }, s.bg, s.url } );
                            }
                            break;
                        }
                    }
                }
            }

            layoutCache.push_back ( std::move ( cached ) );
            totalHeight += cached.height + messageSpacing;
        }
        setSize ( width, juce::jmax ( totalHeight, 10 ) );
    }

    void parseMessage ( juce::String         text,
                        juce::Font           baseFont,
                        juce::Colour         baseFg,
                        juce::Colour         baseBg,
                        std::vector<Span>&   outSpans,
                        juce::Justification& messageJustification )
    {
        // 1. Remove style, script, head, and title blocks before parsing anything else
        text = removeBlocks ( text, "style" );
        text = removeBlocks ( text, "script" );
        text = removeBlocks ( text, "head" );
        text = removeBlocks ( text, "title" );

        // 2. If this message contains structured HTML, strip EVERYTHING before the first structural element
        //    This matches previous client behavior where window title text is ignored
        int firstDiv        = text.indexOfIgnoreCase ( "<div" );
        int firstCenter     = text.indexOfIgnoreCase ( "<center" );
        int firstBody       = text.indexOfIgnoreCase ( "<body" );
        int firstTable      = text.indexOfIgnoreCase ( "<table" );
        int firstStructural = -1;

        if ( firstDiv >= 0 )
            firstStructural = firstDiv;
        if ( firstCenter >= 0 && ( firstStructural < 0 || firstCenter < firstStructural ) )
            firstStructural = firstCenter;
        if ( firstBody >= 0 && ( firstStructural < 0 || firstBody < firstStructural ) )
            firstStructural = firstBody;
        if ( firstTable >= 0 && ( firstStructural < 0 || firstTable < firstStructural ) )
            firstStructural = firstTable;

        // If there's a structural element, strip the prefix entirely (aggressive stripping)
        if ( firstStructural >= 0 )
        {
            text = text.substring ( firstStructural );
        }

        // 3. Check for centering indicators early
        juce::String textLower = text.toLowerCase();
        if ( textLower.contains ( "<center" ) || textLower.contains ( "text-align: center" ) || textLower.contains ( "text-align:center" ) ||
             textLower.contains ( "box" ) || textLower.contains ( "align=\"center\"" ) || textLower.contains ( "align='center'" ) ||
             textLower.contains ( "align=center" ) )
        {
            messageJustification = juce::Justification::horizontallyCentred;
        }

        text = decodeHtml ( text ).trim();
        // Collapse multiple consecutive newlines and spaces
        while ( text.contains ( "\n\n" ) )
            text = text.replace ( "\n\n", "\n" );
        while ( text.contains ( "  " ) )
            text = text.replace ( "  ", " " );
        struct State
        {
            juce::Font   font;
            juce::Colour fg, bg;
            bool         bold;
        };
        std::vector<State> stack;
        stack.push_back ( { baseFont, baseFg, baseBg, false } );

        int pos = 0;
        int len = text.length();

        while ( pos < len )
        {
            int tagStart = text.indexOf ( pos, "<" );
            int textEnd  = ( tagStart >= 0 ) ? tagStart : len;

            if ( textEnd > pos )
                processLinks ( text.substring ( pos, textEnd ), stack.back().font, stack.back().fg, stack.back().bg, outSpans );

            if ( tagStart < 0 )
                break;

            int tagEnd = text.indexOf ( tagStart, ">" );
            if ( tagEnd < 0 )
            {
                processLinks ( text.substring ( tagStart ), stack.back().font, stack.back().fg, stack.back().bg, outSpans );
                break;
            }

            juce::String tag = text.substring ( tagStart, tagEnd + 1 );
            if ( tag.startsWithIgnoreCase ( "<font" ) )
            {
                State        next = stack.back();
                juce::Colour c    = parseColorAttr ( tag, "color" );
                if ( c.getAlpha() > 0 )
                    next.fg = c;
                stack.push_back ( next );
            }
            else if ( tag.startsWithIgnoreCase ( "<span" ) || tag.startsWithIgnoreCase ( "<div" ) || tag.startsWithIgnoreCase ( "<p" ) )
            {
                State        next  = stack.back();
                juce::String style = extractAttr ( tag, "style" );

                // Background color
                int bgPos = style.indexOfIgnoreCase ( "background-color" );
                if ( bgPos >= 0 )
                {
                    int colon = style.indexOf ( bgPos, ":" );
                    if ( colon >= 0 )
                    {
                        int          semi = style.indexOf ( colon, ";" );
                        juce::String val  = ( semi >= 0 ) ? style.substring ( colon + 1, semi ) : style.substring ( colon + 1 );
                        juce::Colour c    = parseColorString ( val.trim() );
                        if ( c.getAlpha() > 0 )
                            next.bg = c;
                    }
                }

                // Font size
                int fsPos = style.indexOfIgnoreCase ( "font-size" );
                if ( fsPos >= 0 )
                {
                    int colon = style.indexOf ( fsPos, ":" );
                    if ( colon >= 0 )
                    {
                        int          semi = style.indexOf ( colon, ";" );
                        juce::String val  = ( semi >= 0 ) ? style.substring ( colon + 1, semi ) : style.substring ( colon + 1 );
                        float        sz   = val.trim().getFloatValue();
                        if ( sz > 4.0f )
                            next.font = next.font.withHeight ( sz * 0.8f ); // Scale slightly for JUCE
                    }
                }

                // Text align (approximate per-message for now)
                if ( style.containsIgnoreCase ( "text-align: center" ) || style.containsIgnoreCase ( "text-align:center" ) )
                    messageJustification = juce::Justification::horizontallyCentred;

                // Handle block level tags - add newline before but only if there's actual content
                if ( !tag.startsWithIgnoreCase ( "<span" ) )
                {
                    // Only add newline if we have actual non-whitespace content before this block
                    if ( !outSpans.empty() )
                    {
                        juce::String lastText = outSpans.back().text.trim();
                        if ( !lastText.isEmpty() )
                            outSpans.push_back ( Span ( "\n", next.font, next.fg, next.bg, "" ) );
                    }
                }

                stack.push_back ( next );
            }
            else if ( tag.startsWithIgnoreCase ( "<h" ) && tag.length() > 2 && juce::CharacterFunctions::isDigit ( tag[2] ) )
            {
                State next = stack.back();
                int   lvl  = tag.substring ( 2, 3 ).getIntValue();
                next.font  = next.font.withStyle ( juce::Font::bold );
                next.font  = next.font.withHeight ( baseFont.getHeight() * ( 1.8f - ( lvl * 0.15f ) ) );

                // Only add newline if we have actual content before the heading
                if ( !outSpans.empty() && !outSpans.back().text.trim().isEmpty() )
                    outSpans.push_back ( Span ( "\n", next.font, next.fg, next.bg, "" ) );

                stack.push_back ( next );
            }
            else if ( tag.equalsIgnoreCase ( "</font>" ) || tag.equalsIgnoreCase ( "</span>" ) || tag.equalsIgnoreCase ( "</div>" ) ||
                      tag.equalsIgnoreCase ( "</p>" ) || ( tag.startsWith ( "</h" ) && tag.endsWith ( ">" ) ) )
            {
                if ( stack.size() > 1 )
                {
                    stack.pop_back();
                    // For block tags, don't add automatic newlines - let the content flow naturally
                    // This prevents double-spacing between elements
                }
            }
            else if ( tag.equalsIgnoreCase ( "<center>" ) )
            {
                messageJustification = juce::Justification::horizontallyCentred;
                stack.push_back ( stack.back() );
            }
            else if ( tag.equalsIgnoreCase ( "</center>" ) )
            {
                if ( stack.size() > 1 )
                    stack.pop_back();
            }
            else if ( tag.equalsIgnoreCase ( "<b>" ) || tag.equalsIgnoreCase ( "<strong>" ) )
            {
                State next = stack.back();
                next.font  = next.font.withStyle ( next.font.getStyleFlags() | juce::Font::bold );
                stack.push_back ( next );
            }
            else if ( tag.equalsIgnoreCase ( "</b>" ) || tag.equalsIgnoreCase ( "</strong>" ) )
            {
                if ( stack.size() > 1 )
                    stack.pop_back();
            }
            else if ( tag.equalsIgnoreCase ( "<hr>" ) || tag.equalsIgnoreCase ( "<hr/>" ) || tag.startsWithIgnoreCase ( "<hr " ) )
            {
                outSpans.push_back ( Span::Hr() );
            }
            else if ( tag.equalsIgnoreCase ( "<br>" ) || tag.equalsIgnoreCase ( "<br/>" ) )
            {
                outSpans.push_back ( Span ( "\n", stack.back().font, stack.back().fg, stack.back().bg, "" ) );
            }

            pos = tagEnd + 1;
        }
    }

    void processLinks ( const juce::String& text, juce::Font font, juce::Colour fg, juce::Colour bg, std::vector<Span>& outSpans )
    {
        auto isTrailingUrlPunctuation = [] ( juce::juce_wchar ch ) {
            switch ( ch )
            {
                case '!':
                case '"':
                case '\'':
                case '(':
                case ')':
                case '+':
                case ',':
                case '.':
                case ':':
                case ';':
                case '<':
                case '=':
                case '>':
                case '?':
                case '[':
                case ']':
                case '{':
                case '}':
                    return true;
                default:
                    return false;
            }
        };

        int pos = 0;
        while ( pos < text.length() )
        {
            int linkStart = text.indexOfIgnoreCase ( pos, "http" );
            if ( linkStart < 0 )
            {
                outSpans.push_back ( Span ( text.substring ( pos ), font, fg, bg, "" ) );
                break;
            }

            if ( linkStart > pos )
                outSpans.push_back ( Span ( text.substring ( pos, linkStart ), font, fg, bg, "" ) );

            int end = text.indexOfAnyOf ( " \n\r\t", linkStart );
            if ( end < 0 )
                end = text.length();

            juce::String url = text.substring ( linkStart, end );
            if ( url.startsWithIgnoreCase ( "http://" ) || url.startsWithIgnoreCase ( "https://" ) )
            {
                int trimmedLength = url.length();
                while ( trimmedLength > 0 && isTrailingUrlPunctuation ( url[trimmedLength - 1] ) )
                    --trimmedLength;

                const juce::String cleanUrl    = url.substring ( 0, trimmedLength );
                const juce::String trailingPunc = url.substring ( trimmedLength );

                outSpans.push_back (
                    Span ( cleanUrl,
                           font.withStyle ( font.getStyleFlags() | juce::Font::underlined ),
                           juce::Colour ( 0xff55aaff ),
                           bg,
                           cleanUrl ) );
                if ( trailingPunc.isNotEmpty() )
                    outSpans.push_back ( Span ( trailingPunc, font, fg, bg, "" ) );
                pos = end;
            }
            else
            {
                outSpans.push_back ( Span ( text.substring ( linkStart, linkStart + 4 ), font, fg, bg, "" ) );
                pos = linkStart + 4;
            }
        }
    }

    juce::Colour parseColorAttr ( const juce::String& tag, const juce::String& attr )
    {
        juce::String val = extractAttr ( tag, attr );
        return val.isEmpty() ? juce::Colours::transparentBlack : parseColorString ( val );
    }

    juce::Colour parseColorString ( juce::String val )
    {
        val = val.trim().toLowerCase();
        if ( val.isEmpty() )
            return juce::Colours::transparentBlack;

        static const std::map<juce::String, juce::uint32> colorNames = {
            { "black", 0xff000000 },     { "white", 0xffffffff },      { "red", 0xffff0000 },      { "green", 0xff008000 },
            { "blue", 0xff0000ff },      { "yellow", 0xffffff00 },     { "cyan", 0xff00ffff },     { "magenta", 0xffff00ff },
            { "gray", 0xff808080 },      { "grey", 0xff808080 },       { "silver", 0xffc0c0c0 },   { "maroon", 0xff800000 },
            { "olive", 0xff808000 },     { "purple", 0xff800080 },     { "teal", 0xff008080 },     { "navy", 0xff000080 },
            { "orange", 0xffffa500 },    { "pink", 0xffffc0cb },       { "gold", 0xffffd700 },     { "violet", 0xffee82ee },
            { "brown", 0xffa52a2a },     { "mediumblue", 0xff0000cd }, { "darkred", 0xff8b0000 },  { "darkblue", 0xff00008b },
            { "darkgreen", 0xff006400 }, { "darkgray", 0xffa9a9a9 },   { "lightblue", 0xffadd8e6 } };

        if ( colorNames.count ( val ) )
            return juce::Colour ( colorNames.at ( val ) );

        if ( val.startsWith ( "#" ) )
        {
            juce::String hex = val.substring ( 1 );
            if ( hex.length() == 3 )
            {
                juce::String r = hex.substring ( 0, 1 ), g = hex.substring ( 1, 2 ), b = hex.substring ( 2, 3 );
                return juce::Colour::fromString ( "ff" + r + r + g + g + b + b );
            }
            if ( hex.length() == 6 )
                return juce::Colour::fromString ( "ff" + hex );
            if ( hex.length() == 8 )
                return juce::Colour::fromString ( hex );
        }

        juce::Colour c = juce::Colour::fromString ( val );
        return c.getAlpha() > 0 ? c : juce::Colours::transparentBlack;
    }

    juce::String extractAttr ( const juce::String& tag, const juce::String& attr )
    {
        int p = tag.indexOfIgnoreCase ( attr + "=" );
        if ( p < 0 )
            return "";
        p += attr.length() + 1;
        if ( p >= tag.length() )
            return "";
        juce::juce_wchar q = tag[p];
        if ( q == '\"' || q == '\'' )
        {
            int e = tag.indexOf ( ++p, juce::String::charToString ( q ) );
            return ( e > p ) ? tag.substring ( p, e ).trim() : "";
        }
        int e = tag.indexOfAnyOf ( " >", p );
        return ( e > p ) ? tag.substring ( p, e ).trim() : tag.substring ( p ).trim().replace ( ">", "" ).trim();
    }

    static juce::String stripHtml ( const juce::String& html )
    {
        juce::String r = html.replace ( "<br>", "\n", true ).replace ( "<br/>", "\n", true );
        r              = r.replace ( "&lt;", "<" ).replace ( "&gt;", ">" ).replace ( "&amp;", "&" ).replace ( "&nbsp;", " " );
        int s          = 0;
        while ( ( s = r.indexOf ( "<" ) ) >= 0 )
        {
            int e = r.indexOf ( s, ">" );
            if ( e > s )
                r = r.substring ( 0, s ) + r.substring ( e + 1 );
            else
                break;
        }
        return r.trim();
    }

    static juce::String decodeHtml ( juce::String t )
    {
        for ( int i = 0; i < 4; ++i )
        {
            juce::String p = t;
            t              = t.replace ( "&lt;", "<", true )
                    .replace ( "&gt;", ">", true )
                    .replace ( "&amp;", "&", true )
                    .replace ( "&quot;", "\"", true )
                    .replace ( "&apos;", "'", true )
                    .replace ( "&nbsp;", " ", true )
                    .replace ( "&#60;", "<" )
                    .replace ( "&#62;", ">" )
                    .replace ( "&#39;", "'" )
                    .replace ( "&#34;", "\"" );
            if ( t == p )
                break;
        }

        // Decode numeric character references (decimal and hex), e.g., &#128512; or &#x1F600;
        for ( ;; )
        {
            int amp = t.indexOf ( "&#" );
            if ( amp < 0 )
                break;
            int semi = t.indexOf ( amp, ";" );
            if ( semi < 0 )
                break;
            juce::String code = t.substring ( amp + 2, semi );
            juce::juce_wchar ch = 0;
            bool ok              = false;
            if ( code.startsWithIgnoreCase ( "x" ) )
            {
                auto hex = code.substring ( 1 );
                juce::uint32 v = hex.getHexValue32();
                ok              = true;
                ch              = (juce::juce_wchar) v;
            }
            else
            {
                int v = code.getIntValue();
                if ( v > 0 )
                {
                    ok = true;
                    ch = (juce::juce_wchar) v;
                }
            }
            if ( ok && ch != 0 )
            {
                juce::String repl ( juce::String::charToString ( ch ) );
                t = t.substring ( 0, amp ) + repl + t.substring ( semi + 1 );
            }
            else
            {
                // If malformed, skip this occurrence to avoid infinite loop
                t = t.substring ( 0, amp ) + t.substring ( amp + 2 );
            }
        }

        return t;
    }

    static juce::String removeBlocks ( juce::String t, const juce::String& tag )
    {
        juce::String startTag = "<" + tag;
        juce::String endTag   = "</" + tag + ">";
        int          pos      = 0;
        while ( ( pos = t.indexOfIgnoreCase ( pos, startTag ) ) >= 0 )
        {
            int blockEnd = t.indexOf ( pos, ">" ); // End of start tag <style...>
            if ( blockEnd < 0 )
                break;
            int end = t.indexOfIgnoreCase ( blockEnd, endTag );
            if ( end >= 0 )
            {
                t   = t.substring ( 0, pos ) + t.substring ( end + endTag.length() );
                pos = pos; // check again at same position
            }
            else
                break;
        }
        return t;
    }

    std::vector<ChatMessage>  messages;
    std::vector<CachedLayout> layoutCache;
    juce::Point<int>          mousePos{ -1, -1 };
    bool                      isRebuilding = false;
};

//==============================================================================
// Chat Component
//==============================================================================
static juce::String getSystemTranslationLanguageCode()
{
    juce::String language = juce::SystemStats::getDisplayLanguage();
    if ( language.isEmpty() )
        language = juce::SystemStats::getUserLanguage();

    language = language.trim().replaceCharacter ( '_', '-' ).toLowerCase();

    if ( language.startsWith ( "zh-hant" ) || language.startsWith ( "zh-tw" ) || language.startsWith ( "zh-hk" ) )
        return "zh-Hant";
    if ( language.startsWith ( "zh" ) )
        return "zh-Hans";
    if ( language.startsWith ( "pt-br" ) )
        return "pt-BR";
    if ( language.startsWith ( "no" ) || language.startsWith ( "nb" ) )
        return "nb";

    const int dash = language.indexOfChar ( '-' );
    if ( dash > 0 )
        language = language.substring ( 0, dash );

    return language.isNotEmpty() ? language : "en";
}

static juce::String resolveTranslateTargetLanguageCode ( const juce::String& preferredCode )
{
    juce::String normalised = preferredCode.trim().replaceCharacter ( '_', '-' ).toLowerCase();
    if ( normalised.isEmpty() || normalised == "system" )
        return getSystemTranslationLanguageCode();
    if ( normalised == "zh-cn" || normalised == "zh-hans" )
        return "zh-Hans";
    if ( normalised == "zh-tw" || normalised == "zh-hk" || normalised == "zh-hant" )
        return "zh-Hant";
    if ( normalised == "pt-br" )
        return "pt-BR";
    if ( normalised == "no" || normalised == "nb" )
        return "nb";

    const int dash = normalised.indexOfChar ( '-' );
    if ( dash > 0 )
        normalised = normalised.substring ( 0, dash );

    return normalised.isNotEmpty() ? normalised : "en";
}

static bool detectedLanguageMatchesTarget ( const juce::var& detected, const juce::String& targetCode )
{
    auto sameAsTarget = [&targetCode] ( const juce::String& languageCode ) {
        return resolveTranslateTargetLanguageCode ( languageCode ) == targetCode;
    };

    if ( auto* detectedObject = detected.getDynamicObject() )
        return sameAsTarget ( detectedObject->getProperty ( "language" ).toString() );

    if ( auto* detectedArray = detected.getArray(); detectedArray != nullptr && !detectedArray->isEmpty() )
    {
        if ( auto* firstObject = detectedArray->getReference ( 0 ).getDynamicObject() )
            return sameAsTarget ( firstObject->getProperty ( "language" ).toString() );
    }

    return false;
}

class ChatComponent : public juce::Component, private juce::Timer
{
public:
    static constexpr const char* SIG_PREFIX = "[[VST]]";

    struct TranslationRequest
    {
        juce::String sender;
        juce::String text;
        bool         isSystemMessage = false;
        juce::uint64 configRevision  = 0;
    };

    class AsyncChatTranslationWorker : private juce::Thread
    {
    public:
        explicit AsyncChatTranslationWorker ( ChatComponent& ownerComponent ) :
            juce::Thread ( "JamulusChatTranslation" ),
            owner ( ownerComponent )
        {
            startThread();
        }

        ~AsyncChatTranslationWorker() override
        {
            stop();
        }

        void enqueue ( TranslationRequest request )
        {
            {
                const juce::ScopedLock lock ( queueLock );
                queue.push_back ( std::move ( request ) );
            }
            workAvailable.signal();
        }

        void stop()
        {
            signalThreadShouldExit();
            workAvailable.signal();
            stopThread ( 6000 );

            const juce::ScopedLock lock ( queueLock );
            queue.clear();
        }

    private:
        void run() override
        {
            while ( !threadShouldExit() )
            {
                TranslationRequest request;
                bool               haveRequest = false;

                {
                    const juce::ScopedLock lock ( queueLock );
                    if ( !queue.empty() )
                    {
                        request = std::move ( queue.front() );
                        queue.pop_front();
                        haveRequest = true;
                    }
                }

                if ( !haveRequest )
                {
                    workAvailable.wait ( 200 );
                    continue;
                }

                juce::String error;
                const juce::String translatedText = owner.translateTextForTarget ( request.text, owner.getTranslateTargetLanguageCode(), error );
                if ( threadShouldExit() )
                    break;

                auto safeOwner = juce::Component::SafePointer<ChatComponent> ( &owner );
                juce::MessageManager::callAsync ( [safeOwner, request, translatedText, error]() {
                    if ( safeOwner == nullptr )
                        return;

                    if ( !error.isEmpty() )
                    {
                        safeOwner->noteTranslationFailure ( error );
                        return;
                    }

                    safeOwner->clearTranslationFailureState();
                    safeOwner->applyTranslatedMessage ( request, translatedText );
                } );
            }
        }

        ChatComponent&         owner;
        juce::CriticalSection  queueLock;
        std::deque<TranslationRequest> queue;
        juce::WaitableEvent    workAvailable;
    };

    jamulus_client_t jamulusClient;
    bool             isPanelMode;

    juce::Label      titleLabel;
    juce::TextButton popOutButton;
    juce::TextButton popInButton;
    juce::Viewport   messageViewport;
    ChatMessageList  messageList;
    juce::TextEditor inputEdit;
    juce::TextButton sendButton;
    RainbowButton    textColorButton;
    juce::TextButton autoTranslateButton;
    juce::Colour     currentTextColor = juce::Colours::white;

    juce::TextButton clearButton;
    juce::TextButton saveButton;
    juce::TextButton closeButton;

    std::function<void()>                                            onClose;
    std::function<void()>                                            onPopOut;
    std::function<void()>                                            onPopIn;
    std::function<void ( const juce::String& )>                      onMessageSent;
    std::function<void ( const juce::String& )>                      onSendRaw;
    std::function<void ( const juce::String&, const juce::String& )> onSignalingMessageReceived;
    std::function<juce::String()>                                   chatMessagePoller;
    std::function<bool()>                                            isAutoTranslateEnabled;
    std::function<juce::String()>                                   getTranslateTargetLanguage;
    std::function<void ( bool )>                                     onAutoTranslateToggled;

    ChatComponent ( jamulus_client_t client, bool panelMode = false ) : jamulusClient ( client ), isPanelMode ( panelMode )
    {
        addAndMakeVisible ( textColorButton );
        textColorButton.setButtonText ( "" );
        textColorButton.setTooltip ( "Choose Text Color" );
        textColorButton.setWantsKeyboardFocus ( false );
        textColorButton.setMouseClickGrabsKeyboardFocus ( false );
        textColorButton.onClick = [this]() { showColorPicker ( false ); };

        addAndMakeVisible ( autoTranslateButton );
        autoTranslateButton.setButtonText ( "AT" );
        autoTranslateButton.setClickingTogglesState ( true );
        autoTranslateButton.setTooltip ( "Toggle auto translate for incoming chat" );
        autoTranslateButton.onClick = [this]() {
            translationConfigRevision.fetch_add ( 1, std::memory_order_relaxed );
            updateAutoTranslateButtonColor();
            if ( onAutoTranslateToggled )
                onAutoTranslateToggled ( autoTranslateButton.getToggleState() );
        };
        updateAutoTranslateButtonColor();

        addAndMakeVisible ( titleLabel );
        titleLabel.setText ( "Chat", juce::dontSendNotification );
        titleLabel.setFont ( juce::Font ( isPanelMode ? 16.0f : 20.0f, juce::Font::bold ) );
        titleLabel.setColour ( juce::Label::textColourId, juce::Colours::white );
        titleLabel.setJustificationType ( juce::Justification::centredLeft );

        // Pop-out button (only in panel mode)
        addAndMakeVisible ( popOutButton );
        popOutButton.setButtonText ( juce::CharPointer_UTF8 ( "\xe2\x86\x97" ) ); // ↗ arrow
        popOutButton.setTooltip ( "Pop out to separate window" );
        popOutButton.onClick = [this]() {
            if ( onPopOut )
                onPopOut();
        };
        popOutButton.setVisible ( isPanelMode );

        // Clear button (Icon in header, "Clear" in footer)
        addAndMakeVisible ( clearButton );
        clearButton.setTooltip ( "Clear chat history" );
        clearButton.onClick = [this]() { messageList.clearMessages(); };

        // Save button (Icon in header, "Save" in footer)
        addAndMakeVisible ( saveButton );
        saveButton.setTooltip ( "Save chat to file" );
        saveButton.onClick = [this]() { saveChat(); };
        if ( isPanelMode )
        {
            clearButton.setButtonText ( "C" ); // 'C' for Clear
            saveButton.setButtonText ( "S" );  // 'S' for Save
        }
        else
        {
            clearButton.setButtonText ( "Clear" );
            saveButton.setButtonText ( "Save" );
        }

        clearButton.setVisible ( true );
        saveButton.setVisible ( true );

        // Pop-in button (only in popup window mode)
        addAndMakeVisible ( popInButton );
        popInButton.setButtonText ( juce::CharPointer_UTF8 ( "\xe2\x86\x99" ) ); // ↙ arrow
        popInButton.setTooltip ( "Pop back into main window" );
        popInButton.onClick = [this]() {
            if ( onPopIn )
                onPopIn();
        };
        popInButton.setVisible ( !isPanelMode );

        // Close/collapse button
        addAndMakeVisible ( closeButton );
        closeButton.setButtonText ( isPanelMode ? "X" : "Close" );
        closeButton.onClick = [this]() {
            if ( onClose )
                onClose();
        };

        // Message viewport
        addAndMakeVisible ( messageViewport );
        messageViewport.setViewedComponent ( &messageList, false );
        messageViewport.setScrollBarsShown ( true, false );

        // Input field
        addAndMakeVisible ( inputEdit );
        inputEdit.setMultiLine ( false );
        inputEdit.setReturnKeyStartsNewLine ( false );
        inputEdit.setTextToShowWhenEmpty ( "Type a message...", juce::Colours::darkgrey );
        inputEdit.onReturnKey = [this]() { sendMessage(); };
        inputEdit.setJustification ( juce::Justification::centredLeft );
        inputEdit.setIndents ( 4, 1 ); // Slight top indent to center vertically
        inputEdit.setWantsKeyboardFocus ( true );
        inputEdit.setMouseClickGrabsKeyboardFocus ( true );
        inputEdit.onTextChange = [this]() {
            // Apply current text color to the input field itself for visual feedback
            inputEdit.setColour ( juce::TextEditor::textColourId, currentTextColor );
        };

        setWantsKeyboardFocus ( true );
        // Removed setMouseClickGrabsKeyboardFocus(true) from parent to avoid stealing focus from inputEdit

        // Send button
        addAndMakeVisible ( sendButton );
        sendButton.setButtonText ( "Send" );
        sendButton.onClick = [this]() { sendMessage(); };

        // (Buttons configured above)

        // Only add welcome message if not restoring from shared storage
        if ( !isPanelMode ) // Will be populated by setMessages() in panel mode
        {
            ChatMessage welcome;
            welcome.text            = "Welcome to Jamulus Chat. Be respectful to other musicians!";
            welcome.isSystemMessage = true;
            welcome.timestamp       = juce::Time::getCurrentTime();
            messageList.addMessage ( welcome );
        }

        textColorButton.setAlwaysOnTop ( true );

        startTimerHz ( 5 );
    }

    void setChatMessagePoller ( std::function<juce::String()> fn )
    {
        chatMessagePoller = std::move ( fn );
    }

    void setTranslationConfigGetters ( std::function<bool()> autoTranslateGetter, std::function<juce::String()> targetLanguageGetter )
    {
        isAutoTranslateEnabled  = std::move ( autoTranslateGetter );
        getTranslateTargetLanguage = std::move ( targetLanguageGetter );
        translationConfigRevision.fetch_add ( 1, std::memory_order_relaxed );
        autoTranslateButton.setToggleState ( shouldAutoTranslate(), juce::dontSendNotification );
        updateAutoTranslateButtonColor();
    }

    std::vector<ChatMessage> getMessages() const { return messageList.getMessages(); }
    void                     setMessages ( const std::vector<ChatMessage>& messages ) { messageList.setMessages ( messages ); }

    juce::Colour getTextColor() const { return currentTextColor; }
    void         setTextColor ( juce::Colour c )
    {
        currentTextColor = c;
        inputEdit.setColour ( juce::TextEditor::textColourId, currentTextColor );
    }

    void mouseDown ( const juce::MouseEvent& e ) override
    {
        // Re-focus the input field if the user clicks anywhere in the chat area
        inputEdit.grabKeyboardFocus();
    }

    void focusGained ( juce::Component::FocusChangeType ) override
    {
        // Forward focus to the input field
        inputEdit.grabKeyboardFocus();
    }

    ~ChatComponent() override
    {
        stopTimer();
        translationWorker.stop();
    }

    juce::String getLocalClientName() const
    {
        if ( !jamulusClient )
            return {};

        if ( const char* name = jamulus_client_get_name ( jamulusClient ) )
            return juce::String::fromUTF8 ( name ).trim();

        return {};
    }

    bool shouldAutoTranslate() const
    {
        return isAutoTranslateEnabled ? isAutoTranslateEnabled() : false;
    }

    juce::String getTranslateTargetLanguageCode() const
    {
        return resolveTranslateTargetLanguageCode ( getTranslateTargetLanguage ? getTranslateTargetLanguage() : juce::String ( "system" ) );
    }

    static bool tryTranslateWithFedilab ( const juce::String& text, const juce::String& targetCode, juce::String& translatedText, juce::String& error )
    {
        juce::URL requestUrl ( "https://translate.fedilab.app/translate" );
        requestUrl = requestUrl.withParameter ( "q", text )
                               .withParameter ( "source", "auto" )
                               .withParameter ( "target", targetCode )
                               .withParameter ( "format", "text" );

        int httpStatusCode = 0;
        auto responseStream = requestUrl.createInputStream (
            juce::URL::InputStreamOptions ( juce::URL::ParameterHandling::inPostData )
                .withConnectionTimeoutMs ( 5000 )
                .withNumRedirectsToFollow ( 2 )
                .withStatusCode ( &httpStatusCode )
                .withExtraHeaders ( "User-Agent: JamulusPlus/1.0\r\nAccept: application/json\r\nContent-Type: application/x-www-form-urlencoded\r\n" )
                .withHttpRequestCmd ( "POST" ) );

        if ( responseStream == nullptr )
        {
            error = "primary translator could not be reached";
            return false;
        }
        if ( httpStatusCode != 0 && httpStatusCode != 200 )
        {
            error = "primary translator returned HTTP " + juce::String ( httpStatusCode );
            return false;
        }

        const juce::String responseText = responseStream->readEntireStreamAsString();
        if ( responseText.isEmpty() )
        {
            error = "primary translator returned an empty response";
            return false;
        }

        const juce::var parsed = juce::JSON::parse ( responseText );
        auto* root = parsed.getDynamicObject();
        if ( root == nullptr )
        {
            error = "primary translator returned invalid JSON";
            return false;
        }

        if ( root->hasProperty ( "error" ) )
        {
            error = root->getProperty ( "error" ).toString().isNotEmpty() ? root->getProperty ( "error" ).toString()
                                                                             : juce::String ( "primary translator reported an error" );
            return false;
        }

        if ( auto detected = root->getProperty ( "detectedLanguage" ); !detected.isVoid() && detectedLanguageMatchesTarget ( detected, targetCode ) )
        {
            translatedText = text;
            return true;
        }

        translatedText = root->getProperty ( "translatedText" ).toString();
        if ( translatedText.isEmpty() )
        {
            error = "primary translator did not return translated text";
            return false;
        }

        return true;
    }

    static bool tryTranslateWithGoogleFallback ( const juce::String& text, const juce::String& targetCode, juce::String& translatedText, juce::String& error )
    {
        juce::URL requestUrl ( "https://translate.googleapis.com/translate_a/single" );
        requestUrl = requestUrl.withParameter ( "client", "gtx" )
                               .withParameter ( "sl", "auto" )
                               .withParameter ( "tl", targetCode )
                               .withParameter ( "dt", "t" )
                               .withParameter ( "q", text );

        int httpStatusCode = 0;
        auto responseStream = requestUrl.createInputStream (
            juce::URL::InputStreamOptions ( juce::URL::ParameterHandling::inAddress )
                .withConnectionTimeoutMs ( 4000 )
                .withNumRedirectsToFollow ( 2 )
                .withStatusCode ( &httpStatusCode )
                .withExtraHeaders ( "User-Agent: JamulusPlus/1.0\r\nAccept: application/json\r\n" )
                .withHttpRequestCmd ( "GET" ) );

        if ( responseStream == nullptr )
        {
            error = "fallback translator could not be reached";
            return false;
        }
        if ( httpStatusCode != 0 && httpStatusCode != 200 )
        {
            error = "fallback translator returned HTTP " + juce::String ( httpStatusCode );
            return false;
        }

        const juce::String responseText = responseStream->readEntireStreamAsString();
        if ( responseText.isEmpty() )
        {
            error = "fallback translator returned an empty response";
            return false;
        }

        const juce::var parsed = juce::JSON::parse ( responseText );
        auto* rootArray = parsed.getArray();
        if ( rootArray == nullptr || rootArray->isEmpty() )
        {
            error = "fallback translator returned invalid JSON";
            return false;
        }

        if ( rootArray->size() > 2 && resolveTranslateTargetLanguageCode ( rootArray->getReference ( 2 ).toString() ) == targetCode )
        {
            translatedText = text;
            return true;
        }

        auto* segments = rootArray->getReference ( 0 ).getArray();
        if ( segments == nullptr || segments->isEmpty() )
        {
            error = "fallback translator did not return translated segments";
            return false;
        }

        juce::String combined;
        for ( const auto& segmentVar : *segments )
        {
            if ( auto* segment = segmentVar.getArray(); segment != nullptr && !segment->isEmpty() )
                combined << segment->getReference ( 0 ).toString();
        }

        if ( combined.isEmpty() )
        {
            error = "fallback translator returned empty translated text";
            return false;
        }

        translatedText = combined;
        return true;
    }

    juce::String translateTextForTarget ( const juce::String& text, const juce::String& targetCode, juce::String& error ) const
    {
        juce::String translated;
        if ( text.trim().isEmpty() )
            return {};

        if ( tryTranslateWithFedilab ( text, targetCode, translated, error ) )
            return translated;

        juce::String fallbackError;
        if ( tryTranslateWithGoogleFallback ( text, targetCode, translated, fallbackError ) )
            return translated;

        if ( error.isEmpty() )
            error = fallbackError;
        else if ( fallbackError.isNotEmpty() )
            error << "; " << fallbackError;

        return {};
    }

    void addChatMessage ( const juce::String& sender, const juce::String& text )
    {
        ChatMessage msg;
        msg.sender    = sender;
        msg.timestamp = juce::Time::getCurrentTime();

        // Identify if this is the local user to apply local-only text color
        juce::String localName = getLocalClientName();
        if ( sender.trim().equalsIgnoreCase ( localName ) )
        {
            // Set message-wide color for local user
            msg.textColor = currentTextColor;
            msg.text      = text;
        }
        else
        {
            msg.text = text;
        }

        msg.isSystemMessage = false;
        messageList.addMessage ( msg );
    }

    void addSystemMessage ( const juce::String& text, juce::Colour color = juce::Colours::transparentBlack )
    {
        ChatMessage msg;
        msg.text            = text; // Keep HTML
        msg.timestamp       = juce::Time::getCurrentTime();
        msg.isSystemMessage = true;
        msg.textColor       = color;
        messageList.addMessage ( msg );
    }

    void maybeTranslateIncomingMessage ( const juce::String& sender, const juce::String& text, bool isSystemMessage, bool isLocalMessage )
    {
        if ( !shouldAutoTranslate() || isLocalMessage )
            return;

        const juce::String plainText = parseHtmlToPlainText ( text ).trim();
        if ( plainText.isEmpty() )
            return;

        TranslationRequest request;
        request.sender          = sender;
        request.text            = plainText;
        request.isSystemMessage = isSystemMessage;
        request.configRevision  = translationConfigRevision.load ( std::memory_order_relaxed );
        translationWorker.enqueue ( std::move ( request ) );
    }

    void applyTranslatedMessage ( const TranslationRequest& request, const juce::String& translatedText )
    {
        if ( request.configRevision != translationConfigRevision.load ( std::memory_order_relaxed ) )
            return;

        const juce::String cleaned = translatedText.trim();
        if ( cleaned.isEmpty() || cleaned == request.text.trim() )
            return;

        const juce::Colour translatedColor ( 0xff6ea6d8 );
        if ( request.isSystemMessage )
            addSystemMessage ( "Translated: " + cleaned, translatedColor );
        else
            addSystemMessage ( "Translated " + request.sender + ": " + cleaned, translatedColor );
    }

    void noteTranslationFailure ( const juce::String& error )
    {
        const double nowMs = juce::Time::getMillisecondCounterHiRes();
        const bool shouldNotify = !translationFailureActive || error != lastTranslationFailureReason || ( nowMs - lastTranslationFailureNoticeMs ) >= 30000.0;

        translationFailureActive       = true;
        lastTranslationFailureReason   = error;
        lastTranslationFailureNoticeMs = nowMs;

        if ( shouldNotify )
            addSystemMessage ( "Auto Translate failed; incoming chat will stay in the original language until the translator responds again.",
                               juce::Colour ( 0xffd08a3c ) );
    }

    void clearTranslationFailureState()
    {
        translationFailureActive = false;
        lastTranslationFailureReason.clear();
    }

    void paint ( juce::Graphics& g ) override
    {
        // Dark grey container background
        g.fillAll ( juce::Colour ( 0xff323232 ) );

        // Header
        g.setColour ( juce::Colour ( 0xff252525 ) );
        g.fillRect ( 0, 0, getWidth(), isPanelMode ? 30 : 40 );

        // Footer background (input area)
        g.setColour ( juce::Colour ( 0xff3a3a3a ) );
        g.fillRect ( 0, getHeight() - 40, getWidth(), 40 );
    }

    void resized() override
    {
        auto bounds = getLocalBounds();

        // Header
        auto header = bounds.removeFromTop ( isPanelMode ? 0 : 40 );
        if ( isPanelMode )
        {
            // Hide header widgets entirely in panel mode
            closeButton.setVisible ( false );
            saveButton.setVisible ( false );
            clearButton.setVisible ( false );
            popOutButton.setVisible ( false );
            titleLabel.setVisible ( false );
        }
        else
        {
            // In popup window mode, show pop-in button in header
            popInButton.setBounds ( header.removeFromRight ( 40 ).reduced ( 5 ) );
            titleLabel.setBounds ( header );
            closeButton.setVisible ( true );
            saveButton.setVisible ( true );
            clearButton.setVisible ( true );
            popOutButton.setVisible ( true );
            titleLabel.setVisible ( true );
        }

        // Footer (only in popup mode)
        if ( !isPanelMode )
        {
            auto footer     = bounds.removeFromBottom ( 50 );
            auto buttonArea = footer.reduced ( 10, 10 );
            closeButton.setBounds ( buttonArea.removeFromRight ( 70 ) );
            buttonArea.removeFromRight ( 5 );
            saveButton.setBounds ( buttonArea.removeFromRight ( 60 ) );
            buttonArea.removeFromRight ( 5 );
            clearButton.setBounds ( buttonArea.removeFromRight ( 60 ) );
        }

        // Input area
        auto inputArea = bounds.removeFromBottom ( isPanelMode ? 35 : 40 ).reduced ( isPanelMode ? 5 : 10, 5 );
        sendButton.setBounds ( inputArea.removeFromRight ( isPanelMode ? 50 : 80 ) );
        inputArea.removeFromRight ( 5 );

        // Color buttons
        textColorButton.setBounds ( inputArea.removeFromLeft ( 24 ) );
        inputArea.removeFromLeft ( 5 );
        autoTranslateButton.setBounds ( inputArea.removeFromLeft ( isPanelMode ? 34 : 40 ) );
        inputArea.removeFromLeft ( 5 );

        inputEdit.setBounds ( inputArea );

        // Message area
        int margin = isPanelMode ? 5 : 10;
        messageViewport.setBounds ( bounds.reduced ( margin, margin / 2 ) );
        messageList.setSize ( messageViewport.getWidth() - messageViewport.getScrollBarThickness(), messageList.getHeight() );
    }

private:
    void timerCallback() override
    {
        if ( autoTranslateButton.getToggleState() != shouldAutoTranslate() )
        {
            autoTranslateButton.setToggleState ( shouldAutoTranslate(), juce::dontSendNotification );
            updateAutoTranslateButtonColor();
        }

        while ( true )
        {
            juce::String fullMsg;
            if ( chatMessagePoller )
            {
                fullMsg = chatMessagePoller();
                if ( fullMsg.isEmpty() )
                    break;
            }
            else
            {
                if ( !jamulusClient )
                    return;
                const char* chatMsg = jamulus_client_get_chat_message ( jamulusClient );
                if ( !chatMsg || chatMsg[0] == '\0' )
                    break;
                fullMsg = juce::String::fromUTF8 ( chatMsg );
            }
            bool         shouldScroll = ( messageViewport.getViewPositionY() >= messageList.getHeight() - messageViewport.getHeight() - 10 );

            // Jamulus server often sends messages starting with "*" for system or status.
            if ( fullMsg.startsWith ( "*" ) )
            {
                juce::String text      = fullMsg.substring ( 1 ).trim();
                juce::String localName = getLocalClientName();
                const bool   isLocalMessage = localName.isNotEmpty() && text.containsIgnoreCase ( localName );
                if ( localName.isNotEmpty() && text.containsIgnoreCase ( localName ) )
                {
                    text = "<font color=\"" + currentTextColor.toDisplayString ( false ) + "\">" + text + "</font>";
                }
                addSystemMessage ( text );
                maybeTranslateIncomingMessage ( {}, text, true, isLocalMessage );
            }
            else
            {
                // Better parsing for server messages.
                // If it contains <font or <b> tags, it's likely a server-formatted message (Time + Name + Message).
                // We shouldn't try to split it with ": " because that colon often exists inside the timestamp.
                if ( fullMsg.containsIgnoreCase ( "<font" ) || fullMsg.containsIgnoreCase ( "<b>" ) )
                {
                    // Treat as a system message to preserve server HTML formatting
                    juce::String localName = getLocalClientName();
                    const bool   isLocalMessage = localName.isNotEmpty() && fullMsg.containsIgnoreCase ( localName );
                    if ( localName.isNotEmpty() && fullMsg.containsIgnoreCase ( localName ) )
                    {
                        // Apply local color for own messages received as server-formatted HTML
                        addSystemMessage ( fullMsg, currentTextColor );
                    }
                    else
                    {
                        addSystemMessage ( fullMsg );
                    }
                    maybeTranslateIncomingMessage ( {}, fullMsg, true, isLocalMessage );
                }
                else
                {
                    int colonPos = fullMsg.indexOf ( ": " );
                    if ( colonPos > 0 )
                    {
                        juce::String sender = fullMsg.substring ( 0, colonPos );
                        juce::String text   = fullMsg.substring ( colonPos + 2 );

                        if ( text.startsWith ( SIG_PREFIX ) )
                        {
                            if ( onSignalingMessageReceived )
                                onSignalingMessageReceived ( sender, text.substring ( juce::String ( SIG_PREFIX ).length() ) );
                        }
                        else
                        {
                            const bool isLocalMessage = sender.trim().equalsIgnoreCase ( getLocalClientName() );
                            addChatMessage ( sender, text );
                            maybeTranslateIncomingMessage ( sender, text, false, isLocalMessage );
                        }
                    }
                    else
                    {
                        addSystemMessage ( fullMsg );
                        maybeTranslateIncomingMessage ( {}, fullMsg, true, false );
                    }
                }
            }

            if ( shouldScroll )
                messageViewport.setViewPosition ( 0, messageList.getHeight() );
        }
    }

    void sendMessage()
    {
        juce::String text = inputEdit.getText().trim();
        if ( text.isEmpty() )
            return;

        if ( onSendRaw )
        {
            onSendRaw ( text );
        }
        else if ( jamulusClient )
        {
            jamulus_client_send_chat_message ( jamulusClient, text.toRawUTF8() );
        }

        if ( onMessageSent )
            onMessageSent ( text );

        inputEdit.clear();
    }

    void showColorPicker ( bool isBackground )
    {
        auto colorSelector = std::make_unique<juce::ColourSelector>();
        colorSelector->setSize ( 300, 200 );
        colorSelector->setCurrentColour ( currentTextColor );

        // Use a safe wrapper for the listener
        auto* listener = new ColorListener ( this, false );
        colorSelector->addChangeListener ( listener );

        juce::CallOutBox::launchAsynchronously ( std::move ( colorSelector ), textColorButton.getScreenBounds(), nullptr );

        // Note: juce::CallOutBox takes ownership of the component, but we need to manage the listener.
        // In a real app we'd attach it to the component. For now, we'll let it leak as a small object
        // to avoid complexity, but better would be to have it owned by the ColourSelector.
    }

    struct ColorListener : public juce::ChangeListener
    {
        ChatComponent* owner;
        bool           isBackground;
        ColorListener ( ChatComponent* o, bool bg ) : owner ( o ), isBackground ( bg ) {}
        void changeListenerCallback ( juce::ChangeBroadcaster* source ) override
        {
            if ( auto* cs = dynamic_cast<juce::ColourSelector*> ( source ) )
            {
                if ( !isBackground )
                {
                    owner->currentTextColor = cs->getCurrentColour();
                    if ( owner->currentTextColor.getAlpha() < 50 )
                        owner->currentTextColor = owner->currentTextColor.withAlpha ( (juce::uint8) 255 );

                    owner->inputEdit.setColour ( juce::TextEditor::textColourId, owner->currentTextColor );
                }
            }
        }
    };

    juce::String parseHtmlToPlainText ( const juce::String& html )
    {
        // Simple HTML tag removal (Jamulus chat can contain HTML)
        juce::String result = html;
        result              = result.replace ( "<br>", "\n", true );
        result              = result.replace ( "<br/>", "\n", true );
        result              = result.replace ( "&lt;", "<" );
        result              = result.replace ( "&gt;", ">" );
        result              = result.replace ( "&amp;", "&" );
        result              = result.replace ( "&nbsp;", " " );

        // Remove other HTML tags
        int start = 0;
        while ( ( start = result.indexOf ( "<" ) ) >= 0 )
        {
            int end = result.indexOf ( start, ">" );
            if ( end > start )
                result = result.substring ( 0, start ) + result.substring ( end + 1 );
            else
                break;
        }

        return result.trim();
    }

    AsyncChatTranslationWorker translationWorker{ *this };
    std::atomic<juce::uint64> translationConfigRevision{ 0 };
    bool         translationFailureActive     = false;
    double       lastTranslationFailureNoticeMs = 0.0;
    juce::String lastTranslationFailureReason;

    void updateAutoTranslateButtonColor()
    {
        if ( autoTranslateButton.getToggleState() )
        {
            const juce::Colour glow = juce::Colour ( 0xff00aa66 );
            autoTranslateButton.setColour ( juce::TextButton::buttonColourId, glow );
            autoTranslateButton.setColour ( juce::TextButton::buttonOnColourId, glow );
            autoTranslateButton.setColour ( juce::TextButton::textColourOffId, juce::Colours::white );
            autoTranslateButton.setColour ( juce::TextButton::textColourOnId, juce::Colours::white );
        }
        else
        {
            const juce::Colour off = juce::Colour ( 0xff4a4a4a );
            autoTranslateButton.setColour ( juce::TextButton::buttonColourId, off );
            autoTranslateButton.setColour ( juce::TextButton::buttonOnColourId, off );
            autoTranslateButton.setColour ( juce::TextButton::textColourOffId, juce::Colours::lightgrey );
            autoTranslateButton.setColour ( juce::TextButton::textColourOnId, juce::Colours::lightgrey );
        }
    }

    void saveChat()
    {
        auto chooser = std::make_shared<juce::FileChooser> (
            "Save Chat Log",
            juce::File::getSpecialLocation ( juce::File::userDocumentsDirectory ).getChildFile ( "jamulus_chat.txt" ),
            "*.txt" );

        auto chatText = messageList.exportToPlainText();

        chooser->launchAsync ( juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles,
                               [chooser, chatText] ( const juce::FileChooser& fc ) {
                                   if ( fc.getResults().size() > 0 )
                                   {
                                       juce::File file = fc.getResult();
                                       file.replaceWithText ( chatText );
                                   }
                               } );
    }
};

//==============================================================================
// Chat Window (for pop-out mode)
//==============================================================================
class ChatWindow : public juce::DocumentWindow
{
public:
    ChatWindow ( jamulus_client_t client ) :
        DocumentWindow ( "Jamulus Chat", juce::Colour ( 0xff323232 ), DocumentWindow::closeButton | DocumentWindow::minimiseButton )
    {
        auto* chat    = new ChatComponent ( client, false );
        chat->onClose = [this]() { closeButtonPressed(); };
        chat->onPopIn = [this]() {
            if ( onPopIn )
                onPopIn();
        };

        setContentOwned ( chat, true );
        setResizable ( true, true );
        setSize ( 350, 500 );
        centreWithSize ( getWidth(), getHeight() );
        setVisible ( true );
        setAlwaysOnTop ( true );
    }

    void closeButtonPressed() override
    {
        if ( onWindowClosed )
            onWindowClosed();
    }

    std::function<void()> onWindowClosed;
    std::function<void()> onPopIn;
};
