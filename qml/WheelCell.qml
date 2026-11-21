import QtQuick

Item {
    id: cell

    property string label: ""
    property real value: NaN

    implicitWidth: 90
    implicitHeight: 64

    Column {
        width: parent.width
        spacing: 1

        Text {
            width: parent.width
            horizontalAlignment: Text.AlignHCenter
            text: cell.label
            color: Theme.dim
            font.family: Theme.fontMain
            font.pixelSize: 9
            font.bold: true
            font.letterSpacing: 1.4
        }

        Text {
            width: parent.width
            horizontalAlignment: Text.AlignHCenter
            text: Theme.fmt(cell.value, 1)
            color: Theme.text
            font.family: Theme.fontMono
            font.pixelSize: 27
            font.weight: Font.Medium
        }

        Text {
            width: parent.width
            horizontalAlignment: Text.AlignHCenter
            text: "km/h"
            color: Theme.dim
            font.family: Theme.fontMain
            font.pixelSize: 8
            font.letterSpacing: 0.8
        }
    }
}
