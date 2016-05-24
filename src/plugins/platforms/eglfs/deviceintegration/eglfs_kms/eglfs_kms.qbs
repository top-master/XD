import qbs

QtPlugin {
    name: "qeglfs-kms-integration"
    condition: project.config.contains("eglfs_gbm")
    category: "egldeviceintegrations"

    Depends { name: "Qt.core-private" }
    Depends { name: "Qt.egldeviceintegration" }
    Depends { name: "Qt.gui" }
    Depends { name: "Qt.gui-private" }
    Depends { name: "Qt.platformsupport-private" }
    Depends { name: "egl" }
    Depends { name: "gbm" }
    Depends { name: "libdrm" }

    cpp.defines: base.concat(["MESA_EGL_NO_X11_HEADERS"]) // Avoid X11 header collision
    cpp.includePaths: base.concat(["../.."])

    files: [
        "eglfs_kms.json",
        "qeglfskmscursor.cpp",
        "qeglfskmscursor.h",
        "qeglfskmsdevice.cpp",
        "qeglfskmsdevice.h",
        "qeglfskmsintegration.cpp",
        "qeglfskmsintegration.h",
        "qeglfskmsmain.cpp",
        "qeglfskmsscreen.cpp",
        "qeglfskmsscreen.h",
    ]
}

// PLUGIN_CLASS_NAME = QEglFSKmsIntegrationPlugin
