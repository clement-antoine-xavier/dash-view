import QtQuick

Item {
    id: root

    property bool active: false
    property var tagRows: []
    property var recentRows: []

    visible: active
    z: 50

    function refresh() {
        tagRows = Vehicle.debugRows();
        recentRows = Vehicle.debugRecent();
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
            text: "TELEMETRY DEBUG"
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
                id: tagPane
                width: Math.round(parent.width * 0.62)
                height: parent.height
                anchors.left: parent.left

                Row {
                    id: tagHeader
                    spacing: 6

                    Text { width: 52;  text: "TAG";    color: Theme.dim; font.family: Theme.fontMain; font.pixelSize: 8; font.bold: true; font.letterSpacing: 1.4 }
                    Text { width: 200; text: "SIGNAL"; color: Theme.dim; font.family: Theme.fontMain; font.pixelSize: 8; font.bold: true; font.letterSpacing: 1.4 }
                    Text { width: 100; text: "VALUE";  color: Theme.dim; font.family: Theme.fontMain; font.pixelSize: 8; font.bold: true; font.letterSpacing: 1.4 }
                    Text { width: 54;  text: "UNIT";   color: Theme.dim; font.family: Theme.fontMain; font.pixelSize: 8; font.bold: true; font.letterSpacing: 1.4 }
                    Text { width: 64;  text: "AGE";    color: Theme.dim; font.family: Theme.fontMain; font.pixelSize: 8; font.bold: true; font.letterSpacing: 1.4 }
                    Text { width: 56;  text: "COUNT";  color: Theme.dim; font.family: Theme.fontMain; font.pixelSize: 8; font.bold: true; font.letterSpacing: 1.4 }
                }

                ListView {
                    id: tagList
                    anchors.top: tagHeader.bottom
                    anchors.topMargin: 8
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.bottom: parent.bottom
                    clip: true
                    model: root.tagRows
                    spacing: 1

                    delegate: Row {
                        width: tagList.width
                        height: 16
                        spacing: 6

                        Text { width: 52;  text: modelData.tag; color: Theme.dim; font.family: Theme.fontMono; font.pixelSize: 10 }
                        Text { width: 200; text: modelData.label; elide: Text.ElideRight; color: modelData.fresh ? Theme.text : Theme.dim; font.family: Theme.fontMain; font.pixelSize: 10 }
                        Text { width: 100; text: Theme.fmt(modelData.value, modelData.decimals); color: modelData.fresh ? Theme.text : Theme.dim; font.family: Theme.fontMono; font.pixelSize: 10 }
                        Text { width: 54;  text: modelData.unit; color: Theme.dim; font.family: Theme.fontMono; font.pixelSize: 10 }
                        Text { width: 64;  text: modelData.ageMs < 0 ? "—" : modelData.ageMs + " ms"; color: Theme.dim; font.family: Theme.fontMono; font.pixelSize: 10 }
                        Text { width: 56;  text: modelData.count; color: Theme.dim; font.family: Theme.fontMono; font.pixelSize: 10; horizontalAlignment: Text.AlignRight }
                    }
                }
            }

            Item {
                id: recentPane
                width: Math.round(parent.width * 0.35)
                height: parent.height
                anchors.right: parent.right

                Text {
                    id: recentTitle
                    text: "RECENT VALUES"
                    color: Theme.dim
                    font.family: Theme.fontMain
                    font.pixelSize: 8
                    font.bold: true
                    font.letterSpacing: 1.4
                }

                Row {
                    id: recentHeader
                    anchors.top: recentTitle.bottom
                    anchors.topMargin: 8
                    spacing: 6

                    Text { width: 52;  text: "AGE";   color: Theme.dim; font.family: Theme.fontMain; font.pixelSize: 8; font.bold: true; font.letterSpacing: 1.4 }
                    Text { width: 48;  text: "TAG";   color: Theme.dim; font.family: Theme.fontMain; font.pixelSize: 8; font.bold: true; font.letterSpacing: 1.4 }
                    Text { width: 170; text: "NAME";  color: Theme.dim; font.family: Theme.fontMain; font.pixelSize: 8; font.bold: true; font.letterSpacing: 1.4 }
                    Text { width: 90;  text: "VALUE"; color: Theme.dim; font.family: Theme.fontMain; font.pixelSize: 8; font.bold: true; font.letterSpacing: 1.4 }
                }

                ListView {
                    id: recentList
                    anchors.top: recentHeader.bottom
                    anchors.topMargin: 8
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.bottom: parent.bottom
                    clip: true
                    model: root.recentRows
                    spacing: 1

                    delegate: Row {
                        width: recentList.width
                        height: 16
                        spacing: 6

                        Text { width: 52;  text: modelData.ageMs + " ms"; color: Theme.dim; font.family: Theme.fontMono; font.pixelSize: 10 }
                        Text { width: 48;  text: modelData.tag; color: Theme.dim; font.family: Theme.fontMono; font.pixelSize: 10 }
                        Text { width: 170; text: modelData.name; elide: Text.ElideRight; color: Theme.text; font.family: Theme.fontMono; font.pixelSize: 10 }
                        Text { width: 90;  text: Theme.fmt(modelData.value, 2); color: Theme.text; font.family: Theme.fontMono; font.pixelSize: 10; horizontalAlignment: Text.AlignRight }
                    }
                }
            }
        }
    }
}
