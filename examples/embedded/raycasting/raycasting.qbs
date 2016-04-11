import qbs

QtExample {
    name: "raycasting"
    condition: Qt.widgets.present
    install: false
    Depends { name: "Qt.widgets"; required: false }
    files: [
        "raycasting.cpp",
        "raycasting.qrc",
    ]
}
