import qbs

QtExample {
    name: "syntaxhighlighter"
    condition: Qt.widgets.present

    Depends { name: "Qt.widgets"; required: false }

    files: [
        "highlighter.cpp",
        "highlighter.h",
        "main.cpp",
        "mainwindow.cpp",
        "mainwindow.h",
    ]
}
