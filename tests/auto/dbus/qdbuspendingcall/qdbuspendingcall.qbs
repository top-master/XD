import qbs

QtAutotest {
    name: "tst_qdbuspendingcall"
    Depends { name: "Qt.dbus" }
    files: ["tst_qdbuspendingcall.cpp"]
}
