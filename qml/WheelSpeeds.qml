import QtQuick

Row {
    spacing: 24

    WheelCell { label: "FL"; value: Vehicle.values.wheelFrontLeft ?? NaN }
    WheelCell { label: "FR"; value: Vehicle.values.wheelFrontRight ?? NaN }
    WheelCell { label: "RL"; value: Vehicle.values.wheelRearLeft ?? NaN }
    WheelCell { label: "RR"; value: Vehicle.values.wheelRearRight ?? NaN }
}
