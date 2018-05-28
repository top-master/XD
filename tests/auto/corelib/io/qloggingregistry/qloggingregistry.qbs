import qbs

QtAutotest {
    name: "tst_qloggingregistry"
    condition: base && Qt.global.config.private_tests
    Depends { name: "Qt.core_private" }
    Depends { name: "Qt.global" }
    files: "tst_qloggingregistry.cpp"
    Group {
        name: "android test data"
        condition: targetsAndroidProper
        files: "android_testdata.qrc"
    }
    QtTestData {
        files: "qtlogging.ini"
    }
}
