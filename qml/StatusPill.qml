import QtQuick

Rectangle {
    id: pill

    property string title: "NO DATA"
    property string subtitle: ""
    property color dotColor: Theme.green
    property bool pulsing: true

    implicitWidth: 208
    implicitHeight: 46
    radius: height / 2
    color: Theme.panel
    border.width: 1
    border.color: Theme.line

    Rectangle {
        id: dot
        width: 8
        height: 8
        radius: 4
        color: pill.dotColor
        anchors.left: parent.left
        anchors.leftMargin: 16
        anchors.verticalCenter: parent.verticalCenter

        SequentialAnimation on opacity {
            running: pill.pulsing
            loops: Animation.Infinite
            NumberAnimation { to: 0.25; duration: 900; easing.type: Easing.InOutSine }
            NumberAnimation { to: 1.0; duration: 900; easing.type: Easing.InOutSine }
        }
    }

    Column {
        anchors.left: dot.right
        anchors.leftMargin: 10
        anchors.verticalCenter: parent.verticalCenter
        spacing: -1

        Text {
            text: pill.title
            color: Theme.text
            font.family: Theme.fontMain
            font.pixelSize: 11
            font.bold: true
            font.letterSpacing: 1.2
        }

        Text {
            text: pill.subtitle
            color: Theme.dim
            font.family: Theme.fontMono
            font.pixelSize: 9
        }
    }
}
