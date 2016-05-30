import qbs

QtAutotest {
    name: "tst_qtemporaryfile"
    files: [
        "tst_qtemporaryfile.cpp",
        "qtemporaryfile.qrc",
    ]
    Group {
        name: "android test data"
        condition: qbs.targetOS.contains("android")
        files: ["android_testdata.qrc"]
    }
}

// TESTDATA += tst_qtemporaryfile.cpp
