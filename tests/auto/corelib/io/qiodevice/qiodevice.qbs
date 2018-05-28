import qbs

QtAutotest {
    name: "tst_qiodevice"
    condition: base && Qt.global.privateConfig.network
    Depends { name: "Qt.global" }
    Depends { name: "Qt.network" }
    Group {
        name: "test source"
        files: "tst_qiodevice.cpp"
        fileTags: "qt.testdata"
        overrideTags: false
    }

    Group {
        name: "android test data"
        condition: targetsAndroidProper
        files: "android_testdata.qrc"
    }
}

// MOC_DIR=tmp
