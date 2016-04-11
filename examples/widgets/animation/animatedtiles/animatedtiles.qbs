import qbs

QtExample {
    name: "animatedtiles"
    condition: Qt.widgets.present

    Depends { name: "Qt.widgets"; required: false }

    files: [
        "animatedtiles.qrc",
        "main.cpp",
    ]
}
