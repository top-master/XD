import qbs

QtExample {
    name: "pinchzoom"
    condition: Qt.widgets.present

    Depends { name: "Qt.widgets"; required: false }

    files: [
        "graphicsview.cpp",
        "graphicsview.h",
        "main.cpp",
        "mice.qrc",
        "mouse.cpp",
        "mouse.h",
    ]
}
