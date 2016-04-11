import qbs

QtExample {
    name: "weatheranchorlayout"
    condition: Qt.widgets.present

    Depends { name: "Qt.widgets"; required: false }

    files: [
        "main.cpp",
        "weatheranchorlayout.qrc",
    ]
}
