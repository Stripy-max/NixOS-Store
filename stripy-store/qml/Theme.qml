pragma Singleton
import QtQuick

// Central design tokens. Every colour in the UI comes from here.
QtObject {
    id: root
    property string mode: "system"                         // system | light | dark (bound from Main.qml)
    property SystemPalette sys: SystemPalette { colorGroup: SystemPalette.Active }
    readonly property bool dark: mode === "dark" || (mode === "system" && sys.window.hslLightness < 0.5)

    readonly property color bg:           dark ? "#14171c" : "#f4f5f7"
    readonly property color surface:      dark ? "#1c2027" : "#ffffff"
    readonly property color surfaceAlt:   dark ? "#242932" : "#eceef2"
    readonly property color surfaceHover: dark ? "#232831" : "#f0f3fa"
    readonly property color border:       dark ? "#313846" : "#d9dde5"
    readonly property color text:         dark ? "#e8ebf0" : "#1b1f27"
    readonly property color dim:          dark ? "#a3adbb" : "#566070"
    readonly property color accent:       dark ? "#6aa0ff" : "#1f5fd6"
    readonly property color accentFg:     dark ? "#0b1220" : "#ffffff"
    readonly property color ok:           dark ? "#4cc38a" : "#17703f"
    readonly property color warn:         dark ? "#e3b341" : "#8a5a00"
    readonly property color danger:       dark ? "#ff8a80" : "#b3261e"

    readonly property int radius: 8
    readonly property int gap: 12
    readonly property int pad: 16

    function statusText(s) {
        switch (s) {
        case "installed": return qsTr("Installed")
        case "unverified": return qsTr("Not verified")
        case "pending-install": return qsTr("Install pending")
        case "pending-remove": return qsTr("Removal pending")
        case "failed-install": case "failed-remove": return qsTr("Failed")
        case "installing": return qsTr("Installing…")
        case "removing": return qsTr("Removing…")
        default: return ""
        }
    }
    function statusColor(s) {
        if (s === "installed") return ok
        if (s === "failed-install" || s === "failed-remove") return danger
        if (s === "none" || s === "") return dim
        return warn
    }
    function actionText(s) {
        switch (s) {
        case "installed": case "unverified": return qsTr("Remove")
        case "pending-install": return qsTr("Finish install")
        case "pending-remove": return qsTr("Finish removal")
        case "failed-install": case "failed-remove": return qsTr("Retry")
        case "installing": case "removing": return qsTr("Working…")
        default: return qsTr("Install")
        }
    }
    function isRemoval(s) {
        return s === "installed" || s === "unverified" || s === "pending-remove" || s === "failed-remove"
    }
}
