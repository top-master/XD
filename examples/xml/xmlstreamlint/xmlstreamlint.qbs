import qbs

QtExample {
    name: "xmlstreamlint"
    condition: Qt.xml.present
    consoleApplication: true

    Depends { name: "Qt.xml"; required: false }

    files: [
        "main.cpp",
    ]
}
