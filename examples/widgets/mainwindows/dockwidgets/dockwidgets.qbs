import qbs

QtExample {
    name: "dockwidgets"
    condition: Qt.widgets.present && (!cpp.defines || !cpp.defines.contains("QT_NO_CURSOR")) // FIXME

    Depends { name: "Qt.printsupport"; required: false }
    Depends { name: "Qt.widgets"; required: false }

    files: [
        "dockwidgets.qrc",
        "main.cpp",
        "mainwindow.cpp",
        "mainwindow.h",
    ]
}
