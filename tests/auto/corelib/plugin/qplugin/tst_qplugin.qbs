import qbs

QtAutotest {
    name: "tst_qplugin"
    destinationDirectory: project.buildDirectory + "/qplugin_test"
    files: [
        "tst_qplugin.cpp",
    ]
}

// TESTDATA += plugins/*
