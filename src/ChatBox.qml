import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Item {
    id: chatBox
    width: 400
    height: 300

    ColumnLayout {
        anchors.fill: parent
        spacing: 6

        ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true

            TextArea {
                id: chatArea
                Layout.fillWidth: true
                Layout.fillHeight: true
                readOnly: true
                wrapMode: TextEdit.Wrap
                textFormat: TextEdit.RichText  // Enable HTML rendering
                text: _chatBox.chatHistory
                font.pixelSize: 12

                // Add padding for better text display
                leftPadding: 10
                rightPadding: 10
                topPadding: 10
                bottomPadding: 10

                // Auto-scroll to bottom when new messages arrive
                onTextChanged: {
                    cursorPosition = length
                    if (length > 0) {
                        // ensure cursor is visible
                        chatArea.flickableItem.contentY = chatArea.flickableItem.contentHeight - chatArea.flickableItem.height
                    }
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 4

            TextField {
                id: chatInput
                Layout.fillWidth: true
                placeholderText: qsTr("Type a message…")
                onAccepted: {
                    _chatBox.sendMessage(text)
                    text = ""
                }
            }

            Button {
                text: qsTr("Send")
                onClicked: {
                    _chatBox.sendMessage(chatInput.text)
                    chatInput.text = ""
                }
            }
        }

        Button {
            text: qsTr("Clear Chat")
            onClicked: {
                _chatBox.clearChat()
            }
        }
    }
}
