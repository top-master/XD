import qbs

QtExample {
    name: "cube"
    condition: Qt.widgets.present && project.opengl

    Depends { name: "Qt.widgets"; required: false }

    files: [
        "geometryengine.cpp",
        "geometryengine.h",
        "main.cpp",
        "mainwidget.cpp",
        "mainwidget.h",
        "shaders.qrc",
        "textures.qrc",
    ]
}
