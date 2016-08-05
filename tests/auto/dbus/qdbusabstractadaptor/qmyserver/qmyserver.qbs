import qbs

QtProduct {
    name: "qmyserver-abstractadaptor"
    targetName: "qmyserver"
    type: ["application"]
    consoleApplication: true
    destinationDirectory: project.buildDirectory + "/qdbusabstractadaptortest/qmyserver"
    Depends { name: "Qt.dbus" }
    files: [
        "qmyserver.cpp",
        "../myobject.h",
    ]
}
