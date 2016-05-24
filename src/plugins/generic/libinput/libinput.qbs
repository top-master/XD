import qbs

QtPlugin {
    name: "qlibinputplugin"
    condition: project.config.contains("libinput")
    category: "generic"

    Depends { name: "Qt.core-private" }
    Depends { name: "Qt.gui" }
    Depends { name: "Qt.gui-private" }
    Depends { name: "Qt.platformsupport-private" }

    files: [
        "libinput.json",
        "main.cpp",
    ]
}

// PLUGIN_EXTENDS = -
// PLUGIN_CLASS_NAME = QLibInputPlugin
