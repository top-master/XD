import qbs

QtExample {
    name: "borderlayout"
    condition: Qt.widgets.present

    Depends { name: "Qt.widgets"; required: false }

    files: [
        "borderlayout.cpp",
        "borderlayout.h",
        "main.cpp",
        "window.cpp",
        "window.h",
    ]
}
