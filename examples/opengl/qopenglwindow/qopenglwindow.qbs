import qbs

QtExample {
    name: "qopenglwindow"
    condition: project.opengl && Qt.gui.present

    Depends { name: "Qt.gui"; required: false }

    cpp.includePaths: base.concat(".")

    files: [
        "background_renderer.cpp",
        "background_renderer.h",
        "main.cpp",
        "shaders.qrc",
    ]
}
