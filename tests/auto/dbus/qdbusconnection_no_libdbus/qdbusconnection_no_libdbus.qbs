import qbs

QtAutotest {
    name: "tst_qdbusconnection_no_libdbus"
    Depends { name: "Qt.dbus" }
    cpp.defines: base.concat([
           'SIMULATE_LOAD_FAIL',
           'tst_QDBusConnectionNoBus=tst_QDBusConnectionNoLibDBus1'
    ])
    files: ["../qdbusconnection_no_bus/tst_qdbusconnection_no_bus.cpp"]
}
