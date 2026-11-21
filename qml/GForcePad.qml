import QtQuick

Item {
    id: root

    property real xg: 0
    property real yg: 0
    property real range: 1.2

    implicitWidth: 190
    implicitHeight: 190

    Item {
        id: pad
        width: Math.min(root.width, root.height)
        height: width
        anchors.centerIn: parent

        Rectangle {
            anchors.fill: parent
            radius: width / 2
            color: Theme.panelAlt
            border.width: 1
            border.color: Theme.line
        }

        Rectangle {
            width: parent.width - 24
            height: 1
            color: Theme.line
            anchors.centerIn: parent
            opacity: 0.8
        }

        Rectangle {
            width: 1
            height: parent.height - 24
            color: Theme.line
            anchors.centerIn: parent
            opacity: 0.8
        }

        Rectangle {
            width: parent.width * 0.5
            height: parent.height * 0.5
            radius: width / 2
            color: "transparent"
            border.width: 1
            border.color: Theme.line
            anchors.centerIn: parent
            opacity: 0.55
        }

        Rectangle {
            id: dot

            readonly property real lateral: Math.max(-1, Math.min(1, root.yg / root.range))
            readonly property real longitudinal: Math.max(-1, Math.min(1, root.xg / root.range))

            width: 14
            height: 14
            radius: 7
            color: Theme.accent
            border.width: 2
            border.color: Theme.bg

            x: pad.width / 2 + lateral * (pad.width / 2 - 12) - width / 2
            y: pad.height / 2 - longitudinal * (pad.height / 2 - 12) - height / 2

            Behavior on x { NumberAnimation { duration: 90; easing.type: Easing.OutQuad } }
            Behavior on y { NumberAnimation { duration: 90; easing.type: Easing.OutQuad } }
        }

        Text {
            text: "G"
            color: Theme.dim
            font.family: Theme.fontMain
            font.pixelSize: 8
            font.letterSpacing: 1.0
            opacity: 0.7
            anchors.bottom: parent.bottom
            anchors.bottomMargin: 10
            anchors.horizontalCenter: parent.horizontalCenter
        }
    }
}
