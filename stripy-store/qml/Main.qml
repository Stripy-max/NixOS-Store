import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ApplicationWindow {
    id: win
    width: 1080; height: 720
    minimumWidth: 420; minimumHeight: 480
    visible: true
    title: "Stripy Store"
    color: Theme.bg

    // All controls take their colours from the central tokens.
    palette.window: Theme.bg
    palette.windowText: Theme.text
    palette.base: Theme.surface
    palette.alternateBase: Theme.surfaceAlt
    palette.text: Theme.text
    palette.button: Theme.surfaceAlt
    palette.buttonText: Theme.text
    palette.highlight: Theme.accent
    palette.highlightedText: Theme.accentFg
    palette.placeholderText: Theme.dim
    palette.mid: Theme.border
    palette.link: Theme.accent
    palette.toolTipBase: Theme.surface
    palette.toolTipText: Theme.text

    Binding { target: Theme; property: "mode"; value: backend.appearance }

    readonly property bool compact: width < 800
    property string page: "home"            // home | categories | list | updates
    property string pendingAttr: ""
    readonly property string navKey: page === "home" ? "home"
        : page === "categories" ? "categories"
        : page === "updates" ? "updates"
        : backend.view === "installed" ? "installed"
        : backend.view === "category" ? "categories" : ""

    // ── navigation ──
    function clearSearch() { if (searchField.text.length) searchField.text = "" }
    function goHome() { clearSearch(); page = "home"; backend.showHome() }
    function goCategories() { clearSearch(); page = "categories"; if (backend.view === "search") backend.showHome() }
    function goInstalled() { clearSearch(); page = "list"; backend.showInstalled() }
    function goUpdates() { clearSearch(); page = "updates"; if (backend.view === "search") backend.showHome() }
    function openCategory(id) { clearSearch(); page = "list"; backend.showCategory(id) }
    function runSearch() {
        var t = searchField.text.trim()
        if (t.length > 0) { page = "list"; backend.search(t) }
        else if (page === "list" && backend.view === "search") goHome()
    }

    // ── actions: always explain the system change before doing it ──
    function request(attr, st, name, lic) {
        if (st === "installing" || st === "removing" || backend.busy) return
        pendingAttr = attr
        if (Theme.isRemoval(st)) {
            confirm.kind = "remove"; confirm.showRemember = false
            confirm.heading = qsTr("Remove %1?").arg(name)
            confirm.body = qsTr("%1 will be removed from the system configuration and the system will be rebuilt. This needs administrator permission and affects all users.\n\nCopies installed some other way (your user profile, Flatpak, …) are not touched.").arg(name)
            confirm.acceptText = qsTr("Remove")
            confirm.open()
        } else if (backend.confirmInstall) {
            confirm.kind = "install"; confirm.showRemember = true
            confirm.heading = qsTr("Install %1?").arg(name)
            confirm.body = qsTr("%1 will be added to the system configuration and the system will be rebuilt. This needs administrator permission, may download packages, and installs for all users.").arg(name)
                + (lic === "unfree" ? "\n\n" + qsTr("%1 is non-free software. Installing it allows that package (and only that package) under the system's non-free policy.").arg(name) : "")
            confirm.acceptText = qsTr("Install")
            confirm.open()
        } else {
            backend.install(attr)
        }
    }
    function requestUpdate() {
        confirm.kind = "update"; confirm.showRemember = false
        confirm.heading = qsTr("Update StripyOS?")
        confirm.body = qsTr("This refreshes the package source (nixpkgs) in %1, then rebuilds the system. All applications and the operating system update together; downloads can be large. flake.lock will change. A previous system generation stays available in the boot menu.").arg(backend.flakeDir)
        confirm.acceptText = qsTr("Update")
        confirm.open()
    }
    function requestApply() {
        confirm.kind = "apply"; confirm.showRemember = false
        confirm.heading = qsTr("Apply configuration?")
        confirm.body = qsTr("The system will be rebuilt from the current configuration so that it matches your application list. This needs administrator permission.")
        confirm.acceptText = qsTr("Apply")
        confirm.open()
    }

    Shortcut { sequences: [StandardKey.Find]; onActivated: searchField.forceActiveFocus() }
    Shortcut { sequence: "Ctrl+,"; onActivated: settings.open() }
    Shortcut { sequence: "Escape"; enabled: searchField.activeFocus; onActivated: { win.goHome(); win.contentItem.forceActiveFocus() } }

    onClosing: close => { if (backend.busy) { close.accepted = false; busyDlg.open() } }

    Timer { id: searchTimer; interval: 300; onTriggered: win.runSearch() }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // ── top bar ──
        Rectangle {
            Layout.fillWidth: true
            implicitHeight: 56
            color: Theme.surface
            Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: Theme.border }
            RowLayout {
                anchors.fill: parent; anchors.leftMargin: 12; anchors.rightMargin: 12; spacing: 10
                Label { visible: !win.compact; text: "Stripy Store"; font.bold: true; font.pixelSize: 17; Layout.rightMargin: 8 }
                TextField {
                    id: searchField
                    Layout.fillWidth: true
                    placeholderText: qsTr("Search applications  (Ctrl+F)")
                    selectByMouse: true
                    Accessible.name: qsTr("Search applications")
                    onTextChanged: searchTimer.restart()
                    onAccepted: { searchTimer.stop(); win.runSearch() }
                }
                ToolButton {
                    text: qsTr("Settings")
                    icon.name: "configure"
                    display: win.compact ? AbstractButton.IconOnly : AbstractButton.TextBesideIcon
                    ToolTip.visible: hovered; ToolTip.text: qsTr("Settings (Ctrl+,)")
                    Accessible.name: qsTr("Settings")
                    onClicked: settings.open()
                }
            }
        }

        // ── integration warning ──
        Rectangle {
            visible: backend.health !== "ok"
            Layout.fillWidth: true
            implicitHeight: visible ? warnRow.implicitHeight + 16 : 0
            color: Theme.surfaceAlt
            Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: Theme.warn }
            RowLayout {
                id: warnRow
                anchors.fill: parent; anchors.margins: 8; spacing: 10
                Label {
                    Layout.fillWidth: true; wrapMode: Text.Wrap
                    text: "⚠ " + (backend.health === "unsupported"
                          ? qsTr("Browsing only: installing is not available on this system.")
                          : qsTr("Setup needed before applications can be installed."))
                          + (backend.problems.length ? "  " + backend.problems[0] : "")
                }
                Button { text: qsTr("Fix in Settings"); onClicked: settings.open() }
            }
        }

        // ── navigation + content ──
        GridLayout {
            Layout.fillWidth: true; Layout.fillHeight: true
            columns: win.compact ? 1 : 2
            rowSpacing: 0; columnSpacing: 0

            Rectangle {
                id: nav
                color: Theme.surface
                Layout.fillWidth: win.compact
                Layout.fillHeight: !win.compact
                Layout.preferredWidth: win.compact ? -1 : 190
                implicitHeight: win.compact ? navFlow.height + 8 : 0
                Rectangle {
                    visible: !win.compact
                    anchors.right: parent.right; height: parent.height; width: 1; color: Theme.border
                }
                Flow {
                    id: navFlow
                    x: 6; y: 4
                    width: parent.width - 12
                    flow: win.compact ? Flow.LeftToRight : Flow.TopToBottom
                    spacing: 2
                    Repeater {
                        model: [
                            {key: "home", text: qsTr("Discover"), icon: "go-home"},
                            {key: "categories", text: qsTr("Categories"), icon: "applications-all"},
                            {key: "installed", text: qsTr("Installed") + (backend.installedCount > 0 ? " (" + backend.installedCount + ")" : ""), icon: "emblem-ok"},
                            {key: "updates", text: qsTr("Updates"), icon: "view-refresh"}
                        ]
                        delegate: ItemDelegate {
                            required property var modelData
                            width: win.compact ? implicitWidth : navFlow.width
                            text: modelData.text
                            icon.name: modelData.icon
                            highlighted: win.navKey === modelData.key
                            Accessible.name: modelData.text
                            onClicked: {
                                if (modelData.key === "home") win.goHome()
                                else if (modelData.key === "categories") win.goCategories()
                                else if (modelData.key === "installed") win.goInstalled()
                                else win.goUpdates()
                            }
                        }
                    }
                }
            }

            Item {
                Layout.fillWidth: true; Layout.fillHeight: true
                HomePage {
                    anchors.fill: parent; visible: win.page === "home"
                    onOpenApp: attr => details.show(attr)
                    onAct: (attr, st, name, lic) => win.request(attr, st, name, lic)
                    onOpenCategory: id => win.openCategory(id)
                }
                CategoriesPage {
                    anchors.fill: parent; visible: win.page === "categories"
                    onOpenCategory: id => win.openCategory(id)
                }
                AppListPage {
                    anchors.fill: parent; visible: win.page === "list"
                    onOpenApp: attr => details.show(attr)
                    onAct: (attr, st, name, lic) => win.request(attr, st, name, lic)
                    onApplyRequested: win.requestApply()
                }
                UpdatesPage {
                    anchors.fill: parent; visible: win.page === "updates"
                    onUpdateRequested: win.requestUpdate()
                    onApplyRequested: win.requestApply()
                }
            }
        }

        OperationBar { Layout.fillWidth: true }
    }

    DetailsPanel {
        id: details
        onAct: (attr, st, name, lic) => win.request(attr, st, name, lic)
    }
    SettingsDialog { id: settings }
    ConfirmDialog {
        id: confirm
        onConfirmed: kind => {
            if (kind === "remove") backend.remove(win.pendingAttr)
            else if (kind === "install") backend.install(win.pendingAttr)
            else if (kind === "update") backend.updateSystem()
            else if (kind === "apply") backend.applyConfiguration()
        }
    }
    Dialog {
        id: busyDlg
        modal: true; title: qsTr("An operation is running")
        anchors.centerIn: Overlay.overlay
        standardButtons: Dialog.Ok
        Label { text: qsTr("Please wait until the current system change has finished before closing Stripy Store."); wrapMode: Text.Wrap; width: 320 }
    }
}
