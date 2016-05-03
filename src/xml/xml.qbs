import qbs
import "headers.qbs" as ModuleHeaders

QtModuleProject {
    id: root
    name: "QtXml"
    simpleName: "xml"
    prefix: project.qtbasePrefix + "src/xml/"

    Product {
        name: root.privateName
        profiles: project.targetProfiles
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
        ModuleHeaders { fileTags: ["hpp_syncable"] }
    }

    QtModule {
        name: root.moduleName
        parentName: root.name
        simpleName: root.simpleName
        targetName: root.targetName
        qmakeProject: root.prefix + "xml.pro"

        Export {
            Depends { name: "cpp" }
            Depends { name: "Qt.core" }
            cpp.includePaths: root.publicIncludePaths
        }

        Depends { name: root.headersName }
        Depends { name: "Qt"; submodules: ["core", "core-private"] }

        cpp.defines: base.concat([
            "QT_BUILD_XML_LIB",
            "QT_NO_USING_NAMESPACE"
        ])

        cpp.includePaths: root.includePaths.concat(base)

        Properties {
            condition: qbs.targetOS.contains("windows")
                       && (qbs.toolchain.contains("msvc") || qbs.toolchain.contains("icc"))
            cpp.linkerFlags: base.concat("/BASE:0x66000000")
        }

        ModuleHeaders { }

        Group {
            name: "sources"
            prefix: root.prefix
            files: [
                "dom/qdom.cpp",
                "sax/qxml.cpp",
            ]
        }
    }
}

// QMAKE_DOCS = $$PWD/doc/qtxml.qdocconf
