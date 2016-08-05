import qbs

QtProduct {
    name: "qpinger"
    type: ["application"]
    consoleApplication: true
    destinationDirectory: project.buildDirectory + "/qdbusabstractinterfacetest/qpinger"
    Depends { name: "Qt.dbus" }
    files: [
        "qpinger.cpp",
        "../interface.cpp",
        "../interface.h",
    ]
}
