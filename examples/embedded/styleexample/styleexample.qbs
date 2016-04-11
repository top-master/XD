import qbs

QtExample {
    name: "styleexample"
    condition: Qt.widgets.present
    install: false
    Depends { name: "Qt.widgets"; required: false }
    files: [
        "main.cpp",
        "styleexample.qrc",
        "stylewidget.cpp",
        "stylewidget.h",
        "stylewidget.ui",
    ]
}
