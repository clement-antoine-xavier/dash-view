import QtQuick
import QtQuick.Shapes

Item {
    id: root

    property real rpm: 0
    property real maxRpm: 7000
    property real redline: maxRpm
    property real speedKmh: NaN
    property string gearText: "N"

    readonly property real gaugeMax: maxRpm > 1000 ? maxRpm * 1.06 : 7500
    readonly property real ratio: Theme.pct(rpm, 0, gaugeMax)
    readonly property real redRatio: Theme.pct(redline, 0, gaugeMax)
    readonly property real cx: width / 2
    readonly property real cy: height / 2
    readonly property real radius: Math.max(1, Math.min(width, height) / 2 - 26)

    Shape {
        anchors.fill: parent
        preferredRendererType: Shape.CurveRenderer

        ShapePath {
            strokeColor: Theme.line
            strokeWidth: 14
            fillColor: "transparent"
            capStyle: ShapePath.RoundCap
            PathAngleArc {
                centerX: root.cx; centerY: root.cy
                radiusX: root.radius; radiusY: root.radius
                startAngle: 135; sweepAngle: 270
                moveToStart: true
            }
        }

        ShapePath {
            strokeColor: Theme.accent
            strokeWidth: 14
            fillColor: "transparent"
            capStyle: ShapePath.RoundCap
            PathAngleArc {
                centerX: root.cx; centerY: root.cy
                radiusX: root.radius; radiusY: root.radius
                startAngle: 135
                sweepAngle: Math.max(0.1, 270 * root.ratio)
                moveToStart: true
            }
        }

        ShapePath {
            strokeColor: Theme.red
            strokeWidth: 14
            fillColor: "transparent"
            capStyle: ShapePath.FlatCap
            PathAngleArc {
                centerX: root.cx; centerY: root.cy
                radiusX: root.radius; radiusY: root.radius
                startAngle: 135 + 270 * root.redRatio
                sweepAngle: Math.max(0, 270 * (1 - root.redRatio))
                moveToStart: true
            }
        }
    }

    Repeater {
        model: 9

        Item {
            width: 2
            height: root.radius
            x: root.cx - width / 2
            y: root.cy - root.radius
            transformOrigin: Item.Bottom
            rotation: 225 + 270 * (index / 8)

            Rectangle {
                width: 2
                height: index % 2 === 0 ? 13 : 7
                color: Theme.dim
                opacity: index % 2 === 0 ? 0.8 : 0.45
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.top: parent.top
            }
        }
    }

    Item {
        id: needle
        width: 4
        height: root.radius * 0.84
        x: root.cx - width / 2
        y: root.cy - height
        transformOrigin: Item.Bottom
        rotation: 225 + 270 * root.ratio

        Behavior on rotation { NumberAnimation { duration: 90; easing.type: Easing.OutQuad } }

        Rectangle {
            anchors.fill: parent
            radius: 2
            color: Theme.text
            opacity: 0.92
        }

        Rectangle {
            width: 8
            height: 34
            radius: 4
            color: Theme.accent
            anchors.bottom: parent.bottom
            anchors.horizontalCenter: parent.horizontalCenter
        }
    }

    Rectangle {
        width: 16
        height: 16
        radius: 8
        anchors.centerIn: parent
        color: Theme.bg
        border.width: 3
        border.color: Theme.accent
    }

    Column {
        width: parent.width
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: parent.verticalCenter
        anchors.topMargin: 54
        spacing: -4

        Text {
            width: parent.width
            horizontalAlignment: Text.AlignHCenter
            text: Theme.fmt(root.speedKmh, 0)
            color: Theme.text
            font.family: Theme.fontMain
            font.pixelSize: 54
            font.weight: Font.DemiBold
        }

        Text {
            width: parent.width
            horizontalAlignment: Text.AlignHCenter
            text: "km/h"
            color: Theme.dim
            font.family: Theme.fontMain
            font.pixelSize: 10
            font.letterSpacing: 2.0
        }
    }

    Rectangle {
        width: 62
        height: 52
        radius: 10
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 30
        color: Theme.panelAlt
        border.width: 1
        border.color: Theme.line

        Text {
            anchors.centerIn: parent
            text: root.gearText
            color: Theme.accent
            font.family: Theme.fontMain
            font.pixelSize: 30
            font.bold: true
        }
    }
}
