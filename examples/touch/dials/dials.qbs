import qbs

QtExample {
    name: "dials"
    condition: Qt.widgets.present

    Depends { name: "Qt.widgets"; required: false }

    files: [
        "dials.ui",
        "main.cpp",
    ]
}
