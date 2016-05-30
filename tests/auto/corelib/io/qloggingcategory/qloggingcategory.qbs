import qbs

QtAutotest {
    name: "tst_qloggingcategory"
    Depends { name: "Qt.core-private" }
    files: [
        "tst_qloggingcategory.cpp",
    ]
}
