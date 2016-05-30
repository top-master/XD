import qbs

QtAutotest {
    name: "tst_qdatastream"
    condition: Qt.gui.present
    Depends { name: "Qt.gui"; required: false }
    files: [
        "tst_qdatastream.cpp",
    ]
    Group {
        name: "resources"
        condition: qbs.targetOS.contains("android")
        files: [
            "testdata.qrc",
        ]
    }
}
