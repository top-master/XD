import qbs

QtExample {
    name: "windowlayout"
    condition: Qt.widgets.present

    Depends { name: "Qt.widgets"; required: false }

    files: [
        "main.cpp",
    ]
}
