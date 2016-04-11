import qbs

QtExample {
    name: "hellogles3"
    condition: project.opengl && Qt.gui.present

    Depends { name: "Qt.gui"; required: false }

    files: [
        "../hellogl2/logo.cpp",
        "../hellogl2/logo.h",
        "glwindow.cpp",
        "glwindow.h",
        "hellogles3.qrc",
        "main.cpp",
    ]
}
