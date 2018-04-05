import qbs
import QtGuiPrivateConfig

QtModuleProject {
    name: "QtKmsSupport"
    simpleName: "kms_support"
    internal: true
    conditionFunction: (function() {
        return QtGuiPrivateConfig.kms;
    })

    QtHeaders {
    }

    QtModule {
        Export {
            Depends { name: "cpp" }
            Depends { name: "Qt.core" }
            Depends { name: "Drm" }
            cpp.includePaths: project.includePaths
        }

        Depends { name: project.headersName }
        Depends { name: "Qt.core_private" }
        Depends { name: "Qt.gui_private" }
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
