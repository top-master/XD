import qbs

QtAutotest {
    name: "tst_qdbuscontext"
    Depends { name: "Qt.dbus" }
    files: ["tst_qdbuscontext.cpp"]
}
