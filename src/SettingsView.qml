import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Item {
    id: root
    anchors.fill: parent

    ScrollView {
        anchors.fill: parent

        ColumnLayout {
            id: mainColumn
            anchors.fill: parent
            spacing: 10

            // 1)
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 150
                border.color: "black"
                border.width: 2
                radius: 10

                // Stack items vertically inside
                ColumnLayout {
                    id: profileColumn
                    anchors.centerIn: parent
                    anchors.margins: 10
                    spacing: 10

                    Text {
                        text: "GENERAL"
                        Layout.alignment: Qt.AlignHCenter
                        font.bold: true
                    }

                    GridLayout {
                        columns: 2
                        rowSpacing: 6
                        columnSpacing: 10

                        Text { text: "Username" }
                        TextField {
                            id: inputField
                            width: 200
                            placeholderText: "Enter alias here"
                            text: _settings.pedtAlias
                            onTextChanged: _settings.pedtAlias = text
                        }

                        Text { text: "Mixer Rows count" }
                        SpinBox {
                            id: spnMixerRows
                            value: _settings.spnMixerRows
                            onValueChanged: _settings.spnMixerRows = value
                        }
                    }
                }
            }

            // 2)
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 400
                border.color: "black"
                border.width: 2
                radius: 10

                ColumnLayout {
                    anchors.centerIn: parent
                    anchors.margins: 10
                    spacing: 10

                    Text {
                        text: "AUDIO"
                        Layout.alignment: Qt.AlignHCenter
                        font.bold: true
                    }

                    GridLayout {
                        columns: 2
                        rowSpacing: 6
                        columnSpacing: 10

                        Text { text: "Audio Device" }
                        ComboBox {
                            id: cbxSoundcard
                            model: _settings.slSndCrdDevNames
                            Component.onCompleted: {
                                let idx = _settings.slSndCrdDevNames.indexOf(_settings.slSndCrdDev)
                                if (idx !== -1) currentIndex = idx
                            }
                            onCurrentIndexChanged: {
                                if (currentIndex >= 0 && currentIndex < _settings.slSndCrdDevNames.length) {
                                    _settings.slSndCrdDev = _settings.slSndCrdDevNames[currentIndex]
                                }
                            }
                        }
                        Connections {
                            target: _settings
                            function onSlSndCrdDevChanged() {
                                let newIdx = _settings.slSndCrdDevNames.indexOf(_settings.slSndCrdDev)
                                if (newIdx !== -1 && newIdx !== cbxSoundcard.currentIndex) {
                                    cbxSoundcard.currentIndex = newIdx
                                }
                            }
                        }

                        // Channel Mappings
                        Text { text: "Input channel - LEFT" }
                        ComboBox {
                            id: cbxLInChan
                            model: _settings.sndCrdInputChannelNames
                            displayText: _settings.sndCardLInChannel
                            onCurrentTextChanged: _settings.sndCardLInChannel = currentText
                        }

                        Text { text: "Input channel - RIGHT" }
                        ComboBox {
                            id: cbxRInChan
                            model: _settings.sndCrdInputChannelNames
                            displayText: _settings.sndCardRInChannel
                            onCurrentTextChanged: _settings.sndCardRInChannel = currentText
                        }

                        Text { text: "Output channel - LEFT" }
                        ComboBox {
                            id: cbxLOutChan
                            model: _settings.sndCrdOutputChannelNames
                            displayText: _settings.sndCardLOutChannel
                            onCurrentTextChanged: _settings.sndCardLOutChannel = currentText
                        }

                        Text { text: "Output channel - RIGHT" }
                        ComboBox {
                            id: cbxROutChan
                            model: _settings.sndCrdOutputChannelNames
                            displayText: _settings.sndCardROutChannel
                            onCurrentTextChanged: _settings.sndCardRoutChannel = currentText
                        }

                        Text { text: "Mono/Stereo mode" }
                        ComboBox {
                            id: cbxAudioChannels
                            model: ListModel {
                                ListElement { text: "Mono" }
                                ListElement { text: "Mono-in/Stereo-out" }
                                ListElement { text: "Stereo" }
                            }
                            currentIndex: _settings.cbxAudioChannels
                            onCurrentIndexChanged: _settings.cbxAudioChannels = currentIndex
                        }

                        Text { text: "Audio quality" }
                        ComboBox {
                            id: cbxAudioQuality
                            model: ListModel {
                                ListElement { text: "Low" }
                                ListElement { text: "Normal" }
                                ListElement { text: "High" }
                            }
                            currentIndex: _settings.cbxAudioQuality
                            onActivated: _settings.cbxAudioQuality = currentIndex
                        }

                        Text { text: "New client level: " + _settings.edtNewClientLevel }
                        Slider {
                            id: newInputLevelDial
                            from: 0
                            to: 100
                            value: _settings.edtNewClientLevel
                            onMoved: _settings.edtNewClientLevel = value
                        }

                        Text { text: "Feedback Detection" }
                        CheckBox {
                            id: chbDetectFeedback
                            checked: _settings.chbDetectFeedback
                        }

                        Text { text: "Buffer Size: " +  _settings.bufSize }
                        ColumnLayout {
                            spacing: 4
                            ButtonGroup {
                                id: bufferDelayGroup
                                exclusive: true
                                buttons: [
                                    rbtBufferDelayPreferred,
                                    rbtBufferDelayDefault,
                                    rbtBufferDelaySafe
                                ]
                            }
                            RadioButton {
                                id: rbtBufferDelayPreferred
                                text: _settings.sndCrdBufferDelayPreferred
                                enabled: _settings.fraSiFactPrefSupported
                                checked: _settings.rbtBufferDelayPreferred
                                onClicked: _settings.rbtBufferDelayPreferred = true
                            }
                            RadioButton {
                                id: rbtBufferDelayDefault
                                text: _settings.sndCrdBufferDelayDefault
                                enabled: _settings.fraSiFactDefSupported
                                checked: _settings.rbtBufferDelayDefault
                                onClicked: _settings.rbtBufferDelayDefault = true
                            }
                            RadioButton {
                                id: rbtBufferDelaySafe
                                text: _settings.sndCrdBufferDelaySafe
                                enabled: _settings.fraSiFactSafeSupported
                                checked: _settings.rbtBufferDelaySafe
                                onClicked: _settings.rbtBufferDelaySafe = true
                            }
                        }
                    }
                }


            }

            // 3)
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 200
                border.color: "black"
                border.width: 2
                radius: 10

                ColumnLayout {
                    anchors.centerIn: parent
                    anchors.margins: 10
                    spacing: 10

                    Text {
                        text: "NETWORK"
                        Layout.alignment: Qt.AlignHCenter
                        font.bold: true
                    }

                    GridLayout {
                        columns: 2
                        rowSpacing: 6
                        columnSpacing: 10

                        Text { text: "Jitter Buffer" }
                        CheckBox {
                            id: chbAutoJitBuf
                            checked: _settings.chbAutoJitBuf
                            onCheckedChanged: _settings.chbAutoJitBuf = checked
                            text: "Auto"
                        }

                        Text { text: "Client buffer: " + _settings.sldNetBuf }
                        Slider {
                            id: sldNetBuf
                            from: _settings.sldNetBufMin
                            to: _settings.sldNetBufMax
                            value: _settings.sldNetBuf
                            onMoved: _settings.sldNetBuf = value
                            enabled: !chbAutoJitBuf.checked
                        }

                        Text { text: "Server Buffer: " + _settings.sldNetBufServer }
                        Slider {
                            id: sldNetBufServer
                            from: _settings.sldNetBufMin
                            to: _settings.sldNetBufMax
                            value: _settings.sldNetBufServer
                            onMoved: _settings.sldNetBufServer = value
                            enabled: !chbAutoJitBuf.checked
                        }

                        Text { text: "Small network buffers" }
                        CheckBox {
                            id: chbEnableOPUS64
                            checked: _settings.chbEnableOPUS64
                            onCheckStateChanged: _settings.chbEnableOPUS64 = checkState
                        }

                        Text { text: "Upload rate:" }
                        Text {
                            text: _main.bConnected ? (_settings.uploadRate + " kbps") : "--"
                        }
                    }
                }
            }
        }
    }
}
