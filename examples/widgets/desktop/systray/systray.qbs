import qbs

QtExample {
    name: "systray"
    condition: Qt.widgets.present

    Depends { name: "Qt.widgets"; required: false }

    files: [
        "main.cpp",
        "systray.qrc",
        "window.cpp",
        "window.h",
    ]
}
