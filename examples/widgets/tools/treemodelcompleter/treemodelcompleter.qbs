import qbs

QtExample {
    name: "treemodelcompleter"
    condition: Qt.widgets.present

    Depends { name: "Qt.widgets"; required: false }

    files: [
        "main.cpp",
        "mainwindow.cpp",
        "mainwindow.h",
        "treemodelcompleter.cpp",
        "treemodelcompleter.h",
        "treemodelcompleter.qrc",
    ]
}
