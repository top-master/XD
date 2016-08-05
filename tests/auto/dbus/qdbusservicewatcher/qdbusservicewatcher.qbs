import qbs

QtAutotest {
    name: "tst_qdbusservicewatcher"
    Depends { name: "Qt.dbus" }
    files: ["tst_qdbusservicewatcher.cpp"]
}
