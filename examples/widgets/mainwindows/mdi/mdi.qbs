import qbs

QtExample {
    name: "mdi"
    condition: Qt.widgets.present && (!cpp.defines || !cpp.defines.contains("QT_NO_CURSOR")) // FIXME

    Depends { name: "Qt.widgets"; required: false }

    files: [
        "main.cpp",
        "mainwindow.cpp",
        "mainwindow.h",
        "mdi.qrc",
        "mdichild.cpp",
        "mdichild.h",
    ]
}
