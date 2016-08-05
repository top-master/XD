import qbs

QtProduct {
    type: ["application"]
    consoleApplication: true
    destinationDirectory: project.buildDirectory + "/qdbusmarshalltest/qpong"
    Depends { name: "Qt.dbus" }
    files: ["qpong.cpp"]
}
