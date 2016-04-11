import qbs

QtExample {
    name: "fridgemagnets"
    condition: Qt.widgets.present && (!cpp.defines || !cpp.defines.contains("QT_NO_DRAGANDDROP")) // FIXME

    Depends { name: "Qt.widgets"; required: false }

    files: [
        "draglabel.cpp",
        "draglabel.h",
        "dragwidget.cpp",
        "dragwidget.h",
        "fridgemagnets.qrc",
        "main.cpp",
    ]
}
