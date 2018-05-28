import qbs

QtAutotest {
    name: "tst_qdir"
    Depends { name: "Qt.core_private" }

    files: "qdir.qrc"
    Group {
        name: "test source"
        files: "tst_qdir.cpp"
        fileTags: "qt.testdata"
        overrideTags: false
    }

    Group {
        name: "android_resources"
        condition: targetsAndroidProper
        files: "android_testdata.qrc"
    }
    QtTestData {
        files: [
            "testdir/**",
            "searchdir/**",
            "resources/**",
            "entrylist/**",
            "types/**",
        ]
    }
}
