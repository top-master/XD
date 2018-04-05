import qbs
import QtGuiConfig
import QtGuiPrivateConfig

QtModuleProject {
    name: "QtEdidSupport"
    simpleName: "edid_support"
    internal: true
    conditionFunction: (function() {
        return QtGuiPrivateConfig.xlib && QtGuiConfig.opengl && !QtGuiConfig.opengles2;
    })
    qbsSearchPaths: [
        project.qtbaseShadowDir + "/src/corelib/qbs",
    ]

    QtHeaders {
    }

    QtModule {
        Export {
            Depends { name: "cpp" }
            cpp.includePaths: project.includePaths
        }

        Depends { name: project.headersName }
        Depends { name: "Qt.core_private" }

        cpp.includePaths: project.includePaths.concat(base)
        cpp.defines: base.concat("QT_NO_CAST_FROM_ASCII")
        files: [
            "qedidparser.cpp",
            "qedidparser_p.h",
        ]
        Group {
            name: "Qt.core precompiled header"
            files: ["../../corelib/global/qt_pch.h"]
        }
    }
}
