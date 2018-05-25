import qbs

QtAutotest {
    name: "tst_qtendian"
    Depends { name: "Qt.core_private" }
    files: [
        "tst_qtendian.cpp",
    ]
}
