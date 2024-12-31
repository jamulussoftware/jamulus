import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Item {
    id: singleLevelMeter
    width: 10    // Set default width
    height: 200  // Set default height

    property real heightPercentage: 0
    property bool chanClipStatus: false

    Rectangle {
        id: backgroundBar
        anchors.fill: parent
        color: "black"
        radius: 2
    }

    Rectangle {
        id: levelBar
        width: parent.width
        height: parent.height * Math.min(heightPercentage, 1)
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        radius: 1
        // When chanClipStatus is true, show a thick red border
        border.color: chanClipStatus ? "red" : "transparent"
        border.width: chanClipStatus ? 4 : 0
        color: heightPercentage > 0.95 ? "#FF260A" :
               heightPercentage > 0.85 ? "#FF8C0A" :
               "#3AF864"
    }
}
