import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Dialog {
    id: dlg
    property string kind: ""
    property string heading: ""
    property string body: ""
    property string acceptText: qsTr("OK")
    property bool showRemember: false
    property bool remember: false
    signal confirmed(string kind)

    modal: true
    anchors.centerIn: Overlay.overlay
    width: Math.min(460, (Overlay.overlay ? Overlay.overlay.width : 460) - 32)
    title: heading
    padding: 16
    onAboutToShow: remember = false

    contentItem: ColumnLayout {
        spacing: 12
        Label { Layout.fillWidth: true; text: dlg.body; wrapMode: Text.Wrap }
        CheckBox {
            visible: dlg.showRemember
            text: qsTr("Don't ask again for installations")
            checked: dlg.remember
            onToggled: dlg.remember = checked
        }
    }
    footer: DialogButtonBox {
        Button { text: qsTr("Cancel"); DialogButtonBox.buttonRole: DialogButtonBox.RejectRole }
        Button { text: dlg.acceptText; highlighted: true; DialogButtonBox.buttonRole: DialogButtonBox.AcceptRole }
    }
    onAccepted: {
        if (showRemember && remember) backend.confirmInstall = false
        confirmed(kind)
    }
}
