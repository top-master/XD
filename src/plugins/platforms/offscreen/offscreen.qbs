import qbs

QtPlugin {
    name: "qoffscreen"
    condition: !qbs.targetOS.contains("windows") || project.config.contains("freetype")
    category: "platforms"

    property bool usesOpenGl: project.config.contains("xlib") && project.opengl
                              && !project.opengles2

    Depends { name: "Qt.core-private" }
    Depends { name: "Qt.gui-private" }
    Depends { name: "Qt.gui" }
    Depends { name: "Qt.opengl"; condition: usesOpenGl }
    Depends { name: "Qt.platformsupport-private" }
    Depends { name: "qglxconvenience"; condition: usesOpenGl }

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
        name: "x11/glx implementation"
        condition: usesOpenGl
        files: [
            "qoffscreenintegration_x11.cpp",
            "qoffscreenintegration_x11.h",
        ]
    }

    Group {
        name: "generic implementation"
        condition: !usesOpenGl
        files: [
            "qoffscreenintegration_dummy.cpp",
        ]
    }
}

// PLUGIN_CLASS_NAME = QOffscreenIntegrationPlugin
// !equals(TARGET, $$QT_DEFAULT_QPA_PLUGIN): PLUGIN_EXTENDS = -
