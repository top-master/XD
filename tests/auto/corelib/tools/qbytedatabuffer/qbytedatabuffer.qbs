import qbs

QtAutotest {
    name: "tst_qbytedatabuffer"
    Depends { name: "Qt.core-private" }
    files: [
        "tst_qbytedatabuffer.cpp",
    ]
}
