import qbs

QtAutotest {
    name: "tst_qdbusmetaobject"
    Depends { name: "Qt.dbus" }
    Depends { name: "Qt.dbus-private" }
    files: ["tst_qdbusmetaobject.cpp"]
}
