import qbs

QtAutotest {
    name: "tst_qdbusabstractinterface"
    destinationDirectory: project.buildDirectory + "/qdbusabstractinterfacetest"
    Depends { name: "qpinger" }
    Depends { name: "Qt.dbus" }
    files: [
        "../interface.cpp",
        "../interface.h",
        "../tst_qdbusabstractinterface.cpp",
    ]

    Group {
        name: "D-Bus interface"
        files: ["../org.qtproject.QtDBus.Pinger.xml"]
        fileTags: ["qt.dbus.interface"]
    }
    Qt.dbus.xml2CppHeaderFlags: ["-i", path + "/../interface.h"] // TODO: QBS-1005
}
