import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Control {
    id: card
    required property string attr
    required property string name
    required property string description
    required property string version
    required property string iconName
    required property string categoriesText
    required property string license
    required property string installState

    signal activated()
    signal action()

    implicitHeight: 76
    hoverEnabled: true
    activeFocusOnTab: true
    Accessible.role: Accessible.Button
    Accessible.name: name + (Theme.statusText(installState).length ? ", " + Theme.statusText(installState) : "")
    Accessible.description: description
    Accessible.onPressAction: activated()
    Keys.onReturnPressed: activated()
    Keys.onEnterPressed: activated()
    Keys.onSpacePressed: activated()

    background: Rectangle {
        radius: Theme.radius
        color: card.hovered ? Theme.surfaceHover : Theme.surface
        border.width: card.visualFocus ? 2 : 1
        border.color: card.visualFocus ? Theme.accent : Theme.border
    }

    contentItem: RowLayout {
        spacing: Theme.gap
        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true
            RowLayout {
                anchors.fill: parent
                spacing: Theme.gap
                AppBadge { appName: card.name; attr: card.attr; iconName: card.iconName; Layout.alignment: Qt.AlignVCenter }
                ColumnLayout {
                    Layout.fillWidth: true
                    Layout.alignment: Qt.AlignVCenter
                    spacing: 1
                    RowLayout {
                        spacing: 6
                        Label { text: card.name; font.bold: true; font.pixelSize: 14; elide: Text.ElideRight; Layout.maximumWidth: 220 }
                        Label {
                            visible: card.license === "unfree"
                            text: qsTr("Non-free"); font.pixelSize: 10; color: Theme.warn
                            leftPadding: 5; rightPadding: 5
                            background: Rectangle { radius: 4; color: "transparent"; border.color: Theme.warn }
                        }
                    }
                    Label {
                        Layout.fillWidth: true
                        text: card.description.length ? card.description : card.attr
                        elide: Text.ElideRight; font.pixelSize: 12; color: Theme.dim
                    }
                    Label {
                        Layout.fillWidth: true
                        visible: text.length > 0
                        text: card.categoriesText.length ? card.categoriesText : (card.version.length ? card.version : "")
                        elide: Text.ElideRight; font.pixelSize: 11; color: Theme.dim; opacity: 0.85
                    }
                }
            }
            MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor; onClicked: card.activated() }
        }
        ColumnLayout {
            Layout.alignment: Qt.AlignVCenter
            spacing: 2
            Label {
                Layout.alignment: Qt.AlignRight
                visible: text.length > 0
                text: Theme.statusText(card.installState)
                font.pixelSize: 11
                color: Theme.statusColor(card.installState)
            }
            Button {
                text: Theme.actionText(card.installState)
                enabled: !backend.busy
                highlighted: !Theme.isRemoval(card.installState)
                Accessible.name: text + " " + card.name
                onClicked: card.action()
            }
        }
    }
}
