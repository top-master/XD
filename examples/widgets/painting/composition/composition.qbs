import qbs

QtExample {
    name: "composition"
    condition: painting_examples_shared.present

    Depends { name: "Qt.opengl"; required: false }
    Depends { name: "painting_examples_shared"; required: false }

    files: [
        "composition.cpp",
        "composition.h",
        "composition.qrc",
        "main.cpp",
    ]

    Properties {
        condition: Qt.opengl.present && !project.config.contains("dynamicgl")
        cpp.defines: base.concat("USE_OPENGL")
    }
}
