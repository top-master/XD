import qbs

QtProduct {
    name: "qmyserver-interface"
    targetName: "qmyserver"
    type: ["application"]
    consoleApplication: true
    destinationDirectory: project.buildDirectory + "/qdbusinterfacetest/qmyserver"
    Depends { name: "Qt.dbus" }
    files: [
        "../myobject.h",
        "qmyserver.cpp",
    ]
}
