import qbs
import qbs.Process

QtPlugin {
    name: "composeplatforminputcontextplugin"
    condition: project.config.contains("xcb-plugin")
    category: "platforminputcontexts"

    Depends { name: "Qt.core-private" }
    Depends { name: "Qt.gui" }
    Depends { name: "Qt.gui-private" }
    Depends { name: "xkbcommon"; condition: !project.config.contains("xkbcommon-qt") }

    // TODO: Should we generalize the PkgConfigProbe to support this kind of stuff?
    Probe {
        id: x11PrefixProbe
        property string x11Prefix: "/usr"
        configure: {
            var pkgConfig = new Process;
            try {
                if (qbs.sysroot) {
                    pkgConfig.setEnv("PKG_CONFIG_SYSROOT_DIR", qbs.sysroot);
                    var libDirs = [qbs.sysroot + "/usr/lib/pkgconfig",
                                   qbs.sysroot + "/usr/share/pkgconfig"];
                    pkgConfig.setEnv("PKG_CONFIG_LIBDIR", libDirs.join(qbs.pathListSeparator));
                }
                if (pkgConfig.exec("pkg-config", ["--variable=prefix", "x11"]) === 0) {
                    var prefix = pkgConfig.readStdOut().trim();
                    if (prefix) {
                        found = true;
                        x11Prefix = prefix;
                    }
                }
            } finally {
                pkgConfig.close();
            }
        }
    }
    cpp.defines: base.concat(['X11_PREFIX="' + x11PrefixProbe.x11Prefix + '"'])

    files: [
        "compose.json",
        "generator/qtablegenerator.cpp",
        "generator/qtablegenerator.h",
        "qcomposeplatforminputcontext.cpp",
        "qcomposeplatforminputcontext.h",
        "qcomposeplatforminputcontextmain.cpp",
    ]
}

// PLUGIN_EXTENDS = -
// PLUGIN_CLASS_NAME = QComposePlatformInputContextPlugin
