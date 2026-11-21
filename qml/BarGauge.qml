import QtQuick

Item {
    id: gauge

    property string title: ""
    property real value: NaN
    property real maxValue: 100
    property string unit: ""
    property int decimals: 0
    property color fill: Theme.accent
    property string valueText: ""

    readonly property real ratio: Theme.pct(value, 0, maxValue)
    readonly property string displayText: valueText.length > 0
        ? valueText
        : Theme.fmt(value, decimals) + (unit.length > 0 ? " " + unit : "")

    implicitHeight: 46

    Text {
        text: gauge.title
        color: Theme.dim
        font.family: Theme.fontMain
        font.pixelSize: 9
        font.bold: true
        font.letterSpacing: 1.4
        anchors.left: parent.left
        anchors.top: parent.top
    }

    Text {
        text: gauge.displayText
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
        height: 8
        radius: 4
        color: Theme.panelAlt
        border.width: 1
        border.color: Theme.line
    }

    Rectangle {
        anchors.left: track.left
        anchors.bottom: track.bottom
        width: Math.max(0, track.width * gauge.ratio)
        height: 8
        radius: 4
        color: gauge.fill

        Behavior on width { NumberAnimation { duration: 120; easing.type: Easing.OutQuad } }
    }
}
