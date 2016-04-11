import qbs

QtExample {
    name: "draggabletext"
    condition: Qt.widgets.present && (!cpp.defines || !cpp.defines.contains("QT_NO_DRAGANDDROP")) // FIXME

    Depends { name: "Qt.widgets"; required: false }

    files: [
        "draggabletext.qrc",
        "dragwidget.cpp",
        "dragwidget.h",
        "main.cpp",
    ]
}
