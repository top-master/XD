import qbs

QtAutotest {
    name: "tst_qhooks"
    Depends { name: "Qt.core_private" }
    files: [
        "tst_qhooks.cpp",
    ]
}
