import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Flickable {
    id: root
    signal openApp(string attr)
    signal act(string attr, string state, string name, string license)
    signal openCategory(string id)

    clip: true
    contentWidth: width
    contentHeight: col.implicitHeight + 2 * Theme.pad
    ScrollBar.vertical: ScrollBar {}
    boundsBehavior: Flickable.StopAtBounds

    readonly property int cols: Math.max(1, Math.floor((width - 2 * Theme.pad + Theme.gap) / 340))
    readonly property real cardW: (width - 2 * Theme.pad - (cols - 1) * Theme.gap) / cols

    ColumnLayout {
        id: col
        x: Theme.pad; y: Theme.pad
        width: root.width - 2 * Theme.pad
        spacing: 22

        ColumnLayout {
            spacing: 2
            Label { text: qsTr("Discover"); font.pixelSize: 24; font.bold: true }
            Label { text: qsTr("Find applications for your system. Search above or browse below."); color: Theme.dim }
        }

        Label {
            visible: backend.featuredModel.count === 0 && backend.picksModel.count === 0
            text: qsTr("The application catalog is empty. You can still use search to find packages.")
            color: Theme.dim; wrapMode: Text.Wrap; Layout.fillWidth: true
        }

        ColumnLayout {
            visible: backend.featuredModel.count > 0
            spacing: 8
            Label { text: qsTr("Featured"); font.pixelSize: 16; font.bold: true }
            Flow {
                Layout.fillWidth: true; spacing: Theme.gap
                Repeater {
                    model: backend.featuredModel
                    delegate: AppCard {
                        width: root.cardW
                        onActivated: root.openApp(attr)
                        onAction: root.act(attr, installState, name, license)
                    }
                }
            }
        }

        ColumnLayout {
            visible: backend.picksModel.count > 0
            spacing: 8
            Label { text: qsTr("Curated picks"); font.pixelSize: 16; font.bold: true }
            Flow {
                Layout.fillWidth: true; spacing: Theme.gap
                Repeater {
                    model: backend.picksModel
                    delegate: AppCard {
                        width: root.cardW
                        onActivated: root.openApp(attr)
                        onAction: root.act(attr, installState, name, license)
                    }
                }
            }
        }

        ColumnLayout {
            visible: backend.categories.length > 0
            spacing: 8
            Label { text: qsTr("Categories"); font.pixelSize: 16; font.bold: true }
            Flow {
                Layout.fillWidth: true; spacing: 8
                Repeater {
                    model: backend.categories
                    delegate: Button {
                        required property var modelData
                        text: modelData.name + "  ·  " + modelData.count
                        icon.name: modelData.icon
                        onClicked: root.openCategory(modelData.id)
                    }
                }
            }
        }
    }
}
