import qbs

QtExample {
    name: "widgets_analogclock"
    targetName: "analogclock"
    condition: Qt.widgets.present

    Depends { name: "Qt.widgets"; required: false }

    files: [
        "analogclock.cpp",
        "analogclock.h",
        "main.cpp",
    ]
}
