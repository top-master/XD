import qbs

QtExample {
    name: "menus"
    condition: Qt.widgets.present && (!cpp.defines || !cpp.defines.contains("QT_NO_CURSOR")) // FIXME

    Depends { name: "Qt.widgets"; required: false }

    files: [
        "main.cpp",
        "mainwindow.cpp",
        "mainwindow.h",
    ]
}
