import qbs

QtExample {
    name: "undo"
    condition: Qt.widgets.present

    Depends { name: "Qt.widgets"; required: false }

    files: [
        "commands.cpp",
        "commands.h",
        "document.cpp",
        "document.h",
        "main.cpp",
        "mainwindow.cpp",
        "mainwindow.h",
        "mainwindow.ui",
        "undo.qrc",
    ]
}
