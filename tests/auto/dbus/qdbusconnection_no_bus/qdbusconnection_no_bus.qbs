import qbs

QtAutotest {
    name: "tst_qdbusconnection_no_bus"
    Depends { name: "Qt.dbus" }
    files: ["tst_qdbusconnection_no_bus.cpp"]
}
