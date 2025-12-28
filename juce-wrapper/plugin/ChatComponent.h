#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../jamulus_wrapper.h"

//==============================================================================
// Chat Message Structure
//==============================================================================
struct ChatMessage
{
    juce::String sender;
    juce::String text;
    juce::Time   timestamp;
    bool         isSystemMessage = false;
};

//==============================================================================
// Chat Message Display Component
//==============================================================================
class ChatMessageList : public juce::Component
{
public:
    ChatMessageList() { setOpaque ( true ); }

    void addMessage ( const ChatMessage& msg )
    {
        messages.push_back ( msg );
        updateHeight();
        repaint();

        // Scroll to bottom
        if ( auto* viewport = findParentComponentOfClass<juce::Viewport>() )
        {
            viewport->setViewPosition ( 0, getHeight() - viewport->getViewHeight() );
        }
    }

    void clearMessages()
    {
        messages.clear();
        updateHeight();
        repaint();
    }

    // Get all messages for sharing between views
    const std::vector<ChatMessage>& getMessages() const { return messages; }

    // Set messages (for restoring from shared storage)
    void setMessages ( const std::vector<ChatMessage>& msgs )
    {
        messages = msgs;
        updateHeight();
        repaint();
    }

    void resized() override
    {
        // Recalculate height when width changes
        updateHeight();
    }

    // Export chat to plain text for saving
    juce::String exportToPlainText() const
    {
        juce::String result;
        for ( const auto& msg : messages )
        {
            juce::String timeStr = msg.timestamp.toString ( false, true, false );
            if ( msg.isSystemMessage )
            {
                result += "[" + timeStr + "] * " + stripHtml ( msg.text ) + "\n";
            }
            else
            {
                result += "[" + timeStr + "] " + msg.sender + ": " + stripHtml ( msg.text ) + "\n";
            }
        }
        return result;
    }

