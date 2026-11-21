import QtQuick

Row {
    id: row
    spacing: 8

    Telltale {
        label: "HB"
        active: (Vehicle.values.highBeam ?? 0) > 0.5
        activeColor: Theme.blue
    }
    Telltale {
        label: "◀"
        active: (Vehicle.values.turnLeft ?? 0) > 0.5
        activeColor: Theme.green
    }
    Telltale {
        label: "▶"
        active: (Vehicle.values.turnRight ?? 0) > 0.5
        activeColor: Theme.green
    }
    Telltale {
        label: "BRK"
        active: (Vehicle.values.brakePct ?? 0) > 3
        activeColor: Theme.red
    }
    Telltale {
        label: "ABS"
        active: (Vehicle.values.abs ?? 0) > 0.5
        activeColor: Theme.accent
    }
    Telltale {
        label: "ASC"
        active: (Vehicle.values.asc ?? 0) > 0.5
        activeColor: Theme.accent
    }
    Telltale {
        label: "FUEL"
        active: (Vehicle.values.fuelLamp ?? 0) > 0.5
        activeColor: Theme.accent
    }
}
