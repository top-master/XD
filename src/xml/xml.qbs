import qbs
import qbs.FileInfo
import QtXmlConfig
import QtXmlPrivateConfig

QtModuleProject {
    name: "QtXml"
    simpleName: "xml"
    conditionFunction: (function() {
        return Qt.global.privateConfig.xml;
    })

    QtHeaders {
        Depends { name: "Qt.global" }
        shadowBuildFiles: [
            project.qtbaseShadowDir + "/src/xml/qtxml-config.h",
            project.qtbaseShadowDir + "/src/xml/qtxml-config_p.h",
        ]
    }

    QtPrivateModule {
        config: QtXmlPrivateConfig
    }

    QtModule {
        Export {
            Depends { name: "cpp" }
            Depends { name: "Qt.core" }
            cpp.includePaths: project.publicIncludePaths
        }

        Depends { name: project.headersName }
        Depends { name: "Qt.core_private" }

        config: QtXmlConfig
        cpp.enableExceptions: true
        cpp.includePaths: project.includePaths.concat(base)
        cpp.defines: base.concat([
            "QT_NO_FOREACH",
            "QT_NO_USING_NAMESPACE",
        ])

        Properties {
            condition: qbs.toolchain.contains("msvc") && qbs.architecture === "x86"
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
