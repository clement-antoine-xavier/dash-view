import QtQuick

Item {
    id: root

    property real deg: NaN
    property real maxDeg: 180

    readonly property real ratio: Theme.pct(deg, -maxDeg, maxDeg)
    implicitHeight: 58

    Text {
        text: "STEERING"
        color: Theme.dim
        font.family: Theme.fontMain
        font.pixelSize: 9
        font.bold: true
        font.letterSpacing: 1.4
        anchors.left: parent.left
        anchors.top: parent.top
    }

    Text {
        text: isNaN(root.deg) ? "—" : Theme.fmt(root.deg, 1) + "°"
        color: Theme.text
        font.family: Theme.fontMono
        font.pixelSize: 12
        anchors.right: parent.right
        anchors.top: parent.top
    }

    Rectangle {
        id: track
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        height: 6
        radius: 3
        color: Theme.panelAlt
        border.width: 1
        border.color: Theme.line
    }

    Rectangle {
        width: 1
        height: 14
        color: Theme.dim
        x: track.x + track.width / 2 - width / 2
        anchors.bottom: track.top
        anchors.bottomMargin: 2
    }

    Rectangle {
        width: 4
        height: 20
        radius: 2
        color: Theme.accent
        x: track.x + track.width * root.ratio - width / 2
        anchors.bottom: track.bottom
        anchors.bottomMargin: -6

        Behavior on x { NumberAnimation { duration: 100; easing.type: Easing.OutQuad } }
    }
}
