import qbs

QtAutotest {
    name: "tst_qmimetype"
    condition: base && Qt.global.privateConfig.private_tests
    Depends { name: "Qt.core_private" }
    files: "tst_qmimetype.cpp"
}
