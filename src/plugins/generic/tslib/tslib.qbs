import qbs

QtPlugin {
    name: "qtslibplugin"
    condition: project.config.contains("tslib")
    category: "generic"

    Depends { name: "Qt.gui" }
    Depends { name: "Qt.gui-private" }
    Depends { name: "Qt.platformsupport-private" }

    cpp.dynamicLibraries: base.concat(["ts"])

    files: [
        "main.cpp",
        "tslib.json",
    ]
}

// PLUGIN_EXTENDS = -
// PLUGIN_CLASS_NAME = QTsLibPlugin
