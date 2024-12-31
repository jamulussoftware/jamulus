import QtQuick 2.15

Rectangle {
    id: handle
    property int minimumX: 200
    property int maximumX: parent.width - 200

    MouseArea {
        anchors.fill: parent
        anchors.margins: -5
        cursorShape: Qt.SplitHCursor
        drag {
            target: parent
            axis: Drag.XAxis
            minimumX: handle.minimumX
            maximumX: handle.maximumX
        }
    }
}