    void paint ( juce::Graphics& g ) override
    {
        g.fillAll ( juce::Colour ( 0xff2a2a2a ) );

        int       y              = 5;
        const int padding        = 8;
        const int messageSpacing = 8; // Increased spacing

        for ( const auto& msg : messages )
        {
            // Time stamp
            juce::String timeStr = msg.timestamp.toString ( false, true, false );

            if ( msg.isSystemMessage )
            {
                g.setFont ( juce::Font ( 12.0f, juce::Font::italic ) );
                g.setColour ( juce::Colour ( 0xffaaaaaa ) );

                auto             attrStr = createAttributedString ( "* " + msg.text, juce::Colour ( 0xffaaaaaa ) );
                juce::TextLayout layout;
                layout.createLayout ( attrStr, static_cast<float> ( getWidth() - padding * 2 ) );
                layout.draw ( g,
                              juce::Rectangle<float> ( static_cast<float> ( padding ),
                                                       static_cast<float> ( y ),
                                                       static_cast<float> ( getWidth() - padding * 2 ),
                                                       1000.0f ) );

                y += static_cast<int> ( layout.getHeight() ) + messageSpacing;
            }
            else
            {
                // Header (Time + Sender)
                g.setFont ( juce::Font ( 12.0f, juce::Font::bold ) );
                g.setColour ( juce::Colour ( 0xff5588dd ) );
                juce::String header = "[" + timeStr + "] " + msg.sender + ": ";
                g.drawText ( header, padding, y, getWidth() - padding * 2, 18, juce::Justification::centredLeft );

                y += 18;

                // Message text with HTML support
                auto             attrStr = createAttributedString ( msg.text, juce::Colour ( 0xffe0e0e0 ) );
                juce::TextLayout layout;
                layout.createLayout ( attrStr, static_cast<float> ( getWidth() - padding * 2 ) );
                layout.draw ( g,
                              juce::Rectangle<float> ( static_cast<float> ( padding ),
                                                       static_cast<float> ( y ),
                                                       static_cast<float> ( getWidth() - padding * 2 ),
                                                       1000.0f ) );

                y += static_cast<int> ( layout.getHeight() ) + messageSpacing;
            }
        }
    }

private:
    // Strip HTML tags for plain text export
    static juce::String stripHtml ( const juce::String& html )
    {
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

    juce::AttributedString createAttributedString ( const juce::String& text, juce::Colour defaultColour )
    {
        juce::AttributedString attr;
        attr.setLineSpacing ( 1.0f );

        int                       pos = 0;
        std::vector<juce::Colour> colourStack;
        colourStack.push_back ( defaultColour );
        std::vector<bool> boldStack;
        boldStack.push_back ( false );

        while ( pos < text.length() )
        {
            int tagStart = text.indexOf ( pos, "<" );
            if ( tagStart < 0 )
            {
                attr.append ( text.substring ( pos ),
                              juce::Font ( 13.0f, boldStack.back() ? juce::Font::bold : juce::Font::plain ),
                              colourStack.back() );
                break;
            }

            if ( tagStart > pos )
                attr.append ( text.substring ( pos, tagStart ),
                              juce::Font ( 13.0f, boldStack.back() ? juce::Font::bold : juce::Font::plain ),
                              colourStack.back() );

            int tagEnd = text.indexOf ( tagStart, ">" );
            if ( tagEnd < 0 )
            {
                attr.append ( text.substring ( tagStart ),
                              juce::Font ( 13.0f, boldStack.back() ? juce::Font::bold : juce::Font::plain ),
                              colourStack.back() );
                break;
            }

            juce::String tag = text.substring ( tagStart, tagEnd + 1 );
            if ( tag.startsWithIgnoreCase ( "<font" ) )
            {
                juce::Colour newColour = colourStack.back();
                int          colorPos  = tag.indexOfIgnoreCase ( "color=" );
                if ( colorPos > 0 )
                {
                    int quoteChar     = tag[colorPos + 6];
                    int colorValStart = ( quoteChar == '\"' || quoteChar == '\'' ) ? colorPos + 7 : colorPos + 6;
                    int colorValEnd =
                        tag.indexOf ( colorValStart, ( quoteChar == '\"' || quoteChar == '\'' ) ? juce::String::charToString ( quoteChar ) : " >" );

                    if ( colorValEnd > colorValStart )
                    {
                        juce::String colorStr = tag.substring ( colorValStart, colorValEnd ).trim();
                        if ( colorStr.startsWith ( "#" ) )
                        {
                            juce::String hex = colorStr.substring ( 1 );
                            if ( hex.length() == 6 )
                                newColour = juce::Colour::fromString ( "ff" + hex );
                            else if ( hex.length() == 3 )
                                newColour =
                                    juce::Colour::fromString ( "ff" + juce::String::charToString ( hex[0] ) + juce::String::charToString ( hex[0] ) +
                                                               juce::String::charToString ( hex[1] ) + juce::String::charToString ( hex[1] ) +
                                                               juce::String::charToString ( hex[2] ) + juce::String::charToString ( hex[2] ) );
                        }
                        else
                        {
                            newColour = juce::Colours::findColourForName ( colorStr, newColour );
                        }
                    }
                }
                colourStack.push_back ( newColour );
            }
            else if ( tag.equalsIgnoreCase ( "</font>" ) )
            {
                if ( colourStack.size() > 1 )
                    colourStack.pop_back();
            }
            else if ( tag.equalsIgnoreCase ( "<b>" ) || tag.equalsIgnoreCase ( "<strong>" ) )
            {
                boldStack.push_back ( true );
            }
            else if ( tag.equalsIgnoreCase ( "</b>" ) || tag.equalsIgnoreCase ( "</strong>" ) )
            {
                if ( boldStack.size() > 1 )
                    boldStack.pop_back();
            }
            else if ( tag.equalsIgnoreCase ( "<br>" ) || tag.equalsIgnoreCase ( "<br/>" ) )
            {
                attr.append ( "\n", juce::Font ( 13.0f ), colourStack.back() );
            }

            pos = tagEnd + 1;
        }

        return attr;
    }

    void updateHeight()
    {
        const int width = getWidth();
        if ( width <= 0 )
            return;

        int       totalHeight    = 10;
        const int padding        = 8;
        const int messageSpacing = 8;

        for ( const auto& msg : messages )
        {
            if ( msg.isSystemMessage )
            {
                auto             attrStr = createAttributedString ( "* " + msg.text, juce::Colour ( 0xffaaaaaa ) );
                juce::TextLayout layout;
                layout.createLayout ( attrStr, static_cast<float> ( juce::jmax ( 10, width - padding * 2 ) ) );
                totalHeight += static_cast<int> ( layout.getHeight() ) + messageSpacing;
            }
            else
            {
                totalHeight += 18; // Header
                auto             attrStr = createAttributedString ( msg.text, juce::Colour ( 0xffe0e0e0 ) );
                juce::TextLayout layout;
                layout.createLayout ( attrStr, static_cast<float> ( juce::jmax ( 10, width - padding * 2 ) ) );
                totalHeight += static_cast<int> ( layout.getHeight() ) + messageSpacing;
            }
        }

        setSize ( width, juce::jmax ( totalHeight, 10 ) );
    }

    std::vector<ChatMessage> messages;
};

//==============================================================================
// Chat Component
//==============================================================================
class ChatComponent : public juce::Component, private juce::Timer
{
public:
    ChatComponent ( jamulus_client_t client, bool panelMode = false ) : jamulusClient ( client ), isPanelMode ( panelMode )
    {
        // Title bar with pop-out button
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
        inputEdit.setTextToShowWhenEmpty ( "Type a message...", juce::Colours::grey );
        inputEdit.onReturnKey = [this]() { sendMessage(); };

        // Send button
        addAndMakeVisible ( sendButton );
        sendButton.setButtonText ( "Send" );
        sendButton.onClick = [this]() { sendMessage(); };

        // Clear button (only in popup mode)
        addAndMakeVisible ( clearButton );
        clearButton.setButtonText ( "Clear" );
        clearButton.onClick = [this]() { messageList.clearMessages(); };
        clearButton.setVisible ( !isPanelMode );

        // Save button - opens system file save dialog
        addAndMakeVisible ( saveButton );
        saveButton.setButtonText ( "Save" );
        saveButton.onClick = [this]() { saveChat(); };
        saveButton.setVisible ( !isPanelMode );

        // Only add welcome message if not restoring from shared storage
        if ( !isPanelMode ) // Will be populated by setMessages() in panel mode
        {
            ChatMessage welcome;
            welcome.text            = "Welcome to Jamulus Chat. Be respectful to other musicians!";
            welcome.isSystemMessage = true;
            welcome.timestamp       = juce::Time::getCurrentTime();
            messageList.addMessage ( welcome );
        }

        startTimerHz ( 5 );
    }

