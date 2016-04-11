import qbs

QtExample {
    name: "listnames"
    condition: Qt.dbus.present
    consoleApplication: qbs.targetOS.contains("windows")
    Depends { name: "Qt.dbus"; required: false }
    files: [
        "listnames.cpp",
    ]
}
