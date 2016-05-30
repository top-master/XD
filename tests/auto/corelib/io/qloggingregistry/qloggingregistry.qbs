import qbs

QtAutotest {
    name: "tst_qloggingregistry"
    condition: project.config.contains("private_tests")
    Depends { name: "Qt.core-private" }
    files: [
        "qtlogging.ini",
        "tst_qloggingregistry.cpp",
    ]
    Group {
        name: "android test data"
        condition: qbs.targetOS.contains("android")
        files: ["android_testdata.qrc"]
    }
}

// TESTDATA += qtlogging.ini
