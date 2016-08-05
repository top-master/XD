import qbs

QtAutotest {
    name: "tst_qdbuspendingreply"
    Depends { name: "Qt.dbus" }
    files: ["tst_qdbuspendingreply.cpp"]
}
