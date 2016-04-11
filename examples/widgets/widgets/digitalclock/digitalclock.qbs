import qbs

QtExample {
    name: "digitalclock"
    condition: Qt.widgets.present

    Depends { name: "Qt.widgets"; required: false }

    files: [
        "digitalclock.cpp",
        "digitalclock.h",
        "main.cpp",
    ]
}
