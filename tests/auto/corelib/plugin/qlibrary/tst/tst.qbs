import qbs

QtAutotest {
    name: "tst_qlibrary"
    consoleApplication: true
    destinationDirectory: project.buildDirectory + "/qlibrary_test"

    files: [
        "../tst_qlibrary.cpp",
    ]
}

// TESTDATA += ../library_path/invalid.so
