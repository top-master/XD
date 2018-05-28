import qbs

QtAutotest {
    name: "tst_qloggingcategory"
    Depends { name: "Qt.core_private" }
    files: "tst_qloggingcategory.cpp"
}
