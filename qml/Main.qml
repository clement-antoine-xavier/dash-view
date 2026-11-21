import QtQuick
import QtQuick.Window

Window {
    id: win

    width: 1280
    height: 800
    minimumWidth: 960
    minimumHeight: 600
    visible: true
    color: Theme.bg
    title: "Dash View"

    Item {
        id: stage
        width: 1440
        height: 900
        anchors.centerIn: parent
        scale: Math.min(win.width / width, win.height / height)

        Dashboard {
            anchors.fill: parent
        }

        DebugOverlay {
            id: debug
            anchors.fill: parent
        }
    }

    Shortcut {
        sequence: "D"
        onActivated: debug.active = !debug.active
    }

    Shortcut {
        sequence: "F"
        onActivated: win.visibility = (win.visibility === Window.FullScreen)
                                     ? Window.Windowed
                                     : Window.FullScreen
    }

    Shortcut {
        sequence: "Escape"
        enabled: debug.active
        onActivated: debug.active = false
    }

    Component.onCompleted: {
        if (StartDebug)
            debug.active = true;
    }
}
