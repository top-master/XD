import qbs

QtExample {
    name: "completer"
    condition: Qt.widgets.present

    Depends { name: "Qt.widgets"; required: false }

    files: [
        "completer.qrc",
        "fsmodel.cpp",
        "fsmodel.h",
        "main.cpp",
        "mainwindow.cpp",
        "mainwindow.h",
    ]
}
