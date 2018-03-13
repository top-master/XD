import qbs

QtPlugin {
    qbsSearchPaths: [project.qtbaseShadowDir + "/src/gui/qbs"]
    name: "qminimalegl"
    condition: QtGuiPrivateConfig.eglfs
    pluginType: "platforms"
    pluginClassName: "QMinimalEglIntegrationPlugin"

    Depends { name: "QtGuiConfig" }
    Depends { name: "QtGuiPrivateConfig" }
    Depends { name: "Qt.core-private" }
    Depends { name: "Qt.gui-private" }
    Depends { name: "Qt.eventdispatcher_support-private" }
    Depends { name: "Qt.fontdatabase_support-private" }
    Depends { name: "Qt.egl_support-private" }
    Depends { name: "Egl" }

    // Avoid X11 header collision, use generic EGL native types
    cpp.defines: base.concat("QT_EGL_NO_X11")

    files: [
        "main.cpp",
        "minimalegl.json",
        "qminimaleglintegration.cpp",
        "qminimaleglintegration.h",
        "qminimaleglscreen.cpp",
        "qminimaleglscreen.h",
        "qminimaleglwindow.cpp",
        "qminimaleglwindow.h",
    ]
    Group {
        condition: QtGuiConfig.opengl
        files: [
            "qminimaleglbackingstore.cpp",
            "qminimaleglbackingstore.h",
        ]
    }

    /*
        !equals(TARGET, $$QT_DEFAULT_QPA_PLUGIN): PLUGIN_EXTENDS = -
    */
}
