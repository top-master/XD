import qbs

QtPlugin {
    name: "qeglfs-kms-egldevice-integration"
    condition: project.config.contains("eglfs_egldevice")
    category: "egldeviceintegrations"

    Depends { name: "Qt.core-private" }
    Depends { name: "Qt.egldeviceintegration" }
    Depends { name: "Qt.gui" }
    Depends { name: "Qt.gui-private" }
    Depends { name: "Qt.platformsupport-private" }
    Depends { name: "egl" }

    cpp.defines: base.concat(["MESA_EGL_NO_X11_HEADERS"]) // Avoid X11 header collision
    cpp.dynamicLibraries: base.concat(["drm"])
    cpp.includePaths: base.concat(["../.."])

    files: [
        "eglfs_kms_egldevice.json",
        "qeglfskmsegldeviceintegration.cpp",
        "qeglfskmsegldeviceintegration.h",
        "qeglfskmsegldevicemain.cpp",
    ]

}

// PLUGIN_CLASS_NAME = QEglFSKmsEglDeviceIntegrationPlugin
