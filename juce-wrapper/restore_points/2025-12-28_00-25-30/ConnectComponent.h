#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../jamulus_wrapper.h"

//==============================================================================
// Server List Item (for display in the table)
//==============================================================================
struct ServerListItem
{
    juce::String name;
    juce::String address;
    int ping = -1;
    int numClients = 0;
    int maxClients = 0;
    juce::String location;
    juce::String version;
    bool isFull = false;
};

//==============================================================================
// Server List Table Model
//==============================================================================
class ServerListModel : public juce::TableListBoxModel
{
public:
    enum Columns { Name = 1, Ping, Clients, Location };
    
    int getNumRows() override { return static_cast<int>(servers.size()); }
    
    void paintRowBackground(juce::Graphics& g, int rowNumber, int /*width*/, int /*height*/, bool rowIsSelected) override
    {
        if (rowIsSelected)
            g.fillAll(juce::Colour(0xff4a6fa5));
        else if (rowNumber % 2)
            g.fillAll(juce::Colour(0xff383838));
        else
            g.fillAll(juce::Colour(0xff303030));
    }
    
    void paintCell(juce::Graphics& g, int rowNumber, int columnId, int width, int height, bool /*rowIsSelected*/) override
    {
        if (rowNumber >= static_cast<int>(servers.size())) return;
        
        const auto& server = servers[static_cast<size_t>(rowNumber)];
        
        g.setColour(juce::Colours::white);
        g.setFont(13.0f);
        
        juce::String text;
        switch (columnId)
        {
            case Name:     text = server.name; break;
            case Ping:     text = server.ping >= 0 ? juce::String(server.ping) + " ms" : "--"; break;
            case Clients:  text = juce::String(server.numClients) + "/" + juce::String(server.maxClients); break;
            case Location: text = server.location; break;
        }
        
        // Color ping by quality
        if (columnId == Ping && server.ping >= 0)
        {
            if (server.ping < 40)
                g.setColour(juce::Colour(0xff00cc66));
            else if (server.ping < 80)
                g.setColour(juce::Colours::yellow);
            else
                g.setColour(juce::Colours::red);
        }
        
        g.drawText(text, 4, 0, width - 8, height, juce::Justification::centredLeft);
    }
    
    void cellDoubleClicked(int rowNumber, int /*columnId*/, const juce::MouseEvent&) override
    {
        if (rowNumber >= 0 && rowNumber < static_cast<int>(servers.size()) && onServerSelected)
            onServerSelected(servers[static_cast<size_t>(rowNumber)]);
    }
    
    void selectedRowsChanged(int lastRowSelected) override
    {
        if (lastRowSelected >= 0 && lastRowSelected < static_cast<int>(servers.size()))
            selectedServer = servers[static_cast<size_t>(lastRowSelected)];
    }
    
    void addServer(const ServerListItem& server)
    {
        // Update existing or add new
        for (auto& s : servers)
        {
            if (s.address == server.address)
            {
                s = server;
                return;
            }
        }
        servers.push_back(server);
    }
    
    void clearServers() { servers.clear(); }
    
    void sortByColumn(int columnId, bool forwards)
    {
        currentSortColumn = columnId;
        currentSortForward = forwards;
        
        auto comparator = [columnId, forwards](const ServerListItem& a, const ServerListItem& b) {
            int result = 0;
            switch (columnId)
            {
                case Name:     result = a.name.compareNatural(b.name); break;
                case Ping:
                    // Sort unknown pings (-1) to the end
                    if (a.ping < 0 && b.ping < 0) result = 0;
                    else if (a.ping < 0) result = 1;
                    else if (b.ping < 0) result = -1;
                    else result = a.ping - b.ping;
                    break;
                case Clients:  result = a.numClients - b.numClients; break;
                case Location: result = a.location.compareNatural(b.location); break;
            }
            return forwards ? result < 0 : result > 0;
        };
        std::sort(servers.begin(), servers.end(), comparator);
    }
    
