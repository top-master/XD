import qbs

QtAutotest {
    name: "tst_qtranslator"
    supportsUiKit: true
    files: [
        "tst_qtranslator.cpp",
        "qtranslator.qrc"
    ]
    Group {
        name: "android testdata"
        condition: targetsAndroidProper
        files: "android_testdata.qrc"
    }
    QtTestData {
        condition: !targetsAndroidProper
        files: [
            "dependencies_la.qm",
            "hellotr_la.qm",
            "msgfmt_from_po.qm",
        ]
    }
}

