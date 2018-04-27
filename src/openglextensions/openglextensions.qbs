import qbs
import QtGuiConfig

QtModuleProject {
    name: "QtOpenGLExtensions"
    simpleName: "openglextensions"
    conditionFunction: (function() {
        return QtGuiConfig.opengl;
    })

    QtHeaders {
    }

    QtPrivateModule {
    }

    QtModule {
        type: base.map(function(t) { if (t === "dynamiclibrary") return "staticlibrary"; })
        Export {
            Depends { name: "cpp" }
            Depends { name: "Qt.widgets" }
            cpp.includePaths: project.publicIncludePaths
        }

        Depends { name: project.headersName }
        Depends { name: "opengl" }
        Depends { name: "Qt.gui" }

        cpp.includePaths: project.includePaths.concat(base)
        cpp.defines: base.concat([
            "QT_NO_FOREACH",
            "QT_NO_USING_NAMESPACE",
        ])

        Properties {
            condition: qbs.toolchain.contains("msvc") && qbs.architecture === "x86"
            cpp.linkerFlags: base.concat("/BASE:0x63000000")
        }

        files: [
            "qopenglextensions.cpp",
            "qopenglextensions.h",
        ]
    }
}
