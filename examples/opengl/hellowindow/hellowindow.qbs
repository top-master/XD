import qbs

QtExample {
    name: "hellowindow"
    condition: Qt.gui.present && project.opengl

    Depends { name: "Qt.core-private" }
    Depends { name: "Qt.gui"; required: false }
    Depends { name: "Qt.gui-private"; required: false }

    files: [
        "hellowindow.cpp",
        "hellowindow.h",
        "main.cpp",
    ]
}
