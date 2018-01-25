import qbs
import qbs.FileInfo

QtModuleProject {
    name: "QtXml"
    simpleName: "xml"
    conditionFunction: (function() {
        return QtGlobalPrivateConfig.xml;
    })

    QtHeaders {
        files: base.concat([
            project.qtbaseShadowDir + "/src/xml/qtxml-config.h",
            project.qtbaseShadowDir + "/src/xml/qtxml-config_p.h",
        ])
        Depends { name: "QtGlobalPrivateConfig" }
    }

    QtPrivateModule {
        Depends { name: "QtGlobalPrivateConfig" }
    }

    QtModule {
        name: project.moduleName
        simpleName: project.simpleName

        Export {
            Depends { name: "cpp" }
            Depends { name: "Qt.core" }
            cpp.includePaths: project.publicIncludePaths
        }

        Depends { name: project.headersName }
        Depends { name: "Qt"; submodules: ["core-private"] }

        cpp.enableExceptions: true
        cpp.includePaths: project.includePaths.concat(base)
        cpp.defines: base.concat([
            "QT_NO_FOREACH",
            "QT_NO_USING_NAMESPACE",
        ])

        Properties {
            condition: qbs.targetOS.contains("windows") && qbs.toolchain.contains("msvc")
            cpp.linkerFlags: base.concat("/BASE:0x61000000")
        }

        files: [
            "qtxmlglobal.h",
        ]

        Group {
            name: "DOM"
            prefix: "dom/"
            files: [
                "qdom.cpp",
                "qdom.h",
            ]
        }

        Group {
            name: "SAX"
            prefix: "sax/"
            files: [
                "qxml.cpp",
                "qxml.h",
                "qxml_p.h",
            ]
        }

        Group {
            name: "Qt.core precompiled header"
            files: ["../corelib/global/qt_pch.h"]
        }
    }
}
