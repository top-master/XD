import qbs

QtAutotest {
    name: "tst_qfileinfo"
    condition: project.config.contains("private_tests")
    Depends { name: "Qt.core-private" }
    Properties {
        condition: qbs.targetOS.contains("windows") && !qbs.targetOS.contains("winrt")
        cpp.dynamicLibraries: base.concat(["advapi32", "netapi32"])
    }

    files: [
        "qfileinfo.qrc",
        "testdata.qrc",
        "tst_qfileinfo.cpp",
    ]
}

// win32: CONFIG += insignificant_test # Crashes on Windows in release builds
