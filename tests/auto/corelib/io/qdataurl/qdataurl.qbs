import qbs

QtAutotest {
    name: "tst_qdataurl"
    Depends { name: "Qt.core_private" }
    files: "tst_qdataurl.cpp"
}
