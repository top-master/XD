import qbs

QtExample {
    name: "pixelator"
    condition: Qt.widgets.present

    Depends { name: "Qt.printsupport"; required: false }
    Depends { name: "Qt.widgets"; required: false }

    files: [
        "imagemodel.cpp",
        "imagemodel.h",
        "images.qrc",
        "main.cpp",
        "mainwindow.cpp",
        "mainwindow.h",
        "pixeldelegate.cpp",
        "pixeldelegate.h",
    ]
}
