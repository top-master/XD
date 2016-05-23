import qbs

Project {
    PkgConfigDependency { name: "gtk+-3.0" }

    QtPlugin {
        name: "qgtk3"
        condition: project.config.contains("gtk3")
        category: "platformthemes"

        Depends { name: "Qt.core" }
        Depends { name: "Qt.core-private" }
        Depends { name: "Qt.gui" }
        Depends { name: "Qt.gui-private" }
        Depends { name: "Qt.platformsupport-private" }
        Depends { name: "gtk+-3.0" }
        Depends { name: "x11" }

        files: [
            "qgtk3dialoghelpers.cpp",
            "qgtk3dialoghelpers.h",
            "qgtk3theme.cpp",
            "qgtk3theme.h",
            "main.cpp",
        ]
    }

    // PLUGIN_EXTENDS = -
    // PLUGIN_CLASS_NAME = QGtk3ThemePlugin
}
