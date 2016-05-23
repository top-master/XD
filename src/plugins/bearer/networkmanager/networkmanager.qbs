import qbs

QtPlugin {
    name: "qnmbearer"
    condition: qbs.targetOS.contains("linux") && !qbs.targetOS.contains("android")
               && Qt.dbus.present
    category: "bearer"

    Depends { name: "Qt.network" }
    Depends { name: "Qt.network-private" }
    Depends { name: "Qt.dbus"; required: false }

    files: [
        "../linux_common/qofonoservice_linux.cpp",
        "../linux_common/qofonoservice_linux_p.h",
        "../qbearerengine_impl.h",
        "../qnetworksession_impl.cpp",
        "../qnetworksession_impl.h",
        "main.cpp",
        "networkmanager.json",
        "qnetworkmanagerengine.cpp",
        "qnetworkmanagerengine.h",
        "qnetworkmanagerservice.cpp",
        "qnetworkmanagerservice.h",
    ]
}

// PLUGIN_CLASS_NAME = QNetworkManagerEnginePlugin
