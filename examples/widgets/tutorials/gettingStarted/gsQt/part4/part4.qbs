import qbs

QtExample {
    name: "gettingStarted_part4"
    targetName: "part4"
    condition: Qt.widgets.present

    Depends { name: "Qt.widgets"; required: false }

    files: [
        "main.cpp",
    ]
}
