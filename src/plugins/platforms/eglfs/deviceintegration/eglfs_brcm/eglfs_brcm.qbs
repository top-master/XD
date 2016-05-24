import qbs

QtPlugin {
    name: "qeglfs-brcm-integration"
    condition: project.eglfs_brcm
    category: "egldeviceintegrations"

    Depends { name: "Qt.core-private" }
    Depends { name: "Qt.gui" }
    Depends { name: "Qt.gui-private" }
    Depends { name: "Qt.platformsupport-private" }
    Depends { name: "Qt.egldeviceintegration" }
    Depends { name: "egl" }

    cpp.defines: base.concat(["MESA_EGL_NO_X11_HEADERS"]) // Avoid X11 header collision
    cpp.dynamicLibraries: base.concat(["bcm_host"])
    cpp.includePaths: base.concat(["../.."])

    files: [
        "eglfs_brcm.json",
        "qeglfsbrcmintegration.cpp",
        "qeglfsbrcmintegration.h",
        "qeglfsbrcmmain.cpp",
    ]
}

// PLUGIN_CLASS_NAME = QEglFSBrcmIntegrationPlugin
