import qbs

QtAutotest {
    name: "tst_qdbusmarshall"
    destinationDirectory: project.buildDirectory + "/qdbusmarshalltest"

    Depends { name: "qpong" }
    Depends { name: "Qt.core-private" }
    Depends { name: "Qt.dbus" }
    Depends { name: "Qt.dbus-private" }
    Depends { name: "dbus-linked"; condition: project.config.contains("dbus-linked") }

    files: ["../tst_qdbusmarshall.cpp"]

    Group {
        name: "nonlinked"
        condition: !project.config.contains("dbus-linked")
        files: ["../../../../../src/dbus/qdbus_symbols.cpp"]
    }
}
