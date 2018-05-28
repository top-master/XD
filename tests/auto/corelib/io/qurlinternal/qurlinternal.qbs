import qbs

QtAutotest {
    name: "tst_qurlinternal"
    condition: Qt.global.config.private_tests
    Depends { name: "Qt.core_private" }
    Depends { name: "Qt.global" }
    files: [
        "tst_qurlinternal.cpp",
        "../../codecs/utf8/utf8data.cpp",
    ]
}
