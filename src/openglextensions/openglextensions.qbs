import qbs

QtModuleProject {
    id: root
    name: "QtOpenGLExtensions"
    simpleName: "openglextensions"
    prefix: project.qtbasePrefix + "src/openglextensions/"
    condition: project.opengl || project.opengles2

    Product {
        name: root.privateName
        profiles: project.targetProfiles
        type: "hpp"
        Depends { name: root.moduleName }
        Export {
            Depends { name: "cpp" }
            cpp.defines: root.defines
            cpp.includePaths: root.includePaths
        }
    }

    QtHeaders {
        name: root.headersName
        sync.module: root.name
        Depends { name: "QtCoreHeaders" }
        Depends { name: "QtGuiHeaders" }
        Group {
            name: "headers"
            files: [
                "*.h",
            ]
            fileTags: ["hpp_syncable"]
        }
    }

    QtModule {
        type: ["staticlibrary", "prl", "pri"]
        name: root.moduleName
        parentName: root.name
        simpleName: root.simpleName
        targetName: root.targetName
        qmakeProject: root.prefix + "openglextensions.pro"

        Export {
            Depends { name: "cpp" }
            Depends { name: "Qt.core" }
            cpp.includePaths: root.publicIncludePaths
        }

        Depends { name: root.headersName }
        Depends { name: "Qt.core" }
        Depends { name: "Qt.gui" }

        cpp.defines: base.concat("QT_NO_CAST_FROM_ASCII")
        cpp.includePaths: root.includePaths.concat(base)

        Group {
            name: "sources"
            prefix: root.prefix
            files: [
                "qopenglextensions.cpp",
                "qopenglextensions.h",
            ]
        }
    }
}

// contains(QT_CONFIG, opengl):CONFIG += opengl
// contains(QT_CONFIG, opengles2):CONFIG += opengles2
