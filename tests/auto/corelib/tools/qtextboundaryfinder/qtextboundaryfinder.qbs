import qbs

QtAutotest {
    name: "tst_qtextboundaryfinder"

    files: [
        "tst_qtextboundaryfinder.cpp",
    ]

    Group {
        name: "android test data"
        condition: qbs.targetOS.contains("android")
        files: ["testdata.qrc"]
    }
}

// TESTDATA += data
