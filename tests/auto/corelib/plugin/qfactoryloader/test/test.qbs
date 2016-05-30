import qbs

QtAutotest {
    name: "tst_qfactoryloader"
    destinationDirectory: project.buildDirectory + "/qfactoryloadertest"
    consoleApplication: true

    Depends { name: "Qt.core-private" }
    Depends { name: "plugin1"; condition: project.disabledFeatures.contains("library") }
    Depends { name: "plugin2"; condition: project.disabledFeatures.contains("library") }

    files: [
        "../plugin1/plugininterface1.h",
        "../plugin2/plugininterface2.h",
        "../tst_qfactoryloader.cpp",
    ]
}
