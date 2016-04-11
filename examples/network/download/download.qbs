import qbs

QtExample {
    name: "download"
    condition: Qt.network.present
    consoleApplication: true

    Depends { name: "Qt.network"; required: false }

    files: [
        "main.cpp",
    ]
}
