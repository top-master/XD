import qbs

QtAutotest {
    name: "tst_qiodevice"
    condition: Qt.network.present
    Depends { name: "Qt.network"; required: false }
    files: [
        "tst_qiodevice.cpp",
    ]
    Group {
        name: "android test data"
        condition: qbs.targetOS.contains("android")
        files: ["android_testdata.qrc"]
    }
}

// TESTDATA += tst_qiodevice.cpp
// MOC_DIR=tmp
