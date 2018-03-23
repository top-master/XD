import qbs
import QtGuiPrivateConfig

QtPlugin {
    name: "qlinuxfb"
    condition: !qbs.targetOS.contains("android") && Qt.global.privateConfig.gui
               && QtGuiPrivateConfig.linuxfb
    pluginType: "platforms"
    pluginClassName: "QLinuxFbIntegrationPlugin"
    qbsSearchPaths: [project.qtbaseShadowDir + "/src/gui/qbs"]

    Depends { name: "Qt.core" }
    Depends { name: "Qt.core-private" }
    Depends { name: "Qt.gui-private" }
    Depends { name: "Qt.eventdispatcher_support-private" }
    Depends { name: "Qt.devicediscovery_support-private" }
    Depends { name: "Qt.service_support-private" }
    Depends { name: "Qt.fb_support-private" }
    Depends { name: "Qt.fontdatabase_support-private" }
    Depends { name: "Qt.input_support-private"; required: false }
    Depends { name: "Qt.kms_support-private"; required: false }
    cpp.defines: ["QT_NO_FOREACH"]
    files: [
        "linuxfb.json",
        "main.cpp",
        "qlinuxfbintegration.cpp",
        "qlinuxfbintegration.h",
        "qlinuxfbscreen.cpp",
        "qlinuxfbscreen.h",
    ]
    Group {
        condition: Qt["kms_support-private"].present
        files: [
            "qlinuxfbdrmscreen.cpp",
            "qlinuxfbdrmscreen.h",
        ]
    }

    /*
        !equals(TARGET, $$QT_DEFAULT_QPA_PLUGIN): PLUGIN_EXTENDS = -
    */
}
