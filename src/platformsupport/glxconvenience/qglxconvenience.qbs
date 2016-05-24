import qbs

Project {
    PkgConfigDependency { name: "xrender" }

    QtProduct {
        name: "qglxconvenience"
        condition: project.config.contains("xlib") && project.config.contains("opengl")
                   && !project.config.contains("opengles2")
        type: ["staticlibrary"]

        Depends { name: "Qt.gui"; condition: project.gui }
        Depends { name: "xrender"; condition: project.config.contains("xrender") }

        cpp.dynamicLibraries: ["Xext", "X11", "m"] // FIXME: $$QMAKE_LIBS_X11

        Export {
            Depends { name: "cpp" }
            Depends { name: "xrender"; condition: project.config.contains("xrender") }
            Depends { name: "Qt.opengl" }
            cpp.dynamicLibraries: ["Xext", "X11", "m"] // FIXME: $$QMAKE_LIBS_X11
        }

        files: [
            "qglxconvenience.cpp",
            "qglxconvenience_p.h",
        ]
    }
}
