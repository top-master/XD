import qbs

Project {
    PkgConfigDependency { name: "wayland-server" }

    QtPlugin {
        name: "qeglfs-viv-wl-integration"
        condition: project.config.contains("eglfs_viv_wl")
        category: "egldeviceintegrations"

        Depends { name: "Qt.core-private" }
        Depends { name: "Qt.egldeviceintegration" }
        Depends { name: "Qt.gui" }
        Depends { name: "Qt.gui-private" }
        Depends { name: "Qt.platformsupport-private" }
        Depends { name: "egl" }
        Depends { name: "wayland-server" }

        cpp.includePaths: base.concat(["../.."])
        cpp.dynamicLibraries: base.concat(["GAL"])
        cpp.defines: base.concat(["LINUX=1", "EGL_API_FB=1"])

        files: [
            "eglfs_viv_wl.json",
            "qeglfsvivwlintegration.cpp",
            "qeglfsvivwlintegration.h",
            "qeglfsvivwlmain.cpp",
        ]
    }
    // PLUGIN_CLASS_NAME = QEglFSVivWaylandIntegrationPlugin
}
