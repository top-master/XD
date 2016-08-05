import qbs

QtAutotest {
    name: "tst_qdbusinterface"
    destinationDirectory: project.buildDirectory + "/qdbusinterfacetest"
    Depends { name: "qmyserver-interface" }
    Depends { name: "Qt.core-private" }
    Depends { name: "Qt.dbus" }
    files: [
        "../myobject.h",
        "../tst_qdbusinterface.cpp",
    ]
}
