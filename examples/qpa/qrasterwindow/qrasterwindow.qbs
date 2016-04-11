import qbs

QtExample {
    name: "qrasterwindow"
    condition: Qt.gui.present
    Depends { name: "Qt.gui"; required: false }
    files: [
        "main.cpp",
    ]
}
