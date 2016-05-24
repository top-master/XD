import qbs

QtPlugin {
    name: "qminimal"
    category: "platforms"

    Depends { name: "Qt.core-private" }
    Depends { name: "Qt.gui-private" }
    Depends { name: "Qt.gui" }
    Depends { name: "Qt.platformsupport-private" }

    files: [
        "main.cpp",
        "minimal.json",
        "qminimalbackingstore.cpp",
        "qminimalbackingstore.h",
        "qminimalintegration.cpp",
        "qminimalintegration.h",
    ]
}

// PLUGIN_CLASS_NAME = QMinimalIntegrationPlugin
// !equals(TARGET, $$QT_DEFAULT_QPA_PLUGIN): PLUGIN_EXTENDS = -
