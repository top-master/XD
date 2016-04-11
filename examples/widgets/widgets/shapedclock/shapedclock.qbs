import qbs

QtExample {
    name: "shapedclock"
    condition: Qt.widgets.present

    Depends { name: "Qt.widgets"; required: false }

    files: [
        "main.cpp",
        "shapedclock.cpp",
        "shapedclock.h",
    ]
}
