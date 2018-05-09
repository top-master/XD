import qbs

QtPlugin {
    name: "qflatpak"
    condition: Qt.global.privateConfig.dbus && Qt.core.config.regularexpression
    pluginType: "platformthemes"
    pluginClassName: "QFlatpakThemePlugin"

    Depends { name: "Qt.core" }
    Depends { name: "Qt.core_private" }
    Depends { name: "Qt.gui_private" }
    Depends { name: "Qt.dbus" }
    Depends { name: "Qt.theme_support_private" }
    files: [
        "main.cpp",
        "qflatpakfiledialog.cpp",
        "qflatpakfiledialog_p.h",
        "qflatpaktheme.cpp",
        "qflatpaktheme.h",
    ]
}
/*
PLUGIN_EXTENDS = -
*/
