import qbs

QtPlugin {
    name: "ibusplatforminputcontextplugin"
    condition: !qbs.targetOS.contains("windows") && !qbs.targetOS.contains("macos")
        && Qt.global.privateConfig.dbus
    pluginType: "platforminputcontexts"
    pluginClassName: "QIbusPlatformInputContextPlugin"
    Depends { name: "Qt.dbus" }
    Depends { name: "Qt.core_private" }
    Depends { name: "Qt.gui_private" }

    files: [
        "ibus.json",
        "main.cpp",
        "qibusinputcontextproxy.cpp",
        "qibusinputcontextproxy.h",
        "qibusplatforminputcontext.cpp",
        "qibusplatforminputcontext.h",
        "qibusproxy.cpp",
        "qibusproxy.h",
        "qibustypes.cpp",
        "qibustypes.h",
    ]
}
/*
PLUGIN_EXTENDS = -
*/
