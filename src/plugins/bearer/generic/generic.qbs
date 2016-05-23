import qbs

QtPlugin {
    name: "qgenericbearer"
    condition: !qbs.targetOS.contains("android")
    category: "bearer"

    Depends { name: "Qt.core-private" }
    Depends { name: "Qt.network" }
    Depends { name: "Qt.network-private" }

    files: [
        "../platformdefs_win.h",
        "../qnetworksession_impl.cpp",
        "../qbearerengine_impl.h",
        "../qnetworksession_impl.h",
        "generic.json",
        "main.cpp",
        "qgenericengine.cpp",
        "qgenericengine.h",
    ]
}

// PLUGIN_CLASS_NAME = QGenericEnginePlugin
