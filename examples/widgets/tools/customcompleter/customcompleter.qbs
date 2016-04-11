import qbs

QtExample {
    name: "customcompleter"
    condition: Qt.widgets.present

    Depends { name: "Qt.widgets"; required: false }

    files: [
        "customcompleter.qrc",
        "main.cpp",
        "mainwindow.cpp",
        "mainwindow.h",
        "textedit.cpp",
        "textedit.h",
    ]
}
