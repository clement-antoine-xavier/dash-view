import QtQuick

Item {
    id: page
    width: 1440
    height: 900

    readonly property var v: Vehicle.values
    readonly property real maxRpm: {
        var m = page.v.maxRpm;
        return (m !== undefined && m > 0) ? m : 7000;
    }
    readonly property real rpm: page.v.rpm ?? 0
    readonly property real speed: page.v.speed ?? NaN
    readonly property string gearText: page.v.gearLabel ?? "—"

    Rectangle {
        anchors.fill: parent
        gradient: Gradient {
            GradientStop { position: 0.0; color: "#0a111a" }
            GradientStop { position: 0.55; color: "#06090e" }
            GradientStop { position: 1.0; color: "#040609" }
        }
    }

    // -------------------------------------------------------------------------
    // Header
    // -------------------------------------------------------------------------
    Item {
        id: header
        x: 32
        y: 22
        width: 1376
        height: 56

        Rectangle {
            id: accentBar
            width: 4
            height: 38
            radius: 2
            color: Theme.accent
            anchors.left: parent.left
            anchors.verticalCenter: parent.verticalCenter
        }

        Column {
            anchors.left: accentBar.right
            anchors.leftMargin: 16
            anchors.verticalCenter: parent.verticalCenter
            spacing: 1

            Text {
                text: "DASH VIEW"
                color: Theme.text
                font.family: Theme.fontMain
                font.pixelSize: 20
                font.bold: true
                font.letterSpacing: 4.0
            }

            Text {
                text: Vehicle.vehicleName + " · " + Vehicle.sourceLabel
                color: Theme.dim
                font.family: Theme.fontMain
                font.pixelSize: 9
                font.letterSpacing: 1.6
            }
        }

        Row {
            anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter
            spacing: 14

            Item {
                width: telltales.implicitWidth
                height: 46

                Telltales {
                    id: telltales
                    anchors.verticalCenter: parent.verticalCenter
                }
            }

            StatusPill {
                title: Vehicle.linkAlive ? Vehicle.sourceLabel : "NO DATA"
                subtitle: Vehicle.valuesPerSecond + " values/s · "
                          + (Vehicle.lastValueAgeMs >= 0 ? Vehicle.lastValueAgeMs + " ms" : "waiting")
                dotColor: Vehicle.linkAlive ? Theme.green : Theme.red
                pulsing: Vehicle.linkAlive
            }
        }
    }

    // -------------------------------------------------------------------------
    // Left column: temperatures, battery, ambient, fuel
    // -------------------------------------------------------------------------
    Item {
        x: 32
        y: 92
        width: 300
        height: 562

        Grid {
            columns: 2
            spacing: 8

            ArcGauge {
                width: 146
                height: 158
                title: "COOLANT"
                unit: "°C"
                decimals: 0
                minValue: 40
                maxValue: 130
                value: page.v.coolantTemp ?? NaN
                accent: (page.v.coolantTemp > 108) ? Theme.red : Theme.cyan
            }

            ArcGauge {
                width: 146
                height: 158
                title: "OIL"
                unit: "°C"
                decimals: 0
                minValue: 40
                maxValue: 140
                value: page.v.oilTemp ?? NaN
                accent: (page.v.oilTemp > 122) ? Theme.red : Theme.accent
            }

            ArcGauge {
                width: 146
                height: 158
                title: "BATTERY"
                unit: "V"
                decimals: 1
                minValue: 10
                maxValue: 16
                value: page.v.batteryVoltage ?? NaN
                accent: (page.v.batteryVoltage < 12.5) ? Theme.red : Theme.green
            }

            ArcGauge {
                width: 146
                height: 158
                title: "AMBIENT"
                unit: "°C"
                decimals: 1
                minValue: -20
                maxValue: 50
                value: page.v.ambientTemp ?? NaN
                accent: Theme.blue
            }
        }

        Panel {
            y: 332
            width: parent.width
            height: 230
            title: "FUEL"

            Text {
                id: rangeValue
                text: Theme.fmt(page.v.fuelRange, 0)
                color: Theme.text
                font.family: Theme.fontMain
                font.pixelSize: 40
                font.weight: Font.DemiBold
                anchors.left: parent.left
                anchors.top: parent.top
            }

            Text {
                text: "km"
                color: Theme.dim
                font.family: Theme.fontMain
                font.pixelSize: 11
                anchors.left: rangeValue.right
                anchors.leftMargin: 6
                anchors.baseline: rangeValue.baseline
            }

            Text {
                text: "RANGE"
                color: Theme.dim
                font.family: Theme.fontMain
                font.pixelSize: 9
                font.bold: true
                font.letterSpacing: 1.4
                anchors.right: parent.right
                anchors.top: parent.top
            }

            Rectangle {
                id: rangeTrack
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: parent.top
                anchors.topMargin: 54
                height: 6
                radius: 3
                color: Theme.panelAlt
                border.width: 1
                border.color: Theme.line
            }

            Rectangle {
                anchors.left: rangeTrack.left
                anchors.top: rangeTrack.top
                width: Math.max(0, rangeTrack.width * Theme.pct(page.v.fuelRange, 0, 600))
                height: 6
                radius: 3
                color: Theme.accent

                Behavior on width { NumberAnimation { duration: 150; easing.type: Easing.OutQuad } }
            }

            Text {
                id: usedLabel
                text: "USED"
                color: Theme.dim
                font.family: Theme.fontMain
                font.pixelSize: 9
                font.bold: true
                font.letterSpacing: 1.4
                anchors.left: parent.left
                anchors.top: rangeTrack.bottom
                anchors.topMargin: 14
            }

            Text {
                text: Theme.fmt(page.v.fuelUsed, 1) + " L"
                color: Theme.text
                font.family: Theme.fontMono
                font.pixelSize: 17
                anchors.right: parent.right
                anchors.top: rangeTrack.bottom
                anchors.topMargin: 9
            }

            Text {
                text: "FUEL LAMP " + (((page.v.fuelLamp ?? 0) > 0.5) ? "ON" : "OFF")
                color: ((page.v.fuelLamp ?? 0) > 0.5) ? Theme.accent : Theme.dim
                font.family: Theme.fontMono
                font.pixelSize: 9
                anchors.left: parent.left
                anchors.bottom: parent.bottom
            }
        }
    }

    // -------------------------------------------------------------------------
    // Center: shift lights and tachometer
    // -------------------------------------------------------------------------
    Item {
        x: 344
        y: 92
        width: 752
        height: 562

        ShiftLights {
            rpm: page.rpm
            redline: page.maxRpm
            anchors.horizontalCenter: parent.horizontalCenter
            y: 4
        }

        Tachometer {
            x: (parent.width - width) / 2
            y: 38
            width: 510
            height: 510
            rpm: page.rpm
            maxRpm: page.maxRpm
            redline: page.maxRpm
            speedKmh: page.speed
            gearText: page.gearText
        }
    }

    // -------------------------------------------------------------------------
    // Right column: pedals, torque, G-force
    // -------------------------------------------------------------------------
    Column {
        x: 1108
        y: 92
        width: 300
        spacing: 10

        Panel {
            width: parent.width
            height: 190
            title: "PEDALS"

            Column {
                anchors.fill: parent
                spacing: 6

                BarGauge {
                    width: parent.width
                    height: 42
                    title: "THROTTLE"
                    unit: "%"
                    decimals: 0
                    value: page.v.throttle ?? NaN
                    fill: Theme.accent
                }

                BarGauge {
                    width: parent.width
                    height: 42
                    title: "ACCELERATOR"
                    unit: "%"
                    decimals: 0
                    value: page.v.accelerator ?? NaN
                    fill: Theme.cyan
                }

                BarGauge {
                    width: parent.width
                    height: 42
                    title: "BRAKE"
                    unit: "%"
                    decimals: 0
                    value: page.v.brakePct ?? NaN
                    fill: Theme.red
                }
            }
        }

        Panel {
            width: parent.width
            height: 110
            title: "TORQUE"

            Text {
                id: torqueValue
                text: Theme.fmt(page.v.engineTorque, 0)
                color: Theme.text
                font.family: Theme.fontMain
                font.pixelSize: 34
                font.weight: Font.DemiBold
                anchors.left: parent.left
                anchors.top: parent.top
            }

            Text {
                text: "Nm"
                color: Theme.dim
                font.family: Theme.fontMain
                font.pixelSize: 10
                anchors.left: torqueValue.right
                anchors.leftMargin: 6
                anchors.baseline: torqueValue.baseline
            }

            Text {
                text: "GEARBOX"
                color: Theme.dim
                font.family: Theme.fontMain
                font.pixelSize: 9
                font.letterSpacing: 1.2
                anchors.right: parent.right
                anchors.top: parent.top
            }

            Text {
                text: Theme.fmt(page.v.gearboxTorque, 0) + " Nm"
                color: Theme.text
                font.family: Theme.fontMono
                font.pixelSize: 13
                anchors.right: parent.right
                anchors.top: parent.top
                anchors.topMargin: 16
            }
        }

        Panel {
            width: parent.width
            height: 242
            title: "G-FORCE"

            GForcePad {
                width: Math.min(parent.width, parent.height) - 16
                height: width
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.top: parent.top
                xg: page.v.accelX ?? 0
                yg: page.v.accelY ?? 0
            }

            Text {
                text: "LONG " + Theme.fmt(page.v.accelX, 2) + " g"
                color: Theme.dim
                font.family: Theme.fontMono
                font.pixelSize: 9
                anchors.left: parent.left
                anchors.bottom: parent.bottom
            }

            Text {
                text: "LAT " + Theme.fmt(page.v.accelY, 2) + " g"
                color: Theme.dim
                font.family: Theme.fontMono
                font.pixelSize: 9
                anchors.right: parent.right
                anchors.bottom: parent.bottom
            }
        }
    }

    // -------------------------------------------------------------------------
    // Bottom strip: wheel speeds, steering/yaw, link status
    // -------------------------------------------------------------------------
    Row {
        x: 32
        y: 678
        spacing: 12

        Panel {
            width: 680
            height: 200
            title: "WHEEL SPEEDS"

            Text {
                text: "VEHICLE"
                color: Theme.dim
                font.family: Theme.fontMain
                font.pixelSize: 9
                font.bold: true
                font.letterSpacing: 1.4
                anchors.left: parent.left
                anchors.top: parent.top
            }

            Text {
                id: bigSpeed
                text: Theme.fmt(page.v.speed, 0)
                color: Theme.text
                font.family: Theme.fontMain
                font.pixelSize: 60
                font.weight: Font.DemiBold
                anchors.left: parent.left
                anchors.top: parent.top
                anchors.topMargin: 12
            }

            Text {
                text: "km/h"
                color: Theme.dim
                font.family: Theme.fontMain
                font.pixelSize: 10
                anchors.left: bigSpeed.right
                anchors.leftMargin: 8
                anchors.baseline: bigSpeed.baseline
            }

            WheelSpeeds {
                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
            }
        }

        Panel {
            width: 340
            height: 200
            title: "STEERING"

            SteeringIndicator {
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: parent.top
                anchors.topMargin: 6
                deg: page.v.steering ?? NaN
            }

            Text {
                text: "YAW RATE"
                color: Theme.dim
                font.family: Theme.fontMain
                font.pixelSize: 9
                font.bold: true
                font.letterSpacing: 1.4
                anchors.left: parent.left
                anchors.bottom: parent.bottom
                anchors.bottomMargin: 22
            }

            Text {
                text: Theme.fmt(page.v.yawRate, 1) + " °/s"
                color: Theme.text
                font.family: Theme.fontMono
                font.pixelSize: 19
                anchors.right: parent.right
                anchors.bottom: parent.bottom
            }
        }

        Panel {
            width: 332
            height: 200
            title: "LINK"

            Column {
                anchors.fill: parent
                spacing: 3

                Text {
                    text: "VEHICLE"
                    color: Theme.dim
                    font.family: Theme.fontMain
                    font.pixelSize: 9
                    font.bold: true
                    font.letterSpacing: 1.4
                }

                Text {
                    width: parent.width
                    text: Vehicle.vehicleName
                    color: Theme.text
                    elide: Text.ElideRight
                    font.family: Theme.fontMain
                    font.pixelSize: 13
                }

                Text {
                    text: "SOURCE"
                    color: Theme.dim
                    font.family: Theme.fontMain
                    font.pixelSize: 9
                    font.bold: true
                    font.letterSpacing: 1.4
                }

                Text {
                    width: parent.width
                    text: Vehicle.sourceName
                    color: Theme.dim
                    elide: Text.ElideRight
                    font.family: Theme.fontMain
                    font.pixelSize: 11
                }

                Text {
                    text: "STATE   " + Vehicle.sourceState
                    color: Theme.dim
                    font.family: Theme.fontMono
                    font.pixelSize: 10
                }

                Text {
                    text: "RATE    " + Vehicle.valuesPerSecond + " values/s"
                    color: Theme.dim
                    font.family: Theme.fontMono
                    font.pixelSize: 10
                }

                Text {
                    text: "AGE     " + (Vehicle.lastValueAgeMs >= 0 ? Vehicle.lastValueAgeMs + " ms" : "—")
                    color: Theme.dim
                    font.family: Theme.fontMono
                    font.pixelSize: 10
                }

                Text {
                    text: "D debug · F fullscreen"
                    color: Theme.dim
                    opacity: 0.7
                    font.family: Theme.fontMain
                    font.pixelSize: 9
                    font.letterSpacing: 0.8
                }
            }
        }
    }
}
