import qbs

QtExample {
    name: "concentriccircles"
    condition: Qt.widgets.present

    Depends { name: "Qt.widgets"; required: false }

    files: [
        "circlewidget.cpp",
        "circlewidget.h",
        "main.cpp",
        "window.cpp",
        "window.h",
    ]
}
