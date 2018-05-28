import qbs

QtAutotest {
    name: "tst_qfilesystemmetadata"
    Depends { name: "Qt.core_private" }
    files: "tst_qfilesystemmetadata.cpp"
}
