#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../jamulus_wrapper.h"
#include <memory>
#include <regex>
#include <unordered_map>

//==============================================================================
// Server List Item (for display in the table)
//==============================================================================
struct PlayerListItem
{
    juce::String name;
    int          instrumentId = 0;
    juce::String countryCode;
};

struct ServerListItem
{
    juce::String                name;
    juce::String                address;
    int                         ping       = -1;
    int                         numClients = 0;
    int                         maxClients = 0;
    juce::String                location;
    juce::String                version;
    bool                        isFull = false;
    juce::String                players;
    std::vector<PlayerListItem> playerItems;
};

struct RowEntry
{
    int serverIndex = -1;
    int playerIndex = -1; // -1 for server row, >=0 for player
};

//==============================================================================
// Server List Table Model
//==============================================================================
class ServerListModel : public juce::TableListBoxModel
{
public:
    enum Columns
    {
        Name = 1,
        Ping,
        Clients,
        Location,
        Version
    };

    int getNumRows() override { return static_cast<int> ( visibleRows.size() ); }

    bool isShowingMusicians() const { return showMusicians; }
    void setShowMusicians ( bool show )
    {
        showMusicians = show;
        rebuildVisibleRows();
    }

    void paintRowBackground ( juce::Graphics& g, int rowNumber, int /*width*/, int /*height*/, bool rowIsSelected ) override
    {
        if ( rowNumber < 0 || rowNumber >= static_cast<int> ( visibleRows.size() ) )
            return;

        const auto& entry      = visibleRows[rowNumber];
        bool        hasPlayers = false;

        if ( entry.serverIndex >= 0 && entry.serverIndex < static_cast<int> ( servers.size() ) )
        {
            const auto& s = servers[static_cast<size_t> ( entry.serverIndex )];
            // Only highlight if there are clients AND we have player names listed
            if ( s.numClients > 0 && s.playerItems.size() > 0 )
                hasPlayers = true;
        }

        if ( rowIsSelected )
            g.fillAll ( juce::Colour ( 0xff4a6fa5 ) );
        else if ( hasPlayers && entry.playerIndex == -1 )
            g.fillAll ( juce::Colour ( 0xff2f3a44 ) ); // Much fainter/darker blue
        else if ( rowNumber % 2 )
            g.fillAll ( juce::Colour ( 0xff383838 ) );
        else
            g.fillAll ( juce::Colour ( 0xff303030 ) );
    }

    void paintCell ( juce::Graphics& g, int rowNumber, int columnId, int width, int height, bool /*rowIsSelected*/ ) override
    {
        if ( rowNumber >= static_cast<int> ( visibleRows.size() ) )
            return;

        const auto& entry  = visibleRows[rowNumber];
        const auto& server = servers[static_cast<size_t> ( entry.serverIndex )];

        g.setColour ( juce::Colours::white );
        g.setFont ( 13.0f );

        // Player row
        if ( entry.playerIndex >= 0 )
        {
            if ( columnId == Name )
            {
                if ( entry.playerIndex < (int) server.playerItems.size() )
                {
                    const auto& player = server.playerItems[entry.playerIndex];

                    int x = 20;

                    // Draw flag
                    if ( player.countryCode.isNotEmpty() && player.countryCode != "none" )
                    {
                        juce::String flagPath = "png/flags/res/flags/" + player.countryCode + ".png";
                        auto         flag     = loadImageFromResource ( flagPath );
                        if ( flag.isValid() )
                        {
                            g.drawImageWithin ( flag, x, ( height - 12 ) / 2, 16, 12, juce::RectanglePlacement::centred );
                            x += 20;
                        }
                    }

                    // Draw instrument
                    juce::String instPath = getInstrumentPath ( player.instrumentId );
                    auto         inst     = loadImageFromResource ( instPath );
                    if ( inst.isValid() )
                    {
                        g.drawImageWithin ( inst, x, ( height - 16 ) / 2, 16, 16, juce::RectanglePlacement::centred );
                        x += 20;
                    }

                    g.setColour ( juce::Colours::lightgrey );
                    juce::String nameText = player.name;
                    juce::String instName = getInstrumentName ( player.instrumentId );
                    if ( instName.isNotEmpty() && instName != "None" )
                        nameText += " (" + instName + ")";

                    g.drawText ( nameText, x, 0, width - x - 4, height, juce::Justification::centredLeft );
                }
            }
            return;
        }

        // Server row
        juce::String text;
        switch ( columnId )
        {
        case Name:
            text = server.name;
            break;
        case Ping:
            text = server.ping >= 0 ? juce::String ( server.ping ) + " ms" : "--";
            break;
        case Clients:
            if ( server.isFull && server.maxClients > 0 )
                text = juce::String ( server.numClients ) + "/" + juce::String ( server.maxClients ) + " FULL";
            else if ( server.maxClients > 0 )
                text = juce::String ( server.numClients ) + "/" + juce::String ( server.maxClients );
            else
                text = juce::String ( server.numClients ) + "/?";
            break;
        case Location:
            text = server.location;
            break;
        case Version:
            text = server.version;
            break;
        }

        // Color ping by quality
        if ( columnId == Ping && server.ping >= 0 )
        {
            if ( server.ping < 40 )
                g.setColour ( juce::Colour ( 0xff00cc66 ) );
            else if ( server.ping < 80 )
                g.setColour ( juce::Colours::yellow );
            else
                g.setColour ( juce::Colours::red );
        }
        // Clients: remove occupancy bar; emphasize text when there are users
        if ( columnId == Clients )
        {
            if ( server.isFull )
            {
                g.setColour ( juce::Colours::red );
            }
            else if ( server.numClients > 0 )
            {
                g.setColour ( juce::Colour ( 0xff00cc66 ) );
                g.setFont ( g.getCurrentFont().boldened() );
            }
            else
            {
                g.setColour ( juce::Colour ( 0x99ff7f50 ) );
                auto f = g.getCurrentFont();
                f.setBold ( false );
                f.setHeight ( 11.0f );
                g.setFont ( f );
            }
        }

        g.drawText ( text, 4, 0, width - 8, height, juce::Justification::centredLeft );
    }

