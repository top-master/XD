import qbs

QtModuleProject {
    name: "QtGraphicsSupport"
    simpleName: "graphics_support"
    internal: true
    conditionFunction: (function(qbs) {
        return qbs.targetOS.contains("darwin");
    })

    QtHeaders {
    }

    QtModule {
        Export {
            Depends { name: "cpp" }
            Depends { name: "Qt.core" }
            cpp.includePaths: project.includePaths
        }

        Depends { name: project.headersName }
        Depends { name: "Qt.core_private" }
        Depends { name: "Qt.gui_private" }

        cpp.includePaths: project.includePaths.concat(base)
        cpp.defines: base.concat("QT_NO_CAST_FROM_ASCII")

        Group {
            files: [
                "qrasterbackingstore.cpp",
                "qrasterbackingstore_p.h",
            ]
        }
    }
}
