import QtQuick
import QtQuick.Controls

// App artwork: a bundled SVG when one exists, otherwise a letter badge.
// Nothing is downloaded and nothing is invented.
Rectangle {
    id: badge
    property string appName: ""
    property string attr: ""
    property string iconName: ""
    property int size: 44

    width: size; height: size; radius: size * 0.22
    readonly property real hue: {
        var h = 0
        for (var i = 0; i < attr.length; ++i) h = (h * 31 + attr.charCodeAt(i)) % 360
        return h / 360
    }
    color: Qt.hsla(hue, 0.45, Theme.dark ? 0.32 : 0.84, 1)
    Accessible.ignored: true

    Label {
        anchors.centerIn: parent
        visible: !img.visible
        text: badge.appName.length ? badge.appName.charAt(0).toUpperCase() : "?"
        font.pixelSize: badge.size * 0.45
        font.bold: true
        color: Qt.hsla(badge.hue, 0.5, Theme.dark ? 0.85 : 0.25, 1)
    }
    Image {
        id: img
        anchors.fill: parent
        anchors.margins: badge.size * 0.12
        visible: status === Image.Ready
        source: badge.iconName.length ? "qrc:/icons/" + badge.iconName + ".svg" : ""
        sourceSize.width: badge.size * 2
        sourceSize.height: badge.size * 2
        asynchronous: true
        fillMode: Image.PreserveAspectFit
    }
}
