import qbs

QtAutotest {
    name: "tst_qbytearray"

    Depends { name: "Qt.core-private" }

    Properties {
        condition: qbs.targetOS.contains("darwin")
        cpp.frameworks: base.concat(["Foundation"])
    }

    files: [
        "tst_qbytearray.cpp",
    ]

    Group {
        name: "mac sources"
        condition: qbs.targetOS.contains("darwin")
        files: [
            "tst_qbytearray_mac.mm",
        ]
    }

    Group {
        name: "android test data"
        condition: qbs.targetOS.contains("android")
        files: ["android_testdata.qrc"]
    }
}

// TESTDATA += rfc3252.txt