    // Called by TableListBox when header is clicked
    void sortOrderChanged(int newSortColumnId, bool isForwards) override
    {
        sortByColumn(newSortColumnId, isForwards);
        if (onSortChanged)
            onSortChanged();
    }
    
    int getCurrentSortColumn() const { return currentSortColumn; }
    bool getCurrentSortForward() const { return currentSortForward; }
    
    void filterByText(const juce::String& filter)
    {
        if (filter.isEmpty())
        {
            filteredServers.clear();
            return;
        }
        
        filteredServers.clear();
        for (const auto& server : servers)
        {
            if (server.name.containsIgnoreCase(filter) ||
                server.location.containsIgnoreCase(filter))
            {
                filteredServers.push_back(server);
            }
        }
    }
    
    ServerListItem getSelectedServer() const { return selectedServer; }
    
    std::function<void(const ServerListItem&)> onServerSelected;
    std::function<void()> onSortChanged;
    
private:
    std::vector<ServerListItem> servers;
    std::vector<ServerListItem> filteredServers;
    ServerListItem selectedServer;
    int currentSortColumn = Name;
    bool currentSortForward = true;
};

//==============================================================================
// Connect Dialog Component
//==============================================================================
class ConnectComponent : public juce::Component, private juce::Timer
{
public:
    ConnectComponent(jamulus_client_t client)
        : jamulusClient(client)
    {
        // Title
        addAndMakeVisible(titleLabel);
        titleLabel.setText("Connect to Server", juce::dontSendNotification);
        titleLabel.setFont(juce::Font(20.0f, juce::Font::bold));
        titleLabel.setColour(juce::Label::textColourId, juce::Colours::white);
        titleLabel.setJustificationType(juce::Justification::centred);
        
        // Directory selector
        addAndMakeVisible(directoryLabel);
        directoryLabel.setText("Directory:", juce::dontSendNotification);
        directoryLabel.setColour(juce::Label::textColourId, juce::Colours::white);
        
        addAndMakeVisible(directoryCombo);
        directoryCombo.addItem("Any Genre 1", 1);
        directoryCombo.addItem("Any Genre 2", 2);
        directoryCombo.addItem("Any Genre 3", 3);
        directoryCombo.addItem("Genre Rock", 4);
        directoryCombo.addItem("Genre Jazz", 5);
        directoryCombo.addItem("Genre Classical/Folk", 6);
        directoryCombo.addItem("Genre Choral/Barbershop", 7);
        directoryCombo.setSelectedId(1);
        directoryCombo.onChange = [this]() {
            requestServerList();
        };
        
        // Search filter
        addAndMakeVisible(filterLabel);
        filterLabel.setText("Filter:", juce::dontSendNotification);
        filterLabel.setColour(juce::Label::textColourId, juce::Colours::white);
        
        addAndMakeVisible(filterEdit);
        filterEdit.setTextToShowWhenEmpty("Search servers...", juce::Colours::grey);
        filterEdit.onTextChange = [this]() {
            serverListModel.filterByText(filterEdit.getText());
            serverListTable.updateContent();
        };
        
        // Server list table
        addAndMakeVisible(serverListTable);
        serverListTable.setModel(&serverListModel);
        serverListTable.setColour(juce::ListBox::backgroundColourId, juce::Colour(0xff2a2a2a));
        serverListTable.setColour(juce::ListBox::outlineColourId, juce::Colour(0xff505050));
        serverListTable.setOutlineThickness(1);
        serverListTable.setRowHeight(24);
        
        auto& header = serverListTable.getHeader();
        header.addColumn("Server Name", ServerListModel::Name, 200, 100, 400, 
                         juce::TableHeaderComponent::defaultFlags | juce::TableHeaderComponent::sortable);
        header.addColumn("Ping", ServerListModel::Ping, 70, 50, 100,
                         juce::TableHeaderComponent::defaultFlags | juce::TableHeaderComponent::sortable);
        header.addColumn("Clients", ServerListModel::Clients, 70, 50, 100,
                         juce::TableHeaderComponent::defaultFlags | juce::TableHeaderComponent::sortable);
        header.addColumn("Location", ServerListModel::Location, 150, 100, 300,
                         juce::TableHeaderComponent::defaultFlags | juce::TableHeaderComponent::sortable);
        header.setStretchToFitActive(true);
        header.setSortColumnId(ServerListModel::Ping, true);  // Default sort by ping ascending
        
        serverListModel.onSortChanged = [this]() {
            serverListTable.updateContent();
            serverListTable.repaint();
        };
        
        serverListModel.onServerSelected = [this](const ServerListItem& server) {
            serverAddressEdit.setText(server.address, false);
            onConnect(server.address);
        };
        
        // Server address manual entry
        addAndMakeVisible(addressLabel);
        addressLabel.setText("Server Address:", juce::dontSendNotification);
        addressLabel.setColour(juce::Label::textColourId, juce::Colours::white);
        
        addAndMakeVisible(serverAddressEdit);
        serverAddressEdit.setTextToShowWhenEmpty("hostname:port", juce::Colours::grey);
        
        // Recent servers
        addAndMakeVisible(recentLabel);
        recentLabel.setText("Recent:", juce::dontSendNotification);
        recentLabel.setColour(juce::Label::textColourId, juce::Colours::white);
        
        addAndMakeVisible(recentCombo);
        recentCombo.addItem("(No recent servers)", 1);
        recentCombo.setSelectedId(1);
        recentCombo.onChange = [this]() {
            if (recentCombo.getSelectedId() > 1)
            {
                // TODO: Load from recent servers list
            }
        };
        
        // Buttons
        addAndMakeVisible(connectButton);
        connectButton.setButtonText("Connect");
        connectButton.onClick = [this]() {
            juce::String addr = serverAddressEdit.getText();
            if (addr.isNotEmpty())
                onConnect(addr);
        };
        
        addAndMakeVisible(cancelButton);
        cancelButton.setButtonText("Cancel");
        cancelButton.onClick = [this]() {
            if (onCancel)
                onCancel();
        };
        
        // Start with faster refresh rate
        startTimerHz(5);
        
        // Request initial server list
        requestServerList();
    }
    
