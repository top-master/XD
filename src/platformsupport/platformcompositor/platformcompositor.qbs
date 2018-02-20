import qbs

QtModuleProject {
    name: "QtPlatformCompositorSupport"
    simpleName: "platformcompositor_support"
    internal: true
    conditionFunction: (function() {
        return QtGuiConfig.opengl;
    })
    qbsSearchPaths: [project.qtbaseShadowDir + "/src/gui/qbs"]

    QtHeaders {
        Depends { name: "QtGuiConfig" }
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
        Depends { name: "QtGuiConfig" }

        cpp.includePaths: project.includePaths.concat(base)
        cpp.defines: base.concat("QT_NO_CAST_FROM_ASCII")
        Group {
            files: [
                "qopenglcompositor.cpp",
                "qopenglcompositor_p.h",
                "qopenglcompositorbackingstore.cpp",
                "qopenglcompositorbackingstore_p.h",
            ]
        }
        Group {
            name: "Qt.core precompiled header"
            files: ["../../corelib/global/qt_pch.h"]
        }
    }
}
