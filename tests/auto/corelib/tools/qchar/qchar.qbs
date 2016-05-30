import qbs

QtAutotest {
    name: "tst_qchar"

    Depends { name: "Qt.core-private" }

    files: [
        "tst_qchar.cpp",
    ]

    Group {
        name: "android test data"
        condition: qbs.targetOS.contains("android")
        files: ["testdata.qrc"]
    }
}

// TESTDATA += data/NormalizationTest.txt
