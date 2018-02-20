import qbs

QtModuleProject {
    name: "QtClipboardSupport"
    simpleName: "clipboard_support"
    internal: true
    conditionFunction: (function() {
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
        Depends { name: "Qt.core-private" }
        Depends { name: "Qt.gui" }

        cpp.includePaths: project.includePaths.concat(base)
        cpp.defines: base.concat("QT_NO_CAST_FROM_ASCII")
        cpp.frameworks: ["AppKit"]

        Group {
            files: [
                "qmacmime.mm",
                "qmacmime_p.h",
            ]
        }
    }
}
