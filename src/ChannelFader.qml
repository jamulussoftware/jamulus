import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Rectangle {
    id: channelFader
    border.width: 2
    radius: 5
    border.color: "#d9d9d9"

    property var channelModel
    // use ? and ?? operators to suppress errors when channelModel is null
    property string channelUserName: channelModel?.channelUserName ?? "" // channelUserNameText.text
    property double channelLevel: channelModel?.channelMeter.doubleVal ?? 0
    property bool channelClipStatus: channelModel?.channelMeter.clipStatus ?? false
    property double faderLevel: channelModel?.faderLevel ?? 0 // volumeFader.value
    property double panLevel: channelModel?.panLevel ?? 0 // panKnob.value
    property bool isMuted: channelModel?.isMuted ?? 0 // muteButton.checked
    property bool isSolo: channelModel?.isSolo ?? 0 // soloButton.checked
    property int groupID: channelModel?.groupID ?? 0  // groupId

    ColumnLayout {
        Layout.fillHeight: true
        Layout.fillWidth: true
        anchors.centerIn: parent
        spacing: 3

        Text {
            id: panKnobLabel
            text: "PAN"
        }

        // Pan Knob
        Dial {
            id: panKnob
            from: 0
            to: 100
            value: panLevel // default - set to AUD_MIX_PAN_MAX / 2
            stepSize: 1
            Layout.alignment: Qt.AlignHCenter
            Layout.preferredWidth: 32
            Layout.preferredHeight: 32
            Layout.bottomMargin: 4
            background: Rectangle {
                // color: "white"
                border.width: 1
                border.color: "#4e4e4e"
                radius: width / 2
            }

            onMoved: {
                channelModel.setPanLevel(value)
            }
        }

        RowLayout {
            spacing: 5
            Layout.alignment: Qt.AlignHCenter
            Layout.preferredHeight: 200
            Layout.preferredWidth: 15

            // Level Meter
            SingleLevelMeter {
                id: levelMeterRectangleUser
                heightPercentage: channelLevel
                chanClipStatus: channelClipStatus
            }

            // Fader
            Slider {
                id: volumeFader
                orientation: Qt.Vertical
                from: 0.0
                to: 100.0
                value: faderLevel // FIXME - set to AUD_MIX_FADER_MAX
                Layout.alignment: Qt.AlignHCenter
                Layout.preferredHeight: parent.height

                onValueChanged: {
                    channelModel.setFaderLevel(value)
                }
            }
        }

        // GRPMTSOLO box
        ColumnLayout {
            Layout.alignment: Qt.AlignHCenter
            spacing: 5

            RowLayout {
                spacing: 5
                Layout.alignment: Qt.AlignHCenter

                Button {
                    id: muteButton
                    checkable: true
                    text: "M"
                    Layout.preferredWidth: 30
                    Layout.preferredHeight: 30
                    font.bold: true
                    checked: isMuted
                    onClicked: {
                        channelModel.setIsMuted(!isMuted)
                    }
                }

                Button {
                    id: soloButton
                    checkable: true
                    text: "S"
                    Layout.preferredWidth: 30
                    Layout.preferredHeight: 30
                    font.bold: true
                    checked: isSolo
                    onClicked: {
                        channelModel.setIsSolo(!isSolo)
                    }
                }
            }

            Button {
                id: groupChooser
                text: groupID > 0 ? groupID.toString() : "GRP"
                Layout.preferredWidth: 50
                Layout.preferredHeight: 25
                Layout.alignment: Qt.AlignHCenter
                font.bold: true
                onClicked: menu.open()

                Menu {
                    id: menu
                    y: groupChooser.height

                    Repeater {
                        model: 9  // Total number of items
                        delegate: MenuItem {
                            property int groupId: index
                            text: index === 0 ? "No group" : "Group " + index
                            onTriggered: channelModel.setGroupID(groupId)
                        }
                    }
                }
            }

        }

        // Username label
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 50
            Layout.alignment: Qt.AlignHCenter
            border.color: "#d9d9d9"
            border.width: 1
            radius: 3

            Label {
                id: channelUserNameText
                anchors.fill: parent
                text: channelUserName
                font.bold: true
                wrapMode: Text.WordWrap
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }
        }

    }
}

