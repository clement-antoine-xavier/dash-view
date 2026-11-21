pragma Singleton

import QtQuick

QtObject {
    readonly property color bg: "#05070a"
    readonly property color panel: "#0b1017"
    readonly property color panelAlt: "#121a24"
    readonly property color line: "#1e2a38"
    readonly property color text: "#e9f0f8"
    readonly property color dim: "#7d8b9c"
    readonly property color accent: "#f4b942"
    readonly property color red: "#ff453a"
    readonly property color green: "#34e5a2"
    readonly property color blue: "#59a9ff"
    readonly property color cyan: "#3fd8e8"
    readonly property int radius: 14

    readonly property string fontMain: "Helvetica Neue"
    readonly property string fontMono: "Menlo"

    function fmt(v, decimals) {
        if (v === undefined || v === null)
            return "—";
        const n = Number(v);
        if (!isFinite(n))
            return "—";
        return n.toFixed(decimals === undefined ? 0 : decimals);
    }

    function pct(v, min, max) {
        if (v === undefined || v === null)
            return 0;
        const n = Number(v);
        if (!isFinite(n) || max === min)
            return 0;
        return Math.max(0, Math.min(1, (n - min) / (max - min)));
    }
}
