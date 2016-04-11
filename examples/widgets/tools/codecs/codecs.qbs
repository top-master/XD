import qbs

QtExample {
    name: "codecs"
    condition: Qt.widgets.present

    Depends { name: "Qt.widgets"; required: false }

    files: [
        "main.cpp",
        "mainwindow.cpp",
        "mainwindow.h",
        "previewform.cpp",
        "previewform.h",
    ]
}
