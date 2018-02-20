import qbs

QtModuleProject {
    name: "QtKmsSupport"
    simpleName: "kms_support"
    internal: true
    conditionFunction: (function() {
        return QtGuiPrivateConfig.kms;
    })
    qbsSearchPaths: [
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
        Depends { name: "QtGuiConfig" }
        Depends { name: "QtGuiPrivateConfig" }
        Depends { name: "Drm" }

        cpp.includePaths: project.includePaths.concat(base)
        cpp.defines: base.concat("QT_NO_CAST_FROM_ASCII")
        Group {
            files: [
                "qkmsdevice.cpp",
                "qkmsdevice_p.h",
            ]
        }
        Group {
            name: "Qt.core precompiled header"
            files: ["../../corelib/global/qt_pch.h"]
        }
    }
}
/*
LIBS_PRIVATE += $$QMAKE_LIBS_DYNLOAD
*/