    juce::String getCellTooltip ( int rowNumber, int columnId ) override
    {
        if ( rowNumber >= 0 && rowNumber < static_cast<int> ( visibleRows.size() ) )
        {
            const auto& entry = visibleRows[rowNumber];
            if ( entry.playerIndex == -1 && entry.serverIndex >= 0 && entry.serverIndex < static_cast<int> ( servers.size() ) )
            {
                const auto& s = servers[static_cast<size_t> ( entry.serverIndex )];
                juce::String tip = s.address;
                if ( s.version.isNotEmpty() )
                    tip += " \xE2\x80\xA2 " + s.version;
                return tip;
            }
        }
        return {};
    }

    void cellClicked ( int rowNumber, int columnId, const juce::MouseEvent& e ) override
    {
        if ( rowNumber < 0 || rowNumber >= static_cast<int> ( visibleRows.size() ) )
            return;
        const auto& entry = visibleRows[rowNumber];
        if ( entry.playerIndex != -1 )
            return;
        if ( entry.serverIndex < 0 || entry.serverIndex >= static_cast<int> ( servers.size() ) )
            return;
        if ( !e.mods.isPopupMenu() )
            return;

        const auto s = servers[static_cast<size_t> ( entry.serverIndex )];
        juce::PopupMenu m;
        m.addItem ( 1, "Connect" );
        m.addItem ( 2, "Copy address" );
        m.addItem ( 3, "Refresh musicians" );
        m.showMenuAsync ( juce::PopupMenu::Options().withTargetComponent ( e.eventComponent ), [this, s] ( int r ) {
            if ( r == 1 )
            {
                if ( onServerSelected )
                    onServerSelected ( s );
            }
            else if ( r == 2 )
            {
                juce::SystemClipboard::copyTextToClipboard ( s.address );
            }
            else if ( r == 3 )
            {
                if ( onRefreshMusicians )
                    onRefreshMusicians ( s.address );
            }
        } );
    }
    void cellDoubleClicked ( int rowNumber, int /*columnId*/, const juce::MouseEvent& ) override
    {
        if ( rowNumber >= 0 && rowNumber < static_cast<int> ( visibleRows.size() ) )
        {
            const auto& entry = visibleRows[rowNumber];
            // Only convert to selection if it's a server row (or handle player click if needed)
            if ( entry.playerIndex == -1 && onServerSelected )
                onServerSelected ( servers[static_cast<size_t> ( entry.serverIndex )] );
        }
    }

    void selectedRowsChanged ( int lastRowSelected ) override
    {
        if ( lastRowSelected >= 0 && lastRowSelected < static_cast<int> ( visibleRows.size() ) )
        {
            const auto& entry = visibleRows[lastRowSelected];
            if ( entry.serverIndex >= 0 && entry.serverIndex < static_cast<int> ( servers.size() ) )
                selectedServer = servers[static_cast<size_t> ( entry.serverIndex )];
        }
    }

    void addServer ( const ServerListItem& server )
    {
        // Update existing or add new
        for ( auto& s : servers )
        {
            if ( s.address == server.address )
            {
                ServerListItem merged = server;
                if ( merged.name.isEmpty() )
                    merged.name = s.name;
                if ( merged.location.isEmpty() )
                    merged.location = s.location;
                if ( merged.version.isEmpty() )
                    merged.version = s.version;
                if ( merged.players.isEmpty() && !s.players.isEmpty() )
                    merged.players = s.players;
                if ( merged.playerItems.empty() && !s.playerItems.empty() )
                    merged.playerItems = s.playerItems;
                s = std::move ( merged );
                rebuildVisibleRows();
                return;
            }
        }
        servers.push_back ( server );
        rebuildVisibleRows();
    }

    void clearServers()
    {
        servers.clear();
        rebuildVisibleRows();
    }

    void sortByColumn ( int columnId, bool forwards )
    {
        currentSortColumn  = columnId;
        currentSortForward = forwards;

        auto comparator = [columnId, forwards] ( const ServerListItem& a, const ServerListItem& b ) {
            int result = 0;
            switch ( columnId )
            {
            case Name:
                result = a.name.compareNatural ( b.name );
                break;
            case Ping:
                // Sort unknown pings (-1) to the end
                if ( a.ping < 0 && b.ping < 0 )
                    result = 0;
                else if ( a.ping < 0 )
                    result = 1;
                else if ( b.ping < 0 )
                    result = -1;
                else
                    result = a.ping - b.ping;
                break;
            case Clients:
                if ( a.numClients != b.numClients )
                    result = a.numClients - b.numClients;
                else if ( a.maxClients != b.maxClients )
                    result = a.maxClients - b.maxClients;
                else
                    result = a.name.compareNatural ( b.name );
                break;
            case Location:
                result = a.location.compareNatural ( b.location );
                break;
            case Version:
                result = makeVersionSortKey ( a.version ).compare ( makeVersionSortKey ( b.version ) );
                break;
            }
            return forwards ? result < 0 : result > 0;
        };
        std::sort ( servers.begin(), servers.end(), comparator );
        rebuildVisibleRows();
    }

    // Called by TableListBox when header is clicked
    void sortOrderChanged ( int newSortColumnId, bool isForwards ) override
    {
        sortByColumn ( newSortColumnId, isForwards );
        if ( onSortChanged )
            onSortChanged();
    }

    int  getCurrentSortColumn() const { return currentSortColumn; }
    bool getCurrentSortForward() const { return currentSortForward; }

    void filterByText ( const juce::String& filter )
    {
        currentFilter = filter;
        rebuildVisibleRows();
    }

    ServerListItem getSelectedServer() const { return selectedServer; }

