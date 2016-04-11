import qbs

QtExample {
    name: "textures"
    condition: Qt.widgets.present && project.opengl

    Depends { name: "Qt.widgets"; required: false }

    files: [
        "glwidget.cpp",
        "glwidget.h",
        "main.cpp",
        "textures.qrc",
        "window.cpp",
        "window.h",
    ]
}
