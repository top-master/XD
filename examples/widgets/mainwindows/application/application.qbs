import qbs

QtExample {
    name: "application"
    condition: Qt.widgets.present && (!cpp.defines || !cpp.defines.contains("QT_NO_CURSOR")) // FIXME

    Depends { name: "Qt.widgets"; required: false }

    files: [
        "application.qrc",
        "main.cpp",
        "mainwindow.cpp",
        "mainwindow.h",
    ]
}