    std::function<void ( const ServerListItem& )> onServerSelected;
    std::function<void()>                         onSortChanged;
    std::function<void ( const juce::String& )>   onRefreshMusicians;

private:
    static juce::String makeVersionSortKey ( const juce::String& versionText )
    {
        const std::string version = versionText.trim().toStdString();
        if ( version.empty() )
            return {};

        static const std::regex semVerRegex ( R"(^(\d+)\.(\d+)\.(\d+)-?([^:]*):?(.*)$)" );
        std::smatch match;
        if ( !std::regex_match ( version, match, semVerRegex ) || match.size() < 6 )
            return versionText;

        const int major = std::stoi ( match[1].str() );
        const int minor = std::stoi ( match[2].str() );
        const int patch = std::stoi ( match[3].str() );
        const juce::String suffix ( match[4].str() );
        const juce::String timestamp ( match[5].str() );

        juce::juce_wchar orderKey = '>';
        if ( suffix.isEmpty() )
            orderKey = '=';
        else if ( suffix.startsWithIgnoreCase ( "rc" ) || suffix.startsWithIgnoreCase ( "beta" ) || suffix.startsWithIgnoreCase ( "alpha" ) )
            orderKey = '<';

        return juce::String ( major ).paddedLeft ( '0', 3 ) +
               juce::String ( minor ).paddedLeft ( '0', 3 ) +
               juce::String ( patch ).paddedLeft ( '0', 3 ) +
               juce::String::charToString ( orderKey ) +
               ( timestamp.isNotEmpty() ? timestamp : suffix );
    }

    void rebuildVisibleRows()
    {
        visibleRows.clear();
        for ( int i = 0; i < static_cast<int> ( servers.size() ); ++i )
        {
            const auto& s = servers[i];

            juce::String filter = currentFilter.trim();
            bool         matches = true;
            if ( filter.isNotEmpty() )
            {
                if ( filter == "#" )
                {
                    matches = ( s.numClients > 0 );
                }
                else
                {
                    matches = s.name.containsIgnoreCase ( filter ) || s.location.containsIgnoreCase ( filter );
                    if ( !matches )
                    {
                        for ( const auto& p : s.playerItems )
                        {
                            if ( p.name.containsIgnoreCase ( filter ) )
                            {
                                matches = true;
                                break;
                            }
                        }
                    }
                }
            }

            if ( matches )
            {
                visibleRows.push_back ( { i, -1 } );

                if ( showMusicians && s.playerItems.size() > 0 )
                {
                    for ( int p = 0; p < (int) s.playerItems.size(); ++p )
                        visibleRows.push_back ( { i, p } );
                }
            }
        }
    }

    std::vector<ServerListItem> servers;
    std::vector<RowEntry>       visibleRows;
    juce::String                currentFilter;
    ServerListItem              selectedServer;
    int                         currentSortColumn  = Name;
    bool                        currentSortForward = true;
    bool                        showMusicians      = false;

    std::map<juce::String, juce::Image> imageCache;

    juce::Image loadImageFromResource ( juce::String path )
    {
        if ( imageCache.count ( path ) )
            return imageCache[path];

        void* data = nullptr;
        int   size = 0;
        if ( jamulus_load_resource ( path.toRawUTF8(), &data, &size ) )
        {
            auto image = juce::ImageFileFormat::loadFrom ( data, (size_t) size );
            jamulus_free_resource ( data );
            imageCache[path] = image;
            return image;
        }
        return {};
    }

