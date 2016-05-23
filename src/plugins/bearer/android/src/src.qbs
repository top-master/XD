import qbs

QtPlugin {
    name: "qandroidbearer"
    condition: qbs.targetOS.contains("android")
    category: "bearer"

    Depends { name: "Qt.core-private" }
    Depends { name: "Qt.network-private" }

    cpp.includePaths: base.concat('wrappers')

    files: [
        "../../qbearerengine_impl.h",
        "../../qnetworksession_impl.cpp",
        "../../qnetworksession_impl.h",
        "main.cpp",
        "qandroidbearerengine.cpp",
        "qandroidbearerengine.h",
        "wrappers/androidconnectivitymanager.cpp",
        "wrappers/androidconnectivitymanager.h",
    ]
}

// PLUGIN_CLASS_NAME = QAndroidBearerEnginePlugin
