import qbs

QtAutotest {
    name: "tst_qtemporaryfile"

    Depends { name: "Qt.testlib_private" }

    files: "qtemporaryfile.qrc"

    Group {
        name: "source and test resource"
        files: "tst_qtemporaryfile.cpp"
        fileTags: "qt.testdata"
        overrideTags: false
    }

    Group {
        name: "android test data"
        condition: targetsAndroidProper
        files: "android_testdata.qrc"
    }
}
