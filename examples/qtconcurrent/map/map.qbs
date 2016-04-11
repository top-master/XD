import qbs

QtExample {
    name: "mapdemo"
    condition: Qt.concurrent.present && Qt.gui.present
    consoleApplication: true

    Depends { name: "Qt.concurrent"; required: false }
    Depends { name: "Qt.gui"; required: false }

    files: [
        "main.cpp",
    ]
}
