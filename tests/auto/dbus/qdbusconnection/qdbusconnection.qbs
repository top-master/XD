import qbs

QtAutotest {
    name: "tst_qdbusconnection"
    Depends { name: "Qt.dbus" }
    files: [
        "tst_qdbusconnection.cpp",
        "tst_qdbusconnection.h",
    ]
}
