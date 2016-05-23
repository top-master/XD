import qbs

QtPlugin {
    name: "qcorewlanbearer"
    condition: qbs.targetOS.contains("darwin") && project.config.contains("corewlan")
    category: "bearer"

    Depends { name: "Qt.core-private" }
    Depends { name: "Qt.network" }
    Depends { name: "Qt.network-private" }

    cpp.frameworks: ["CoreWLAN", "Foundation", "Security", "SystemConfiguration"]

    files: [
        "../qbearerengine_impl.h",
        "../qnetworksession_impl.cpp",
        "../qnetworksession_impl.h",
        "corewlan.json",
        "main.cpp",
        "qcorewlanengine.h",
        "qcorewlanengine.mm",
    ]
}

// PLUGIN_CLASS_NAME = QCoreWlanEnginePlugin
