import qbs

Project {
    name: "tools"

    references: [
        "bootstrap/bootstrap.qbs",
        "bootstrap-dbus/bootstrap-dbus.qbs",
        "moc/moc.qbs",
        // "qdbuscpp2xml/qdbuscpp2xml.qbs",
        "qdbusxml2cpp/qdbusxml2cpp.qbs",
        // "qlalr/qlalr.qbs",
        "rcc/rcc.qbs",
        // "uic/uic.qbs",
    ]
}
