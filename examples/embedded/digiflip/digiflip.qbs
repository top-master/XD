import qbs

QtExample {
    name: "digiflip"
    condition: Qt.widgets.present
    install: false
    Depends { name: "Qt.widgets"; required: false }
    files: [
        "digiflip.cpp",
    ]
}
