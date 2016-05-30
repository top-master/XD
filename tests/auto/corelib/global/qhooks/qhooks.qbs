import qbs

QtAutotest {
    name: "tst_qhooks"
    Depends { name: "Qt.core-private" }
    files: [
        "tst_qhooks.cpp",
    ]
}
