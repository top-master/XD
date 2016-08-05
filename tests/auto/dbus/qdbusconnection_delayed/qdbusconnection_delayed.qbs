import qbs

QtAutotest {
    name: "tst_qdbusconnection_delayed"
    Depends { name: "Qt.dbus" }
    cpp.defines: base.concat['SRCDIR="' + path + '"']
    files: ["tst_qdbusconnection_delayed.cpp"]
}

//CONFIG += parallel_test
