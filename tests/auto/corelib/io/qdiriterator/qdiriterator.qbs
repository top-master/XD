import qbs

QtAutotest {
    name: "tst_qdiriterator"
    Depends { name: "Qt.core_private" }
    files: [
        "qdiriterator.qrc",
        "tst_qdiriterator.cpp",
    ]
    QtTestData {
        files: "entrylist/**"
    }
}
