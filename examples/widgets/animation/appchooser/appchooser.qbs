import qbs

QtExample {
    name: "appchooser"
    condition: Qt.widgets.present

    Depends { name: "Qt.widgets"; required: false }

    files: [
        "appchooser.qrc",
        "main.cpp",
    ]
}
