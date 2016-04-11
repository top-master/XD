import qbs

Project {
    name: "examples (actual products)"
    references: [
        "corelib/corelib.qbs",
        "dbus/dbus.qbs",
        "embedded/embedded.qbs",
        "gui/gui.qbs",
        "network/network.qbs",
        "opengl/opengl.qbs",
        "qpa/qpa.qbs",
        "qtconcurrent/qtconcurrent.qbs",
        "qtestlib/qtestlib.qbs",
        "sql/sql.qbs",
        "touch/touch.qbs",
        "widgets/widgets.qbs",
        "xml/xml.qbs",
    ]
}
