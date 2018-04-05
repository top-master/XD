import qbs

QtModuleProject {
    name: "QtFbSupport"
    simpleName: "fb_support"
    internal: true

    QtHeaders {
    }

    QtModule {
        qbsSearchPaths: [project.qtbaseShadowDir + "/src/corelib/qbs"]

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

        files: [
            "qfbbackingstore.cpp",
            "qfbbackingstore_p.h",
            "qfbcursor.cpp",
            "qfbcursor_p.h",
            "qfbscreen.cpp",
            "qfbscreen_p.h",
            "qfbvthandler.cpp",
            "qfbvthandler_p.h",
            "qfbwindow.cpp",
            "qfbwindow_p.h",
        ]

        Group {
            name: "Qt.core precompiled header"
            files: ["../../corelib/global/qt_pch.h"]
        }
    }
}
