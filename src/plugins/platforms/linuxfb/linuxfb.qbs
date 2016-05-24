import qbs

QtPlugin {
    name: "qlinuxfb"
    condition: project.linuxfb
    category: "platforms"

    Depends { name: "Qt.core-private" }
    Depends { name: "Qt.gui-private" }
    Depends { name: "Qt.gui" }
    Depends { name: "Qt.platformsupport-private" }

    files: [
        "linuxfb.json",
        "main.cpp",
        "qlinuxfbintegration.cpp",
        "qlinuxfbintegration.h",
        "qlinuxfbscreen.cpp",
        "qlinuxfbscreen.h",
    ]
}

// PLUGIN_CLASS_NAME = QLinuxFbIntegrationPlugin
// !equals(TARGET, $$QT_DEFAULT_QPA_PLUGIN): PLUGIN_EXTENDS = -
// CONFIG += qpa/genericunixfontdatabase
