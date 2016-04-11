import qbs

QtExample {
    name: "qopenglwidget"
    condition: Qt.widgets.present && project.opengl

    Depends { name: "Qt.widgets"; required: false }

    files: [
        "bubble.cpp",
        "bubble.h",
        "glwidget.cpp",
        "glwidget.h",
        "main.cpp",
        "mainwindow.cpp",
        "mainwindow.h",
        "texture.qrc",
    ]
}
