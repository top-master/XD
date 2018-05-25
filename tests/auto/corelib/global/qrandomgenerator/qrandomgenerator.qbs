import qbs

QtAutotest {
    name: "tst_qrandomgenerator"
    Depends { name: "Qt.core_private" }
    files: "tst_qrandomgenerator.cpp"
}