    juce::String getInstrumentPath ( int id )
    {
        static const char* paths[] = { "none.png",         "drumset.png",
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

        if ( id >= 0 && id < 50 )
            return "png/instr/res/instruments/" + juce::String ( paths[id] );

        return "png/instr/res/instruments/none.png";
    }

    juce::String getInstrumentName ( int id )
    {
        static const char* names[] = { "None",
                                       "Drum Set",
                                       "Djembe",
                                       "Electric Guitar",
                                       "Acoustic Guitar",
                                       "Bass Guitar",
                                       "Keyboard",
                                       "Synthesizer",
                                       "Grand Piano",
                                       "Accordion",
                                       "Vocal",
                                       "Microphone",
                                       "Harmonica",
                                       "Trumpet",
                                       "Trombone",
                                       "French Horn",
                                       "Tuba",
                                       "Saxophone",
                                       "Clarinet",
                                       "Flute",
                                       "Violin",
                                       "Cello",
                                       "Double Bass",
                                       "Recorder",
                                       "Streamer",
                                       "Listener",
                                       "Guitar+Vocal",
                                       "Keyboard+Vocal",
                                       "Bodhran",
                                       "Bassoon",
                                       "Oboe",
                                       "Harp",
                                       "Viola",
                                       "Congas",
                                       "Bongo",
                                       "Vocal Bass",
                                       "Vocal Tenor",
                                       "Vocal Alto",
                                       "Vocal Soprano",
                                       "Banjo",
                                       "Mandolin",
                                       "Ukulele",
                                       "Bass Ukulele",
                                       "Vocal Baritone",
                                       "Vocal Lead",
                                       "Mountain Dulcimer",
                                       "Scratching",
                                       "Rapping",
                                       "Vibraphone",
                                       "Conductor" };

        if ( id >= 0 && id < 50 )
            return names[id];

        return "";
    }
};

//==============================================================================
// Connect Dialog Component
//==============================================================================
class ConnectComponent : public juce::Component, private juce::Timer
{
public:
    ConnectComponent ( jamulus_client_t client ) : jamulusClient ( client )
    {
        directoryImpl = std::make_unique<WrapperListJucePingDirectory> ( *this );
        // Title
        addAndMakeVisible ( titleLabel );
        titleLabel.setText ( "Connect to Server", juce::dontSendNotification );
        titleLabel.setFont ( juce::Font ( 20.0f, juce::Font::bold ) );
        titleLabel.setColour ( juce::Label::textColourId, juce::Colours::white );
        titleLabel.setJustificationType ( juce::Justification::centred );

        // Directory selector
        addAndMakeVisible ( directoryLabel );
        directoryLabel.setText ( "Directory:", juce::dontSendNotification );
        directoryLabel.setColour ( juce::Label::textColourId, juce::Colours::white );

        addAndMakeVisible ( directoryCombo );
        directoryCombo.addItem ( "Any Genre 1", 1 );
        directoryCombo.addItem ( "Any Genre 2", 2 );
        directoryCombo.addItem ( "Any Genre 3", 3 );
        directoryCombo.addItem ( "Genre Rock", 4 );
        directoryCombo.addItem ( "Genre Jazz", 5 );
        directoryCombo.addItem ( "Genre Classical/Folk", 6 );
        directoryCombo.addItem ( "Genre Choral/Barbershop", 7 );
        // Avoid Unicode ellipsis to prevent mojibake on some systems
        directoryCombo.addItem ( "Custom Directory...", 8 );
        directoryCombo.setSelectedId ( 1 );
        directoryCombo.onChange = [this]() {
            int sel = directoryCombo.getSelectedId();
            bool isCustom = ( sel == 8 );
            customDirLabel.setVisible ( isCustom );
            customDirEdit.setVisible ( isCustom );
            if ( isCustom )
            {
                if ( customDirEdit.getText().isNotEmpty() )
                    requestServerList();
                customDirEdit.grabKeyboardFocus();
            }
            else
            {
                requestServerList();
            }
            resized();
        };

        addAndMakeVisible ( customDirLabel );
        customDirLabel.setText ( "Custom Dir:", juce::dontSendNotification );
        customDirLabel.setColour ( juce::Label::textColourId, juce::Colours::white );
        customDirLabel.setVisible ( false );

        addAndMakeVisible ( customDirEdit );
        customDirEdit.setTextToShowWhenEmpty ( "host:port (e.g. mydir.example.com:22124)", juce::Colours::grey );
        customDirEdit.setVisible ( false );
        customDirEdit.onReturnKey = [this]() { requestServerList(); };

        // Search filter
        addAndMakeVisible ( filterLabel );
        filterLabel.setText ( "Filter:", juce::dontSendNotification );
        filterLabel.setColour ( juce::Label::textColourId, juce::Colours::white );

        addAndMakeVisible ( filterEdit );
        filterEdit.setFont ( juce::Font ( 14.0f ) );
        filterEdit.setBorder ( juce::BorderSize<int> ( 4, 6, 4, 6 ) );
        filterEdit.setTextToShowWhenEmpty ( "", juce::Colours::transparentBlack );
        filterEdit.onTextChange = [this]() {
            serverListModel.filterByText ( filterEdit.getText() );
            serverListTable.updateContent();
            updateEmptyState();
            updateFilterPlaceholder();
        };

        addAndMakeVisible ( filterPlaceholder );
        filterPlaceholder.setText ( "Search servers...", juce::dontSendNotification );
        filterPlaceholder.setColour ( juce::Label::textColourId, juce::Colours::grey );
        filterPlaceholder.setJustificationType ( juce::Justification::centredLeft );
        filterPlaceholder.setInterceptsMouseClicks ( false, false );

        // Show All Musicians Checkbox
        addAndMakeVisible ( showMusiciansButton );
        showMusiciansButton.setButtonText ( "Show All Musicians" );
        showMusiciansButton.setColour ( juce::ToggleButton::textColourId, juce::Colours::white );
        showMusiciansButton.onClick = [this] {
            serverListModel.setShowMusicians ( showMusiciansButton.getToggleState() );
            serverListTable.updateContent();
            serverListTable.repaint();

            if ( showMusiciansButton.getToggleState() )
            {
                const int numServersNow = directoryImpl ? directoryImpl->getNumServers() : 0;
                for ( int i = 0; i < numServersNow; ++i )
                {
                    jamulus_server_info_t info;
                    if ( directoryImpl && directoryImpl->getServerInfo ( i, info ) )
                    {
                        const juce::String players = juce::String::fromUTF8 ( info.players ).trim();
                        if ( info.numClients > 0 && players.isEmpty() && info.address[0] != '\0' )
                            directoryImpl->requestPlayers ( info.address );
                    }
                }
            }
        };

        // Server list table
        addAndMakeVisible ( serverListTable );
        serverListTable.setModel ( &serverListModel );
        serverListTable.setColour ( juce::ListBox::backgroundColourId, juce::Colour ( 0xff2a2a2a ) );
        serverListTable.setColour ( juce::ListBox::outlineColourId, juce::Colour ( 0xff505050 ) );
        serverListTable.setOutlineThickness ( 1 );
        serverListTable.setRowHeight ( 24 );

        auto& header = serverListTable.getHeader();
        header.addColumn ( "Server Name",
                           ServerListModel::Name,
                           180,
                           100,
                           360,
                           juce::TableHeaderComponent::defaultFlags | juce::TableHeaderComponent::sortable );
        header.addColumn ( "Ping",
                           ServerListModel::Ping,
                           70,
                           50,
                           100,
                           juce::TableHeaderComponent::defaultFlags | juce::TableHeaderComponent::sortable );
        header.addColumn ( "Clients",
                           ServerListModel::Clients,
                           90,
                           70,
                           140,
                           juce::TableHeaderComponent::defaultFlags | juce::TableHeaderComponent::sortable );
        header.addColumn ( "Location",
                           ServerListModel::Location,
                           150,
                           100,
                           260,
                           juce::TableHeaderComponent::defaultFlags | juce::TableHeaderComponent::sortable );
        header.addColumn ( "Version",
                           ServerListModel::Version,
                           120,
                           90,
                           180,
                           juce::TableHeaderComponent::defaultFlags | juce::TableHeaderComponent::sortable );
        header.setStretchToFitActive ( true );
        header.setSortColumnId ( ServerListModel::Ping, true ); // Default sort by ping ascending
        serverListModel.sortByColumn ( ServerListModel::Ping, true );

        serverListModel.onSortChanged = [this]() {
            serverListTable.updateContent();
            serverListTable.repaint();
            updateEmptyState();
        };

        addAndMakeVisible ( emptyLabel );
        emptyLabel.setColour ( juce::Label::textColourId, juce::Colours::grey );
        emptyLabel.setJustificationType ( juce::Justification::centred );
        emptyLabel.setInterceptsMouseClicks ( false, false );
        updateEmptyState();

        serverListModel.onServerSelected = [this] ( const ServerListItem& server ) {
            serverAddressEdit.setText ( server.address, false );
            onConnect ( server.address, server.name );
        };
        serverListModel.onRefreshMusicians = [this] ( const juce::String& addr ) {
            if ( onDirRequestPlayers )
                onDirRequestPlayers ( addr );
        };

        // Server address manual entry
        addAndMakeVisible ( addressLabel );
        addressLabel.setText ( "Server Address:", juce::dontSendNotification );
        addressLabel.setColour ( juce::Label::textColourId, juce::Colours::white );
        addressLabel.setJustificationType ( juce::Justification::centredLeft );

        addAndMakeVisible ( serverAddressEdit );
        serverAddressEdit.setTextToShowWhenEmpty ( "hostname:port", juce::Colours::grey );
        serverAddressEdit.setJustification ( juce::Justification::centredLeft );
        serverAddressEdit.setIndents ( 4, 1 ); // Slight top indent to center vertically
        serverAddressEdit.setWantsKeyboardFocus ( true );
        serverAddressEdit.setMouseClickGrabsKeyboardFocus ( true );
        serverAddressEdit.onReturnKey = [this]() {
            juce::String addr = serverAddressEdit.getText();
            if ( addr.isNotEmpty() )
                onConnect ( addr, {} );
        };

        setWantsKeyboardFocus ( true );
        setMouseClickGrabsKeyboardFocus ( true );

        // Recent servers
        addAndMakeVisible ( recentLabel );
        recentLabel.setText ( "Recent:", juce::dontSendNotification );
        recentLabel.setColour ( juce::Label::textColourId, juce::Colours::white );

        addAndMakeVisible ( recentCombo );
        recentCombo.addItem ( "(No recent servers)", 1 );
        recentCombo.setSelectedId ( 1 );
        recentCombo.onChange = [this]() {
            if ( recentCombo.getSelectedId() > 1 )
            {
                // TODO: Load from recent servers list
            }
        };

        // Buttons
        addAndMakeVisible ( connectButton );
        connectButton.setButtonText ( "Connect" );
        connectButton.onClick = [this]() {
            juce::String addr = serverAddressEdit.getText();
            if ( addr.isNotEmpty() )
                onConnect ( addr, {} );
        };

        addAndMakeVisible ( cancelButton );
        cancelButton.setButtonText ( "Cancel" );
        cancelButton.onClick = [this]() {
            if ( onCancel )
                onCancel();
        };

        // Start with faster refresh rate
        startTimerHz ( 5 );

        // Request initial server list
        requestServerList();
    }

