import qbs

QtExample {
    name: "eventtransitions"
    condition: Qt.widgets.present

    Depends { name: "Qt.widgets"; required: false }

    files: [
        "main.cpp",
    ]
}
