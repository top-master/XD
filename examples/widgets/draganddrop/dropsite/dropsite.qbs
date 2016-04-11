import qbs

QtExample {
    name: "dropsite"
    condition: Qt.widgets.present && (!cpp.defines || !cpp.defines.contains("QT_NO_DRAGANDDROP")) // FIXME

    Depends { name: "Qt.widgets"; required: false }

    files: [
        "droparea.cpp",
        "droparea.h",
        "dropsitewindow.cpp",
        "dropsitewindow.h",
        "main.cpp",
    ]
}