    ~ConnectComponent() override { stopTimer(); }

    void triggerInitialRequest()
    {
        requestServerList();
    }

    bool keyPressed ( const juce::KeyPress& key ) override
    {
        if ( key == juce::KeyPress ( juce::KeyPress::escapeKey ) )
        {
            if ( onCancel )
            {
                onCancel();
                return true;
            }
        }
        if ( key == juce::KeyPress ( juce::KeyPress::returnKey ) )
        {
            auto sel = serverListModel.getSelectedServer();
            if ( sel.address.isNotEmpty() )
            {
                onConnect ( sel.address, sel.name );
                return true;
            }
            juce::String addr = serverAddressEdit.getText();
            if ( addr.isNotEmpty() )
            {
                onConnect ( addr, {} );
                return true;
            }
        }
        if ( key.getKeyCode() == juce::KeyPress::F5Key )
        {
            requestServerList();
            return true;
        }
        if ( key.getModifiers().isCtrlDown() && ( key.getKeyCode() == 'L' || key.getTextCharacter() == 'l' ) )
        {
            serverAddressEdit.grabKeyboardFocus();
            return true;
        }
        return false;
    }

    void focusGained ( juce::Component::FocusChangeType ) override
    {
        // Forward focus to the input field
        serverAddressEdit.grabKeyboardFocus();
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

        // Header
        titleLabel.setBounds ( bounds.removeFromTop ( 40 ) );

        // Footer with buttons
        auto footer     = bounds.removeFromBottom ( 50 );
        auto buttonArea = footer.reduced ( 20, 10 );
        cancelButton.setBounds ( buttonArea.removeFromRight ( 100 ) );
        buttonArea.removeFromRight ( 10 );
        connectButton.setBounds ( buttonArea.removeFromRight ( 100 ) );

        // Content
        bounds = bounds.reduced ( 20, 10 );

        // Directory and filter row
        auto topRow = bounds.removeFromTop ( 30 );
        directoryLabel.setBounds ( topRow.removeFromLeft ( 70 ) );
        directoryCombo.setBounds ( topRow.removeFromLeft ( 150 ) );
        topRow.removeFromLeft ( 20 );
        filterLabel.setBounds ( topRow.removeFromLeft ( 50 ) );
        filterEdit.setBounds ( topRow );
        {
            auto r = filterEdit.getBounds();
            r      = r.reduced ( 4, 0 );
            filterPlaceholder.setBounds ( r );
        }

        bounds.removeFromTop ( 5 );

        // Custom directory row (only visible when selected)
        if ( customDirEdit.isVisible() )
        {
            auto customRow = bounds.removeFromTop ( 26 );
            customDirLabel.setBounds ( customRow.removeFromLeft ( 110 ) );
            customDirEdit.setBounds ( customRow );
            bounds.removeFromTop ( 5 );
        }

        // Show Musicians Button
        showMusiciansButton.setBounds ( bounds.removeFromTop ( 20 ).removeFromLeft ( 150 ) );

        bounds.removeFromTop ( 5 );

        // Server list
        auto listArea = bounds.removeFromTop ( bounds.getHeight() - 80 );
        serverListTable.setBounds ( listArea );
        emptyLabel.setBounds ( listArea );

        bounds.removeFromTop ( 10 );

        // Address entry row
        auto addressRow = bounds.removeFromTop ( 30 );
        addressLabel.setBounds ( addressRow.removeFromLeft ( 110 ) );
        serverAddressEdit.setBounds ( addressRow );

        bounds.removeFromTop ( 10 );

        // Recent servers row
        auto recentRow = bounds.removeFromTop ( 30 );
        recentLabel.setBounds ( recentRow.removeFromLeft ( 110 ) );
        recentCombo.setBounds ( recentRow );
    }

    std::function<void ( const juce::String& )> onConnectRequested;
    std::function<void ( const juce::String&, const juce::String& )> onConnectRequestedDetailed;
    std::function<void()>                       onCancel;

    void setDirectoryCallbacks ( std::function<void()> clearCb,
                                 std::function<void ( int, const juce::String& )> requestListCb,
                                 std::function<int()> getNumServersCb,
                                 std::function<bool ( int, jamulus_server_info_t& )> getServerInfoCb,
                                 std::function<void ( const juce::String& )> requestPlayersCb,
                                 std::function<void()> pingServerListCb )
    {
        onDirClear           = std::move ( clearCb );
        onDirRequestList     = std::move ( requestListCb );
        onDirGetNumServers   = std::move ( getNumServersCb );
        onDirGetServerInfo   = std::move ( getServerInfoCb );
        onDirRequestPlayers  = std::move ( requestPlayersCb );
        onDirPingServerList  = std::move ( pingServerListCb );
    }

private:
    struct IServerDirectory
    {
        virtual ~IServerDirectory() = default;
        virtual void clear()                                           = 0;
        virtual void requestList ( int dirType, const juce::String& )  = 0;
        virtual int  getNumServers()                                   = 0;
        virtual bool getServerInfo ( int index, jamulus_server_info_t& ) = 0;
        virtual void requestPlayers ( const juce::String& addr )       = 0;
        virtual void pingTick()                                        = 0;
    };

