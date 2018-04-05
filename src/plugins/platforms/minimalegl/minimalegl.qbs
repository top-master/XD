import qbs

QtPlugin {
    name: "qminimalegl"
    condition: Qt.gui_private.config.eglfs
    pluginType: "platforms"
    pluginClassName: "QMinimalEglIntegrationPlugin"

    Depends { name: "Qt.core_private" }
    Depends { name: "Qt.gui_private" }
    Depends { name: "Qt.eventdispatcher_support_private" }
    Depends { name: "Qt.fontdatabase_support_private" }
    Depends { name: "Qt.egl_support_private" }
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
        condition: Qt.gui.config.opengl
        files: [
            "qminimaleglbackingstore.cpp",
            "qminimaleglbackingstore.h",
        ]
    }

    /*
        !equals(TARGET, $$QT_DEFAULT_QPA_PLUGIN): PLUGIN_EXTENDS = -
    */
}
