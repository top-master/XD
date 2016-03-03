import qbs

QtAutotest {
    name: "tst_qtranslator"
    files: [
        "tst_qtranslator.cpp",
        "qtranslator.qrc"
    ]
    Group {
        name: "android testdata"
        condition: qbs.targetOS.contains("android")
        files: "android_testdata.qrc"
    }
    // else: TESTDATA += hellotr_la.qm msgfmt_from_po.qm
}


