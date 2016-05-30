import qbs

QtAutotest {
    name: "tst_qcryptographichash"

    files: [
        "tst_qcryptographichash.cpp",
    ]

    Group {
        name: "android test data"
        condition: qbs.targetOS.contains("android")
        files: ["testdata.qrc"]
    }
}

// TESTDATA += data/*
