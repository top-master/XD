import qbs

QtAutotest {
    name: "tst_qstorageinfo"
    condition: base && !targetsUWP
    Depends { name: "Qt.core_private" }
    files: "tst_qstorageinfo.cpp"
}
