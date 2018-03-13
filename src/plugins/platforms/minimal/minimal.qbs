import qbs

QtPlugin {
    name: "qminimal"
    condition: !qbs.targetOS.contains("android") && QtGlobalPrivateConfig.gui
    pluginType: "platforms"
    pluginClassName: "QMinimalIntegrationPlugin"

    Depends { name: "QtGlobalPrivateConfig" }
    Depends { name: "Qt.core-private" }
    Depends { name: "Qt.gui-private" }
    Depends { name: "Qt.eventdispatcher_support-private" }
    Depends { name: "Qt.fontdatabase_support-private" }
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
