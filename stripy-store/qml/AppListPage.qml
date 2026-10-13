import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ColumnLayout {
    id: root
    signal openApp(string attr)
    signal act(string attr, string state, string name, string license)
    signal applyRequested()
    spacing: 8

    RowLayout {
        Layout.fillWidth: true
        Layout.leftMargin: Theme.pad; Layout.rightMargin: Theme.pad; Layout.topMargin: Theme.pad
        ColumnLayout {
            spacing: 0
            Label { text: backend.viewTitle; font.pixelSize: 22; font.bold: true; elide: Text.ElideRight; Layout.maximumWidth: root.width * 0.6 }
            Label { text: qsTr("%n app(s)", "", grid.count); color: Theme.dim; font.pixelSize: 12 }
        }
        Item { Layout.fillWidth: true }
        BusyIndicator { running: backend.searching; visible: running; implicitWidth: 28; implicitHeight: 28; Accessible.name: qsTr("Searching") }
        ComboBox {
            id: sortBox
            Accessible.name: qsTr("Sort by")
            textRole: "text"; valueRole: "id"
            model: backend.view === "search"
                ? [{id: "relevance", text: qsTr("Best match")}, {id: "az", text: qsTr("Name A–Z")}, {id: "za", text: qsTr("Name Z–A")}, {id: "installed", text: qsTr("Installed first")}]
                : [{id: "recommended", text: qsTr("Recommended")}, {id: "az", text: qsTr("Name A–Z")}, {id: "za", text: qsTr("Name Z–A")}, {id: "installed", text: qsTr("Installed first")}]
            currentIndex: Math.max(0, indexOfValue(backend.sort))
            onActivated: backend.setSort(currentValue)
        }
    }

    Rectangle {
        visible: backend.searchNotice.length > 0
        Layout.fillWidth: true; Layout.leftMargin: Theme.pad; Layout.rightMargin: Theme.pad
        implicitHeight: noteLabel.implicitHeight + 16
        radius: Theme.radius; color: Theme.surfaceAlt; border.color: Theme.border
        Label { id: noteLabel; anchors.fill: parent; anchors.margins: 8; text: backend.searchNotice; wrapMode: Text.Wrap; color: Theme.dim }
    }

    Rectangle {
        visible: backend.view === "installed" && backend.pendingCount > 0
        Layout.fillWidth: true; Layout.leftMargin: Theme.pad; Layout.rightMargin: Theme.pad
        implicitHeight: pend.implicitHeight + 16
        radius: Theme.radius; color: Theme.surfaceAlt; border.color: Theme.warn
        RowLayout {
            id: pend
            anchors.fill: parent; anchors.margins: 8
            Label {
                Layout.fillWidth: true; wrapMode: Text.Wrap
                text: qsTr("Some entries are not confirmed as applied to the running system. Applying the configuration will make them match.")
            }
            Button { text: qsTr("Apply configuration"); enabled: !backend.busy && backend.health === "ok"; onClicked: root.applyRequested() }
        }
    }

    GridView {
        id: grid
        Layout.fillWidth: true; Layout.fillHeight: true
        Layout.leftMargin: Theme.pad; Layout.rightMargin: Theme.pad - 12
        clip: true
        model: backend.appModel
        boundsBehavior: Flickable.StopAtBounds
        ScrollBar.vertical: ScrollBar {}
        readonly property int cols: Math.max(1, Math.floor(width / 340))
        cellWidth: Math.floor(width / cols)
        cellHeight: 88
        delegate: AppCard {
            width: grid.cellWidth - 12
            height: 76
            onActivated: root.openApp(attr)
            onAction: root.act(attr, installState, name, license)
        }

        Label {
            anchors.centerIn: parent
            width: parent.width * 0.8
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.Wrap
            color: Theme.dim
            visible: grid.count === 0 && !backend.searching
            text: backend.view === "search" ? qsTr("No applications match “%1”. Try a different word, or a more general one.").arg(backend.query)
                : backend.view === "installed" ? qsTr("Nothing has been installed with Stripy Store yet. Open Discover to find applications.")
                : qsTr("There are no applications in this category yet.")
        }
    }
}
