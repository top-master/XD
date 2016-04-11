import qbs

QtExample {
    name: "charactermap"
    condition: Qt.widgets.present

    Depends { name: "Qt.widgets"; required: false }

    files: [
        "characterwidget.cpp",
        "characterwidget.h",
        "main.cpp",
        "mainwindow.cpp",
        "mainwindow.h",
    ]
}
