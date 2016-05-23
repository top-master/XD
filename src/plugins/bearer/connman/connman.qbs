import qbs

QtPlugin {
    name: "qconnmanbearer"
    category: "bearer"
    condition: qbs.targetOS.contains("linux") && !qbs.targetOS.contains("android")
               && Qt.dbus.present

    Depends { name: "Qt.network" }
    Depends { name: "Qt.network-private" }
    Depends { name: "Qt.dbus"; required: false }

    files: [
        "../linux_common/qofonoservice_linux.cpp",
        "../linux_common/qofonoservice_linux_p.h",
        "../qbearerengine_impl.h",
        "../qnetworksession_impl.cpp",
        "../qnetworksession_impl.h",
        "connman.json",
        "main.cpp",
        "qconnmanengine.cpp",
        "qconnmanengine.h",
        "qconnmanservice_linux.cpp",
        "qconnmanservice_linux_p.h",
    ]
}

// PLUGIN_CLASS_NAME = QConnmanEnginePlugin
// CONFIG += link_pkgconfig
