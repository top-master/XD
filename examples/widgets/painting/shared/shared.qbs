import qbs

QtProduct {
    name: "painting_examples_shared"
    condition: Qt.widgets.present
    type: ["staticlibrary"]

    Depends { name: "Qt.opengl"; required: false }
    Depends { name: "Qt.widgets"; required: false }

    files: [
        "arthurstyle.cpp",
        "arthurstyle.h",
        "arthurwidgets.cpp",
        "arthurwidgets.h",
        "hoverpoints.cpp",
        "hoverpoints.h",
        "shared.qrc",
    ]

    Properties {
        condition: Qt.opengl.present || project.config.contains("opengles2")
        cpp.defines: base.concat("QT_OPENGL_SUPPORT")
    }

    Export {
        Depends { name: "cpp" }
        cpp.includePaths: product.sourceDirectory
        Depends { name: "Qt.opengl"; required: false }
        Depends { name: "Qt.widgets"; required: false }
        Properties {
            condition: Qt.opengl.present || project.config.contains("opengles2")
            cpp.defines: base.concat("QT_OPENGL_SUPPORT")
        }
    }
}
