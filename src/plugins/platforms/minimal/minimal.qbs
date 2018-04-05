import qbs

QtPlugin {
    name: "qminimal"
    condition: !qbs.targetOS.contains("android") && Qt.global.privateConfig.gui
    pluginType: "platforms"
    pluginClassName: "QMinimalIntegrationPlugin"

    Depends { name: "Qt.core_private" }
    Depends { name: "Qt.gui_private" }
    Depends { name: "Qt.eventdispatcher_support_private" }
    Depends { name: "Qt.fontdatabase_support_private" }
    files: [
        "main.cpp",
        "minimal.json",
        "qminimalbackingstore.cpp",
        "qminimalbackingstore.h",
        "qminimalintegration.cpp",
        "qminimalintegration.h",
    ]
    /*
        !equals(TARGET, $$QT_DEFAULT_QPA_PLUGIN): PLUGIN_EXTENDS = -
    */
}
