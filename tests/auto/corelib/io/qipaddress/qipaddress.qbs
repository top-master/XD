import qbs

QtAutotest {
    name: "tst_qipaddress"
    condition: base && Qt.global.privateConfig.private_tests
    Depends { name: "Qt.core_private" }
    Depends { name: "Qt.global" }
    files: "tst_qipaddress.cpp"
}
