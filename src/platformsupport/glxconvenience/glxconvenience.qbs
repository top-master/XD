import qbs
import QtGuiConfig
import QtGuiPrivateConfig

QtModuleProject {
    name: "QtGlxSupport"
    simpleName: "glx_support"
    internal: true
    conditionFunction: (function() {
        return QtGuiPrivateConfig.xlib && QtGuiConfig.opengl && !QtGuiConfig.opengles2;
    })
    qbsSearchPaths: [
        project.qtbaseShadowDir + "/src/corelib/qbs",
        project.qtbaseShadowDir + "/src/gui/qbs"
    ]

    QtHeaders {
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
        Depends { name: "Libdl"; condition: Qt["core-private"].config.dlopen }

        cpp.includePaths: project.includePaths.concat(base)
        cpp.defines: base.concat("QT_NO_CAST_FROM_ASCII")
        Group {
            files: [
                "qglxconvenience.cpp",
                "qglxconvenience_p.h",
            ]
        }
        Group {
            name: "Qt.core precompiled header"
            files: ["../../corelib/global/qt_pch.h"]
        }
    }
}


/*
LIBS_PRIVATE += $$QMAKE_LIBS_X11
*/
