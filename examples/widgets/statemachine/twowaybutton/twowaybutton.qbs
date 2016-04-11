import qbs

QtExample {
    name: "twowaybutton"
    condition: Qt.widgets.present

    Depends { name: "Qt.widgets"; required: false }

    files: [
        "main.cpp",
    ]
}
