import qbs

Project {
    references: [
        "examples/examples.qbs",
        "src/android/android.qbs",
        "src/concurrent/concurrent.qbs",
        "src/printsupport/printsupport.qbs",
        "src/xml/xml.qbs",
        "src/3rdparty/3rdparty.qbs",
        "src/tools/tools.qbs",
        "src/corelib/corelib.qbs",
        "src/dbus/dbus.qbs",
        "src/network/network.qbs",
        "src/opengl/opengl.qbs",
        "src/openglextensions/openglextensions.qbs",
        "src/platformsupport/platformsupport.qbs",
        "src/platformheaders/platformheaders.qbs",
        "src/gui/gui.qbs",
        "src/sql/sql.qbs",
        "src/testlib/testlib.qbs",
        "src/widgets/widgets.qbs",
        "src/plugins/plugins.qbs",
        "tests",
        // TODO: winmain
    ]
}
