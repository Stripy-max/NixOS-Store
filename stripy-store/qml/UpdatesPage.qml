import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Flickable {
    id: root
    signal updateRequested()
    signal applyRequested()
    clip: true
    contentWidth: width
    contentHeight: col.implicitHeight + 2 * Theme.pad
    boundsBehavior: Flickable.StopAtBounds

    ColumnLayout {
        id: col
        x: Theme.pad; y: Theme.pad
        width: Math.min(root.width - 2 * Theme.pad, 640)
        spacing: 14
        Label { text: qsTr("Updates"); font.pixelSize: 24; font.bold: true }
        Label {
            Layout.fillWidth: true; wrapMode: Text.Wrap; color: Theme.dim
            text: qsTr("All applications come from one package source (nixpkgs), pinned in your system configuration. Applications are not updated one by one: updating refreshes the package source and rebuilds the system, so applications and the operating system update together.")
        }
        Rectangle {
            Layout.fillWidth: true; implicitHeight: info.implicitHeight + 24
            radius: Theme.radius; color: Theme.surface; border.color: Theme.border
            ColumnLayout {
                id: info
                anchors.fill: parent; anchors.margins: 12; spacing: 4
                Label { text: qsTr("Current package source"); font.bold: true }
                Label { text: backend.pinInfo.length ? backend.pinInfo : qsTr("Unknown (flake.lock not found or unreadable)"); color: Theme.dim }
                Label { text: backend.scope; color: Theme.dim; wrapMode: Text.WrapAnywhere; Layout.fillWidth: true; font.pixelSize: 12 }
            }
        }
        Label {
            Layout.fillWidth: true; wrapMode: Text.Wrap; color: Theme.dim
            text: qsTr("Stripy Store cannot tell in advance whether a newer package source exists without downloading it, so it does not guess. Press the button to check and apply the newest one.")
        }
        RowLayout {
            spacing: 8
            Button {
                text: qsTr("Update StripyOS…"); highlighted: true
                enabled: !backend.busy && backend.health === "ok"
                onClicked: root.updateRequested()
            }
            Button {
                text: qsTr("Re-apply current configuration…")
                enabled: !backend.busy && backend.health === "ok"
                onClicked: root.applyRequested()
            }
        }
        Label {
            visible: backend.health !== "ok"
            Layout.fillWidth: true; wrapMode: Text.Wrap; color: Theme.warn
            text: qsTr("Updates are unavailable until the system integration is set up (see Settings).")
        }
    }
}
