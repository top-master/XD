import qbs

QtExample {
    name: "htmlinfo"
    condition: Qt.xml.present
    consoleApplication: true

    Depends { name: "Qt.xml"; required: false }

    files: [
        "main.cpp",
        "resources.qrc",
    ]
}
