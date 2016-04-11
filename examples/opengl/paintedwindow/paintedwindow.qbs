import qbs

QtExample {
    name: "paintedwindow"
    condition: project.opengl && Qt.gui.present

    Depends { name: "Qt.gui"; required: false }

    files: [
        "main.cpp",
        "paintedwindow.cpp",
        "paintedwindow.h",
    ]
}
