import qbs

QtPlugin {
    name: "ibusplatforminputcontextplugin"
    condition: Qt.dbus.present && !qbs.targetOS.contains("darwin")
               && !qbs.targetOS.contains("windows")
    category: "platforminputcontexts"

    Depends { name: "Qt.dbus"; required: false }
    Depends { name: "Qt.gui" }
    Depends { name: "Qt.gui-private" }

    files: [
        "ibus.json",
        "main.cpp",
        "qibusinputcontextproxy.cpp",
        "qibusinputcontextproxy.h",
        "qibusplatforminputcontext.cpp",
        "qibusplatforminputcontext.h",
        "qibusproxy.h",
        "qibusproxy.cpp",
        "qibustypes.cpp",
        "qibustypes.h",
    ]
}

// PLUGIN_EXTENDS = -
// PLUGIN_CLASS_NAME = QIbusPlatformInputContextPlugin
