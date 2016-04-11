import qbs

QtExample {
    name: "mainwindow"
    condition: Qt.widgets.present && (!cpp.defines || !cpp.defines.contains("QT_NO_CURSOR")) // FIXME

    Depends { name: "Qt.gui"; required: false }
    Depends { name: "Qt.widgets"; required: false }

    files: [
        "colorswatch.cpp",
        "colorswatch.h",
        "main.cpp",
        "mainwindow.cpp",
        "mainwindow.h",
        "mainwindow.qrc",
        "toolbar.cpp",
        "toolbar.h",
    ]
}
