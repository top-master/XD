import qbs

QtPlugin {
    name: "qeglfs-mali-integration"
    condition: project.eglfs_mali
    category: "egldeviceintegrations"

    Depends { name: "Qt.core-private" }
    Depends { name: "Qt.egldeviceintegration" }
    Depends { name: "Qt.gui" }
    Depends { name: "Qt.gui-private" }
    Depends { name: "Qt.platformsupport-private" }
    Depends { name: "egl" }

    cpp.defines: base.concat(["MESA_EGL_NO_X11_HEADERS"]) // Avoid X11 header collision
    cpp.includePaths: base.concat(["../.."])

    files: [
        "eglfs_mali.json",
        "qeglfsmaliintegration.cpp",
        "qeglfsmaliintegration.h",
        "qeglfsmalimain.cpp",
    ]
}

// PLUGIN_CLASS_NAME = QEglFSMaliIntegrationPlugin
