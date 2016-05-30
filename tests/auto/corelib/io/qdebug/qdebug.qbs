import qbs

QtAutotest {
    name: "tst_qdebug"
    Depends { name: "Qt.concurrent" }
    files: [
        "tst_qdebug.cpp",
    ]
}