    struct WrapperDirectory : public IServerDirectory
    {
        explicit WrapperDirectory ( ConnectComponent& o ) : owner ( o ) {}
        void clear() override
        {
            if ( owner.onDirClear )
                owner.onDirClear();
        }
        void requestList ( int dirType, const juce::String& custom ) override
        {
            if ( owner.onDirRequestList )
                owner.onDirRequestList ( dirType, custom );
        }
        int getNumServers() override
        {
            return owner.onDirGetNumServers ? owner.onDirGetNumServers() : 0;
        }
        bool getServerInfo ( int index, jamulus_server_info_t& info ) override
        {
            return owner.onDirGetServerInfo ? owner.onDirGetServerInfo ( index, info ) : false;
        }
        void requestPlayers ( const juce::String& addr ) override
        {
            if ( owner.onDirRequestPlayers )
                owner.onDirRequestPlayers ( addr );
        }
        void pingTick() override
        {
            if ( owner.onDirPingServerList )
                owner.onDirPingServerList();
        }
        ConnectComponent& owner;
    };

    struct WrapperListJucePingDirectory : public IServerDirectory
    {
        explicit WrapperListJucePingDirectory ( ConnectComponent& o ) : owner ( o ), socket ( false )
        {
            socket.setEnablePortReuse ( true );
            socket.bindToPort ( 0 );
            worker.reset ( new PingWorker ( *this ) );
            worker->startThread();
        }
        ~WrapperListJucePingDirectory() override
        {
            if ( worker )
            {
                worker->stopThread ( 1000 );
                worker.reset();
            }
        }
        static constexpr uint16_t CLM_PING_MS = 1001;
        static constexpr uint16_t CLM_PING_MS_WITHNUMCLIENTS = 1002;
        static constexpr int HEADER_LEN = 7;
        void clear() override
        {
            if ( owner.onDirClear )
                owner.onDirClear();
            pingData.clear();
        }
        void requestList ( int dirType, const juce::String& custom ) override
        {
            if ( owner.onDirRequestList )
                owner.onDirRequestList ( dirType, custom );
        }
        int getNumServers() override
        {
            return owner.onDirGetNumServers ? owner.onDirGetNumServers() : 0;
        }
        bool getServerInfo ( int index, jamulus_server_info_t& info ) override
        {
            return owner.onDirGetServerInfo ? owner.onDirGetServerInfo ( index, info ) : false;
        }
        void requestPlayers ( const juce::String& addr ) override
        {
            if ( owner.onDirRequestPlayers )
                owner.onDirRequestPlayers ( addr );
        }
        void pingTick() override { kick.signal(); }
        struct PingEntry { int pingMs = -1; int numClients = -1; juce::uint32 lastSentMs = 0; };
        std::unordered_map<std::string, PingEntry> pingData;
        std::unordered_map<std::string, std::string> ipToKey;
        ConnectComponent& owner;
        juce::DatagramSocket socket;
        int nextIndex = 0;
        juce::WaitableEvent kick;
        static bool parseAddress ( const juce::String& a, juce::String& host, int& port )
        {
            juce::String s = a.trim();
            host           = {};
            port           = 22124;
            if ( s.isEmpty() ) return false;
            if ( s.startsWithChar ( '[' ) )
            {
                auto r = s.indexOfChar ( ']' );
                if ( r > 1 )
                {
                    host = s.substring ( 1, r );
                    if ( r + 1 < s.length() && s[r + 1] == ':' )
                    {
                        juce::String p = s.substring ( r + 2 );
                        if ( p.containsOnly ( "0123456789" ) ) port = p.getIntValue();
                    }
                    return host.isNotEmpty();
                }
            }
            const int lastColon = s.lastIndexOfChar ( ':' );
            if ( lastColon > 0 && s.indexOfChar ( ':' ) == lastColon )
            {
                host = s.substring ( 0, lastColon );
                juce::String p = s.substring ( lastColon + 1 );
                if ( p.containsOnly ( "0123456789" ) ) port = p.getIntValue();
                return host.isNotEmpty();
            }
            // Likely raw IPv6 without port or bare hostname
            host = s;
            return true;
        }
        static bool isIpLiteral ( const juce::String& h )
        {
            return h.containsChar ( '.' ) || h.containsChar ( ':' );
        }
        static juce::String toKey ( const juce::String& ip, int port )
        {
            if ( ip.containsChar ( ':' ) && !ip.startsWith ( "[" ) ) return "[" + ip + "]:" + juce::String ( port );
            return ip.containsChar ( ':' ) ? ( ip + ":" + juce::String ( port ) ) : ( ip + ":" + juce::String ( port ) );
        }
        void pingBatch()
        {
            if ( owner.onDirPingServerList )
                owner.onDirPingServerList();
        }
        struct PingWorker : public juce::Thread
        {
            WrapperListJucePingDirectory& o;
            explicit PingWorker ( WrapperListJucePingDirectory& oo ) : juce::Thread ( "DirPing" ), o ( oo ) {}
            void run() override
            {
                for (;;)
                {
                    if ( threadShouldExit() ) break;
                    o.pingBatch();
                    o.kick.wait ( 2500 );
                }
            }
        };
        std::unique_ptr<PingWorker> worker;
        static void putLE ( std::vector<uint8_t>& v, uint32_t val, int bytes )
        {
            for ( int i = 0; i < bytes; ++i ) v.push_back ( (uint8_t) ( ( val >> ( i * 8 ) ) & 0xff ) );
        }
        static uint16_t crc16 ( const uint8_t* data, int len )
        {
            uint32_t poly = ( 1u << 5 ) | ( 1u << 12 );
            uint32_t bitOutMask = ( 1u << 16 );
            uint32_t state = ~uint32_t ( 0 );
            for ( int idx = 0; idx < len; ++idx )
            {
                uint8_t b = data[idx];
                for ( int i = 0; i < 8; ++i )
                {
                    state <<= 1;
                    if ( ( state & bitOutMask ) != 0 ) state |= 1u;
                    if ( ( b & ( 1u << ( 7 - i ) ) ) != 0 ) state ^= 1u;
                    if ( ( state & 1u ) != 0 ) state ^= poly;
                }
            }
            state = ~state;
            return (uint16_t) ( state & ( bitOutMask - 1 ) );
        }
        static std::vector<uint8_t> buildPingFrame ( uint32_t nowMs )
        {
            std::vector<uint8_t> out;
            putLE ( out, 0, 2 );
            putLE ( out, (uint32_t) CLM_PING_MS_WITHNUMCLIENTS, 2 );
            putLE ( out, 0, 1 );
            putLE ( out, 5, 2 );
            putLE ( out, nowMs, 4 );
            putLE ( out, 0, 1 ); // dummy num-clients as per Jamulus client
            const uint16_t c = crc16 ( out.data(), (int) out.size() );
            putLE ( out, c, 2 );
            return out;
        }
        void handleReply ( const uint8_t* buf, int len, const juce::String& ip, int port )
        {
            if ( len < HEADER_LEN ) return;
            const uint16_t tag = (uint16_t) ( buf[0] | ( buf[1] << 8 ) );
            if ( tag != 0 ) return;
            const uint16_t id = (uint16_t) ( buf[2] | ( buf[3] << 8 ) );
            // cnt at [4], length (little-endian) at [5] (LSB) and [6] (MSB)
            const uint16_t dataLen = (uint16_t) ( buf[5] | ( buf[6] << 8 ) );
            const int total = HEADER_LEN + dataLen + 2;
            if ( len < total ) return;
            const uint16_t gotCrc = (uint16_t) ( buf[HEADER_LEN + dataLen] | ( buf[HEADER_LEN + dataLen + 1] << 8 ) );
            const uint16_t calcCrc = crc16 ( buf, HEADER_LEN + dataLen );
            if ( gotCrc != calcCrc ) return;
            if ( id != CLM_PING_MS_WITHNUMCLIENTS && id != CLM_PING_MS ) return;
            if ( dataLen < 4 ) return;
            uint32_t txMs = (uint32_t) ( buf[7] | ( buf[8] << 8 ) | ( buf[9] << 16 ) | ( buf[10] << 24 ) );
            int numC = -1;
            if ( id == CLM_PING_MS_WITHNUMCLIENTS && dataLen >= 5 ) numC = buf[11];
            const juce::uint32 nowMs = juce::Time::getMillisecondCounter();
            int rtt = (int) juce::jlimit ( 0, 10000, (int) ( nowMs - txMs ) );
            bool mapped = false;
            for ( auto& kv : pingData )
            {
                auto& e = kv.second;
                if ( e.lastSentMs == txMs )
                {
                    e.pingMs = rtt;
                    if ( numC >= 0 ) e.numClients = numC;
                    mapped = true;
                    break;
                }
            }
            if ( !mapped )
            {
                juce::String ipKey = toKey ( ip, port );
                auto         it    = ipToKey.find ( ipKey.toStdString() );
                std::string  finalKey = ( it != ipToKey.end() ? it->second : ipKey.toStdString() );
                auto&        e        = pingData[finalKey];
                e.pingMs = rtt;
                if ( numC >= 0 ) e.numClients = numC;
            }
        }
    };

