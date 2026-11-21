import QtQuick

Row {
    id: row

    property real rpm: 0
    property real redline: 7000
    property int count: 11

    readonly property real ratio: redline > 0 ? rpm / redline : 0
    spacing: 6

    Repeater {
        model: row.count

        Rectangle {
            readonly property real threshold: 0.62 + 0.34 * index / Math.max(1, row.count - 1)
            readonly property bool lit: row.ratio >= threshold

            width: 20
            height: 12
            radius: 3
            color: lit ? (index < 6 ? Theme.green : index < 9 ? Theme.accent : Theme.red) : Theme.panelAlt
            border.width: 1
            border.color: lit ? color : Theme.line
            opacity: lit ? 1.0 : 0.5

            Behavior on color { ColorAnimation { duration: 60 } }
        }
    }
}
