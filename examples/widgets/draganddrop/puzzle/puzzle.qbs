import qbs

QtExample {
    name: "draganddroppuzzle"
    targetName: "puzzle"
    condition: Qt.widgets.present && (!cpp.defines || !cpp.defines.contains("QT_NO_DRAGANDDROP")) // FIXME

    Depends { name: "Qt.widgets"; required: false }

    files: [
        "main.cpp",
        "mainwindow.cpp",
        "mainwindow.h",
        "pieceslist.cpp",
        "pieceslist.h",
        "puzzle.qrc",
        "puzzlewidget.cpp",
        "puzzlewidget.h",
    ]
}
