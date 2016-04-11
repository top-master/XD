import qbs

QtExample {
    name: "itemviewpuzzle"
    targetName: "puzzle"
    condition: Qt.widgets.present && (!cpp.defines || !cpp.defines.contains("QT_NO_DRAGANDDROP")) // FIXME

    Depends { name: "Qt.widgets"; required: false }

    files: [
        "main.cpp",
        "mainwindow.cpp",
        "mainwindow.h",
        "piecesmodel.cpp",
        "piecesmodel.h",
        "puzzle.qrc",
        "puzzlewidget.cpp",
        "puzzlewidget.h",
    ]
}