    void timerCallback() override
    {
        if ( !isShowing() )
            return;
        updateServerListFromClient();

        const auto nowMs = juce::Time::getMillisecondCounter();
        if ( serverInfoIncomplete && directoryImpl && ( nowMs - lastServerEnrichPingMs ) >= 250 )
        {
            lastServerEnrichPingMs = nowMs;
            directoryImpl->pingTick();
        }

        pingCount++;
        if ( !serverInfoIncomplete && pingCount >= 5 )
        {
            pingCount = 0;
            if ( guiReady && ( juce::Time::getMillisecondCounter() - guiReadyTimeMs ) > 500 )
            {
                if ( directoryImpl && isShowing() )
                    directoryImpl->pingTick();
            }
        }
    }

    void requestServerList()
    {
        if ( !directoryImpl )
            return;

        directoryImpl->clear();
        serverListModel.clearServers();
        serverListTable.updateContent();
        updateEmptyState();

        int sel = directoryCombo.getSelectedId();
        int dirType = 0;
        if ( sel == 8 )
        {
            juce::String addr = customDirEdit.getText().trim();
            if ( addr.isEmpty() )
                return;
            dirType = JAMULUS_DIRECTORY_CUSTOM;
            if ( directoryImpl ) directoryImpl->requestList ( dirType, addr );
        } else
        {
            dirType = sel - 1;
            if ( dirType < 0 || dirType > 7 )
                dirType = 0;
            if ( directoryImpl ) directoryImpl->requestList ( dirType, {} );
        }

        requestPending = true;
        requestTime    = juce::Time::getMillisecondCounter();
        serverInfoIncomplete = true;
        lastServerEnrichPingMs = 0;
    }

