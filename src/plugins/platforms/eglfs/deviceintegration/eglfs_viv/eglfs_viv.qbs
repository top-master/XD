import qbs

QtPlugin {
    name: "qeglfs-viv-integration"
    condition: project.eglfs_viv
    category: "egldeviceintegrations"

    Depends { name: "Qt.core-private" }
    Depends { name: "Qt.egldeviceintegration" }
    Depends { name: "Qt.gui" }
    Depends { name: "Qt.gui-private" }
    Depends { name: "Qt.platformsupport-private" }
    Depends { name: "egl" }

    cpp.includePaths: base.concat(["../.."])
    cpp.dynamicLibraries: base.concat(["GAL"])
    cpp.defines: base.concat(["LINUX=1", "EGL_API_FB=1"])

    files: [
        "eglfs_viv.json",
        "qeglfsvivintegration.cpp",
        "qeglfsvivintegration.h",
        "qeglfsvivmain.cpp",
    ]
}

// PLUGIN_CLASS_NAME = QEglFSVivIntegrationPlugin
