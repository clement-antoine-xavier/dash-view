import QtQuick

Rectangle {
    id: panel

    property string title: ""
    property string hint: ""
    default property alias content: body.data

    color: Theme.panel
    radius: Theme.radius
    border.width: 1
    border.color: Theme.line

    Text {
        id: heading
        visible: panel.title.length > 0
        text: panel.title
        color: Theme.dim
        font.family: Theme.fontMain
        font.pixelSize: 10
        font.bold: true
        font.letterSpacing: 1.8
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.leftMargin: 16
        anchors.topMargin: 13
    }

    Text {
        visible: panel.hint.length > 0
        text: panel.hint
        color: Theme.dim
        opacity: 0.65
        font.family: Theme.fontMono
        font.pixelSize: 9
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.rightMargin: 16
        anchors.topMargin: 14
    }

    Item {
        id: body
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: heading.visible ? heading.bottom : parent.top
        anchors.bottom: parent.bottom
        anchors.leftMargin: 16
        anchors.rightMargin: 16
        anchors.topMargin: 8
        anchors.bottomMargin: 14
    }
}
