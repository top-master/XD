import qbs

QtExample {
    name: "tablet"
    condition: Qt.widgets.present

    Depends { name: "Qt.widgets"; required: false }

    files: [
        "images.qrc",
        "main.cpp",
        "mainwindow.cpp",
        "mainwindow.h",
        "tabletapplication.cpp",
        "tabletapplication.h",
        "tabletcanvas.cpp",
        "tabletcanvas.h",
    ]
}
