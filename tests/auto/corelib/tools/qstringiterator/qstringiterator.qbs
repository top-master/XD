import qbs

QtAutotest {
    name: "tst_qstringiterator"
    Depends { name: "Qt.core-private" }
    files: [
        "tst_qstringiterator.cpp",
    ]
}
