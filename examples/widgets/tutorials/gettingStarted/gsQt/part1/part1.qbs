import qbs

QtExample {
    name: "gettingStarted_part1"
    targetName: "part1"
    condition: Qt.widgets.present

    Depends { name: "Qt.widgets"; required: false }

    files: [
        "main.cpp",
    ]
}
