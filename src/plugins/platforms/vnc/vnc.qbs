import qbs
import QtGuiPrivateConfig

QtPlugin {
    name: "qvnc"
    condition: QtGuiPrivateConfig.vnc
    pluginType: "platforms"
    pluginClassName: "QVncIntegrationPlugin"
    qbsSearchPaths: [project.qtbaseShadowDir + "/src/gui/qbs"]

    Depends { name: "Qt.core-private" }
    Depends { name: "Qt.gui-private" }
    Depends { name: "Qt.network" }
    Depends { name: "Qt.service_support-private" }
    Depends { name: "Qt.theme_support-private" }
    Depends { name: "Qt.fb_support-private" }
    Depends { name: "Qt.eventdispatcher_support-private" }
    Depends { name: "Qt.fontdatabase_support-private" }
    Depends { name: "Qt.input_support-private" }
    cpp.defines: ["QT_NO_FOREACH"]
    files: [
        "main.cpp",
        "qvnc.cpp",
        "qvnc_p.h",
        "qvncclient.cpp",
        "qvncclient.h",
        "qvncintegration.cpp",
        "qvncintegration.h",
        "qvncscreen.cpp",
        "qvncscreen.h",
        "vnc.json",
    ]
    /*
        !equals(TARGET, $$QT_DEFAULT_QPA_PLUGIN): PLUGIN_EXTENDS = -
    */
}