    void updateServerListFromClient()
    {
        if ( !directoryImpl )
            return;

        int prevCount  = lastServerCount;
        int numServers = directoryImpl->getNumServers();

        bool needsRepaint = false;
        bool incompleteInfo = false;

        for ( int i = 0; i < numServers; ++i )
        {
            jamulus_server_info_t info;
            if ( directoryImpl->getServerInfo ( i, info ) )
            {
                ServerListItem item;
                item.name       = juce::String::fromUTF8 ( info.name );
                if ( item.name.trim().isEmpty() )
                    continue;
                item.address    = juce::String::fromUTF8 ( info.address );
                item.ping       = info.ping;
                item.numClients = info.numClients;
                item.maxClients = info.maxClients;
                item.location   = juce::String::fromUTF8 ( info.city );
                if ( item.location.isEmpty() )
                    item.location = juce::String::fromUTF8 ( info.country );
                else if ( info.country[0] != '\0' )
                    item.location += ", " + juce::String::fromUTF8 ( info.country );
                item.version  = juce::String::fromUTF8 ( info.version );
                item.isFull = ( info.maxClients > 0 && info.numClients >= info.maxClients );
                if ( item.ping < 0 || item.version.isEmpty() )
                    incompleteInfo = true;

                if ( info.players[0] != '\0' )
                {
                    item.players = juce::String::fromUTF8 ( info.players );
                    juce::StringArray lines;
                    lines.addTokens ( item.players, "\n", "" );
                    for ( auto& line : lines )
                    {
                        PlayerListItem    p;
                        juce::StringArray parts;
                        parts.addTokens ( line, "|", "" );
                        if ( parts.size() >= 1 )
                            p.name = parts[0];
                        if ( parts.size() >= 2 )
                            p.instrumentId = parts[1].getIntValue();
                        if ( parts.size() >= 3 )
                            p.countryCode = parts[2];
                        item.playerItems.push_back ( p );
                    }
                }

                serverListModel.addServer ( item );

                if ( serverListModel.isShowingMusicians() && info.numClients > 0 && item.playerItems.empty() )
                {
                    if ( directoryImpl ) directoryImpl->requestPlayers ( info.address );
                }
                needsRepaint = true;
            }
        }

        if ( needsRepaint || numServers != lastServerCount )
        {
            lastServerCount = numServers;
            serverInfoIncomplete = incompleteInfo;
            // Re-apply current sort order
            serverListModel.sortByColumn ( serverListModel.getCurrentSortColumn(), serverListModel.getCurrentSortForward() );
            serverListTable.updateContent();
            serverListTable.repaint();
            updateEmptyState();
            if ( prevCount == 0 && numServers > 0 && directoryImpl )
            {
                directoryImpl->pingTick();
                lastServerEnrichPingMs = juce::Time::getMillisecondCounter();
            }
        }
        else
        {
            serverInfoIncomplete = incompleteInfo;
        }

        if ( requestPending && numServers == 0 )
        {
            uint32_t elapsed = juce::Time::getMillisecondCounter() - requestTime;
            if ( elapsed > 3000 )
            {
                int dirType = 0;
                int sel = directoryCombo.getSelectedId();
                if ( sel == 8 )
                {
                    juce::String addr = customDirEdit.getText().trim();
                    if ( addr.isNotEmpty() )
                    {
                        dirType = JAMULUS_DIRECTORY_CUSTOM;
                        if ( directoryImpl ) directoryImpl->requestList ( dirType, addr );
                    }
                }
                else
                {
                    dirType = sel - 1;
                    if ( dirType < 0 || dirType > 7 )
                        dirType = 0;
                    if ( directoryImpl ) directoryImpl->requestList ( dirType, {} );
                }
                requestTime = juce::Time::getMillisecondCounter();
            }
        }
        else if ( numServers > 0 )
        {
            requestPending = false;
        }
    }

    void onConnect ( const juce::String& address, const juce::String& serverName )
    {
        const auto normalized = normalizeAddress ( address );

        if ( onConnectRequestedDetailed )
            onConnectRequestedDetailed ( normalized, serverName );

        if ( onConnectRequested )
            onConnectRequested ( normalized );
    }

    juce::String normalizeAddress ( juce::String a )
    {
        a = a.trim();
        auto lower = a.toLowerCase();
        if ( lower.startsWith ( "jamulus://" ) )
            a = a.substring ( 10 );
        else if ( lower.startsWith ( "udp://" ) )
            a = a.substring ( 6 );
        if ( !a.containsChar ( ':' ) )
            a += ":22124";
        else
        {
            if ( !a.startsWithChar ( '[' ) )
            {
                auto lastColon = a.lastIndexOfChar ( ':' );
                if ( lastColon > 0 )
                {
                    auto host = a.substring ( 0, lastColon );
                    auto port = a.substring ( lastColon + 1 );
                    if ( host.containsChar ( ':' ) && !host.startsWithChar ( '[' ) )
                        a = "[" + host + "]:" + port;
                }
            }
        }
        return a;
    }

    void updateEmptyState()
    {
        bool isEmpty = ( serverListModel.getNumRows() == 0 );
        emptyLabel.setVisible ( isEmpty );
        if ( isEmpty )
            emptyLabel.setText ( "No servers found. Check directory or filter.", juce::dontSendNotification );
    }

    void updateFilterPlaceholder()
    {
        bool empty   = filterEdit.getText().isEmpty();
        bool focused = filterEdit.hasKeyboardFocus ( true );
        filterPlaceholder.setVisible ( empty && !focused );
    }

    jamulus_client_t jamulusClient;
    int              lastServerCount = 0;
    bool             requestPending  = false;
    uint32_t         requestTime     = 0;
    int              pingCount       = 0;
    bool             serverInfoIncomplete = false;
    juce::uint32     lastServerEnrichPingMs = 0;
    std::unique_ptr<IServerDirectory> directoryImpl;

    std::function<void()>                                 onDirClear;
    std::function<void ( int, const juce::String& )>      onDirRequestList;
    std::function<int()>                                  onDirGetNumServers;
    std::function<bool ( int, jamulus_server_info_t& )>   onDirGetServerInfo;
    std::function<void ( const juce::String& )>           onDirRequestPlayers;
    std::function<void()>                                 onDirPingServerList;

    juce::Label        titleLabel;
    juce::Label        directoryLabel;
    juce::ComboBox     directoryCombo;
    juce::Label        customDirLabel;
    juce::TextEditor   customDirEdit;
    juce::Label        filterLabel;
    juce::TextEditor   filterEdit;
    juce::Label        filterPlaceholder;
    juce::ToggleButton showMusiciansButton;

    juce::TableListBox serverListTable;
    ServerListModel    serverListModel;
    juce::TooltipWindow tooltipWindow { this, 300 };
    juce::Label        emptyLabel;

    juce::Label      addressLabel;
    juce::TextEditor serverAddressEdit;
    juce::Label      recentLabel;
    juce::ComboBox   recentCombo;

    juce::TextButton connectButton;
    juce::TextButton cancelButton;

    // Only start pinging once GUI is fully shown
    bool        guiReady      = false;
    juce::uint32 guiReadyTimeMs = 0;

    void visibilityChanged() override
    {
        if ( isShowing() && !guiReady )
        {
            guiReady       = true;
            guiReadyTimeMs = juce::Time::getMillisecondCounter();
        }
    }
};
