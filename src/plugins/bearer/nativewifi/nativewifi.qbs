import qbs

QtPlugin {
    name: "qnativewifibearer"
    condition: qbs.targetOS.contains("windows") && !qbs.targetOS.contains("winrt")
    category: "bearer"

    Depends { name: "Qt.core-private" }
    Depends { name: "Qt.network" }
    Depends { name: "Qt.network-private" }

    files: [
        "../qbearerengine_impl.h",
        "../qnetworksession_impl.cpp",
        "../qnetworksession_impl.h",
        "main.cpp",
        "nativewifi.json",
        "platformdefs.h",
        "qnativewifiengine.cpp",
        "qnativewifiengine.h",
    ]
}

// PLUGIN_CLASS_NAME = QNativeWifiEnginePlugin
