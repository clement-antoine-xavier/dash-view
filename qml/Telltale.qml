import QtQuick

Rectangle {
    id: telltale

    property string label: ""
    property bool active: false
    property color activeColor: Theme.accent

    implicitWidth: Math.max(42, labelText.implicitWidth + 18)
    implicitHeight: 26
    radius: 7

    color: active ? Qt.rgba(activeColor.r, activeColor.g, activeColor.b, 0.16) : "transparent"
    border.width: 1
    border.color: active ? activeColor : Theme.line
    opacity: active ? 1.0 : 0.75

    Behavior on color { ColorAnimation { duration: 100 } }
    Behavior on border.color { ColorAnimation { duration: 100 } }

    Text {
        id: labelText
        anchors.centerIn: parent
        text: telltale.label
        color: telltale.active ? telltale.activeColor : Theme.dim
        font.family: Theme.fontMain
        font.pixelSize: 10
        font.bold: true
        font.letterSpacing: 0.8
    }
}
