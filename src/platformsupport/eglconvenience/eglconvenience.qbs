import qbs

QtModuleProject {
    name: "QtEglSupport"
    simpleName: "egl_support"
    internal: true
    conditionFunction: (function() {
        return QtGuiPrivateConfig.egl;
    })
    qbsSearchPaths: [
        project.qtbaseShadowDir + "/src/corelib/qbs",
        project.qtbaseShadowDir + "/src/gui/qbs"
    ]

    QtHeaders {
        Depends { name: "QtGuiPrivateConfig" }
    }

    QtModule {
        Export {
            Depends { name: "cpp" }
            Depends { name: "Qt.core" }
            cpp.includePaths: project.includePaths
        }

        Depends { name: project.headersName }
        Depends { name: "Qt.core-private" }
        Depends { name: "Qt.gui-private" }
        Depends { name: "QtCorePrivateConfig" }
        Depends { name: "QtGuiConfig" }
        Depends { name: "QtGuiPrivateConfig" }
        //Depends { name: "Xlib"; condition: QtGuiPrivateConfig.xlib }    ### Xlib doesn't exist yet.
        Depends { name: "Libdl"; condition: QtCorePrivateConfig.dlopen }

        cpp.includePaths: project.includePaths.concat(base)
        cpp.defines: base.concat(
            "QT_NO_CAST_FROM_ASCII",
            "QT_EGL_NO_X11"    // Avoid X11 header collision, use generic EGL native types
        )
        Group {
            files: [
                "qeglconvenience.cpp",
                "qeglconvenience_p.h",
                "qeglstreamconvenience.cpp",
                "qeglstreamconvenience_p.h",
                "qt_egl_p.h",
            ]
        }
        Group {
            condition: QtGuiConfig.opengl
            files: [
                "qeglpbuffer.cpp",
                "qeglpbuffer_p.h",
                "qeglplatformcontext.cpp",
                "qeglplatformcontext_p.h",
            ]
        }
        Group {
            condition: QtGuiPrivateConfig.xlib
            files: [
                "qxlibeglintegration_p.h",
                "qxlibeglintegration.cpp",
            ]
        }
        Group {
            name: "Qt.core precompiled header"
            files: ["../../corelib/global/qt_pch.h"]
        }
    }
}
