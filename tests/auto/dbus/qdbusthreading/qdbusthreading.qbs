import qbs

QtAutotest {
    name: "tst_qdbusthreading"
    Depends { name: "Qt.dbus" }
    files: ["tst_qdbusthreading.cpp"]
}
