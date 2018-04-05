import qbs
import QtGuiPrivateConfig

QtModuleProject {
    name: "QtServiceSupport"
    simpleName: "service_support"
    internal: true
    condition: (qbs.targetOS.contains("unix") && !qbs.targetOS.contains("darwin"))
        || QtGuiPrivateConfig.xcb
    qbsSearchPaths: [project.qtbaseShadowDir + "/src/gui/qbs"]

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
            prefix: "genericunix/"
            files: [
                "qgenericunixservices_p.h",
                "qgenericunixservices.cpp",
            ]
        }
        Group {
            name: "Qt.core precompiled header"
            files: ["../../corelib/global/qt_pch.h"]
        }
    }
}
