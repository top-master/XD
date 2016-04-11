import qbs

QtExample {
    name: "threadedqopenglwidget"
    condition: Qt.widgets.present && project.opengl

    Depends { name: "Qt.widgets"; required: false }

    files: [
        "glwidget.cpp",
        "glwidget.h",
        "main.cpp",
        "mainwindow.cpp",
        "mainwindow.h",
    ]
}
