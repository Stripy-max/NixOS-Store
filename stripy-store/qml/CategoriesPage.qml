import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Flickable {
    id: root
    signal openCategory(string id)
    clip: true
    contentWidth: width
    contentHeight: col.implicitHeight + 2 * Theme.pad
    ScrollBar.vertical: ScrollBar {}
    boundsBehavior: Flickable.StopAtBounds

    readonly property int cols: Math.max(1, Math.floor((width - 2 * Theme.pad + Theme.gap) / 260))
    readonly property real cellW: (width - 2 * Theme.pad - (cols - 1) * Theme.gap) / cols

    ColumnLayout {
        id: col
        x: Theme.pad; y: Theme.pad
        width: root.width - 2 * Theme.pad
        spacing: 14
        Label { text: qsTr("Categories"); font.pixelSize: 24; font.bold: true }
        Label {
            visible: backend.categories.length === 0
            text: qsTr("No categories are available."); color: Theme.dim
        }
        Flow {
            Layout.fillWidth: true; spacing: Theme.gap
            Repeater {
                model: backend.categories
                delegate: Button {
                    required property var modelData
                    width: root.cellW; height: 64
                    icon.name: modelData.icon
                    icon.width: 28; icon.height: 28
                    text: modelData.name + "\n" + qsTr("%n app(s)", "", modelData.count)
                    onClicked: root.openCategory(modelData.id)
                }
            }
        }
    }
}
