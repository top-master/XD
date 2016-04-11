import qbs

QtExample {
    name: "gettingStarted_part5"
    targetName: "part5"
    condition: Qt.widgets.present

    Depends { name: "Qt.widgets"; required: false }

    files: [
        "main.cpp",
    ]
}
