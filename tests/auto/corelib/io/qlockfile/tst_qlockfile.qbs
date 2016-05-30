import qbs

QtAutotest {
    name: "tst_qlockfile"
    consoleApplication: true
    destinationDirectory: project.buildDirectory + "/qlockfiletest"
    Depends { name: "Qt.concurrent" }
    Depends { name: "qlockfile_test_helper" }
    Properties {
        condition: qbs.targetOS.contains("windows") && !qbs.targetOS.contains("winrt")
        cpp.dynamicLibraries: base.concat(["advapi32"])
    }

    files: [
        "tst_qlockfile.cpp",
    ]
}
