import qbs

QtExample {
    name: "imageviewer"
    condition: Qt.widgets.present

    Depends { name: "Qt.printsupport"; required: false }
    Depends { name: "Qt.widgets"; required: false }

    files: [
        "imageviewer.cpp",
        "imageviewer.h",
        "main.cpp",
    ]
}
