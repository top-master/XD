import qbs

QtAutotest {
    name: "tst_qdbusabstractadaptor"
    destinationDirectory: project.buildDirectory + "/qdbusabstractadaptortest"
    Depends { name: "qmyserver-abstractadaptor" }
    Depends { name: "Qt.core-private" }
    Depends { name: "Qt.dbus" }
    files: [
        "../myobject.h",
        "../tst_qdbusabstractadaptor.cpp",
    ]
}
