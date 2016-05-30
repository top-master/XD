import qbs

QtAutotest {
    name: "tst_qpluginloader"
    consoleApplication: true
    destinationDirectory: project.buildDirectory + "/qpluginloader_test"

    Depends { name: "Qt.core-private"; condition: project.config.contains("private_tests") }

    files: [
        "../fakeplugin.cpp",
        "../theplugin/plugininterface.h",
        "../tst_qpluginloader.cpp",
    ]
}

// TESTDATA += ../elftest ../machtest
