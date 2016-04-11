import qbs

QtExample {
    name: "windowflags"
    condition: Qt.widgets.present

    Depends { name: "Qt.gui"; required: false }
    Depends { name: "Qt.widgets"; required: false }

    files: [
        "controllerwindow.cpp",
        "controllerwindow.h",
        "main.cpp",
        "previewwindow.cpp",
        "previewwindow.h",
    ]
}
