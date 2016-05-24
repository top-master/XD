import qbs

QtPlugin {
    name: "qevdevtabletplugin"
    condition: project.config.contains("evdev")
    category: "generic"

    Depends { name: "Qt.core-private" }
    Depends { name: "Qt.gui" }
    Depends { name: "Qt.gui-private" }
    Depends { name: "Qt.platformsupport-private" }

    files: [
        "evdevtablet.json",
        "main.cpp",
    ]
}

// PLUGIN_EXTENDS = -
// PLUGIN_CLASS_NAME = QEvdevTabletPlugin
