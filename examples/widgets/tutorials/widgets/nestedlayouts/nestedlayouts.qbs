import qbs

QtExample {
    name: "nestedlayouts"
    condition: Qt.widgets.present

    Depends { name: "Qt.widgets"; required: false }

    files: [
        "main.cpp",
    ]
}
