import qbs

QtExample {
    name: "simpleanchorlayout"
    condition: Qt.widgets.present

    Depends { name: "Qt.widgets"; required: false }

    files: [
        "main.cpp",
    ]
}