    ~ConnectComponent() override { stopTimer(); }
    
    void paint(juce::Graphics& g) override
    {
        g.fillAll(juce::Colour(0xff323232));
        
        // Header
        g.setColour(juce::Colour(0xff252525));
        g.fillRect(0, 0, getWidth(), 40);
    }
    
    void resized() override
    {
        auto bounds = getLocalBounds();
        
        // Header
        titleLabel.setBounds(bounds.removeFromTop(40));
        
        // Footer with buttons
        auto footer = bounds.removeFromBottom(50);
        auto buttonArea = footer.reduced(20, 10);
        cancelButton.setBounds(buttonArea.removeFromRight(100));
        buttonArea.removeFromRight(10);
        connectButton.setBounds(buttonArea.removeFromRight(100));
        
        // Content
        bounds = bounds.reduced(20, 10);
        
        // Directory and filter row
        auto topRow = bounds.removeFromTop(30);
        directoryLabel.setBounds(topRow.removeFromLeft(70));
        directoryCombo.setBounds(topRow.removeFromLeft(150));
        topRow.removeFromLeft(20);
        filterLabel.setBounds(topRow.removeFromLeft(50));
        filterEdit.setBounds(topRow);
        
        bounds.removeFromTop(10);
        
        // Server list
        auto listArea = bounds.removeFromTop(bounds.getHeight() - 80);
        serverListTable.setBounds(listArea);
        
        bounds.removeFromTop(10);
        
        // Address entry row
        auto addressRow = bounds.removeFromTop(30);
        addressLabel.setBounds(addressRow.removeFromLeft(110));
        serverAddressEdit.setBounds(addressRow);
        
        bounds.removeFromTop(10);
        
        // Recent servers row
        auto recentRow = bounds.removeFromTop(30);
        recentLabel.setBounds(recentRow.removeFromLeft(110));
        recentCombo.setBounds(recentRow);
    }
    
