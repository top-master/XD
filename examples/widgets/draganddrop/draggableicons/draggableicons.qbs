import qbs

QtExample {
    name: "draggableicons"
    condition: Qt.widgets.present && (!cpp.defines || !cpp.defines.contains("QT_NO_DRAGANDDROP")) // FIXME

    Depends { name: "Qt.widgets"; required: false }

    files: [
        "draggableicons.qrc",
        "dragwidget.cpp",
        "dragwidget.h",
        "main.cpp",
    ]
}
