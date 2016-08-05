import qbs

QtAutotest {
    name: "tst_qdbusmetatype"
    Depends { name: "Qt.dbus" }
    files: ["tst_qdbusmetatype.cpp"]
}
