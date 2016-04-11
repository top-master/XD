import qbs

QtExample {
    name: "scribble"
    condition: Qt.widgets.present

    Depends { name: "Qt.printsupport"; required: false }
    Depends { name: "Qt.widgets"; required: false }

    files: [
        "main.cpp",
        "mainwindow.cpp",
        "mainwindow.h",
        "scribblearea.cpp",
        "scribblearea.h",
    ]
}
