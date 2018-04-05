import qbs
import QtGuiPrivateConfig

QtPlugin {
    name: "qoffscreen"
    condition: !qbs.targetOS.contains("android") && Qt.global.privateConfig.gui
               && QtGuiPrivateConfig.freetype
    pluginType: "platforms"
    pluginClassName: "QOffscreenIntegrationPlugin"
    qbsSearchPaths: [project.qtbaseShadowDir + "/src/gui/qbs"]

    Depends { name: "Qt.core_private" }
    Depends { name: "Qt.gui_private" }
    Depends { name: "Qt.eventdispatcher_support_private" }
    Depends { name: "Qt.fontdatabase_support_private" }
    Depends { name: "Qt.glx_support_private"; required: false }
    cpp.defines: ["QT_NO_FOREACH"]
    files: [
        "main.cpp",
        "offscreen.json",
        "qoffscreencommon.cpp",
        "qoffscreencommon.h",
        "qoffscreenintegration.cpp",
        "qoffscreenintegration.h",
        "qoffscreenwindow.cpp",
        "qoffscreenwindow.h",
    ]

    Group {
        condition: Qt.glx_support_private.present
        files: [
            "qoffscreenintegration_x11.cpp",
            "qoffscreenintegration_x11.h",
        ]
    }
    Group {
        condition: !Qt.glx_support_private.present
        files: [
            "qoffscreenintegration_dummy.cpp",
        ]
    }

    /*
        !equals(TARGET, $$QT_DEFAULT_QPA_PLUGIN): PLUGIN_EXTENDS = -
    */
}
