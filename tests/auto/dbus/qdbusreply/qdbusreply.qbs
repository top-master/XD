import qbs

QtAutotest {
    name: "tst_qdbusreply"
    Depends { name: "Qt.dbus" }
    files: ["tst_qdbusreply.cpp"]
}
