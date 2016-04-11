import qbs

QtExample {
    name: "openglwindow"
    condition: Qt.gui.present && (project.opengl || project.opengles2)
    Depends { name: "Qt.gui"; required: false }
    cpp.includePaths: base.concat(".")
    files: [
        "main.cpp",
        "openglwindow.cpp",
        "openglwindow.h",
    ]
}
