import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import "."  // Ensure SingleLevelMeter.qml is in the import path

Rectangle {
    id: levelContainer
    Layout.fillWidth: true
    Layout.fillHeight: true
    color: "black"
    radius: 2

    property real levelValueL: 0
    property real levelValueR: 0

    RowLayout {
        id: levelBarsLayout
        anchors.fill: parent
        spacing: 0

        SingleLevelMeter {
            id: levelBarL
            heightPercentage: levelValueL
        }

        SingleLevelMeter {
            id: levelBarR
            heightPercentage: levelValueR
        }
    }
}