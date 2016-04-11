import qbs

QtExample {
    name: "transformations"
    condition: Qt.widgets.present

    Depends { name: "Qt.widgets"; required: false }

    files: [
        "main.cpp",
        "renderarea.cpp",
        "renderarea.h",
        "window.cpp",
        "window.h",
    ]
}
