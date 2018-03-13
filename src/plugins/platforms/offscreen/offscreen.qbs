import qbs

QtPlugin {
    name: "qoffscreen"
    condition: !qbs.targetOS.contains("android") && QtGlobalPrivateConfig.gui
               && QtGuiPrivateConfig.freetype
    pluginType: "platforms"
    pluginClassName: "QOffscreenIntegrationPlugin"
    qbsSearchPaths: [project.qtbaseShadowDir + "/src/gui/qbs"]

    Depends { name: "QtGlobalPrivateConfig" }
    Depends { name: "Qt.core-private" }
    Depends { name: "Qt.gui-private" }
    Depends { name: "Qt.eventdispatcher_support-private" }
    Depends { name: "Qt.fontdatabase_support-private" }
    Depends { name: "Qt.glx_support-private"; required: false }
    Depends { name: "QtGuiPrivateConfig" }
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
        condition: Qt["glx_support-private"].present
        files: [
            "qoffscreenintegration_x11.cpp",
            "qoffscreenintegration_x11.h",
        ]
    }
    Group {
        condition: !Qt["glx_support-private"].present
        files: [
            "qoffscreenintegration_dummy.cpp",
        ]
    }

    /*
        !equals(TARGET, $$QT_DEFAULT_QPA_PLUGIN): PLUGIN_EXTENDS = -
    */
}
