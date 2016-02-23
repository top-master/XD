import qbs
import "qbs/imports/QtProject.qbs" as QtProject

/*
  This template has all modules enabled by default, with all configurable features
  disabled by default. That means it builds a very minimal subset of Qt. It is
  designed to be used in conjunction with a "subclass" which sets all the
  relevant properties to more sane values. See configure.qbs, which will generate
  a static subclass of this project, or perform a configure with the -qbs option,
  which will also generate a suitable subclass of this project.
*/
QtProject {
    name: "Qt"

    qbsSearchPaths: "qbs"

    references: [
        qtbasePrefix + "src/android/android.qbs",
        qtbasePrefix + "src/3rdparty/3rdparty.qbs",
        qtbasePrefix + "src/tools/tools.qbs",
        qtbasePrefix + "src/corelib/corelib.qbs",
        qtbasePrefix + "src/dbus/dbus.qbs",
        qtbasePrefix + "src/network/network.qbs",
        qtbasePrefix + "src/platformsupport/platformsupport.qbs",
        qtbasePrefix + "src/platformheaders/platformheaders.qbs",
        qtbasePrefix + "src/gui/gui.qbs",
        qtbasePrefix + "src/sql/sql.qbs",
        qtbasePrefix + "src/testlib/testlib.qbs",
        qtbasePrefix + "src/widgets/widgets.qbs",
        qtbasePrefix + "src/plugins/plugins.qbs",
        // TODO: android, concurrent, dbus, --opengl--, openglextensions,
        // plugins, printsupport, remaining tools, winmain, --xml--
    ]
}
