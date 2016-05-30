import qbs

QtAutotest {
    name: "tst_qdir"
    Depends { name: "Qt.core-private" }
    Properties {
        condition: project.config.contains("builtin_testdata")
        cpp.defines: base.concat(["BUILTIN_TESTDATA"])
    }

    files: [
        "qdir.qrc",
        "tst_qdir.cpp",
    ]
    Group {
        name: "android_resources"
        condition: qbs.targetOS.contains("android")
        files: ["android_testdata.qrc"]
    }
}

// TESTDATA += testdir testData searchdir resources entrylist types tst_qdir.cpp
//win32: CONFIG += insignificant_test # Crashes on Windows in release builds
