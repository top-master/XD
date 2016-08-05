import qbs

QtAutotest {
    name: "tst_qdbusconnection_no_app"
    Depends { name: "Qt.dbus" }
    cpp.defines: base.concat([
            'SRCDIR="' + path + '"',
            'tst_QDBusConnection=tst_QDBusConnection_NoApplication'
    ])
    files: [
        "tst_qdbusconnection_no_app.cpp",
        "../qdbusconnection/tst_qdbusconnection.h",
    ]
}
