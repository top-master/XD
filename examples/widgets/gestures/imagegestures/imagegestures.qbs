import qbs

QtExample {
    name: "imagegestures"
    condition: Qt.widgets.present

    Depends { name: "Qt.widgets"; required: false }

    files: [
        "imagewidget.cpp",
        "imagewidget.h",
        "main.cpp",
        "mainwidget.cpp",
        "mainwidget.h",
    ]
}