    ~ChatComponent() override { stopTimer(); }

    void addChatMessage ( const juce::String& sender, const juce::String& text )
    {
        ChatMessage msg;
        msg.sender          = sender;
        msg.text            = text; // Keep HTML for color processing in display
        msg.timestamp       = juce::Time::getCurrentTime();
        msg.isSystemMessage = false;
        messageList.addMessage ( msg );
    }

    void addSystemMessage ( const juce::String& text )
    {
        ChatMessage msg;
        msg.text            = text; // Keep HTML
        msg.timestamp       = juce::Time::getCurrentTime();
        msg.isSystemMessage = true;
        messageList.addMessage ( msg );
    }

    // Get all messages for persistence
    const std::vector<ChatMessage>& getMessages() const { return messageList.getMessages(); }

    // Set messages (restore from shared storage)
    void setMessages ( const std::vector<ChatMessage>& msgs ) { messageList.setMessages ( msgs ); }

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

        // Header
        auto header = bounds.removeFromTop ( isPanelMode ? 30 : 40 );
        if ( isPanelMode )
        {
            closeButton.setBounds ( header.removeFromRight ( 30 ).reduced ( 2 ) );
            popOutButton.setBounds ( header.removeFromRight ( 30 ).reduced ( 2 ) );
            titleLabel.setBounds ( header.reduced ( 5, 0 ) );
        }
        else
        {
            // In popup window mode, show pop-in button in header
            popInButton.setBounds ( header.removeFromRight ( 40 ).reduced ( 5 ) );
            titleLabel.setBounds ( header );
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
        inputEdit.setBounds ( inputArea );

        // Message area
        int margin = isPanelMode ? 5 : 10;
        messageViewport.setBounds ( bounds.reduced ( margin, margin / 2 ) );
        messageList.setSize ( messageViewport.getWidth() - messageViewport.getScrollBarThickness(), messageList.getHeight() );
    }

    std::function<void()>                       onClose;
    std::function<void()>                       onPopOut;
    std::function<void()>                       onPopIn;
    std::function<void ( const juce::String& )> onMessageSent;

private:
    void timerCallback() override
    {
        // Poll for all new chat messages from the Jamulus client
        if ( !jamulusClient )
            return;

        while ( true )
        {
            const char* chatMsg = jamulus_client_get_chat_message ( jamulusClient );
            if ( !chatMsg || chatMsg[0] == '\0' )
                break;

            juce::String fullMsg ( chatMsg );

            // Jamulus server often sends messages starting with "*" for system or status.
            // If it already has a "*", don't treat it as a private "Name: Message" chunk.
            if ( fullMsg.startsWith ( "*" ) )
            {
                addSystemMessage ( fullMsg.substring ( 1 ).trim() );
            }
            else
            {
                int colonPos = fullMsg.indexOf ( ": " );
                if ( colonPos > 0 )
                {
                    juce::String sender = fullMsg.substring ( 0, colonPos );
                    juce::String text   = fullMsg.substring ( colonPos + 2 );
                    addChatMessage ( sender, text );
                }
                else
                {
                    addSystemMessage ( fullMsg );
                }
            }
        }
    }

    void sendMessage()
    {
        juce::String text = inputEdit.getText().trim();
        if ( text.isEmpty() )
            return;

        // Send to Jamulus
        if ( jamulusClient )
        {
            jamulus_client_send_chat_message ( jamulusClient, text.toRawUTF8() );
        }

        // Note: We no longer add to local display here because Jamulus
        // servers echo your own messages back to you. Adding it here
        // would cause the message to appear twice.

        if ( onMessageSent )
            onMessageSent ( text );

        inputEdit.clear();
    }

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

    jamulus_client_t jamulusClient;
    bool             isPanelMode;

    juce::Label      titleLabel;
    juce::TextButton popOutButton;
    juce::TextButton popInButton;
    juce::Viewport   messageViewport;
    ChatMessageList  messageList;
    juce::TextEditor inputEdit;
    juce::TextButton sendButton;
    juce::TextButton clearButton;
    juce::TextButton saveButton;
    juce::TextButton closeButton;

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
