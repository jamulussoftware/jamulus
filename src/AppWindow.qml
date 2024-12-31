import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

ApplicationWindow {
    visible: true
    minimumWidth: 600
    minimumHeight: 490
    title: qsTr("Jamulus")

    ColumnLayout {
        anchors.fill: parent

        // Tab Bar
        TabBar {
            id: tabBar
            Layout.fillWidth: true

            TabButton {
                text: qsTr("Home")
                checked: stackLayout.currentIndex === 0
                onClicked: stackLayout.currentIndex = 0
            }

            TabButton {
                text: qsTr("Settings")
                checked: stackLayout.currentIndex === 1
                onClicked: stackLayout.currentIndex = 1
            }
        }

        // Stack Layout for Tab Content
        StackLayout {
            id: stackLayout
            Layout.fillWidth: true
            Layout.fillHeight: true

            // Home Tab Content
            Item {
                id: homeTab
                Layout.fillWidth: true
                Layout.fillHeight: true

                MainView {
                    anchors.fill: parent
                }
            }

            // Settings Tab Content
            Item {
                id: settingsTab
                Layout.fillWidth: true
                Layout.fillHeight: true

                SettingsView {
                    // anchors.fill: parent
                }
            }
        }

        Popup {
            id: userPopup
            x: (parent.width - width) / 2
            y: (parent.height - height) / 2
            width: 300
            height: 150
            visible: _main.userMsg !== ""   // Show the popup when there's a message
            closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

            Rectangle {
                anchors.fill: parent
                color: "white"

                Label {
                    id: userMessage
                    text: _main.userMsg   // Display the message from _main
                    anchors.centerIn: parent
                    wrapMode: Text.WordWrap
                    horizontalAlignment: Text.AlignHCenter
                    width: parent.width - 20  // Ensure some padding from the edges
                }
            }

            onVisibleChanged: {
                if (!visible) {
                    _main.userMsg = ""
                }
            }
        }
    }

    // React to changes in _client.userMsg
    function onUserMsgChanged(newMsg) {
        if (newMsg !== "") {
            userPopup.open()
        }
    }
}
