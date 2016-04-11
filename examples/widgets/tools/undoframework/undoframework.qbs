import qbs

QtExample {
    name: "undoframework"
    condition: Qt.widgets.present

    Depends { name: "Qt.widgets"; required: false }

    files: [
        "commands.cpp",
        "commands.h",
        "diagramitem.cpp",
        "diagramitem.h",
        "diagramscene.cpp",
        "diagramscene.h",
        "main.cpp",
        "mainwindow.cpp",
        "mainwindow.h",
        "undoframework.qrc",
    ]
}
