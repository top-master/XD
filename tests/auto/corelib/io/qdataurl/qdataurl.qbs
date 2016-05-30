import qbs

QtAutotest {
    name: "tst_qdataurl"
    Depends { name: "Qt.core-private" }
    files: [
        "tst_qdataurl.cpp",
    ]
}
