import QtQuick

Item {
    id: root

    property bool active: false
    property var sigRows: []
    property var frameRows: []

    visible: active
    z: 50

    function refresh() {
        sigRows = Vehicle.debugRows();
        frameRows = Vehicle.debugFrames();
    }

    onActiveChanged: {
        if (active)
            refresh();
    }

    Timer {
        interval: 250
        repeat: true
        running: root.active
        onTriggered: root.refresh()
    }

    Rectangle {
        anchors.fill: parent
        color: "#ee030609"
    }

    Rectangle {
        anchors.fill: parent
        anchors.margins: 22
        radius: 16
        color: "#f50a1017"
        border.width: 1
        border.color: Theme.line

        Text {
            x: 24
            y: 18
            text: "DECODER DEBUG"
            color: Theme.text
            font.family: Theme.fontMain
            font.pixelSize: 15
            font.bold: true
            font.letterSpacing: 3.0
        }

        Text {
            anchors.right: parent.right
            anchors.rightMargin: 24
            y: 22
            text: "D close"
            color: Theme.dim
            font.family: Theme.fontMono
            font.pixelSize: 10
        }

        Item {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            anchors.margins: 24
            anchors.topMargin: 56

            Item {
                id: sigPane
                width: Math.round(parent.width * 0.64)
                height: parent.height
                anchors.left: parent.left

                Row {
                    id: sigHeader
                    spacing: 6

                    Text { width: 52;  text: "ID";     color: Theme.dim; font.family: Theme.fontMain; font.pixelSize: 8; font.bold: true; font.letterSpacing: 1.4 }
                    Text { width: 220; text: "SIGNAL"; color: Theme.dim; font.family: Theme.fontMain; font.pixelSize: 8; font.bold: true; font.letterSpacing: 1.4 }
                    Text { width: 110; text: "RAW";    color: Theme.dim; font.family: Theme.fontMain; font.pixelSize: 8; font.bold: true; font.letterSpacing: 1.4 }
                    Text { width: 100; text: "VALUE";  color: Theme.dim; font.family: Theme.fontMain; font.pixelSize: 8; font.bold: true; font.letterSpacing: 1.4 }
                    Text { width: 50;  text: "UNIT";   color: Theme.dim; font.family: Theme.fontMain; font.pixelSize: 8; font.bold: true; font.letterSpacing: 1.4 }
                    Text { width: 64;  text: "AT";     color: Theme.dim; font.family: Theme.fontMain; font.pixelSize: 8; font.bold: true; font.letterSpacing: 1.4 }
                    Text { width: 60;  text: "AGE";    color: Theme.dim; font.family: Theme.fontMain; font.pixelSize: 8; font.bold: true; font.letterSpacing: 1.4 }
                }

                ListView {
                    id: sigList
                    anchors.top: sigHeader.bottom
                    anchors.topMargin: 8
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.bottom: parent.bottom
                    clip: true
                    model: root.sigRows
                    spacing: 1

                    delegate: Row {
                        width: sigList.width
                        height: 16
                        spacing: 6

                        Text { width: 52;  text: modelData.id; color: Theme.dim; font.family: Theme.fontMono; font.pixelSize: 10 }
                        Text { width: 220; text: modelData.label; elide: Text.ElideRight; color: modelData.fresh ? Theme.text : Theme.dim; font.family: Theme.fontMain; font.pixelSize: 10 }
                        Text { width: 110; text: modelData.rawHex; color: modelData.fresh ? Theme.text : Theme.dim; font.family: Theme.fontMono; font.pixelSize: 10 }
                        Text { width: 100; text: Theme.fmt(modelData.value, 2); color: modelData.fresh ? Theme.text : Theme.dim; font.family: Theme.fontMono; font.pixelSize: 10 }
                        Text { width: 50;  text: modelData.unit; color: Theme.dim; font.family: Theme.fontMono; font.pixelSize: 10 }
                        Text { width: 64;  text: modelData.at; color: Theme.dim; font.family: Theme.fontMono; font.pixelSize: 10 }
                        Text { width: 60;  text: modelData.ageMs < 0 ? "—" : modelData.ageMs + " ms"; color: Theme.dim; font.family: Theme.fontMono; font.pixelSize: 10 }
                    }
                }
            }

            Item {
                id: framePane
                width: Math.round(parent.width * 0.33)
                height: parent.height
                anchors.right: parent.right

                Row {
                    id: frameHeader
                    spacing: 6

                    Text { width: 52;  text: "ID";      color: Theme.dim; font.family: Theme.fontMain; font.pixelSize: 8; font.bold: true; font.letterSpacing: 1.4 }
                    Text { width: 32;  text: "DLC";     color: Theme.dim; font.family: Theme.fontMain; font.pixelSize: 8; font.bold: true; font.letterSpacing: 1.4 }
                    Text { width: 230; text: "PAYLOAD"; color: Theme.dim; font.family: Theme.fontMain; font.pixelSize: 8; font.bold: true; font.letterSpacing: 1.4 }
                    Text { width: 44;  text: "CNT";     color: Theme.dim; font.family: Theme.fontMain; font.pixelSize: 8; font.bold: true; font.letterSpacing: 1.4 }
                    Text { width: 52;  text: "AGE";     color: Theme.dim; font.family: Theme.fontMain; font.pixelSize: 8; font.bold: true; font.letterSpacing: 1.4 }
                }

                ListView {
                    id: frameList
                    anchors.top: frameHeader.bottom
                    anchors.topMargin: 8
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.bottom: parent.bottom
                    clip: true
                    model: root.frameRows
                    spacing: 1

                    delegate: Row {
                        width: frameList.width
                        height: 16
                        spacing: 6

                        Text { width: 52;  text: modelData.id; color: Theme.text; font.family: Theme.fontMono; font.pixelSize: 10 }
                        Text { width: 32;  text: modelData.dlc; color: Theme.dim; font.family: Theme.fontMono; font.pixelSize: 10 }
                        Text { width: 230; text: modelData.payload; elide: Text.ElideRight; color: Theme.text; font.family: Theme.fontMono; font.pixelSize: 10 }
                        Text { width: 44;  text: modelData.count; color: Theme.dim; font.family: Theme.fontMono; font.pixelSize: 10; horizontalAlignment: Text.AlignRight }
                        Text { width: 52;  text: modelData.ageMs + " ms"; color: Theme.dim; font.family: Theme.fontMono; font.pixelSize: 10; horizontalAlignment: Text.AlignRight }
                    }
                }
            }
        }
    }
}
