import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import "."

Rectangle {
    anchors.fill: parent

    RowLayout {
        anchors.fill: parent
        spacing: 0

        // Left Control Panel
        Rectangle {
            Layout.preferredWidth: 150
            radius: 10
            border.color: "#d9d9d9"
            Layout.fillHeight: true

            ColumnLayout {
                anchors.fill: parent

                Text {
                    text: "INPUT"
                    font.pixelSize: 14
                    Layout.alignment: Qt.AlignHCenter
                }

                // input meter section
                Rectangle {
                    Layout.alignment: Qt.AlignHCenter
                    Layout.preferredHeight: 200
                    Layout.preferredWidth: 20

                    StereoLevelMeter {
                        id: levelMeterRectangle
                        anchors.fill: parent
                        Layout.alignment: Qt.AlignBottom | Qt.AlignHCenter
                        levelValueL: _main.inputMeterL.doubleVal
                        levelValueR: _main.inputMeterR.doubleVal
                    }
                }

                // mute / pan section
                GridLayout {
                    Layout.alignment: Qt.AlignHCenter
                    columns: 2

                    Button {
                        text: "M"
                        Layout.preferredWidth: 40
                        Layout.preferredHeight: 40
                        Layout.alignment: Qt.AlignHCenter
                        font.bold: true
                        checkable: true
                        checked: _main.muteOut
                        onClicked: _main.muteOut = checked
                    }

                    Dial {
                        id: inPanDial
                        Layout.preferredWidth: 45
                        Layout.preferredHeight: 45
                        from: 0 // FIXME set from defines
                        to: 100
                        value: _main.audioInPan
                        onMoved: _main.audioInPan = value

                        //TODO: customise Dial something like this - need to set dimensions correctly
                        // background: Rectangle {
                        //     x: inPanDial.width / 2 - width / 2
                        //     y: inPanDial.height / 2 - height / 2
                        //     implicitWidth: 140
                        //     implicitHeight: 140
                        //     width: Math.max(64, Math.min(inPanDial.width, inPanDial.height))
                        //     height: width
                        //     color: "transparent"
                        //     radius: width / 2
                        //     border.color: inPanDial.pressed ? "#17a81a" : "#21be2b"
                        //     opacity: inPanDial.enabled ? 1 : 0.3
                        // }

                        // handle: Rectangle {
                        //     id: handleItem
                        //     x: inPanDial.background.x + inPanDial.background.width / 2 - width / 2
                        //     y: inPanDial.background.y + inPanDial.background.height / 2 - height / 2
                        //     width: 16
                        //     height: 16
                        //     color: inPanDial.pressed ? "#17a81a" : "#21be2b"
                        //     radius: 8
                        //     antialiasing: true
                        //     opacity: inPanDial.enabled ? 1 : 0.3
                        //     transform: [
                        //         Translate {
                        //             y: -Math.min(inPanDial.background.width, inPanDial.background.height) * 0.4 + handleItem.height / 2
                        //         },
                        //         Rotation {
                        //             angle: inPanDial.angle
                        //             origin.x: handleItem.width / 2
                        //             origin.y: handleItem.height / 2
                        //         }
                        //     ]
                        // }

                    }

                    Text {
                        text: "mute"
                        font.pixelSize: 10
                        Layout.alignment: Qt.AlignHCenter
                    }

                    Text {
                        text: "pan"
                        font.pixelSize: 10
                        Layout.alignment: Qt.AlignHCenter
                    }

                }

                // Network Check Section
                GridLayout {
                    Layout.alignment: Qt.AlignCenter
                    columns: 2
                    Layout.leftMargin: 5

                    Text {
                        text: "PING"
                    }
                    Rectangle {
                        Layout.preferredWidth: 40
                        Layout.preferredHeight: 15
                        color: "#000000"
                        Label {
                            anchors.centerIn: parent
                            text: _main.pingVal
                            color: { _main.pingVal < 40 ? "green" : "red" }
                            font.bold: true
                        }
                    }

                    Text {
                        text: "DELAY"
                    }
                    Rectangle {
                        Layout.preferredWidth: 40
                        Layout.preferredHeight: 15
                        color: "#000000"
                        Label {
                            anchors.centerIn: parent
                            color:  _main.delayVal <= 43 ? "green" :
                                    _main.delayVal <= 68 ? "yellow" :
                                    "red"
                            text: _main.delayVal
                            font.bold: true
                        }
                    }

                    Text {
                        text: "JITTER"
                    }
                    Rectangle {
                        Layout.preferredWidth: 40
                        Layout.preferredHeight: 15
                        color: "#000000"
                        Rectangle {
                            anchors.centerIn: parent
                            visible: _main.jitterWarn
                            width: 20
                            height: 8
                            radius: 2
                            color: _main.jitterWarn ? "red" : "#000000"
                        }
                    }
                }

                GridLayout {
                    id: sessionLinkInputBox
                    Layout.alignment: Qt.AlignHCenter
                    columns: 1

                    TextField {
                        id: sessionlinkText
                        Layout.preferredWidth: 110
                        text: _main.sessionlinkText
                        onTextChanged: _main.sessionlinkText = text;
                        Layout.alignment: Qt.AlignHCenter
                    }

                    Button {
                        id: connectButton
                        text: "Connect"
                        Layout.preferredWidth: 80
                        Layout.preferredHeight: 30
                        Layout.alignment: Qt.AlignHCenter
                        font.bold: true
                        onClicked: _main.onConnectButtonClicked()
                        visible: !_main.bConnected
                    }

                    Button {
                        id: disconnectButton
                        text: "Disconnect"
                        Layout.preferredWidth: 80
                        Layout.preferredHeight: 30
                        Layout.alignment: Qt.AlignHCenter
                        font.bold: true
                        onClicked: _main.onDisconnectButtonClicked()
                        visible: _main.bConnected
                    }
                }
            }
        }

        // Main Area
        Rectangle {
            radius: 10
            border.color: "#d9d9d9"
            Layout.fillHeight: true
            Layout.fillWidth: true

            ColumnLayout {
                anchors.fill: parent
                spacing: 20

                // Connect / control area
                RowLayout {
                    Layout.preferredHeight: 80
                    spacing: 10

                    // Status area
                    Rectangle {
                        id: statusView
                        Layout.preferredHeight: 30
                        Layout.alignment: Qt.AlignLeft
                        Layout.leftMargin: 5
                        border.width: 1
                        border.color: "#d9d9d9"

                        GridLayout {
                            columns: 2

                            Text {
                                text: "Session Status: "
                            }
                            Text {
                                text: _main.sessionStatus
                            }

                            Text {
                                text: "Session Server: "
                            }
                            Text {
                                text: _main.sessionName
                            }

                            Text {
                                text: "Recording: "
                            }
                            Text {
                                text: _main.recordingStatus
                            }
                        }
                    }

                }

                Item {
                    // spacer item
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    Rectangle {
                        anchors.fill: parent;
                        color: "white"
                    } // to visualize the spacer
                }

                // Mixerboard area
                Rectangle {
                    id: mixerboardView
                    Layout.fillWidth: true
                    Layout.preferredHeight: 385
                    border.color: "#d9d9d9"
                    border.width: 2
                    Layout.alignment: Qt.AlignBottom

                    ListView {
                        id: listView
                        anchors.fill: parent
                        orientation: ListView.Horizontal
                        spacing: 5
                        // snapMode: ListView.NoSnap
                        // boundsBehavior: Flickable.StopAtBounds

                        model: _audioMixerBoard.channels

                        delegate: ChannelFader {
                            id: channelFader
                            width: 85
                            height: listView.height
                            channelModel: modelData
                        }
                    }
                }

            }
        }

        // // Splitter for resizing
        // SplitHandle {
        //     id: splitter
        //     Layout.fillHeight: true
        //     Layout.preferredWidth: 4
        //     // color: "#d9d9d9"
        // }

        // Chat Panel
        Rectangle {
            id: chatPanel
            Layout.fillHeight: true
            Layout.preferredWidth: 300
            Layout.minimumWidth: 200
            Layout.maximumWidth: parent.width * 0.4
            visible: !btnShowChat.checked
            radius: 10
            border.color: "#d9d9d9"

            ChatBox {
                anchors.fill: parent
                anchors.margins: 10
            }
        }

    }

    // Toggle button
    Button {
        id: btnShowChat
        text: checked ? qsTr("Show Chat") : qsTr("Hide Chat")
        anchors.top: parent.top
        anchors.right: parent.right
        checkable: true
        checked: false
        onCheckedChanged: chatPanel.visible = !checked
    }
}
