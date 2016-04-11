import qbs

QtExample {
    name: "contextinfo"
    condition: Qt.widgets.present && project.opengl
    install: false

    Depends { name: "Qt.widgets"; required: false }

    files: [
        "main.cpp",
        "renderwindow.cpp",
        "renderwindow.h",
        "widget.cpp",
        "widget.h",
    ]
}
