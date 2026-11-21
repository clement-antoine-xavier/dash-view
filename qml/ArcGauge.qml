import QtQuick
import QtQuick.Shapes

Item {
    id: gauge

    property string title: ""
    property real value: NaN
    property real minValue: 0
    property real maxValue: 100
    property string unit: ""
    property int decimals: 0
    property color accent: Theme.cyan

    readonly property real ratio: Theme.pct(value, minValue, maxValue)
    implicitWidth: 146
    implicitHeight: 158

    Shape {
        anchors.fill: parent
        preferredRendererType: Shape.CurveRenderer

        ShapePath {
            strokeColor: Theme.line
            strokeWidth: 8
            fillColor: "transparent"
            capStyle: ShapePath.RoundCap
            PathAngleArc {
                centerX: gauge.width / 2
                centerY: gauge.height / 2 - 5
                radiusX: Math.max(1, Math.min(gauge.width, gauge.height) / 2 - 16)
                radiusY: Math.max(1, Math.min(gauge.width, gauge.height) / 2 - 16)
                startAngle: 150
                sweepAngle: 240
                moveToStart: true
            }
        }

        ShapePath {
            strokeColor: gauge.accent
            strokeWidth: 8
            fillColor: "transparent"
            capStyle: ShapePath.RoundCap
            PathAngleArc {
                centerX: gauge.width / 2
                centerY: gauge.height / 2 - 5
                radiusX: Math.max(1, Math.min(gauge.width, gauge.height) / 2 - 16)
                radiusY: Math.max(1, Math.min(gauge.width, gauge.height) / 2 - 16)
                startAngle: 150
                sweepAngle: Math.max(0.1, 240 * gauge.ratio)
                moveToStart: true
            }
        }
    }

    Column {
        width: parent.width
        anchors.verticalCenter: parent.verticalCenter
        anchors.verticalCenterOffset: -4
        spacing: 0

        Text {
            width: parent.width
            horizontalAlignment: Text.AlignHCenter
            text: Theme.fmt(gauge.value, gauge.decimals)
            color: Theme.text
            font.family: Theme.fontMain
            font.pixelSize: 28
            font.weight: Font.DemiBold
        }

        Text {
            width: parent.width
            horizontalAlignment: Text.AlignHCenter
            text: gauge.unit
            color: Theme.dim
            font.family: Theme.fontMain
            font.pixelSize: 9
            font.letterSpacing: 1.2
        }
    }

    Text {
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 1
        text: gauge.title
        color: Theme.dim
        font.family: Theme.fontMain
        font.pixelSize: 9
        font.bold: true
        font.letterSpacing: 1.4
    }
}