    std::function<void(const juce::String&)> onConnectRequested;
    std::function<void()> onCancel;
    
private:
    void timerCallback() override
    {
        // Update server list from the client
        updateServerListFromClient();
        
        // Periodically ping servers to get/update ping times
        pingCount++;
        if (pingCount >= 5)  // Ping every ~1 second (5 * 200ms)
        {
            pingCount = 0;
            if (jamulusClient)
                jamulus_client_ping_server_list(jamulusClient);
        }
    }
    
    void requestServerList()
    {
        if (!jamulusClient) return;
        
        // Clear current list
        jamulus_client_clear_server_list(jamulusClient);
        serverListModel.clearServers();
        serverListTable.updateContent();
        
        // Map combo selection to directory type (matches EDirectoryType enum)
        int dirType = directoryCombo.getSelectedId() - 1;
        if (dirType < 0 || dirType > 6) dirType = 0;
        
        // Request server list
        jamulus_client_request_server_list(jamulusClient, dirType);
        
        // Set flag to indicate we're waiting
        requestPending = true;
        requestTime = juce::Time::getMillisecondCounter();
    }
    
    void updateServerListFromClient()
    {
        if (!jamulusClient) return;
        
        int numServers = jamulus_client_get_num_servers(jamulusClient);
        
        // Always update the model to catch ping/client count updates
        bool needsRepaint = false;
        
        for (int i = 0; i < numServers; ++i)
        {
            jamulus_server_info_t info;
            if (jamulus_client_get_server_info(jamulusClient, i, &info))
            {
                ServerListItem item;
                item.name = juce::String(info.name);
                item.address = juce::String(info.address);
                item.ping = info.ping;
                item.numClients = info.numClients;
                item.maxClients = info.maxClients;
                item.location = juce::String(info.city);
                if (item.location.isEmpty())
                    item.location = juce::String(info.country);
                else if (info.country[0] != '\0')
                    item.location += ", " + juce::String(info.country);
                item.isFull = (info.numClients >= info.maxClients);
                
                serverListModel.addServer(item);
                needsRepaint = true;
            }
        }
        
        if (needsRepaint || numServers != lastServerCount)
        {
            lastServerCount = numServers;
            // Re-apply current sort order
            serverListModel.sortByColumn(serverListModel.getCurrentSortColumn(), 
                                         serverListModel.getCurrentSortForward());
            serverListTable.updateContent();
            serverListTable.repaint();
        }
        
        // Re-request if we haven't received anything in a while
        if (requestPending && numServers == 0)
        {
            uint32_t elapsed = juce::Time::getMillisecondCounter() - requestTime;
            if (elapsed > 3000)  // Re-request after 3 seconds
            {
                int dirType = directoryCombo.getSelectedId() - 1;
                if (dirType < 0 || dirType > 5) dirType = 0;
                jamulus_client_request_server_list(jamulusClient, dirType);
                requestTime = juce::Time::getMillisecondCounter();
            }
        }
        else if (numServers > 0)
        {
            requestPending = false;
        }
    }
    
    void onConnect(const juce::String& address)
    {
        if (onConnectRequested)
            onConnectRequested(address);
    }
    
    jamulus_client_t jamulusClient;
    int lastServerCount = 0;
    bool requestPending = false;
    uint32_t requestTime = 0;
    int pingCount = 0;
    
    juce::Label titleLabel;
    juce::Label directoryLabel;
    juce::ComboBox directoryCombo;
    juce::Label filterLabel;
    juce::TextEditor filterEdit;
    
    juce::TableListBox serverListTable;
    ServerListModel serverListModel;
    
    juce::Label addressLabel;
    juce::TextEditor serverAddressEdit;
    juce::Label recentLabel;
    juce::ComboBox recentCombo;
    
    juce::TextButton connectButton;
    juce::TextButton cancelButton;
};
