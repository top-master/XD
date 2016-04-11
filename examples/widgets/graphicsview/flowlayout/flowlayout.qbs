import qbs

QtExample {
    name: "graphicsviewflowlayout"
    targetName: "flowlayout"
    condition: Qt.widgets.present

    Depends { name: "Qt.widgets"; required: false }

    files: [
        "flowlayout.cpp",
        "flowlayout.h",
        "main.cpp",
        "window.cpp",
        "window.h",
    ]
}
