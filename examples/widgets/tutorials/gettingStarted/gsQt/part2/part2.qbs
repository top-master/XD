import qbs

QtExample {
    name: "gettingStarted_part2"
    targetName: "part2"
    condition: Qt.widgets.present

    Depends { name: "Qt.widgets"; required: false }

    files: [
        "main.cpp",
    ]
}
