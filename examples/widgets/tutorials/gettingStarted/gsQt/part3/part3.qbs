import qbs

QtExample {
    name: "gettingStarted_part3"
    targetName: "part3"
    condition: Qt.widgets.present

    Depends { name: "Qt.widgets"; required: false }

    files: [
        "main.cpp",
    ]
}
