import qbs

QtExample {
    name: "2dpainting"
    condition: Qt.widgets.present && project.opengl

    Depends { name: "Qt.widgets"; required: false }

    files: [
        "glwidget.cpp",
        "glwidget.h",
        "helper.cpp",
        "helper.h",
        "main.cpp",
        "widget.cpp",
        "widget.h",
        "window.cpp",
        "window.h",
    ]
}
