import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Drawer {
    id: drawer
    signal act(string attr, string state, string name, string license)

    property string attr: ""
    property var info: ({})
    readonly property var win: ApplicationWindow.window

    edge: Qt.RightEdge
    width: Math.min(440, win ? win.width : 440)
    height: win ? win.height : 600
    modal: win ? win.width < 800 : true
    dim: modal
    padding: 0

    function show(a) {
        attr = a
        refresh()
        backend.fetchDetails(a)
        open()
    }
    function refresh() { info = backend.appInfo(attr) }

    Connections {
        target: backend
        function onStateRevisionChanged() { if (drawer.opened) drawer.refresh() }
        function onDetailsReady(a) { if (a === drawer.attr) drawer.refresh() }
    }

    component Row2: ColumnLayout {
        property string label
        property string value
        visible: value.length > 0
        spacing: 0
        Layout.fillWidth: true
        Label { text: parent.label; font.pixelSize: 11; color: Theme.dim }
        Label { text: parent.value; wrapMode: Text.Wrap; Layout.fillWidth: true }
    }

    background: Rectangle { color: Theme.bg; border.color: Theme.border }

    contentItem: ScrollView {
        id: sv
        clip: true
        contentWidth: availableWidth
        ColumnLayout {
            width: sv.availableWidth
            spacing: 14
            Item { implicitHeight: 4 }
            RowLayout {
                Layout.fillWidth: true; Layout.leftMargin: 16; Layout.rightMargin: 16; spacing: 14
                AppBadge { appName: drawer.info.name || ""; attr: drawer.attr; size: 64 }
                ColumnLayout {
                    Layout.fillWidth: true; spacing: 2
                    Label { text: drawer.info.name || ""; font.pixelSize: 22; font.bold: true; wrapMode: Text.Wrap; Layout.fillWidth: true }
                    Label { text: drawer.info.version ? qsTr("Version %1").arg(drawer.info.version) : ""; visible: text.length > 0; color: Theme.dim }
                }
                Button { text: qsTr("Close"); flat: true; onClicked: drawer.close(); Accessible.name: qsTr("Close details") }
            }
            ColumnLayout {
                Layout.fillWidth: true; Layout.leftMargin: 16; Layout.rightMargin: 16; spacing: 12

                Label {
                    Layout.fillWidth: true; wrapMode: Text.Wrap
                    text: drawer.info.description || qsTr("No description available.")
                }

                RowLayout {
                    spacing: 10
                    Button {
                        text: Theme.actionText(drawer.info.state || "none")
                        highlighted: !Theme.isRemoval(drawer.info.state || "none")
                        enabled: !backend.busy
                        onClicked: drawer.act(drawer.attr, drawer.info.state || "none", drawer.info.name || drawer.attr, drawer.info.license || "unknown")
                    }
                    Label {
                        text: Theme.statusText(drawer.info.state || "none")
                        color: Theme.statusColor(drawer.info.state || "none")
                    }
                }
                Label {
                    visible: !!drawer.info.failure
                    Layout.fillWidth: true; wrapMode: Text.Wrap; color: Theme.danger
                    text: drawer.info.failure || ""
                }

                Rectangle { Layout.fillWidth: true; height: 1; color: Theme.border }

                Row2 { label: qsTr("License"); value: {
                        var l = drawer.info.license
                        var t = drawer.info.licenseText ? " (" + drawer.info.licenseText + ")" : ""
                        if (l === "unfree") return qsTr("Non-free software") + t
                        if (l === "free") return qsTr("Free software") + t
                        return qsTr("Not checked yet") } }
                Row2 { label: qsTr("Categories"); value: drawer.info.categoriesText || "" }
                Row2 { label: qsTr("Source"); value: qsTr("nixpkgs, installed system-wide") }
                Row2 { label: qsTr("Package name"); value: drawer.attr }

                ColumnLayout {
                    visible: !!drawer.info.homepage
                    spacing: 0
                    Label { text: qsTr("Website"); font.pixelSize: 11; color: Theme.dim }
                    Label {
                        Layout.fillWidth: true; wrapMode: Text.WrapAnywhere
                        text: "<a href=\"" + drawer.info.homepage + "\">" + drawer.info.homepage + "</a>"
                        textFormat: Text.StyledText
                        linkColor: Theme.accent
                        onLinkActivated: link => backend.openHomepage(link)
                    }
                }
                Label {
                    visible: !drawer.info.detailsLoaded
                    Layout.fillWidth: true; wrapMode: Text.Wrap; color: Theme.dim; font.pixelSize: 11
                    text: qsTr("Extra details (version, license) are read from the package source when available.")
                }
            }
            Item { implicitHeight: 16 }
        }
    }
}
