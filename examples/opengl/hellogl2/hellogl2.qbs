import qbs

QtExample {
    name: "hellogl2"
    condition: Qt.widgets.present && project.opengl

    Depends { name: "Qt.widgets"; required: false }

    files: [
        "glwidget.cpp",
        "glwidget.h",
        "logo.cpp",
        "logo.h",
        "main.cpp",
        "mainwindow.cpp",
        "mainwindow.h",
        "window.cpp",
        "window.h",
    ]
}
