import qbs

QtExample {
    name: "windows"
    condition: Qt.gui.present
    consoleApplication: true

    Depends { name: "Qt.core-private"; required: false }
    Depends { name: "Qt.gui"; required: false }
    Depends { name: "Qt.gui-private"; required: false }

    files: [
        "main.cpp",
        "window.cpp",
        "window.h",
    ]
}
