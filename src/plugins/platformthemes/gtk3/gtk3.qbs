import qbs

QtPlugin {
    name: "qgtk3"
    condition: Qt.global.privateConfig.widgets && Qt.widgets_private.config.gtk3
    pluginType: "platformthemes"
    pluginClassName: "QGtk3ThemePlugin"
    qbsSearchPaths: [
        project.qtbaseShadowDir + "/src/widgets/qbs",
    ]

    Depends { name: "Qt.core_private" }
    Depends { name: "Qt.gui_private" }
    Depends { name: "Qt.widgets_private" }
    Depends { name: "Qt.theme_support_private" }
    Depends { name: "Gtk3" }
    cpp.defines: ["GDK_VERSION_MIN_REQUIRED=GDK_VERSION_3_6"]
    files: [
        "main.cpp",
        "qgtk3dialoghelpers.cpp",
        "qgtk3dialoghelpers.h",
        "qgtk3menu.cpp",
        "qgtk3menu.h",
        "qgtk3theme.cpp",
        "qgtk3theme.h",
    ]
// PLUGIN_EXTENDS = -
}
