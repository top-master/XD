import qbs

QtPlugin {
    name: "qminimalegl"
    condition: project.config.contains("eglfs")
    category: "platforms"

    Depends { name: "Qt.core-private" }
    Depends { name: "Qt.gui-private" }
    Depends { name: "Qt.gui" }
    Depends { name: "Qt.opengl" }
    Depends { name: "Qt.platformsupport-private" }
    Depends { name: "egl" }

    cpp.defines: base.concat(["MESA_EGL_NO_X11_HEADERS"]) // Avoid X11 header collision

    files: [
        "main.cpp",
        "minimalegl.json",
        "qminimaleglbackingstore.cpp",
        "qminimaleglbackingstore.h",
        "qminimaleglintegration.cpp",
        "qminimaleglintegration.h",
        "qminimaleglscreen.cpp",
        "qminimaleglscreen.h",
        "qminimaleglwindow.cpp",
        "qminimaleglwindow.h",
    ]
}

// PLUGIN_CLASS_NAME = QMinimalEglIntegrationPlugin
// !equals(TARGET, $$QT_DEFAULT_QPA_PLUGIN): PLUGIN_EXTENDS = -
// CONFIG += qpa/genericunixfontdatabase
