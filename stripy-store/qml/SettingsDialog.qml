import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Dialog {
    id: dlg
    modal: true
    title: qsTr("Settings")
    anchors.centerIn: Overlay.overlay
    width: Math.min(560, (Overlay.overlay ? Overlay.overlay.width : 560) - 24)
    height: Math.min(640, (Overlay.overlay ? Overlay.overlay.height : 640) - 24)
    padding: 16
    standardButtons: Dialog.Close
    onAboutToShow: { dirField.text = backend.flakeDir; hostField.text = backend.host; saveMsg.text = "" }

    contentItem: ScrollView {
        id: sv
        clip: true
        contentWidth: availableWidth
        ColumnLayout {
            width: sv.availableWidth
            spacing: 14

            Label { text: qsTr("Appearance"); font.bold: true }
            ComboBox {
                Layout.fillWidth: true
                Accessible.name: qsTr("Appearance")
                textRole: "text"; valueRole: "id"
                model: [{id: "system", text: qsTr("Follow the system")}, {id: "light", text: qsTr("Light")}, {id: "dark", text: qsTr("Dark")}]
                currentIndex: Math.max(0, indexOfValue(backend.appearance))
                onActivated: backend.appearance = currentValue
            }
            CheckBox {
                text: qsTr("Ask before installing")
                checked: backend.confirmInstall
                onToggled: backend.confirmInstall = checked
            }

            Rectangle { Layout.fillWidth: true; height: 1; color: Theme.border }

            Label { text: qsTr("System integration"); font.bold: true }
            Label {
                Layout.fillWidth: true; wrapMode: Text.Wrap
                color: backend.health === "ok" ? Theme.ok : Theme.warn
                text: backend.health === "ok" ? "✓ " + qsTr("Ready. %1").arg(backend.scope)
                    : backend.problems.join("\n\n")
            }
            Label { text: qsTr("Configuration folder (contains flake.nix)"); font.pixelSize: 12; color: Theme.dim }
            TextField { id: dirField; Layout.fillWidth: true; selectByMouse: true; Accessible.name: qsTr("Configuration folder") }
            Label { text: qsTr("Configuration name (nixosConfigurations.<name>)"); font.pixelSize: 12; color: Theme.dim }
            TextField { id: hostField; Layout.fillWidth: true; selectByMouse: true; Accessible.name: qsTr("Configuration name") }
            RowLayout {
                spacing: 8
                Button {
                    text: qsTr("Save and re-check"); enabled: !backend.busy
                    onClicked: { var e = backend.setConfig(dirField.text, hostField.text); saveMsg.text = e.length ? e : qsTr("Saved.") }
                }
                Button {
                    visible: backend.canCreateAppsFile
                    text: qsTr("Create apps.json")
                    onClicked: backend.createAppsFile()
                }
                Button { text: qsTr("Re-check"); onClicked: backend.recheck() }
            }
            Label { id: saveMsg; Layout.fillWidth: true; wrapMode: Text.Wrap; color: Theme.dim }

            Rectangle { Layout.fillWidth: true; height: 1; color: Theme.border }

            Label { text: qsTr("Diagnostics"); font.bold: true }
            Label { Layout.fillWidth: true; wrapMode: Text.WrapAnywhere; color: Theme.dim; font.pixelSize: 12
                text: qsTr("Operation log file: %1").arg(backend.logFilePath) }
            Label {
                visible: backend.catalogWarnings.length > 0
                Layout.fillWidth: true; wrapMode: Text.Wrap; color: Theme.warn; font.pixelSize: 12
                text: qsTr("Catalog notes:") + "\n" + backend.catalogWarnings.join("\n")
            }
            Label { text: qsTr("Stripy Store %1 · part of StripyOS").arg(backend.appVersion); color: Theme.dim; font.pixelSize: 12 }
        }
    }
}
