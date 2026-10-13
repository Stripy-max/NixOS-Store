import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: bar
    property bool expanded: false
    visible: backend.busy || backend.hasResult
    implicitHeight: visible ? lay.implicitHeight + 20 : 0
    color: Theme.surface
    border.color: backend.busy ? Theme.border : (backend.resultOk ? Theme.ok : Theme.danger)
    border.width: 1
    Accessible.role: Accessible.StatusBar
    Accessible.name: backend.busy ? backend.opTitle + ". " + backend.stage : backend.resultText

    ColumnLayout {
        id: lay
        anchors.fill: parent; anchors.margins: 10; spacing: 6
        RowLayout {
            spacing: 10
            Label {
                Layout.fillWidth: true; wrapMode: Text.Wrap
                text: backend.busy
                    ? "<b>" + backend.opTitle + "</b> — " + backend.stage
                    : (backend.resultOk ? "✓ " : "✗ ") + backend.resultText
                textFormat: Text.StyledText
            }
            Button { text: bar.expanded ? qsTr("Hide details") : qsTr("Details"); flat: true; onClicked: bar.expanded = !bar.expanded }
            Button { text: qsTr("Dismiss"); flat: true; visible: !backend.busy; onClicked: { bar.expanded = false; backend.dismissResult() } }
        }
        ProgressBar { Layout.fillWidth: true; visible: backend.busy; indeterminate: true }
        ScrollView {
            visible: bar.expanded
            Layout.fillWidth: true; Layout.preferredHeight: 170
            TextArea {
                readOnly: true; selectByMouse: true
                text: backend.log.length ? backend.log : qsTr("No output yet.")
                font.family: "monospace"; font.pixelSize: 11
                onTextChanged: cursorPosition = length
                Accessible.name: qsTr("Operation output")
            }
        }
    }
}
